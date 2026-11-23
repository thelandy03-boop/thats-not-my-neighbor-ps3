// That's Not My Neighbor - PS3 Demake (PC Desktop Target)
// High-performance local development & testing runner using SDL2 & Embedded Lua 5.4

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include <SDL.h>
#include <SDL_test_font.h>
#include <vorbis/vorbisfile.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#define WINDOW_WIDTH  1280
#define WINDOW_HEIGHT 720
#define MAX_TEXTURES  256
#define MAX_SOUNDS    64
#define MAX_VOICES    16

// -------------------------------------------------------------
// Texture Cache (SDL2 + stb_image)
// -------------------------------------------------------------
typedef struct {
    char name[128];
    SDL_Texture* texture;
    int width;
    int height;
    bool valid;
} DesktopTexture;

static DesktopTexture s_textures[MAX_TEXTURES];
static int s_texture_count = 0;
static SDL_Renderer* s_renderer = NULL;

static const char* resolve_asset_path(const char* filepath, char* buffer, size_t buf_size) {
    if (!filepath || !filepath[0]) return "";
    FILE* f = fopen(filepath, "rb");
    if (f) { fclose(f); return filepath; }

    snprintf(buffer, buf_size, "usrdir/assets/gfx/%s", filepath);
    f = fopen(buffer, "rb");
    if (f) { fclose(f); return buffer; }

    snprintf(buffer, buf_size, "usrdir/assets/gfx/neighbors/%s", filepath);
    f = fopen(buffer, "rb");
    if (f) { fclose(f); return buffer; }

    snprintf(buffer, buf_size, "usrdir/assets/%s", filepath);
    f = fopen(buffer, "rb");
    if (f) { fclose(f); return buffer; }

    return filepath;
}

static DesktopTexture* get_or_load_texture(const char* name) {
    if (!name || !name[0]) return NULL;

    for (int i = 0; i < s_texture_count; i++) {
        if (s_textures[i].valid && strcmp(s_textures[i].name, name) == 0) {
            return &s_textures[i];
        }
    }

    if (s_texture_count >= MAX_TEXTURES) {
        printf("[Desktop Renderer] Texture cache full!\n");
        return NULL;
    }

    char resolved_path[512];
    const char* path = resolve_asset_path(name, resolved_path, sizeof(resolved_path));

    int w = 0, h = 0, channels = 0;
    unsigned char* raw_rgba = stbi_load(path, &w, &h, &channels, 4);
    if (!raw_rgba) {
        // Warning suppressed for missing sprites fallback
        return NULL;
    }

    SDL_Texture* tex = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_RGBA32,
                                         SDL_TEXTUREACCESS_STATIC, w, h);
    if (!tex) {
        stbi_image_free(raw_rgba);
        return NULL;
    }

    SDL_UpdateTexture(tex, NULL, raw_rgba, w * 4);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    stbi_image_free(raw_rgba);

    DesktopTexture* entry = &s_textures[s_texture_count++];
    strncpy(entry->name, name, sizeof(entry->name) - 1);
    entry->name[sizeof(entry->name) - 1] = '\0';
    entry->texture = tex;
    entry->width = w;
    entry->height = h;
    entry->valid = true;

    return entry;
}

// -------------------------------------------------------------
// Audio Subsystem (SDL2 Built-in Mixer + Ogg Vorbis)
// -------------------------------------------------------------
typedef struct {
    char name[128];
    int16_t* samples;
    uint32_t frame_count;
    bool valid;
} CachedSound;

typedef struct {
    int16_t* samples;
    uint32_t frame_count;
    uint32_t position;
    bool active;
    bool loop;
    float volume;
} AudioVoice;

static CachedSound s_sounds[MAX_SOUNDS];
static int s_sound_count = 0;
static AudioVoice s_voices[MAX_VOICES];
static AudioVoice s_bgm;
static SDL_AudioDeviceID s_audio_dev = 0;

static const char* resolve_audio_path(const char* filepath, char* buffer, size_t buf_size) {
    if (!filepath || !filepath[0]) return "";
    FILE* f = fopen(filepath, "rb");
    if (f) { fclose(f); return filepath; }

    snprintf(buffer, buf_size, "usrdir/assets/audio/%s", filepath);
    f = fopen(buffer, "rb");
    if (f) { fclose(f); return buffer; }

    snprintf(buffer, buf_size, "usrdir/assets/%s", filepath);
    f = fopen(buffer, "rb");
    if (f) { fclose(f); return buffer; }

    return filepath;
}

static int16_t* decode_ogg(const char* path, uint32_t* out_frames) {
    OggVorbis_File vf;
    if (ov_fopen(path, &vf) < 0) {
        return NULL;
    }

    vorbis_info* vi = ov_info(&vf, -1);
    int channels = vi->channels;
    long rate = vi->rate;

    ogg_int64_t total_samples = ov_pcm_total(&vf, -1);
    if (total_samples <= 0) total_samples = 44100 * 30; // fallback buffer

    size_t alloc_bytes = (size_t)total_samples * channels * sizeof(int16_t);
    int16_t* raw_buf = (int16_t*)malloc(alloc_bytes);
    if (!raw_buf) {
        ov_clear(&vf);
        return NULL;
    }

    size_t bytes_read = 0;
    int bitstream = 0;
    while (bytes_read < alloc_bytes) {
        long ret = ov_read(&vf, (char*)raw_buf + bytes_read, (int)(alloc_bytes - bytes_read), 0, 2, 1, &bitstream);
        if (ret <= 0) break;
        bytes_read += ret;
    }
    ov_clear(&vf);

    uint32_t read_frames = (uint32_t)(bytes_read / (sizeof(int16_t) * channels));

    // Resample / convert to 44100Hz stereo if needed
    if (rate == 44100 && channels == 2) {
        *out_frames = read_frames;
        return raw_buf;
    }

    SDL_AudioCVT cvt;
    if (SDL_BuildAudioCVT(&cvt, AUDIO_S16SYS, (Uint8)channels, (int)rate, AUDIO_S16SYS, 2, 44100) < 0) {
        *out_frames = read_frames;
        return raw_buf;
    }

    cvt.len = (int)bytes_read;
    cvt.buf = (Uint8*)malloc(cvt.len * cvt.len_mult);
    memcpy(cvt.buf, raw_buf, bytes_read);
    free(raw_buf);

    if (SDL_ConvertAudio(&cvt) < 0) {
        free(cvt.buf);
        return NULL;
    }

    *out_frames = (uint32_t)(cvt.len_cvt / (sizeof(int16_t) * 2));
    return (int16_t*)cvt.buf;
}

static int16_t* decode_wav(const char* path, uint32_t* out_frames) {
    SDL_AudioSpec wav_spec;
    Uint32 wav_len = 0;
    Uint8* wav_buf = NULL;

    if (!SDL_LoadWAV(path, &wav_spec, &wav_buf, &wav_len)) {
        return NULL;
    }

    SDL_AudioCVT cvt;
    if (SDL_BuildAudioCVT(&cvt, wav_spec.format, wav_spec.channels, wav_spec.freq,
                          AUDIO_S16SYS, 2, 44100) < 0) {
        free(wav_buf);
        return NULL;
    }

    cvt.len = (int)wav_len;
    cvt.buf = (Uint8*)malloc(cvt.len * cvt.len_mult);
    memcpy(cvt.buf, wav_buf, wav_len);
    SDL_FreeWAV(wav_buf);

    if (SDL_ConvertAudio(&cvt) < 0) {
        free(cvt.buf);
        return NULL;
    }

    *out_frames = (uint32_t)(cvt.len_cvt / (sizeof(int16_t) * 2));
    return (int16_t*)cvt.buf;
}

static CachedSound* get_or_load_sound(const char* name) {
    if (!name || !name[0]) return NULL;

    for (int i = 0; i < s_sound_count; i++) {
        if (s_sounds[i].valid && strcmp(s_sounds[i].name, name) == 0) {
            return &s_sounds[i];
        }
    }

    if (s_sound_count >= MAX_SOUNDS) return NULL;

    char resolved[512];
    const char* path = resolve_audio_path(name, resolved, sizeof(resolved));

    int16_t* samples = NULL;
    uint32_t frames = 0;

    const char* dot = strrchr(path, '.');
    if (dot && (strcasecmp(dot, ".ogg") == 0)) {
        samples = decode_ogg(path, &frames);
    } else {
        samples = decode_wav(path, &frames);
    }

    if (!samples || frames == 0) {
        return NULL;
    }

    CachedSound* snd = &s_sounds[s_sound_count++];
    strncpy(snd->name, name, sizeof(snd->name) - 1);
    snd->name[sizeof(snd->name) - 1] = '\0';
    snd->samples = samples;
    snd->frame_count = frames;
    snd->valid = true;

    return snd;
}

static void audio_callback(void* userdata, Uint8* stream, int len) {
    (void)userdata;
    int16_t* out = (int16_t*)stream;
    int frames = len / (int)(sizeof(int16_t) * 2);

    memset(stream, 0, len);

    // 1. Mix BGM
    if (s_bgm.active && s_bgm.samples && s_bgm.frame_count > 0) {
        for (int i = 0; i < frames; i++) {
            if (s_bgm.position >= s_bgm.frame_count) {
                if (s_bgm.loop) {
                    s_bgm.position = 0;
                } else {
                    s_bgm.active = false;
                    break;
                }
            }
            int32_t left = (int32_t)(s_bgm.samples[s_bgm.position * 2 + 0] * s_bgm.volume);
            int32_t right = (int32_t)(s_bgm.samples[s_bgm.position * 2 + 1] * s_bgm.volume);
            out[i * 2 + 0] += (int16_t)left;
            out[i * 2 + 1] += (int16_t)right;
            s_bgm.position++;
        }
    }

    // 2. Mix Active SFX Voices
    for (int v = 0; v < MAX_VOICES; v++) {
        if (!s_voices[v].active || !s_voices[v].samples) continue;

        for (int i = 0; i < frames; i++) {
            if (s_voices[v].position >= s_voices[v].frame_count) {
                s_voices[v].active = false;
                break;
            }

            int32_t l = out[i * 2 + 0] + (int32_t)(s_voices[v].samples[s_voices[v].position * 2 + 0] * s_voices[v].volume);
            int32_t r = out[i * 2 + 1] + (int32_t)(s_voices[v].samples[s_voices[v].position * 2 + 1] * s_voices[v].volume);

            if (l > 32767) l = 32767;
            if (l < -32768) l = -32768;
            if (r > 32767) r = 32767;
            if (r < -32768) r = -32768;

            out[i * 2 + 0] = (int16_t)l;
            out[i * 2 + 1] = (int16_t)r;

            s_voices[v].position++;
        }
    }
}

static void audio_init(void) {
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = 44100;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 1024;
    want.callback = audio_callback;

    s_audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (s_audio_dev > 0) {
        SDL_PauseAudioDevice(s_audio_dev, 0);
        printf("[Desktop Audio] Opened 44100Hz 16-bit Stereo Audio Output.\n");
    } else {
        printf("[Desktop Audio] Audio device failed to open: %s\n", SDL_GetError());
    }
}

// -------------------------------------------------------------
// Input State (Keyboard + Mouse + Gamepad)
// -------------------------------------------------------------
typedef struct {
    bool cross;
    bool circle;
    bool square;
    bool triangle;
    bool up;
    bool down;
    bool left;
    bool right;
    bool l1;
    bool r1;
    bool l2;
    bool r2;
    bool start;
    bool select;
    float analog_lx;
    float analog_ly;
    float analog_rx;
    float analog_ry;
    bool mouse_moved;
    float mouse_x;
    float mouse_y;
} DesktopPadState;

static DesktopPadState s_pad;
static bool s_running = true;
static SDL_GameController* s_controller = NULL;

static void update_desktop_input(void) {
    s_pad.mouse_moved = false;

    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT) {
            s_running = false;
        } else if (ev.type == SDL_MOUSEMOTION) {
            s_pad.mouse_moved = true;
            s_pad.mouse_x = (float)ev.motion.x;
            s_pad.mouse_y = (float)ev.motion.y;
        } else if (ev.type == SDL_MOUSEBUTTONDOWN) {
            if (ev.button.button == SDL_BUTTON_LEFT)  s_pad.cross = true;
            if (ev.button.button == SDL_BUTTON_RIGHT) s_pad.circle = true;
        } else if (ev.type == SDL_MOUSEBUTTONUP) {
            if (ev.button.button == SDL_BUTTON_LEFT)  s_pad.cross = false;
            if (ev.button.button == SDL_BUTTON_RIGHT) s_pad.circle = false;
        } else if (ev.type == SDL_CONTROLLERDEVICEADDED) {
            if (!s_controller) {
                s_controller = SDL_GameControllerOpen(ev.cdevice.which);
                if (s_controller) {
                    printf("[Desktop Controller] Connected: %s\n", SDL_GameControllerName(s_controller));
                }
            }
        } else if (ev.type == SDL_CONTROLLERDEVICEREMOVED) {
            if (s_controller && ev.cdevice.which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(s_controller))) {
                SDL_GameControllerClose(s_controller);
                s_controller = NULL;
                printf("[Desktop Controller] Disconnected.\n");
            }
        }
    }

    // Keyboard poll
    const Uint8* kb = SDL_GetKeyboardState(NULL);

    s_pad.up    = kb[SDL_SCANCODE_UP] || kb[SDL_SCANCODE_W];
    s_pad.down  = kb[SDL_SCANCODE_DOWN] || kb[SDL_SCANCODE_S];
    s_pad.left  = kb[SDL_SCANCODE_LEFT] || kb[SDL_SCANCODE_A];
    s_pad.right = kb[SDL_SCANCODE_RIGHT] || kb[SDL_SCANCODE_D];

    if (kb[SDL_SCANCODE_RETURN] || kb[SDL_SCANCODE_SPACE]) s_pad.cross = true;
    if (kb[SDL_SCANCODE_ESCAPE] || kb[SDL_SCANCODE_BACKSPACE]) s_pad.circle = true;
    s_pad.square   = kb[SDL_SCANCODE_Q] || kb[SDL_SCANCODE_1];
    s_pad.triangle = kb[SDL_SCANCODE_E] || kb[SDL_SCANCODE_3];
    s_pad.l1       = kb[SDL_SCANCODE_TAB] || kb[SDL_SCANCODE_LSHIFT];
    s_pad.r1       = kb[SDL_SCANCODE_RSHIFT] || kb[SDL_SCANCODE_SPACE];
    s_pad.start    = kb[SDL_SCANCODE_F1] || kb[SDL_SCANCODE_P];
    s_pad.select   = kb[SDL_SCANCODE_F2] || kb[SDL_SCANCODE_M];

    // Analog sticks from keyboard arrows / WASD
    s_pad.analog_lx = 0.0f;
    s_pad.analog_ly = 0.0f;
    if (s_pad.left)  s_pad.analog_lx -= 1.0f;
    if (s_pad.right) s_pad.analog_lx += 1.0f;
    if (s_pad.up)    s_pad.analog_ly -= 1.0f;
    if (s_pad.down)  s_pad.analog_ly += 1.0f;

    // Gamepad merge
    if (s_controller) {
        if (SDL_GameControllerGetButton(s_controller, SDL_CONTROLLER_BUTTON_A)) s_pad.cross = true;
        if (SDL_GameControllerGetButton(s_controller, SDL_CONTROLLER_BUTTON_B)) s_pad.circle = true;
        if (SDL_GameControllerGetButton(s_controller, SDL_CONTROLLER_BUTTON_X)) s_pad.square = true;
        if (SDL_GameControllerGetButton(s_controller, SDL_CONTROLLER_BUTTON_Y)) s_pad.triangle = true;
        if (SDL_GameControllerGetButton(s_controller, SDL_CONTROLLER_BUTTON_DPAD_UP)) s_pad.up = true;
        if (SDL_GameControllerGetButton(s_controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN)) s_pad.down = true;
        if (SDL_GameControllerGetButton(s_controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT)) s_pad.left = true;
        if (SDL_GameControllerGetButton(s_controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) s_pad.right = true;
        if (SDL_GameControllerGetButton(s_controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER)) s_pad.l1 = true;
        if (SDL_GameControllerGetButton(s_controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)) s_pad.r1 = true;
        if (SDL_GameControllerGetButton(s_controller, SDL_CONTROLLER_BUTTON_START)) s_pad.start = true;
        if (SDL_GameControllerGetButton(s_controller, SDL_CONTROLLER_BUTTON_BACK)) s_pad.select = true;

        Sint16 lx = SDL_GameControllerGetAxis(s_controller, SDL_CONTROLLER_AXIS_LEFTX);
        Sint16 ly = SDL_GameControllerGetAxis(s_controller, SDL_CONTROLLER_AXIS_LEFTY);
        if (abs(lx) > 3200) s_pad.analog_lx = (float)lx / 32767.0f;
        if (abs(ly) > 3200) s_pad.analog_ly = (float)ly / 32767.0f;
    }
}

// -------------------------------------------------------------
// Lua Bridge Bindings (100% Identical to PS3 Native Bridge)
// -------------------------------------------------------------
static int l_draw_sprite(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    float x = (float)luaL_checknumber(L, 2);
    float y = (float)luaL_checknumber(L, 3);
    float w = (float)luaL_optnumber(L, 4, 100.0);
    float h = (float)luaL_optnumber(L, 5, 100.0);

    DesktopTexture* tex = get_or_load_texture(name);
    if (tex && tex->texture) {
        SDL_Rect dst = { (int)x, (int)y, (int)w, (int)h };
        SDL_RenderCopy(s_renderer, tex->texture, NULL, &dst);
    } else {
        // Fallback placeholder rect
        SDL_SetRenderDrawColor(s_renderer, 60, 60, 60, 200);
        SDL_Rect r = { (int)x, (int)y, (int)w, (int)h };
        SDL_RenderFillRect(s_renderer, &r);
    }
    return 0;
}

static int l_draw_rect(lua_State* L) {
    float x = (float)luaL_checknumber(L, 1);
    float y = (float)luaL_checknumber(L, 2);
    float w = (float)luaL_checknumber(L, 3);
    float h = (float)luaL_checknumber(L, 4);

    uint8_t r = (uint8_t)luaL_optinteger(L, 5, 255);
    uint8_t g = (uint8_t)luaL_optinteger(L, 6, 255);
    uint8_t b = (uint8_t)luaL_optinteger(L, 7, 255);
    uint8_t a = (uint8_t)luaL_optinteger(L, 8, 255);

    SDL_SetRenderDrawColor(s_renderer, r, g, b, a);
    SDL_Rect rect = { (int)x, (int)y, (int)w, (int)h };
    SDL_RenderFillRect(s_renderer, &rect);
    return 0;
}

static int l_draw_text(lua_State* L) {
    float x = (float)luaL_checknumber(L, 1);
    float y = (float)luaL_checknumber(L, 2);
    const char* text = luaL_checkstring(L, 3);

    uint8_t r = (uint8_t)luaL_optinteger(L, 4, 255);
    uint8_t g = (uint8_t)luaL_optinteger(L, 5, 255);
    uint8_t b = (uint8_t)luaL_optinteger(L, 6, 255);
    uint8_t a = (uint8_t)luaL_optinteger(L, 7, 255);

    SDL_SetRenderDrawColor(s_renderer, r, g, b, a);
    SDLTest_DrawString(s_renderer, (int)x, (int)y, text);
    return 0;
}

static int l_play_sfx(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    CachedSound* snd = get_or_load_sound(name);
    if (!snd) return 0;

    SDL_LockAudioDevice(s_audio_dev);
    for (int i = 0; i < MAX_VOICES; i++) {
        if (!s_voices[i].active) {
            s_voices[i].samples = snd->samples;
            s_voices[i].frame_count = snd->frame_count;
            s_voices[i].position = 0;
            s_voices[i].active = true;
            s_voices[i].loop = false;
            s_voices[i].volume = 0.9f;
            break;
        }
    }
    SDL_UnlockAudioDevice(s_audio_dev);
    return 0;
}

static int l_play_music(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    bool loop = lua_toboolean(L, 2);

    CachedSound* snd = get_or_load_sound(name);
    if (!snd) return 0;

    SDL_LockAudioDevice(s_audio_dev);
    s_bgm.samples = snd->samples;
    s_bgm.frame_count = snd->frame_count;
    s_bgm.position = 0;
    s_bgm.active = true;
    s_bgm.loop = loop;
    s_bgm.volume = 0.8f;
    SDL_UnlockAudioDevice(s_audio_dev);
    return 0;
}

static int l_stop_music(lua_State* L) {
    (void)L;
    SDL_LockAudioDevice(s_audio_dev);
    s_bgm.active = false;
    SDL_UnlockAudioDevice(s_audio_dev);
    return 0;
}

static int l_get_pad_input(lua_State* L) {
    lua_newtable(L);

    lua_pushboolean(L, s_pad.cross);    lua_setfield(L, -2, "cross");
    lua_pushboolean(L, s_pad.circle);   lua_setfield(L, -2, "circle");
    lua_pushboolean(L, s_pad.square);   lua_setfield(L, -2, "square");
    lua_pushboolean(L, s_pad.triangle); lua_setfield(L, -2, "triangle");

    lua_pushboolean(L, s_pad.up);       lua_setfield(L, -2, "up");
    lua_pushboolean(L, s_pad.down);     lua_setfield(L, -2, "down");
    lua_pushboolean(L, s_pad.left);     lua_setfield(L, -2, "left");
    lua_pushboolean(L, s_pad.right);    lua_setfield(L, -2, "right");

    lua_pushboolean(L, s_pad.l1);       lua_setfield(L, -2, "l1");
    lua_pushboolean(L, s_pad.r1);       lua_setfield(L, -2, "r1");
    lua_pushboolean(L, s_pad.l2);       lua_setfield(L, -2, "l2");
    lua_pushboolean(L, s_pad.r2);       lua_setfield(L, -2, "r2");

    lua_pushboolean(L, s_pad.start);    lua_setfield(L, -2, "start");
    lua_pushboolean(L, s_pad.select);   lua_setfield(L, -2, "select");

    lua_pushnumber(L, s_pad.analog_lx); lua_setfield(L, -2, "analog_lx");
    lua_pushnumber(L, s_pad.analog_ly); lua_setfield(L, -2, "analog_ly");
    lua_pushnumber(L, s_pad.analog_rx); lua_setfield(L, -2, "analog_rx");
    lua_pushnumber(L, s_pad.analog_ry); lua_setfield(L, -2, "analog_ry");

    lua_pushboolean(L, s_pad.mouse_moved); lua_setfield(L, -2, "mouse_moved");
    lua_pushnumber(L, s_pad.mouse_x);      lua_setfield(L, -2, "mouse_x");
    lua_pushnumber(L, s_pad.mouse_y);      lua_setfield(L, -2, "mouse_y");

    return 1;
}

static int l_get_ticks_ms(lua_State* L) {
    lua_pushinteger(L, (lua_Integer)SDL_GetTicks());
    return 1;
}

static int l_exit_game(lua_State* L) {
    (void)L;
    s_running = false;
    return 0;
}

static int l_log_info(lua_State* L) {
    const char* msg = luaL_checkstring(L, 1);
    printf("[Lua] %s\n", msg);
    return 0;
}

// -------------------------------------------------------------
// Main Entry Point
// -------------------------------------------------------------
int main(int argc, char** argv) {
    (void)argc; (void)argv;

    printf("====================================================\n");
    printf(" That's Not My Neighbor - PS3 Demake (PC Desktop Runner)\n");
    printf(" Native SDL2 720p 60FPS + Embedded Vanilla Lua 5.4\n");
    printf("====================================================\n");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
        fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "That's Not My Neighbor - PS3 Demake [PC Target // 720p 60FPS]",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI
    );
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    s_renderer = SDL_CreateRenderer(window, -1,
                                    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!s_renderer) {
        printf("[Desktop Renderer] Hardware acceleration unavailable, using software renderer...\n");
        s_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!s_renderer) {
        fprintf(stderr, "SDL_CreateRenderer error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_SetRenderDrawBlendMode(s_renderer, SDL_BLENDMODE_BLEND);

    audio_init();

    // Initialize Gamepad if present
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            s_controller = SDL_GameControllerOpen(i);
            if (s_controller) {
                printf("[Desktop Controller] Initialized: %s\n", SDL_GameControllerName(s_controller));
                break;
            }
        }
    }

    // Spin up Lua State
    lua_State* L = luaL_newstate();
    if (!L) {
        fprintf(stderr, "Failed to create Lua state.\n");
        return 1;
    }
    luaL_openlibs(L);

    // Register Bridge Functions
    lua_register(L, "draw_sprite", l_draw_sprite);
    lua_register(L, "draw_rect", l_draw_rect);
    lua_register(L, "draw_text", l_draw_text);
    lua_register(L, "play_sfx", l_play_sfx);
    lua_register(L, "play_music", l_play_music);
    lua_register(L, "stop_music", l_stop_music);
    lua_register(L, "get_pad_input", l_get_pad_input);
    lua_register(L, "get_ticks_ms", l_get_ticks_ms);
    lua_register(L, "exit_game", l_exit_game);
    lua_register(L, "log_info", l_log_info);

    // Configure package.path
    if (luaL_dostring(L, "package.path = 'usrdir/scripts/?.lua;./?.lua;' .. package.path") != LUA_OK) {
        fprintf(stderr, "[Desktop Lua] Warning configuring package.path\n");
    }

    // Load main.lua
    printf("[Desktop Lua] Loading usrdir/scripts/main.lua...\n");
    if (luaL_dofile(L, "usrdir/scripts/main.lua") != LUA_OK) {
        fprintf(stderr, "[Desktop Lua] Error loading main.lua:\n%s\n", lua_tostring(L, -1));
        lua_close(L);
        return 1;
    }

    // Call game_init()
    lua_getglobal(L, "game_init");
    if (lua_isfunction(L, -1)) {
        if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
            fprintf(stderr, "[Desktop Lua] Error running game_init():\n%s\n", lua_tostring(L, -1));
        }
    } else {
        lua_pop(L, 1);
    }

    printf("[Desktop Engine] Running 60 FPS Game Loop. Controls: Mouse / Keyboard (Arrows, Enter, Esc) / DualShock\n");

    const Uint32 frame_delay = 1000 / 60; // ~16.6ms
    while (s_running) {
        Uint32 frame_start = SDL_GetTicks();

        update_desktop_input();

        // 1. Update Game Logic
        lua_getglobal(L, "game_update");
        if (lua_isfunction(L, -1)) {
            if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
                fprintf(stderr, "[Desktop Lua] Error in game_update(): %s\n", lua_tostring(L, -1));
                lua_pop(L, 1);
            }
        } else {
            lua_pop(L, 1);
        }

        // 2. Render Frame
        SDL_SetRenderDrawColor(s_renderer, 0, 0, 0, 255);
        SDL_RenderClear(s_renderer);

        lua_getglobal(L, "game_render");
        if (lua_isfunction(L, -1)) {
            if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
                fprintf(stderr, "[Desktop Lua] Error in game_render(): %s\n", lua_tostring(L, -1));
                lua_pop(L, 1);
            }
        } else {
            lua_pop(L, 1);
        }

        SDL_RenderPresent(s_renderer);

        // Cap to 60 FPS
        Uint32 frame_time = SDL_GetTicks() - frame_start;
        if (frame_time < frame_delay) {
            SDL_Delay(frame_delay - frame_time);
        }
    }

    printf("[Desktop Engine] Shutting down...\n");
    lua_getglobal(L, "game_shutdown");
    if (lua_isfunction(L, -1)) {
        lua_pcall(L, 0, 0, 0);
    }
    lua_close(L);

    // Free textures
    for (int i = 0; i < s_texture_count; i++) {
        if (s_textures[i].texture) {
            SDL_DestroyTexture(s_textures[i].texture);
        }
    }

    if (s_controller) SDL_GameControllerClose(s_controller);
    if (s_audio_dev > 0) SDL_CloseAudioDevice(s_audio_dev);
    SDL_DestroyRenderer(s_renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    printf("[Desktop Engine] Clean exit.\n");
    return 0;
}
