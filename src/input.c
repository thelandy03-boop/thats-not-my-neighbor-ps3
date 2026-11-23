#include "input.h"
#include <stdio.h>
#include <string.h>
#include <io/pad.h>

static PadState s_pad_state;

bool input_init(void) {
    memset(&s_pad_state, 0, sizeof(PadState));
    ioPadInit(7);
    printf("[Input] PSL1GHT DualShock 3 subsystem initialized.\n");
    return true;
}

void input_poll(void) {
    memset(&s_pad_state, 0, sizeof(PadState));

    padInfo info;
    if (ioPadGetInfo(&info) != 0) return;

    for (int i = 0; i < 7; i++) {
        if (info.status[i]) {
            padData data;
            if (ioPadGetData(i, &data) == 0 && data.len > 0) {
                // PSL1GHT standard bit definitions:
                // button[2] = SELECT, L3, R3, START, UP, RIGHT, DOWN, LEFT
                // button[3] = L2, R2, L1, R1, TRIANGLE, CIRCLE, CROSS, SQUARE
                s_pad_state.cross    = (data.button[3] & (1 << 6)) != 0;
                s_pad_state.circle   = (data.button[3] & (1 << 5)) != 0;
                s_pad_state.square   = (data.button[3] & (1 << 7)) != 0;
                s_pad_state.triangle = (data.button[3] & (1 << 4)) != 0;

                s_pad_state.up       = (data.button[2] & (1 << 4)) != 0;
                s_pad_state.right    = (data.button[2] & (1 << 5)) != 0;
                s_pad_state.down     = (data.button[2] & (1 << 6)) != 0;
                s_pad_state.left     = (data.button[2] & (1 << 7)) != 0;

                s_pad_state.l1       = (data.button[3] & (1 << 2)) != 0;
                s_pad_state.r1       = (data.button[3] & (1 << 3)) != 0;
                s_pad_state.l2       = (data.button[3] & (1 << 0)) != 0;
                s_pad_state.r2       = (data.button[3] & (1 << 1)) != 0;

                s_pad_state.start    = (data.button[2] & (1 << 3)) != 0;
                s_pad_state.select   = (data.button[2] & (1 << 0)) != 0;

                s_pad_state.analog_lx = data.ANA_L_H;
                s_pad_state.analog_ly = data.ANA_L_V;
                s_pad_state.analog_rx = data.ANA_R_H;
                s_pad_state.analog_ry = data.ANA_R_V;

                if (s_pad_state.analog_lx > -0.15f && s_pad_state.analog_lx < 0.15f) s_pad_state.analog_lx = 0.0f;
                if (s_pad_state.analog_ly > -0.15f && s_pad_state.analog_ly < 0.15f) s_pad_state.analog_ly = 0.0f;
                break;
            }
        }
    }
}

const PadState* input_get_state(void) {
    return &s_pad_state;
}

void input_cleanup(void) {
    ioPadEnd();
    printf("[Input] PSL1GHT DualShock 3 subsystem cleaned up.\n");
}
