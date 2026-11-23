#include "graphics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#define HAVE_SDL2 1
#elif __has_include(<SDL.h>)
#include <SDL.h>
#define HAVE_SDL2 1
#endif

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO_DISABLED
#define STBI_ONLY_PNG
#include "stb_image.h"

// 1920x1080 32-bit Framebuffer (~8.3 MB)
static uint32_t* s_framebuffer = NULL;

#ifdef HAVE_SDL2
static SDL_Window* s_sdl_window = NULL;
static SDL_Renderer* s_sdl_renderer = NULL;
static SDL_Texture* s_sdl_texture = NULL;
#endif

static const char* resolve_asset_path(const char* filepath, char* buffer, size_t buf_size) {
    if (!filepath) return NULL;
    FILE* f = fopen(filepath, "rb");
    if (f) {
        fclose(f);
        return filepath;
    }
    snprintf(buffer, buf_size, "/app_home/%s", filepath);
    f = fopen(buffer, "rb");
    if (f) {
        fclose(f);
        return buffer;
    }
    snprintf(buffer, buf_size, "/dev_hdd0/game/TNMN00001/USRDIR/%s", filepath);
    f = fopen(buffer, "rb");
    if (f) {
        fclose(f);
        return buffer;
    }
    return filepath;
}

static inline uint32_t blend_pixel(uint32_t dst, uint32_t src) {
    uint32_t a = (src >> 24) & 0xFF;
    if (a == 0) return dst;
    if (a == 255) return src;

    uint32_t dst_r = (dst >> 16) & 0xFF;
    uint32_t dst_g = (dst >> 8) & 0xFF;
    uint32_t dst_b = dst & 0xFF;

    uint32_t src_r = (src >> 16) & 0xFF;
    uint32_t src_g = (src >> 8) & 0xFF;
    uint32_t src_b = src & 0xFF;

    uint32_t out_r = (src_r * a + dst_r * (255 - a)) / 255;
    uint32_t out_g = (src_g * a + dst_g * (255 - a)) / 255;
    uint32_t out_b = (src_b * a + dst_b * (255 - a)) / 255;

    return (0xFF000000) | (out_r << 16) | (out_g << 8) | out_b;
}

bool graphics_init(void) {
    if (!s_framebuffer) {
        s_framebuffer = (uint32_t*)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
        if (!s_framebuffer) {
            printf("[Graphics] Failed to allocate framebuffer memory.\n");
            return false;
        }
    }
    graphics_clear(0xFF181410);

#ifdef HAVE_SDL2
    if (SDL_Init(SDL_INIT_VIDEO) == 0) {
        s_sdl_window = SDL_CreateWindow("That's Not My Neighbor", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        if (s_sdl_window) {
            s_sdl_renderer = SDL_CreateRenderer(s_sdl_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
            if (s_sdl_renderer) {
                s_sdl_texture = SDL_CreateTexture(s_sdl_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
            }
        }
        printf("[Graphics] SDL2 Video subsystem initialized.\n");
    }
#endif

    printf("[Graphics] Native 1080p framebuffer (1920x1080) initialized successfully.\n");
    return true;
}

void graphics_clear(uint32_t color) {
    if (!s_framebuffer) return;
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        s_framebuffer[i] = color;
    }
}

void graphics_draw_rect(int x, int y, int w, int h, uint32_t color) {
    if (!s_framebuffer) return;

    int x1 = (x < 0) ? 0 : x;
    int y1 = (y < 0) ? 0 : y;
    int x2 = (x + w > SCREEN_WIDTH) ? SCREEN_WIDTH : (x + w);
    int y2 = (y + h > SCREEN_HEIGHT) ? SCREEN_HEIGHT : (y + h);

    if (x1 >= x2 || y1 >= y2) return;

    for (int row = y1; row < y2; row++) {
        uint32_t* dst_row = &s_framebuffer[row * SCREEN_WIDTH];
        for (int col = x1; col < x2; col++) {
            dst_row[col] = blend_pixel(dst_row[col], color);
        }
    }
}

void graphics_draw_cursor(float x, float y, uint32_t color) {
    int cx = (int)x;
    int cy = (int)y;
    graphics_draw_rect(cx - 10, cy - 2, 21, 5, 0xFF000000); // Black shadow
    graphics_draw_rect(cx - 2, cy - 10, 5, 21, 0xFF000000);
    graphics_draw_rect(cx - 8, cy - 1, 17, 3, color);      // White crosshair
    graphics_draw_rect(cx - 1, cy - 8, 3, 17, color);
}

void graphics_draw_text(int x, int y, const char* text, uint32_t color) {
    // Basic debug text rasterizer stub
    if (!text) return;
    int len = strlen(text);
    graphics_draw_rect(x - 2, y - 2, (len * 11) + 4, 18, 0x88000000); // Label backing
}

Texture* texture_load_from_file(const char* filepath) {
    char path_buf[512];
    const char* resolved_path = resolve_asset_path(filepath, path_buf, sizeof(path_buf));

    int w, h, channels;
    unsigned char* data = stbi_load(resolved_path, &w, &h, &channels, 4);
    if (!data) {
        printf("[Graphics] Failed to load PNG texture: %s (Reason: %s)\n", resolved_path, stbi_failure_reason());
        return NULL;
    }

    Texture* tex = (Texture*)malloc(sizeof(Texture));
    tex->width = w;
    tex->height = h;
    tex->channels = 4;
    tex->pixels = (uint32_t*)malloc(w * h * sizeof(uint32_t));

    // Convert RGBA bytes to 32-bit ARGB format
    for (int i = 0; i < w * h; i++) {
        uint32_t r = data[i * 4 + 0];
        uint32_t g = data[i * 4 + 1];
        uint32_t b = data[i * 4 + 2];
        uint32_t a = data[i * 4 + 3];
        tex->pixels[i] = (a << 24) | (r << 16) | (g << 8) | b;
    }

    stbi_image_free(data);
    printf("[Graphics] Loaded PNG texture: %s (%dx%d)\n", resolved_path, w, h);
    return tex;
}

void texture_free(Texture* tex) {
    if (!tex) return;
    if (tex->pixels) free(tex->pixels);
    free(tex);
}

void graphics_draw_texture(const Texture* tex, int x, int y) {
    if (!tex || !tex->pixels || !s_framebuffer) return;

    int src_x1 = 0;
    int src_y1 = 0;
    int dst_x1 = x;
    int dst_y1 = y;

    if (dst_x1 < 0) {
        src_x1 = -dst_x1;
        dst_x1 = 0;
    }
    if (dst_y1 < 0) {
        src_y1 = -dst_y1;
        dst_y1 = 0;
    }

    int draw_w = tex->width - src_x1;
    int draw_h = tex->height - src_y1;

    if (dst_x1 + draw_w > SCREEN_WIDTH)  draw_w = SCREEN_WIDTH - dst_x1;
    if (dst_y1 + draw_h > SCREEN_HEIGHT) draw_h = SCREEN_HEIGHT - dst_y1;

    if (draw_w <= 0 || draw_h <= 0) return;

    for (int row = 0; row < draw_h; row++) {
        uint32_t* dst_ptr = &s_framebuffer[(dst_y1 + row) * SCREEN_WIDTH + dst_x1];
        const uint32_t* src_ptr = &tex->pixels[(src_y1 + row) * tex->width + src_x1];
        for (int col = 0; col < draw_w; col++) {
            dst_ptr[col] = blend_pixel(dst_ptr[col], src_ptr[col]);
        }
    }
}

void graphics_draw_texture_scaled(const Texture* tex, int x, int y, int dest_w, int dest_h) {
    if (!tex || !tex->pixels || !s_framebuffer || dest_w <= 0 || dest_h <= 0) return;

    for (int dy = 0; dy < dest_h; dy++) {
        int py = y + dy;
        if (py < 0 || py >= SCREEN_HEIGHT) continue;

        int sy = (dy * tex->height) / dest_h;
        const uint32_t* src_row = &tex->pixels[sy * tex->width];
        uint32_t* dst_row = &s_framebuffer[py * SCREEN_WIDTH];

        for (int dx = 0; dx < dest_w; dx++) {
            int px = x + dx;
            if (px < 0 || px >= SCREEN_WIDTH) continue;

            int sx = (dx * tex->width) / dest_w;
            dst_row[px] = blend_pixel(dst_row[px], src_row[sx]);
        }
    }
}

void graphics_draw_texture_clipped(const Texture* tex, int x, int y, int clip_y, int clip_h) {
    if (!tex || !tex->pixels || !s_framebuffer || clip_h <= 0) return;

    int start_y = (clip_y < 0) ? 0 : clip_y;
    int end_y = ((clip_y + clip_h) > tex->height) ? tex->height : (clip_y + clip_h);

    for (int row = start_y; row < end_y; row++) {
        int py = y + row;
        if (py < 0 || py >= SCREEN_HEIGHT) continue;

        uint32_t* dst_ptr = &s_framebuffer[py * SCREEN_WIDTH];
        const uint32_t* src_ptr = &tex->pixels[row * tex->width];

        for (int col = 0; col < tex->width; col++) {
            int px = x + col;
            if (px < 0 || px >= SCREEN_WIDTH) continue;
            dst_ptr[px] = blend_pixel(dst_ptr[px], src_ptr[col]);
        }
    }
}

void graphics_flip(void) {
#ifdef HAVE_SDL2
    if (s_sdl_renderer && s_sdl_texture && s_framebuffer) {
        SDL_UpdateTexture(s_sdl_texture, NULL, s_framebuffer, SCREEN_WIDTH * sizeof(uint32_t));
        SDL_RenderClear(s_sdl_renderer);
        SDL_RenderCopy(s_sdl_renderer, s_sdl_texture, NULL, NULL);
        SDL_RenderPresent(s_sdl_renderer);
    }
#endif
}

void graphics_cleanup(void) {
#ifdef HAVE_SDL2
    if (s_sdl_texture)  { SDL_DestroyTexture(s_sdl_texture);   s_sdl_texture = NULL; }
    if (s_sdl_renderer) { SDL_DestroyRenderer(s_sdl_renderer); s_sdl_renderer = NULL; }
    if (s_sdl_window)   { SDL_DestroyWindow(s_sdl_window);     s_sdl_window = NULL; }
    SDL_Quit();
#endif

    if (s_framebuffer) {
        free(s_framebuffer);
        s_framebuffer = NULL;
    }
    printf("[Graphics] Framebuffer cleaned up successfully.\n");
}
