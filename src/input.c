#include "input.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#define HAVE_SDL2 1
#elif __has_include(<SDL.h>)
#include <SDL.h>
#define HAVE_SDL2 1
#endif

static PadState s_pad_state;
#ifdef HAVE_SDL2
static SDL_GameController* s_controller = NULL;
#endif

bool input_init(void) {
    memset(&s_pad_state, 0, sizeof(PadState));
#ifdef HAVE_SDL2
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK) == 0) {
        if (SDL_NumJoysticks() > 0) {
            s_controller = SDL_GameControllerOpen(0);
            if (s_controller) {
                printf("[Input] Connected Game Controller: %s\n", SDL_GameControllerName(s_controller));
            }
        }
    }
#endif
    printf("[Input] Input subsystem initialized successfully.\n");
    return true;
}

void input_poll(void) {
    memset(&s_pad_state, 0, sizeof(PadState));

#ifdef HAVE_SDL2
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_CONTROLLERDEVICEADDED && !s_controller) {
            s_controller = SDL_GameControllerOpen(event.cdevice.which);
        } else if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
            if (s_controller) {
                SDL_GameControllerClose(s_controller);
                s_controller = NULL;
            }
        }
    }

    // 1. Keyboard fallback (works out of the box on PC/RPCS3)
    const Uint8* keys = SDL_GetKeyboardState(NULL);
    if (keys) {
        s_pad_state.cross    = keys[SDL_SCANCODE_SPACE] || keys[SDL_SCANCODE_RETURN] || keys[SDL_SCANCODE_Z];
        s_pad_state.circle   = keys[SDL_SCANCODE_ESCAPE] || keys[SDL_SCANCODE_X] || keys[SDL_SCANCODE_BACKSPACE];
        s_pad_state.triangle = keys[SDL_SCANCODE_C];
        s_pad_state.square   = keys[SDL_SCANCODE_V];
        s_pad_state.up       = keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_W];
        s_pad_state.down     = keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_S];
        s_pad_state.left     = keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A];
        s_pad_state.right    = keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D];
        s_pad_state.l1       = keys[SDL_SCANCODE_Q];
        s_pad_state.r1       = keys[SDL_SCANCODE_E];

        float k_lx = 0.0f, k_ly = 0.0f;
        if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT])  k_lx -= 1.0f;
        if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) k_lx += 1.0f;
        if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP])    k_ly -= 1.0f;
        if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN])  k_ly += 1.0f;
        s_pad_state.analog_lx = k_lx;
        s_pad_state.analog_ly = k_ly;
    }

    // 2. DualShock 3 / Game Controller polling
    if (s_controller) {
        s_pad_state.cross    |= SDL_GameControllerGetButton(s_controller, SDL_CONTROLLER_BUTTON_A);
        s_pad_state.circle   |= SDL_GameControllerGetButton(s_controller, SDL_CONTROLLER_BUTTON_B);
        s_pad_state.square   |= SDL_GameControllerGetButton(s_controller, SDL_CONTROLLER_BUTTON_X);
        s_pad_state.triangle |= SDL_GameControllerGetButton(s_controller, SDL_CONTROLLER_BUTTON_Y);
        s_pad_state.up       |= SDL_GameControllerGetButton(s_controller, SDL_CONTROLLER_BUTTON_DPAD_UP);
        s_pad_state.down     |= SDL_GameControllerGetButton(s_controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
        s_pad_state.left     |= SDL_GameControllerGetButton(s_controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
        s_pad_state.right    |= SDL_GameControllerGetButton(s_controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
        s_pad_state.l1       |= SDL_GameControllerGetButton(s_controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
        s_pad_state.r1       |= SDL_GameControllerGetButton(s_controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
        s_pad_state.start    |= SDL_GameControllerGetButton(s_controller, SDL_CONTROLLER_BUTTON_START);
        s_pad_state.select   |= SDL_GameControllerGetButton(s_controller, SDL_CONTROLLER_BUTTON_BACK);

        Sint16 ax = SDL_GameControllerGetAxis(s_controller, SDL_CONTROLLER_AXIS_LEFTX);
        Sint16 ay = SDL_GameControllerGetAxis(s_controller, SDL_CONTROLLER_AXIS_LEFTY);
        if (abs(ax) > 3200) s_pad_state.analog_lx = ax / 32767.0f;
        if (abs(ay) > 3200) s_pad_state.analog_ly = ay / 32767.0f;

        Sint16 rx = SDL_GameControllerGetAxis(s_controller, SDL_CONTROLLER_AXIS_RIGHTX);
        Sint16 ry = SDL_GameControllerGetAxis(s_controller, SDL_CONTROLLER_AXIS_RIGHTY);
        if (abs(rx) > 3200) s_pad_state.analog_rx = rx / 32767.0f;
        if (abs(ry) > 3200) s_pad_state.analog_ry = ry / 32767.0f;
    }
#endif
}

const PadState* input_get_state(void) {
    return &s_pad_state;
}

void input_cleanup(void) {
#ifdef HAVE_SDL2
    if (s_controller) {
        SDL_GameControllerClose(s_controller);
        s_controller = NULL;
    }
#endif
    printf("[Input] Input subsystem cleaned up.\n");
}
