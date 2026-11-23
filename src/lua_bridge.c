#include "lua_bridge.h"
#include "renderer.h"
#include "audio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/time.h>

#if defined(__CELLOS_LV2__) || defined(_PS3)
#include <io/pad.h>
#endif

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
} GamePadState;

static GamePadState s_current_pad;
static bool s_exit_requested = false;

void lua_bridge_poll_pad(void) {
    memset(&s_current_pad, 0, sizeof(GamePadState));

#if defined(__CELLOS_LV2__) || defined(_PS3)
    padInfo info;
    if (ioPadGetInfo(&info) == 0) {
        for (int i = 0; i < 7; i++) {
            if (info.status[i]) {
                padData data;
                if (ioPadGetData(i, &data) == 0 && data.len > 0) {
                    s_current_pad.select   = (data.button[2] & (1 << 0)) != 0;
                    s_current_pad.start    = (data.button[2] & (1 << 3)) != 0;
                    s_current_pad.up       = (data.button[2] & (1 << 4)) != 0;
                    s_current_pad.right    = (data.button[2] & (1 << 5)) != 0;
                    s_current_pad.down     = (data.button[2] & (1 << 6)) != 0;
                    s_current_pad.left     = (data.button[2] & (1 << 7)) != 0;

                    s_current_pad.l2       = (data.button[3] & (1 << 0)) != 0;
                    s_current_pad.r2       = (data.button[3] & (1 << 1)) != 0;
                    s_current_pad.l1       = (data.button[3] & (1 << 2)) != 0;
                    s_current_pad.r1       = (data.button[3] & (1 << 3)) != 0;
                    s_current_pad.triangle = (data.button[3] & (1 << 4)) != 0;
                    s_current_pad.circle   = (data.button[3] & (1 << 5)) != 0;
                    s_current_pad.cross    = (data.button[3] & (1 << 6)) != 0;
                    s_current_pad.square   = (data.button[3] & (1 << 7)) != 0;

                    s_current_pad.analog_lx = ((float)data.ANA_L_H - 128.0f) / 128.0f;
                    s_current_pad.analog_ly = ((float)data.ANA_L_V - 128.0f) / 128.0f;
                    s_current_pad.analog_rx = ((float)data.ANA_R_H - 128.0f) / 128.0f;
                    s_current_pad.analog_ry = ((float)data.ANA_R_V - 128.0f) / 128.0f;

                    // Deadzone threshold
                    if (s_current_pad.analog_lx > -0.15f && s_current_pad.analog_lx < 0.15f) s_current_pad.analog_lx = 0.0f;
                    if (s_current_pad.analog_ly > -0.15f && s_current_pad.analog_ly < 0.15f) s_current_pad.analog_ly = 0.0f;

                    // Fallback cursor control with D-Pad
                    if (s_current_pad.left)  s_current_pad.analog_lx = -1.0f;
                    if (s_current_pad.right) s_current_pad.analog_lx =  1.0f;
                    if (s_current_pad.up)    s_current_pad.analog_ly = -1.0f;
                    if (s_current_pad.down)  s_current_pad.analog_ly =  1.0f;

                    // Clean exit combo: START + SELECT
                    if (s_current_pad.start && s_current_pad.select) {
                        s_exit_requested = true;
                    }
                    break;
                }
            }
        }
    }
#endif
}

bool lua_bridge_should_exit(void) {
    return s_exit_requested;
}

static int l_draw_sprite(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    float x = (float)luaL_checknumber(L, 2);
    float y = (float)luaL_checknumber(L, 3);
    float w = (float)luaL_optnumber(L, 4, 100.0);
    float h = (float)luaL_optnumber(L, 5, 100.0);

    renderer_draw_sprite(name, x, y, w, h);
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

    uint32_t color = (a << 24) | (r << 16) | (g << 8) | b;
    renderer_draw_rect(x, y, w, h, color);
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

    uint32_t color = (a << 24) | (r << 16) | (g << 8) | b;
    renderer_draw_text(x, y, text, color);
    return 0;
}

static int l_get_pad_input(lua_State* L) {
    lua_newtable(L);

    lua_pushboolean(L, s_current_pad.cross);    lua_setfield(L, -2, "cross");
    lua_pushboolean(L, s_current_pad.circle);   lua_setfield(L, -2, "circle");
    lua_pushboolean(L, s_current_pad.square);   lua_setfield(L, -2, "square");
    lua_pushboolean(L, s_current_pad.triangle); lua_setfield(L, -2, "triangle");

    lua_pushboolean(L, s_current_pad.up);       lua_setfield(L, -2, "up");
    lua_pushboolean(L, s_current_pad.down);     lua_setfield(L, -2, "down");
    lua_pushboolean(L, s_current_pad.left);     lua_setfield(L, -2, "left");
    lua_pushboolean(L, s_current_pad.right);    lua_setfield(L, -2, "right");

    lua_pushboolean(L, s_current_pad.l1);       lua_setfield(L, -2, "l1");
    lua_pushboolean(L, s_current_pad.r1);       lua_setfield(L, -2, "r1");
    lua_pushboolean(L, s_current_pad.l2);       lua_setfield(L, -2, "l2");
    lua_pushboolean(L, s_current_pad.r2);       lua_setfield(L, -2, "r2");

    lua_pushboolean(L, s_current_pad.start);    lua_setfield(L, -2, "start");
    lua_pushboolean(L, s_current_pad.select);   lua_setfield(L, -2, "select");

    lua_pushnumber(L, s_current_pad.analog_lx); lua_setfield(L, -2, "analog_lx");
    lua_pushnumber(L, s_current_pad.analog_ly); lua_setfield(L, -2, "analog_ly");
    lua_pushnumber(L, s_current_pad.analog_rx); lua_setfield(L, -2, "analog_rx");
    lua_pushnumber(L, s_current_pad.analog_ry); lua_setfield(L, -2, "analog_ry");

    return 1;
}

static int l_play_sfx(lua_State* L) {
    const char* sound_name = luaL_checkstring(L, 1);
    audio_play_sfx(sound_name);
    return 0;
}

static int l_play_music(lua_State* L) {
    const char* music_name = luaL_checkstring(L, 1);
    bool loop = lua_toboolean(L, 2);
    audio_play_music(music_name, loop);
    return 0;
}

static int l_stop_music(lua_State* L) {
    (void)L;
    audio_stop_music();
    return 0;
}

static int l_get_ticks_ms(lua_State* L) {
    static struct timeval start_tv = {0, 0};
    if (start_tv.tv_sec == 0 && start_tv.tv_usec == 0) {
        gettimeofday(&start_tv, NULL);
    }
    struct timeval now;
    gettimeofday(&now, NULL);

    double ms = (double)(now.tv_sec - start_tv.tv_sec) * 1000.0 +
                (double)(now.tv_usec - start_tv.tv_usec) / 1000.0;
    lua_pushnumber(L, ms);
    return 1;
}

static int l_log_info(lua_State* L) {
    const char* msg = luaL_checkstring(L, 1);
    printf("[Lua] %s\n", msg);
    return 0;
}

static int l_exit_game(lua_State* L) {
    (void)L;
    s_exit_requested = true;
    return 0;
}

static const luaL_Reg engine_funcs[] = {
    {"draw_sprite",    l_draw_sprite},
    {"draw_rect",      l_draw_rect},
    {"draw_text",      l_draw_text},
    {"get_pad_input",  l_get_pad_input},
    {"play_sfx",       l_play_sfx},
    {"play_music",     l_play_music},
    {"stop_music",     l_stop_music},
    {"get_ticks_ms",   l_get_ticks_ms},
    {"log_info",       l_log_info},
    {"exit_game",      l_exit_game},
    {NULL, NULL}
};

void lua_bridge_register(lua_State* L) {
    // Register global C functions directly into Lua
    for (const luaL_Reg* r = engine_funcs; r->name != NULL; r++) {
        lua_register(L, r->name, r->func);
    }

    printf("[LuaBridge] Registered %d native C engine functions into Lua.\n",
           (int)(sizeof(engine_funcs)/sizeof(engine_funcs[0]) - 1));
}
