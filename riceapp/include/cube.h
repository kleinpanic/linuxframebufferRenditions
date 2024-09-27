#ifndef CUBE_H
#define CUBE_H

typedef struct {
    float x, y, z;
} Vertex;

typedef struct {
    int width, height;
} Screen;

// Function declarations
void init_framebuffer();
void draw_cube(Vertex vertices[8], Screen *screen);
void translate(Vertex *v, float dx, float dy, float dz);
void rotate_cube(Vertex vertices[8], float angleX, float angleY);
void update_screen(Screen *screen);
void handle_collision(Vertex *v, Screen *screen);

#endif

