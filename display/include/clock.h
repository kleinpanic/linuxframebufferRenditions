// include/clock.h
#ifndef CLOCK_H
#define CLOCK_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <math.h>
#include <string.h> 
#include <sys/ioctl.h>
#include <errno.h>


#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define CENTER_X (SCREEN_WIDTH / 2)
#define CENTER_Y (SCREEN_HEIGHT / 2)
#define RADIUS 200
#define HOUR_HAND_LENGTH 100
#define MINUTE_HAND_LENGTH 150

typedef struct {
    int x;
    int y;
} Point;

void draw_circle(int *framebuffer, struct fb_var_screeninfo vinfo);
void draw_hand(int *framebuffer, struct fb_var_screeninfo vinfo, float angle, int length, int color);
void draw_clock_face(int *framebuffer, struct fb_var_screeninfo vinfo);
void update_time(int *framebuffer, struct fb_var_screeninfo vinfo);
void draw_text(int *framebuffer, struct fb_var_screeninfo vinfo, const char *text, int x, int y, int size, int color);
void update_system_info(int *framebuffer, struct fb_var_screeninfo vinfo);
int get_cpu_usage();
int get_ram_usage();
int get_disk_usage();
int get_battery_percentage();
int get_cpu_temperature();

#endif
