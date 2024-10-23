// include/timer.h
#ifndef TIMER_H
#define TIMER_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <linux/fb.h>       // Include this for framebuffer structures
#include <fcntl.h>
#include <sys/mman.h>
#include <math.h>
#include <string.h> 
#include <sys/ioctl.h>
#include <errno.h>
#include <stdint.h>
#include <sys/statvfs.h>    // For disk usage calculations

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define CENTER_X (SCREEN_WIDTH / 2)
#define CENTER_Y (SCREEN_HEIGHT / 2)
#define RADIUS 200
#define HOUR_HAND_LENGTH 100
#define MINUTE_HAND_LENGTH 150

#define RING_COLOR 0xFFFFFF  // White color
#define TIMER_COLOR 0xFFA500 // Orange color
#define TIMER_START_VALUE 60 // Start value of the countdown timer in seconds

typedef struct {
    int x;
    int y;
} Point;

typedef struct fb_var_screeninfo fb_var_screeninfo; // Alias for convenience

typedef int framebuffer_t;  // Define the framebuffer_t as an int for this context

void draw_ring(int *framebuffer, fb_var_screeninfo vinfo, int center_x, int center_y, int radius, int thickness, int color);
void draw_static_ring(int *framebuffer, fb_var_screeninfo vinfo);
void draw_dynamic_ring(int *framebuffer, fb_var_screeninfo vinfo);
void draw_countdown_timer(int *framebuffer, fb_var_screeninfo vinfo);
void draw_clock_face(int *framebuffer, fb_var_screeninfo vinfo);
void update_time(int *framebuffer, fb_var_screeninfo vinfo);
void draw_text(int *framebuffer, fb_var_screeninfo vinfo, const char *text, int x, int y, int size, int color);

#endif // TIMER_H
