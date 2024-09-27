// src/clock.c
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

// Update the clock hands and date/time display
void update_time(int *framebuffer, struct fb_var_screeninfo vinfo) {
    time_t rawtime;
    struct tm *timeinfo;
    char date_buffer[80];
    char time_buffer[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    // Calculate the angle for the hour hand
    float hour_angle_degrees = (30 * timeinfo->tm_hour) + (timeinfo->tm_min * 0.5);
    float hour_angle = - hour_angle_degrees * M_PI / 180.0 + M_PI / 2; // Convert to radians and adjust to 12 o'clock start

    // Calculate the angle for the minute hand
    float minute_angle_degrees = 6 * timeinfo->tm_min;
    float minute_angle = - minute_angle_degrees * M_PI / 180.0 + M_PI / 2; // Convert to radians and adjust to 12 o'clock start

    // Draw both hands in white (0xFFFFFF)
    draw_hand(framebuffer, vinfo, hour_angle, HOUR_HAND_LENGTH, 0xFFFFFF); // Hour hand is shorter
    draw_hand(framebuffer, vinfo, minute_angle, MINUTE_HAND_LENGTH, 0xFFFFFF); // Minute hand is longer

    // Display the date
    strftime(date_buffer, sizeof(date_buffer), "%Y-%m-%d", timeinfo);
    draw_text(framebuffer, vinfo, date_buffer, CENTER_X - 100, CENTER_Y + 300, 3, 0xFFFFFF); // Moved lower and increased size

    // Display the time
    strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", timeinfo);
    draw_text(framebuffer, vinfo, time_buffer, CENTER_X - 80, CENTER_Y + 350, 3, 0xFFFFFF); // Moved lower and increased size
}

int main() {
    // Open the framebuffer device
    int fbfd = open("/dev/fb0", O_RDWR);
    if (fbfd == -1) {
        perror("Error: cannot open framebuffer device");
        exit(1);
    }

    // Get variable screen information
    struct fb_var_screeninfo vinfo;
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
        perror("Error reading variable information");
        close(fbfd);
        exit(2);
    }

    // Map the framebuffer to user memory
    size_t screensize = vinfo.yres_virtual * vinfo.xres_virtual * vinfo.bits_per_pixel / 8;
    int *framebuffer = (int *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if (framebuffer == MAP_FAILED) {
        perror("Error mapping framebuffer device to memory");
        close(fbfd);
        exit(3);
    }

    // Continuously update the clock
    while (1) {
        draw_clock_face(framebuffer, vinfo);
        update_time(framebuffer, vinfo);
        sleep(1); // Sleep for 1 second to update the clock every second
    }

    // Cleanup
    munmap(framebuffer, screensize);
    close(fbfd);

    return 0;
}
