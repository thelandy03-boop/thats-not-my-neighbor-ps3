#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "common.h"
#include <stdint.h>

bool graphics_init(void);
void graphics_clear(uint32_t color);
void graphics_draw_rect(int x, int y, int w, int h, uint32_t color);
void graphics_draw_cursor(float x, float y, uint32_t color);
void graphics_draw_text(int x, int y, const char* text, uint32_t color);
void graphics_flip(void);
void graphics_cleanup(void);

#endif // GRAPHICS_H
