#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "common.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int width;
    int height;
    int channels;
    uint32_t* pixels; // 32-bit ARGB/RGBA format
} Texture;

bool graphics_init(void);
void graphics_clear(uint32_t color);
void graphics_draw_rect(int x, int y, int w, int h, uint32_t color);
void graphics_draw_cursor(float x, float y, uint32_t color);
void graphics_draw_text(int x, int y, const char* text, uint32_t color);
void graphics_flip(void);
void graphics_cleanup(void);

// PNG Texture API powered by stb_image
Texture* texture_load_from_file(const char* filepath);
void texture_free(Texture* tex);
void graphics_draw_texture(const Texture* tex, int x, int y);
void graphics_draw_texture_scaled(const Texture* tex, int x, int y, int dest_w, int dest_h);
void graphics_draw_texture_clipped(const Texture* tex, int x, int y, int clip_y, int clip_h);

#endif // GRAPHICS_H
