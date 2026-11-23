#include "renderer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO_DISABLED
#define STBI_ONLY_PNG
#include "stb_image.h"

#if defined(__CELLOS_LV2__) || defined(_PS3)
#include <tiny3d.h>
#include <sysutil/video.h>
#else
// Mock definitions for host-side compilation / static analysis
typedef uint32_t u32;
typedef uint8_t  u8;
#define TINY3D_QUADS 0
#define TINY3D_CLEAR_ALL 0
#define TEXTURE_LINEAR 0
#define TINY3D_TEX_FORMAT_A8R8G8B8 0
static inline int tiny3d_Init(u32 flags) { (void)flags; return 0; }
static inline void tiny3d_Exit(void) {}
static inline void tiny3d_Project2D(void) {}
static inline void tiny3d_Clear(u32 color, u32 clearflags) { (void)color; (void)clearflags; }
static inline void tiny3d_SetPolygon(int type) { (void)type; }
static inline void tiny3d_VertexPos(float x, float y, float z) { (void)x; (void)y; (void)z; }
static inline void tiny3d_VertexColor(u32 rgba) { (void)rgba; }
static inline void tiny3d_VertexTexture(float u, float v) { (void)u; (void)v; }
static inline void tiny3d_End(void) {}
static inline void tiny3d_Flip(void) {}
static inline u32 tiny3d_AllocTexture(u32 size) { (void)size; return 0; }
static inline void* tiny3d_TexturePointer(u32 offset) { (void)offset; return NULL; }
static inline void tiny3d_SetTexture(u32 unit, u32 offset, u32 width, u32 height, u32 stride, u32 format, int smooth) {
    (void)unit; (void)offset; (void)width; (void)height; (void)stride; (void)format; (void)smooth;
}
#endif

#define MAX_CACHED_TEXTURES 128

typedef struct {
    char name[64];
    uint32_t rsx_offset;
    void* host_pixels;
    int width;
    int height;
    bool valid;
} CachedTexture;

static CachedTexture s_texture_cache[MAX_CACHED_TEXTURES];
static int s_texture_count = 0;
static bool s_renderer_initialized = false;

const char* renderer_resolve_path(const char* filepath, char* buffer, size_t buf_size) {
    if (!filepath) return NULL;

    // Check direct relative path
    FILE* f = fopen(filepath, "rb");
    if (f) {
        fclose(f);
        return filepath;
    }

    // Check usrdir/assets/gfx/
    snprintf(buffer, buf_size, "usrdir/assets/gfx/%s", filepath);
    f = fopen(buffer, "rb");
    if (f) {
        fclose(f);
        return buffer;
    }

    // Check usrdir/assets/gfx/neighbors/
    snprintf(buffer, buf_size, "usrdir/assets/gfx/neighbors/%s", filepath);
    f = fopen(buffer, "rb");
    if (f) {
        fclose(f);
        return buffer;
    }

    // Check /app_home/usrdir/assets/gfx/
    snprintf(buffer, buf_size, "/app_home/usrdir/assets/gfx/%s", filepath);
    f = fopen(buffer, "rb");
    if (f) {
        fclose(f);
        return buffer;
    }

    // Check /app_home/usrdir/assets/gfx/neighbors/
    snprintf(buffer, buf_size, "/app_home/usrdir/assets/gfx/neighbors/%s", filepath);
    f = fopen(buffer, "rb");
    if (f) {
        fclose(f);
        return buffer;
    }

    // Check /dev_hdd0/game/TNMN00001/USRDIR/assets/gfx/
    snprintf(buffer, buf_size, "/dev_hdd0/game/TNMN00001/USRDIR/assets/gfx/%s", filepath);
    f = fopen(buffer, "rb");
    if (f) {
        fclose(f);
        return buffer;
    }

    // Check /dev_hdd0/game/TNMN00001/USRDIR/assets/gfx/neighbors/
    snprintf(buffer, buf_size, "/dev_hdd0/game/TNMN00001/USRDIR/assets/gfx/neighbors/%s", filepath);
    f = fopen(buffer, "rb");
    if (f) {
        fclose(f);
        return buffer;
    }

    return filepath;
}

bool renderer_init(void) {
    if (s_renderer_initialized) return true;

    printf("[Renderer] Initializing Tiny3D RSX 720p (1280x720)...\n");

    // Initialize Tiny3D with 2MB internal command buffer
    if (tiny3d_Init(1024 * 1024 * 2) < 0) {
        printf("[Renderer] Error: Failed to initialize Tiny3D!\n");
        return false;
    }

    memset(s_texture_cache, 0, sizeof(s_texture_cache));
    s_texture_count = 0;
    s_renderer_initialized = true;

    printf("[Renderer] Tiny3D initialized successfully.\n");
    return true;
}

static CachedTexture* get_or_load_texture(const char* name) {
    if (!name || name[0] == '\0') return NULL;

    // Check if texture is already loaded
    for (int i = 0; i < s_texture_count; i++) {
        if (s_texture_cache[i].valid && strncmp(s_texture_cache[i].name, name, 63) == 0) {
            return &s_texture_cache[i];
        }
    }

    if (s_texture_count >= MAX_CACHED_TEXTURES) {
        printf("[Renderer] Warning: Texture cache full (%d textures)\n", MAX_CACHED_TEXTURES);
        return NULL;
    }

    char resolved_path[512];
    const char* path = renderer_resolve_path(name, resolved_path, sizeof(resolved_path));

    int w = 0, h = 0, channels = 0;
    unsigned char* raw_rgba = stbi_load(path, &w, &h, &channels, 4);
    if (!raw_rgba) {
        printf("[Renderer] Failed to load PNG '%s' (Resolved: '%s', Reason: %s)\n",
               name, path, stbi_failure_reason());
        return NULL;
    }

    CachedTexture* tex = &s_texture_cache[s_texture_count++];
    strncpy(tex->name, name, 63);
    tex->name[63] = '\0';
    tex->width = w;
    tex->height = h;

    uint32_t texture_size = w * h * 4;

#if defined(__CELLOS_LV2__) || defined(_PS3)
    // Allocate RSX texture buffer (16-byte aligned)
    tex->rsx_offset = tiny3d_AllocTexture(texture_size);
    void* rsx_ptr = tiny3d_TexturePointer(tex->rsx_offset);

    if (rsx_ptr) {
        // Convert RGBA to ARGB (0xAARRGGBB) for Tiny3D RSX format
        uint32_t* dst = (uint32_t*)rsx_ptr;
        for (int i = 0; i < w * h; i++) {
            uint32_t r = raw_rgba[i * 4 + 0];
            uint32_t g = raw_rgba[i * 4 + 1];
            uint32_t b = raw_rgba[i * 4 + 2];
            uint32_t a = raw_rgba[i * 4 + 3];
            dst[i] = (a << 24) | (r << 16) | (g << 8) | b;
        }
        stbi_image_free(raw_rgba);
        tex->host_pixels = NULL;
    } else {
        tex->host_pixels = raw_rgba;
    }
#else
    tex->host_pixels = raw_rgba;
    tex->rsx_offset = 0;
#endif

    tex->valid = true;
    printf("[Renderer] Loaded & cached texture '%s' (%dx%d, %u bytes RSX)\n",
           name, w, h, texture_size);
    return tex;
}

void renderer_clear(uint32_t color) {
    tiny3d_Clear(color, TINY3D_CLEAR_ALL);
    tiny3d_Project2D();
}

void renderer_draw_rect(float x, float y, float w, float h, uint32_t color) {
    tiny3d_SetPolygon(TINY3D_QUADS);

    tiny3d_VertexPos(x, y, 65535.0f);
    tiny3d_VertexColor(color);

    tiny3d_VertexPos(x + w, y, 65535.0f);
    tiny3d_VertexColor(color);

    tiny3d_VertexPos(x + w, y + h, 65535.0f);
    tiny3d_VertexColor(color);

    tiny3d_VertexPos(x, y + h, 65535.0f);
    tiny3d_VertexColor(color);

    tiny3d_End();
}

void renderer_draw_sprite(const char* name, float x, float y, float w, float h) {
    renderer_draw_sprite_tinted(name, x, y, w, h, 0xFFFFFFFF);
}

void renderer_draw_sprite_tinted(const char* name, float x, float y, float w, float h, uint32_t tint_color) {
    CachedTexture* tex = get_or_load_texture(name);
    if (!tex || !tex->valid) {
        // Fallback: draw placeholder rectangle
        renderer_draw_rect(x, y, w, h, 0x444444FF);
        return;
    }

#if defined(__CELLOS_LV2__) || defined(_PS3)
    tiny3d_SetTexture(0, tex->rsx_offset, tex->width, tex->height, tex->width * 4,
                      TINY3D_TEX_FORMAT_A8R8G8B8, TEXTURE_LINEAR);
#endif

    tiny3d_SetPolygon(TINY3D_QUADS);

    tiny3d_VertexPos(x, y, 65535.0f);
    tiny3d_VertexColor(tint_color);
    tiny3d_VertexTexture(0.0f, 0.0f);

    tiny3d_VertexPos(x + w, y, 65535.0f);
    tiny3d_VertexColor(tint_color);
    tiny3d_VertexTexture(1.0f, 0.0f);

    tiny3d_VertexPos(x + w, y + h, 65535.0f);
    tiny3d_VertexColor(tint_color);
    tiny3d_VertexTexture(1.0f, 1.0f);

    tiny3d_VertexPos(x, y + h, 65535.0f);
    tiny3d_VertexColor(tint_color);
    tiny3d_VertexTexture(0.0f, 1.0f);

    tiny3d_End();
}

void renderer_draw_text(float x, float y, const char* text, uint32_t color) {
    if (!text) return;
    float cur_x = x;
    while (*text) {
        // Render simple 8x12 character blocks for debug strings
        if (*text != ' ') {
            renderer_draw_rect(cur_x, y, 8.0f, 12.0f, color);
        }
        cur_x += 10.0f;
        text++;
    }
}

void renderer_flip(void) {
    tiny3d_Flip();
}

void renderer_cleanup(void) {
    if (!s_renderer_initialized) return;

    for (int i = 0; i < s_texture_count; i++) {
        if (s_texture_cache[i].host_pixels) {
            stbi_image_free(s_texture_cache[i].host_pixels);
            s_texture_cache[i].host_pixels = NULL;
        }
        s_texture_cache[i].valid = false;
    }
    s_texture_count = 0;

    tiny3d_Exit();
    s_renderer_initialized = false;
    printf("[Renderer] Tiny3D subsystem cleaned up.\n");
}
