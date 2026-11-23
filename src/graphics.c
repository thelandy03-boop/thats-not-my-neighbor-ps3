#include "graphics.h"
#include <stdio.h>

bool graphics_init(void) {
    printf("[Graphics] Framebuffer / RSX initialized (1280x720).\n");
    return true;
}

void graphics_clear(uint32_t color) {
    // Fill framebuffer with background color
}

void graphics_draw_rect(int x, int y, int w, int h, uint32_t color) {
    // Draw solid color rectangle
}

void graphics_draw_cursor(float x, float y, uint32_t color) {
    // Draw crosshair or pointer on desk screen
    graphics_draw_rect((int)x - 6, (int)y - 1, 13, 3, color);
    graphics_draw_rect((int)x - 1, (int)y - 6, 3, 13, color);
}

void graphics_draw_text(int x, int y, const char* text, uint32_t color) {
    // Render text string
}

void graphics_flip(void) {
    // Swap front/back buffers & vsync
}

void graphics_cleanup(void) {
    printf("[Graphics] Cleanup completed.\n");
}
