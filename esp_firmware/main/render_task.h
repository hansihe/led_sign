#ifndef _RENDER_TASK
#define _RENDER_TASK

#include "colors.h"

#define PIXEL_GRID_WIDTH 20
#define PIXEL_GRID_HEIGHT 6

void init_render_task();

void lockPixels();
void unlockPixels();

void updatePixelData(color_f3_t pixelData[PIXEL_GRID_WIDTH][PIXEL_GRID_HEIGHT]);
void setFadeLength(int fadeLengthN);

#endif
