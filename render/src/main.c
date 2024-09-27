#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <math.h>
#include <stdint.h>

#define CUBE_SIZE 200.0
#define COLOR 0xFFFFFF  // White for 32-bit or RGB565 for 16-bit
#define FRAME_DELAY 50000  // Slower: Microseconds (~20fps)
#define ROTATION_SPEED 0.003  // Slower rotation speed

// Structure for framebuffer info
struct framebuffer_info {
    int fb_fd;
    uint8_t* fb_ptr;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    long int screensize;
};

// Structure for 3D point
typedef struct {
    float x, y, z;
} Point3D;

// Cube vertices
Point3D cube[8] = {
    {-CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE},
    { CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE},
    { CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE},
    {-CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE},
    {-CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE},
    { CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE},
    { CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE},
    {-CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE}
};

// Cube edges
int edges[12][2] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 0},  // Bottom face
    {4, 5}, {5, 6}, {6, 7}, {7, 4},  // Top face
    {0, 4}, {1, 5}, {2, 6}, {3, 7}   // Connecting edges
};

// Function to initialize framebuffer
struct framebuffer_info init_framebuffer(const char* fb_path) {
    struct framebuffer_info fb_info;
    
    fb_info.fb_fd = open(fb_path, O_RDWR);
    if (fb_info.fb_fd == -1) {
        perror("Error opening framebuffer device");
        exit(1);
    }

    // Get fixed screen information
    if (ioctl(fb_info.fb_fd, FBIOGET_FSCREENINFO, &fb_info.finfo)) {
        perror("Error reading fixed information");
        exit(1);
    }

    // Get variable screen information
    if (ioctl(fb_info.fb_fd, FBIOGET_VSCREENINFO, &fb_info.vinfo)) {
        perror("Error reading variable information");
        exit(1);
    }

    // Calculate the screen size in bytes
    fb_info.screensize = fb_info.vinfo.yres_virtual * fb_info.finfo.line_length;

    // Map framebuffer to memory
    fb_info.fb_ptr = (uint8_t*)mmap(0, fb_info.screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_info.fb_fd, 0);
    if ((intptr_t)fb_info.fb_ptr == -1) {
        perror("Error mapping framebuffer to memory");
        exit(1);
    }

    return fb_info;
}

// Function to clear the screen
void clear_screen(struct framebuffer_info* fb_info) {
    for (int i = 0; i < fb_info->screensize; i++) {
        fb_info->fb_ptr[i] = 0;  // Set each pixel to black
    }
}

// Function to set a pixel in the framebuffer
void set_pixel(struct framebuffer_info* fb_info, int x, int y, uint32_t color) {
    if (x >= 0 && x < fb_info->vinfo.xres && y >= 0 && y < fb_info->vinfo.yres) {
        long location = (x + fb_info->vinfo.xoffset) * (fb_info->vinfo.bits_per_pixel / 8) +
                        (y + fb_info->vinfo.yoffset) * fb_info->finfo.line_length;

        // Handle different bits per pixel
        if (fb_info->vinfo.bits_per_pixel == 32) {
            *((uint32_t*)(fb_info->fb_ptr + location)) = color;
        } else if (fb_info->vinfo.bits_per_pixel == 16) {
            uint16_t rgb565 = ((color & 0xF80000) >> 8) | ((color & 0x00FC00) >> 5) | ((color & 0x0000F8) >> 3);
            *((uint16_t*)(fb_info->fb_ptr + location)) = rgb565;
        } else {
            printf("Unsupported bits per pixel: %d\n", fb_info->vinfo.bits_per_pixel);
        }
    }
}

// Improved Bresenham's Line Algorithm with edge cases handling
void draw_line(struct framebuffer_info* fb_info, int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (1) {
        set_pixel(fb_info, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;

        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

// Function to rotate 3D point
void rotate(Point3D* p, float angleX, float angleY, float angleZ) {
    // Rotation around X-axis
    float y = p->y * cos(angleX) - p->z * sin(angleX);
    float z = p->y * sin(angleX) + p->z * cos(angleX);
    p->y = y;
    p->z = z;

    // Rotation around Y-axis
    float x = p->x * cos(angleY) + p->z * sin(angleY);
    z = -p->x * sin(angleY) + p->z * cos(angleY);
    p->x = x;
    p->z = z;

    // Rotation around Z-axis
    x = p->x * cos(angleZ) - p->y * sin(angleZ);
    y = p->x * sin(angleZ) + p->y * cos(angleZ);
    p->x = x;
    p->y = y;
}

// Function to project 3D point onto 2D screen
void project(Point3D p, int* x2D, int* y2D, int screenWidth, int screenHeight, float dist) {
    float scale = dist / (p.z + dist);  // Perspective scaling
    *x2D = (int)(screenWidth / 2 + p.x * scale);
    *y2D = (int)(screenHeight / 2 + p.y * scale);

    // Ensure the projected point remains within screen bounds
    if (*x2D < 0) *x2D = 0;
    if (*x2D >= screenWidth) *x2D = screenWidth - 1;
    if (*y2D < 0) *y2D = 0;
    if (*y2D >= screenHeight) *y2D = screenHeight - 1;
}

// Main function
int main() {
    struct framebuffer_info fb_info = init_framebuffer("/dev/fb0");

    float angleX = 0, angleY = 0, angleZ = 0;
    float dist = 400.0f;

    while (1) {
        clear_screen(&fb_info);

        Point3D transformed[8];
        int projected[8][2];

        // Rotate and project each vertex
        for (int i = 0; i < 8; i++) {
            transformed[i] = cube[i];
            rotate(&transformed[i], angleX, angleY, angleZ);
            project(transformed[i], &projected[i][0], &projected[i][1], fb_info.vinfo.xres, fb_info.vinfo.yres, dist);
        }

        // Draw the cube edges
        for (int i = 0; i < 12; i++) {
            draw_line(&fb_info, projected[edges[i][0]][0], projected[edges[i][0]][1],
                      projected[edges[i][1]][0], projected[edges[i][1]][1], COLOR);
        }

        // Increment angles for slower rotation
        angleX += ROTATION_SPEED;
        angleY += ROTATION_SPEED * 0.5;
        angleZ += ROTATION_SPEED * 0.25;

        usleep(FRAME_DELAY);  // Slower frame rate for smoother rotation
    }

    munmap(fb_info.fb_ptr, fb_info.screensize);
    close(fb_info.fb_fd);
    return 0;
}

