#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "renderer.h"
#include "audio.h"
#include "lua_bridge.h"

#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"

#if defined(__CELLOS_LV2__) || defined(_PS3)
#include <sys/process.h>
#include <io/pad.h>
#include <sysutil/sysutil.h>

// Allocate 2MB stack for Cell OS PPU thread
SYS_PROCESS_PARAM(1001, 0x200000);

static void sysutil_exit_callback(uint64_t status, uint64_t param, void* userdata) {
    (void)param; (void)userdata;
    if (status == SYSUTIL_EXIT_GAME) {
        printf("[TNMN-PS3] System requested exit (PS Button / Sysutil).\n");
        exit(0);
    }
}
#endif

static const char* resolve_script_path(const char* script_name, char* buffer, size_t buf_size) {
    FILE* f = fopen(script_name, "rb");
    if (f) {
        fclose(f);
        return script_name;
    }

    snprintf(buffer, buf_size, "usrdir/scripts/%s", script_name);
    f = fopen(buffer, "rb");
    if (f) {
        fclose(f);
        return buffer;
    }

    snprintf(buffer, buf_size, "/app_home/usrdir/scripts/%s", script_name);
    f = fopen(buffer, "rb");
    if (f) {
        fclose(f);
        return buffer;
    }

    snprintf(buffer, buf_size, "/dev_hdd0/game/TNMN00001/USRDIR/scripts/%s", script_name);
    f = fopen(buffer, "rb");
    if (f) {
        fclose(f);
        return buffer;
    }

    return script_name;
}

static void configure_lua_package_path(lua_State* L) {
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "path");
    const char* cur_path = lua_tostring(L, -1);

    char new_path[2048];
    snprintf(new_path, sizeof(new_path),
             "%s;./?.lua;usrdir/scripts/?.lua;/app_home/usrdir/scripts/?.lua;/dev_hdd0/game/TNMN00001/USRDIR/scripts/?.lua",
             cur_path ? cur_path : "");

    lua_pop(L, 1);
    lua_pushstring(L, new_path);
    lua_setfield(L, -2, "path");
    lua_pop(L, 1);
}

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    printf("\n========================================================\n");
    printf(" That's Not My Neighbor - PS3 Demake (Native PSL1GHT v2)\n");
    printf(" Cell PPU / RSX Tiny3D Engine + Vanilla Lua 5.4 Core   \n");
    printf("========================================================\n\n");

#if defined(__CELLOS_LV2__) || defined(_PS3)
    // Register sysutil callback to cleanly handle PS button
    sysUtilRegisterCallback(0, sysutil_exit_callback, NULL);
    ioPadInit(7);
#endif

    // 1. Initialize RSX Video & Tiny3D (720p 1280x720)
    if (!renderer_init()) {
        printf("[TNMN-PS3] Fatal Error: Could not initialize renderer.\n");
        return -1;
    }

    // 2. Initialize Cell Audio subsystem
    if (!audio_init()) {
        printf("[TNMN-PS3] Warning: Audio init failed, continuing without sound.\n");
    }

    // 3. Initialize Vanilla Lua 5.4 State
    printf("[TNMN-PS3] Initializing Lua 5.4 runtime...\n");
    lua_State* L = luaL_newstate();
    if (!L) {
        printf("[TNMN-PS3] Fatal: Out of memory creating Lua state.\n");
        renderer_cleanup();
        return -1;
    }

    luaL_openlibs(L);
    configure_lua_package_path(L);
    lua_bridge_register(L);

    // 4. Load & Execute Base Lua Script: usrdir/scripts/main.lua
    char script_path_buf[512];
    const char* script_file = resolve_script_path("main.lua", script_path_buf, sizeof(script_path_buf));
    printf("[TNMN-PS3] Executing main script: %s\n", script_file);

    if (luaL_dofile(L, script_file) != LUA_OK) {
        printf("[TNMN-PS3] Lua Script Error in %s:\n%s\n",
               script_file, lua_tostring(L, -1));
        lua_pop(L, 1);
    }

    // Call optional game_init() in Lua
    lua_getglobal(L, "game_init");
    if (lua_isfunction(L, -1)) {
        if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
            printf("[TNMN-PS3] Error in Lua game_init(): %s\n", lua_tostring(L, -1));
            lua_pop(L, 1);
        }
    } else {
        lua_pop(L, 1);
    }

    printf("[TNMN-PS3] Entering 60 FPS Main Loop...\n");

    // 5. Main 60 FPS Game Loop
    bool running = true;
    while (running) {
#if defined(__CELLOS_LV2__) || defined(_PS3)
        sysUtilCheckCallback();
#endif
        // Poll DualShock 3 controller
        lua_bridge_poll_pad();
        if (lua_bridge_should_exit()) {
            printf("[TNMN-PS3] Exit signal detected from controller.\n");
            break;
        }

        // Call Lua game_update()
        lua_getglobal(L, "game_update");
        if (lua_isfunction(L, -1)) {
            if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
                printf("[TNMN-PS3] Error in Lua game_update(): %s\n", lua_tostring(L, -1));
                lua_pop(L, 1);
            }
        } else {
            lua_pop(L, 1);
        }

        // Check if game requested shutdown
        lua_getglobal(L, "game_is_running");
        if (lua_isfunction(L, -1)) {
            if (lua_pcall(L, 0, 1, 0) == LUA_OK) {
                if (lua_isboolean(L, -1) && !lua_toboolean(L, -1)) {
                    running = false;
                }
                lua_pop(L, 1);
            }
        } else {
            lua_pop(L, 1);
        }

        // Clear and call Lua game_render()
        renderer_clear(0x181410FF);

        lua_getglobal(L, "game_render");
        if (lua_isfunction(L, -1)) {
            if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
                printf("[TNMN-PS3] Error in Lua game_render(): %s\n", lua_tostring(L, -1));
                lua_pop(L, 1);
            }
        } else {
            lua_pop(L, 1);
        }

        // Flip RSX Framebuffer (VSync)
        renderer_flip();

        // 60 FPS cadence
        usleep(1000);
    }

    printf("[TNMN-PS3] Shutting down...\n");

    // Call optional game_shutdown() in Lua
    lua_getglobal(L, "game_shutdown");
    if (lua_isfunction(L, -1)) {
        lua_pcall(L, 0, 0, 0);
    } else {
        lua_pop(L, 1);
    }

    // 6. Clean Deallocation
    lua_close(L);
    audio_cleanup();
#if defined(__CELLOS_LV2__) || defined(_PS3)
    ioPadEnd();
    sysUtilUnregisterCallback(0);
#endif
    renderer_cleanup();

    printf("[TNMN-PS3] Clean shutdown complete. Goodbye.\n");
    return 0;
}
