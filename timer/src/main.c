#include "../include/timer.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/statvfs.h>

#define RING_COLOR 0xFFFFFF    // White for the static ring
#define TIMER_COLOR 0xFFA500   // Orange for the dynamic ring and timer
#define TIMER_START_VALUE 60   // Start value of the countdown timer in seconds

int countdown = TIMER_START_VALUE;

// Set a pixel at (x, y) with color in the framebuffer
void set_pixel(int *framebuffer, struct fb_var_screeninfo vinfo, int x, int y, int color) {
    if (x >= 0 && x < vinfo.xres_virtual && y >= 0 && y < vinfo.yres_virtual) {
        framebuffer[y * vinfo.xres_virtual + x] = color;
    }
}

// Draw a ring with specified thickness
void draw_ring(int *framebuffer, struct fb_var_screeninfo vinfo, int center_x, int center_y, int radius, int thickness, int color) {
    for (int r = radius - thickness / 2; r <= radius + thickness / 2; r++) {
        for (int angle = 0; angle < 360; ++angle) {
            int x = center_x + (int)(r * cos(angle * M_PI / 180));
            int y = center_y + (int)(r * sin(angle * M_PI / 180));
            set_pixel(framebuffer, vinfo, x, y, color);
        }
    }
}

// Draw the static ring for the clock face
void draw_static_ring(int *framebuffer, struct fb_var_screeninfo vinfo) {
    draw_ring(framebuffer, vinfo, CENTER_X, CENTER_Y, RADIUS, 5, RING_COLOR); // Draw a thick white ring
}

// Draw the dynamic ring with decreasing radius
void draw_dynamic_ring(int *framebuffer, struct fb_var_screeninfo vinfo) {
    int dynamic_radius = RADIUS * countdown / TIMER_START_VALUE;  // Scale the radius based on remaining time
    draw_ring(framebuffer, vinfo, CENTER_X, CENTER_Y, dynamic_radius, 3, TIMER_COLOR); // Draw an orange ring
}

// Draw the countdown timer inside the ring
void draw_countdown_timer(int *framebuffer, struct fb_var_screeninfo vinfo) {
    char timer_text[10];
    sprintf(timer_text, "%d", countdown);
    draw_text(framebuffer, vinfo, timer_text, CENTER_X - 10, CENTER_Y - 10, 3, TIMER_COLOR); // Center the text
}

// Draw the clock face with rings and numbers
void draw_clock_face(int *framebuffer, struct fb_var_screeninfo vinfo) {
    memset(framebuffer, 0, vinfo.yres_virtual * vinfo.xres_virtual * sizeof(int)); // Clear screen
    draw_static_ring(framebuffer, vinfo);     // Draw the static white ring
    draw_dynamic_ring(framebuffer, vinfo);    // Draw the dynamic orange ring
    draw_countdown_timer(framebuffer, vinfo); // Draw the countdown timer
}

// Update the clock hands, date/time display, and system information
void update_time(int *framebuffer, struct fb_var_screeninfo vinfo) {
    time_t rawtime;
    struct tm *timeinfo;
    char date_buffer[80];
    char time_buffer[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    float hour_angle_degrees = (30 * (timeinfo->tm_hour % 12)) + (timeinfo->tm_min * 0.5);
    float hour_angle = -hour_angle_degrees * M_PI / 180.0 + M_PI / 2;

    float minute_angle_degrees = 6 * timeinfo->tm_min;
    float minute_angle = -minute_angle_degrees * M_PI / 180.0 + M_PI / 2;

    draw_hand(framebuffer, vinfo, hour_angle, HOUR_HAND_LENGTH, 0xFFFFFF);    // Hour hand
    draw_hand(framebuffer, vinfo, minute_angle, MINUTE_HAND_LENGTH, 0xFFFFFF); // Minute hand

    strftime(date_buffer, sizeof(date_buffer), "%Y-%m-%d", timeinfo);
    draw_text(framebuffer, vinfo, date_buffer, CENTER_X - 100, CENTER_Y + 200, 3, 0xFFFFFF);

    strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", timeinfo);
    draw_text(framebuffer, vinfo, time_buffer, CENTER_X - 80, CENTER_Y + 250, 3, 0xFFFFFF);

    update_system_info(framebuffer, vinfo); // Display system info

    // Countdown timer logic
    draw_countdown_timer(framebuffer, vinfo);

    if (countdown > 0) {
        countdown--;
    }
}

// Main function to continuously update the clock
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
