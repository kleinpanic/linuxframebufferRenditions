#include "../include/clock.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/statvfs.h>  // Include this for statvfs

// Set a pixel at (x, y) with color in the framebuffer
void set_pixel(int *framebuffer, struct fb_var_screeninfo vinfo, int x, int y, int color) {
    if (x >= 0 && x < vinfo.xres_virtual && y >= 0 && y < vinfo.yres_virtual) {
        framebuffer[y * vinfo.xres_virtual + x] = color;
    }
}

// Bresenham's line algorithm for drawing lines
void draw_line(int *framebuffer, struct fb_var_screeninfo vinfo, int x0, int y0, int x1, int y1, int color) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (1) {
        set_pixel(framebuffer, vinfo, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

// Draw a simple filled rectangle to represent text characters
void draw_char(int *framebuffer, struct fb_var_screeninfo vinfo, char c, int x, int y, int size, int color) {
    static const char font[10][5][3] = {
        { "111", "101", "101", "101", "111" },  // '0'
        { "110", "010", "010", "010", "111" },  // '1'
        { "111", "001", "111", "100", "111" },  // '2'
        { "111", "001", "111", "001", "111" },  // '3'
        { "101", "101", "111", "001", "001" },  // '4'
        { "111", "100", "111", "001", "111" },  // '5'
        { "111", "100", "111", "101", "111" },  // '6'
        { "111", "001", "001", "001", "001" },  // '7'
        { "111", "101", "111", "101", "111" },  // '8'
        { "111", "101", "111", "001", "111" }   // '9'
    };

    if (c >= '0' && c <= '9') {
        int index = c - '0';
        for (int row = 0; row < 5; ++row) {
            for (int col = 0; col < 3; ++col) {
                if (font[index][row][col] == '1') {
                    for (int i = 0; i < size; ++i) {
                        for (int j = 0; j < size; ++j) {
                            set_pixel(framebuffer, vinfo, x + col * size + i, y + row * size + j, color);
                        }
                    }
                }
            }
        }
    }
}

// Draw a string of characters
void draw_text(int *framebuffer, struct fb_var_screeninfo vinfo, const char *text, int x, int y, int size, int color) {
    for (const char *p = text; *p; ++p) {
        draw_char(framebuffer, vinfo, *p, x, y, size, color);
        x += size * 4; // Move to the next character position
    }
}

// Draw numbers around the clock face
void draw_circle(int *framebuffer, struct fb_var_screeninfo vinfo) {
    for (int i = 1; i <= 12; ++i) {
        float angle = (i * 30 - 90) * M_PI / 180.0;
        int x = CENTER_X + (int)(RADIUS * cos(angle));
        int y = CENTER_Y + (int)(RADIUS * sin(angle));
        
        char buffer[3];
        sprintf(buffer, "%d", i);
        draw_text(framebuffer, vinfo, buffer, x - 10, y - 10, 3, 0xFFFFFF); // Increased the size to '3' for visibility
    }
}

// Draw clock hands
void draw_hand(int *framebuffer, struct fb_var_screeninfo vinfo, float angle, int length, int color) {
    int x_end = CENTER_X + length * cos(angle);
    int y_end = CENTER_Y - length * sin(angle);
    draw_line(framebuffer, vinfo, CENTER_X, CENTER_Y, x_end, y_end, color);
}

// Draw the clock face with numbers
void draw_clock_face(int *framebuffer, struct fb_var_screeninfo vinfo) {
    memset(framebuffer, 0, vinfo.yres_virtual * vinfo.xres_virtual * sizeof(int)); // Clear screen
    draw_circle(framebuffer, vinfo); // Draw the numbers
}

// Function to read the first line of a file
int read_first_line(const char *path, char *buffer, size_t size) {
    FILE *file = fopen(path, "r");
    if (file == NULL) return -1;
    if (fgets(buffer, size, file) == NULL) {
        fclose(file);
        return -1;
    }
    fclose(file);
    return 0;
}

// Get system data
int get_cpu_usage() {
    char buffer[256];
    unsigned long long int user, nice, system, idle;
    read_first_line("/proc/stat", buffer, sizeof(buffer));
    sscanf(buffer, "cpu %llu %llu %llu %llu", &user, &nice, &system, &idle);
    
    static unsigned long long int prev_user = 0, prev_nice = 0, prev_system = 0, prev_idle = 0;
    unsigned long long int total_diff = (user - prev_user) + (nice - prev_nice) + (system - prev_system);
    unsigned long long int idle_diff = idle - prev_idle;
    int cpu_usage = (total_diff * 100) / (total_diff + idle_diff);

    prev_user = user;
    prev_nice = nice;
    prev_system = system;
    prev_idle = idle;
    
    return cpu_usage;
}

int get_ram_usage() {
    char buffer[256];
    unsigned long mem_total, mem_available;
    read_first_line("/proc/meminfo", buffer, sizeof(buffer));
    sscanf(buffer, "MemTotal: %lu kB", &mem_total);
    read_first_line("/proc/meminfo", buffer, sizeof(buffer));
    sscanf(buffer, "MemAvailable: %lu kB", &mem_available);

    int ram_usage = ((mem_total - mem_available) * 100) / mem_total;
    return ram_usage;
}

int get_disk_usage() {
    struct statvfs stat;
    if (statvfs("/", &stat) != 0) return -1;
    
    unsigned long total_blocks = stat.f_blocks;
    unsigned long free_blocks = stat.f_bfree;
    int disk_usage = ((total_blocks - free_blocks) * 100) / total_blocks;
    
    return disk_usage;
}

int get_battery_percentage() {
    char buffer[16];
    read_first_line("/sys/class/power_supply/BAT0/capacity", buffer, sizeof(buffer));
    return atoi(buffer);
}

int get_cpu_temperature() {
    char buffer[16];
    read_first_line("/sys/class/thermal/thermal_zone0/temp", buffer, sizeof(buffer));
    return atoi(buffer) / 1000;
}

// Draw a percentage bar using [#####] style
void draw_percentage_bar(int *framebuffer, struct fb_var_screeninfo vinfo, int x, int y, int percentage, int size, int color) {
    char bar[6];
    int num_hashes = (percentage / 20);
    for (int i = 0; i < 5; ++i) {
        bar[i] = i < num_hashes ? '#' : ' ';
    }
    bar[5] = '\0';
    draw_text(framebuffer, vinfo, bar, x, y, size, color);
}

// Update system info on the screen
void update_system_info(int *framebuffer, struct fb_var_screeninfo vinfo) {
    int battery_percentage = get_battery_percentage();
    int cpu_usage = get_cpu_usage();
    int ram_usage = get_ram_usage();
    int disk_usage = get_disk_usage();
    int cpu_temp = get_cpu_temperature();

    char buffer[80];

    // Draw battery info
    sprintf(buffer, "Battery: %d%%", battery_percentage);
    draw_text(framebuffer, vinfo, buffer, CENTER_X - 100, CENTER_Y + 300, 2, 0xFFFFFF);
    draw_percentage_bar(framebuffer, vinfo, CENTER_X + 60, CENTER_Y + 300, battery_percentage, 2, 0xFFFFFF);

    // Draw CPU usage
    sprintf(buffer, "CPU: %d%% Temp: %d°C", cpu_usage, cpu_temp);
    draw_text(framebuffer, vinfo, buffer, CENTER_X - 100, CENTER_Y + 350, 2, 0xFFFFFF);
    draw_percentage_bar(framebuffer, vinfo, CENTER_X + 60, CENTER_Y + 350, cpu_usage, 2, 0xFFFFFF);

    // Draw RAM usage
    sprintf(buffer, "RAM: %d%%", ram_usage);
    draw_text(framebuffer, vinfo, buffer, CENTER_X - 100, CENTER_Y + 400, 2, 0xFFFFFF);
    draw_percentage_bar(framebuffer, vinfo, CENTER_X + 60, CENTER_Y + 400, ram_usage, 2, 0xFFFFFF);

    // Draw Disk usage
    sprintf(buffer, "Disk: %d%%", disk_usage);
    draw_text(framebuffer, vinfo, buffer, CENTER_X - 100, CENTER_Y + 450, 2, 0xFFFFFF);
    draw_percentage_bar(framebuffer, vinfo, CENTER_X + 60, CENTER_Y + 450, disk_usage, 2, 0xFFFFFF);
}

// Update the clock hands and date/time display
void update_time(int *framebuffer, struct fb_var_screeninfo vinfo) {
    time_t rawtime;
    struct tm *timeinfo;
    char date_buffer[80];
    char time_buffer[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    float hour_angle_degrees = (30 * timeinfo->tm_hour) + (timeinfo->tm_min * 0.5);
    float hour_angle = - hour_angle_degrees * M_PI / 180.0 + M_PI / 2;

    float minute_angle_degrees = 6 * timeinfo->tm_min;
    float minute_angle = - minute_angle_degrees * M_PI / 180.0 + M_PI / 2;

    draw_hand(framebuffer, vinfo, hour_angle, HOUR_HAND_LENGTH, 0xFFFFFF);
    draw_hand(framebuffer, vinfo, minute_angle, MINUTE_HAND_LENGTH, 0xFFFFFF);

    strftime(date_buffer, sizeof(date_buffer), "%Y-%m-%d", timeinfo);
    draw_text(framebuffer, vinfo, date_buffer, CENTER_X - 100, CENTER_Y + 200, 3, 0xFFFFFF);

    strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", timeinfo);
    draw_text(framebuffer, vinfo, time_buffer, CENTER_X - 80, CENTER_Y + 250, 3, 0xFFFFFF);

    update_system_info(framebuffer, vinfo);
}

int main() {
    int fbfd = open("/dev/fb0", O_RDWR);
    if (fbfd == -1) {
        perror("Error: cannot open framebuffer device");
        exit(1);
    }

    struct fb_var_screeninfo vinfo;
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
        perror("Error reading variable information");
        close(fbfd);
        exit(2);
    }

    size_t screensize = vinfo.yres_virtual * vinfo.xres_virtual * vinfo.bits_per_pixel / 8;
    int *framebuffer = (int *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if (framebuffer == MAP_FAILED) {
        perror("Error mapping framebuffer device to memory");
        close(fbfd);
        exit(3);
    }

    while (1) {
        draw_clock_face(framebuffer, vinfo);
        update_time(framebuffer, vinfo);
        sleep(1); 
    }

    munmap(framebuffer, screensize);
    close(fbfd);

    return 0;
}

