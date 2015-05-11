#ifndef SHAPE_DRAWER
#define SHAPE_DRAWER

#include "constants.h"

void move_buffer(char (*buffer)[SLICES][WIDTH]);

void draw_circle(float x, float y, float r, float h, bool fill);
void draw_point(float x1, float y1, float z1, float r);
void draw_line(float x1, float y1, float z1, float x2, float y2, float z2);

void set_hfunc(int (*h)(float d));

#endif