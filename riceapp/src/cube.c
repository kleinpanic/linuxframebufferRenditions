#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "cube.h"

// Framebuffer and screen parameters
int fbfd = 0;
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
long int screensize = 0;
char *fbp = 0;

// Cube vertex data
Vertex vertices[8] = {
    {-50, -50, -50}, {50, -50, -50}, {50, 50, -50}, {-50, 50, -50},
    {-50, -50, 50}, {50, -50, 50}, {50, 50, 50}, {-50, 50, 50}
};

float velocityX = 2.0, velocityY = 1.5;
float rotationSpeed = 0.02;
float cubeX = 300, cubeY = 200;
Screen screen = {800, 600}; // Screen resolution

// Initialize framebuffer
void init_framebuffer() {
    fbfd = open("/dev/fb0", O_RDWR);
    if (fbfd == -1) {
        perror("Error opening framebuffer device");
        exit(1);
    }
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
        perror("Error reading fixed information");
        exit(2);
    }
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
        perror("Error reading variable information");
        exit(3);
    }
    screensize = vinfo.yres_virtual * finfo.line_length;
    fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if ((int)fbp == -1) {
        perror("Error mapping framebuffer device to memory");
        exit(4);
    }
}

// Clear screen
void clear_screen() {
    for (int y = 0; y < screen.height; y++) {
        for (int x = 0; x < screen.width; x++) {
            long int location = (x + vinfo.xoffset) * (vinfo.bits_per_pixel / 8) + (y + vinfo.yoffset) * finfo.line_length;
            *((unsigned int*)(fbp + location)) = 0x000000; // Black color
        }
    }
}

// Put pixel on screen
void put_pixel(int x, int y, unsigned int color) {
    if (x >= 0 && x < screen.width && y >= 0 && y < screen.height) {
        long int location = (x + vinfo.xoffset) * (vinfo.bits_per_pixel / 8) + (y + vinfo.yoffset) * finfo.line_length;
        *((unsigned int*)(fbp + location)) = color;
    }
}

// Draw line between two points (Bresenham's line algorithm)
void draw_line(int x1, int y1, int x2, int y2, unsigned int color) {
    int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = dx + dy, e2;
    
    while (1) {
        put_pixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

// Project 3D coordinates to 2D
void project(Vertex v, int *x, int *y) {
    float scale = 200 / (v.z + 200);  // Perspective projection scaling
    *x = (int)(v.x * scale + cubeX);
    *y = (int)(v.y * scale + cubeY);
}

// Draw the 3D cube
void draw_cube(Vertex vertices[8]) {
    int projectedX[8], projectedY[8];
    for (int i = 0; i < 8; i++) {
        project(vertices[i], &projectedX[i], &projectedY[i]);
    }
    
    // Draw front face
    for (int i = 0; i < 4; i++) {
        draw_line(projectedX[i], projectedY[i], projectedX[(i+1)%4], projectedY[(i+1)%4], 0xFFFFFF);
    }
    
    // Draw back face
    for (int i = 4; i < 8; i++) {
        draw_line(projectedX[i], projectedY[i], projectedX[((i+1)%4)+4], projectedY[((i+1)%4)+4], 0x00FF00);
    }
    
    // Draw edges between front and back faces
    for (int i = 0; i < 4; i++) {
        draw_line(projectedX[i], projectedY[i], projectedX[i+4], projectedY[i+4], 0xFF0000);
    }
}

// Translate vertices
void translate(Vertex *v, float dx, float dy, float dz) {
    v->x += dx;
    v->y += dy;
    v->z += dz;
}

// Rotate cube vertices around X and Y axes
void rotate_cube(Vertex vertices[8], float angleX, float angleY) {
    float cosX = cos(angleX), sinX = sin(angleX);
    float cosY = cos(angleY), sinY = sin(angleY);
    
    for (int i = 0; i < 8; i++) {
        float x = vertices[i].x, y = vertices[i].y, z = vertices[i].z;
        
        // Rotation around X-axis
        vertices[i].y = y * cosX - z * sinX;
        vertices[i].z = y * sinX + z * cosX;
        
        // Rotation around Y-axis
        vertices[i].x = x * cosY - z * sinY;
        vertices[i].z = x * sinY + z * cosY;
    }
}

// Handle screen edge collision
void handle_collision() {
    if (cubeX >= screen.width - 100 || cubeX <= 100) velocityX = -velocityX;
    if (cubeY >= screen.height - 100 || cubeY <= 100) velocityY = -velocityY;
}

int main() {
    init_framebuffer();
    
    float angleX = 0.0, angleY = 0.0;
    
    while (1) {
        // Clear the screen
        clear_screen();
        
        // Translate the cube across the screen
        cubeX += velocityX;
        cubeY += velocityY;
        handle_collision();
        
        // Rotate the cube
        rotate_cube(vertices, angleX, angleY);
        
        // Draw the cube on the screen
        draw_cube(vertices);
        
        // Update rotation angles for the next iteration
        angleX += rotationSpeed;
        angleY += rotationSpeed;

        // Add delay to control frame rate (~60 FPS)
        usleep(16000);
    }
    
    munmap(fbp, screensize);
    close(fbfd);
    return 0;
}

