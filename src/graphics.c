#include "graphics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>

#include <rsx/rsx.h>
#include <rsx/gcm_sys.h>
#include <sysutil/video.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO_DISABLED
#define STBI_ONLY_PNG
#include "stb_image.h"

#define CB_SIZE     (0x10000)
#define HOST_SIZE   (1024 * 1024)

static gcmContextData* s_gcm_context = NULL;
static uint32_t* s_display_buffers[2] = {NULL, NULL};
static uint32_t s_buffer_offsets[2] = {0, 0};
static int s_curr_buffer = 0;

// Active drawing pointer (points to current backbuffer)
static uint32_t* s_framebuffer = NULL;

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
    void* host_addr = memalign(1024 * 1024, HOST_SIZE);
    if (!host_addr) {
        printf("[Graphics] Failed to allocate RSX host memory.\n");
        return false;
    }

    if (rsxInit(&s_gcm_context, CB_SIZE, HOST_SIZE, host_addr) != 0) {
        printf("[Graphics] Failed to initialize RSX.\n");
        return false;
    }

    videoState state;
    if (videoGetState(0, 0, &state) != 0) {
        printf("[Graphics] Failed to get video state.\n");
        return false;
    }

    videoConfiguration vconfig;
    memset(&vconfig, 0, sizeof(videoConfiguration));
    vconfig.resolution = VIDEO_RESOLUTION_1080;
#ifdef VIDEO_BUFFER_FORMAT_XRGB
    vconfig.format = VIDEO_BUFFER_FORMAT_XRGB;
#else
    vconfig.format = 0;
#endif
    vconfig.pitch = SCREEN_WIDTH * 4;
    vconfig.aspect = VIDEO_ASPECT_16_9;

    if (videoConfigure(0, &vconfig, NULL, 0) != 0) {
        // Fallback to 720p if 1080p is unavailable
        vconfig.resolution = VIDEO_RESOLUTION_720;
        videoConfigure(0, &vconfig, NULL, 0);
    }

    videoGetState(0, 0, &state);
    gcmSetFlipMode(GCM_FLIP_VSYNC);

    // Allocate 2 framebuffers in RSX video memory
    uint32_t buffer_size = SCREEN_WIDTH * SCREEN_HEIGHT * 4;
    for (int i = 0; i < 2; i++) {
        s_display_buffers[i] = (uint32_t*)rsxMemalign(64, buffer_size);
        if (!s_display_buffers[i]) {
            printf("[Graphics] Failed to allocate RSX display buffer %d.\n", i);
            return false;
        }
        rsxAddressToOffset(s_display_buffers[i], &s_buffer_offsets[i]);
        gcmSetDisplayBuffer(i, s_buffer_offsets[i], SCREEN_WIDTH * 4, SCREEN_WIDTH, SCREEN_HEIGHT);
    }

    gcmSetWaitFlip(s_gcm_context);
    s_curr_buffer = 0;
    s_framebuffer = s_display_buffers[s_curr_buffer];

    graphics_clear(0xFF181410);
    printf("[Graphics] Native PSL1GHT RSX 1080p Double-Buffer initialized successfully.\n");
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
    graphics_draw_rect(cx - 10, cy - 2, 21, 5, 0xFF000000);
    graphics_draw_rect(cx - 2, cy - 10, 5, 21, 0xFF000000);
    graphics_draw_rect(cx - 8, cy - 1, 17, 3, color);
    graphics_draw_rect(cx - 1, cy - 8, 3, 17, color);
}

void graphics_draw_text(int x, int y, const char* text, uint32_t color) {
    if (!text) return;
    int len = strlen(text);
    graphics_draw_rect(x - 2, y - 2, (len * 11) + 4, 18, 0x88000000);
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
    if (!tex || !tex->pixels || !s_framebuffer) return;
    if (dest_w <= 0 || dest_h <= 0) return;

    for (int dy = 0; dy < dest_h; dy++) {
        int py = y + dy;
        if (py < 0 || py >= SCREEN_HEIGHT) continue;

        int sy = (dy * tex->height) / dest_h;
        uint32_t* dst_ptr = &s_framebuffer[py * SCREEN_WIDTH];
        const uint32_t* src_ptr = &tex->pixels[sy * tex->width];

        for (int dx = 0; dx < dest_w; dx++) {
            int px = x + dx;
            if (px < 0 || px >= SCREEN_WIDTH) continue;
            int sx = (dx * tex->width) / dest_w;
            dst_ptr[px] = blend_pixel(dst_ptr[px], src_ptr[sx]);
        }
    }
}

void graphics_draw_texture_clipped(const Texture* tex, int x, int y, int clip_y, int clip_h) {
    if (!tex || !tex->pixels || !s_framebuffer) return;
    if (clip_h <= 0) return;

    int start_y = (clip_y < 0) ? 0 : clip_y;
    int end_y = ((clip_y + clip_h) > tex->height) ? tex->height : (clip_y + clip_h);

    for (int row = start_y; row < end_y; row++) {
        int py = y + (row - start_y);
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
    if (!s_gcm_context) return;

    gcmSetFlip(s_gcm_context, s_curr_buffer);
    rsxFlushBuffer(s_gcm_context);
    gcmSetWaitFlip(s_gcm_context);

    // Swap to the next buffer
    s_curr_buffer = !s_curr_buffer;
    s_framebuffer = s_display_buffers[s_curr_buffer];
}

void graphics_cleanup(void) {
    printf("[Graphics] RSX subsystem cleaned up.\n");
}
