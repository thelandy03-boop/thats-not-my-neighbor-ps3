#ifndef RENDERER_H
#define RENDERER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 720

// Initialize video output and Tiny3D at 720p (1280x720)
bool renderer_init(void);

// Shut down Tiny3D and free RSX video buffers
void renderer_cleanup(void);

// Clear the frame with a 32-bit RGBA color (0xRRGGBBAA)
void renderer_clear(uint32_t color);

// Draw a filled colored rectangle
void renderer_draw_rect(float x, float y, float w, float h, uint32_t color);

// Draw a textured sprite from cached/loaded PNG
void renderer_draw_sprite(const char* name, float x, float y, float w, float h);

// Draw a textured sprite with RGBA color modulation
void renderer_draw_sprite_tinted(const char* name, float x, float y, float w, float h, uint32_t tint_color);

// Draw a simple 8x8 bitmap font string for debug / numbers
void renderer_draw_text(float x, float y, const char* text, uint32_t color);

// Swap display buffers (VSync locked 60 FPS)
void renderer_flip(void);

// Resolves file paths across dev_hdd0, app_home, and relative paths
const char* renderer_resolve_path(const char* filepath, char* buffer, size_t buf_size);

#endif // RENDERER_H
