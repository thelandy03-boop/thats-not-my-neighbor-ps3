#include "input.h"
#include "common.h"
#include <io/pad.h>

static PadState s_pad_state;
static padInfo s_pad_info;
static padData s_pad_data;

bool input_init(void) {
    memset(&s_pad_state, 0, sizeof(PadState));
    ioPadInit(7);
    return true;
}

void input_poll(void) {
    ioPadGetInfo(&s_pad_info);

    if (s_pad_info.status[0]) {
        ioPadGetData(0, &s_pad_data);

        s_pad_state.cross    = (s_pad_data.button[3] & PAD_CTRL_CROSS) != 0;
        s_pad_state.circle   = (s_pad_data.button[3] & PAD_CTRL_CIRCLE) != 0;
        s_pad_state.triangle = (s_pad_data.button[3] & PAD_CTRL_TRIANGLE) != 0;
        s_pad_state.square   = (s_pad_data.button[3] & PAD_CTRL_SQUARE) != 0;

        s_pad_state.up       = (s_pad_data.button[2] & PAD_CTRL_UP) != 0;
        s_pad_state.down     = (s_pad_data.button[2] & PAD_CTRL_DOWN) != 0;
        s_pad_state.left     = (s_pad_data.button[2] & PAD_CTRL_LEFT) != 0;
        s_pad_state.right    = (s_pad_data.button[2] & PAD_CTRL_RIGHT) != 0;

        s_pad_state.l1       = (s_pad_data.button[3] & PAD_CTRL_L1) != 0;
        s_pad_state.r1       = (s_pad_data.button[3] & PAD_CTRL_R1) != 0;
        s_pad_state.l2       = (s_pad_data.button[3] & PAD_CTRL_L2) != 0;
        s_pad_state.r2       = (s_pad_data.button[3] & PAD_CTRL_R2) != 0;

        s_pad_state.start    = (s_pad_data.button[2] & PAD_CTRL_START) != 0;
        s_pad_state.select   = (s_pad_data.button[2] & PAD_CTRL_SELECT) != 0;

        // Normalize analog stick (-1.0 to 1.0)
        s_pad_state.analog_lx = ((float)s_pad_data.button[6] - 128.0f) / 128.0f;
        s_pad_state.analog_ly = ((float)s_pad_data.button[7] - 128.0f) / 128.0f;
        s_pad_state.analog_rx = ((float)s_pad_data.button[4] - 128.0f) / 128.0f;
        s_pad_state.analog_ry = ((float)s_pad_data.button[5] - 128.0f) / 128.0f;

        // Deadzone filter
        if (s_pad_state.analog_lx > -0.15f && s_pad_state.analog_lx < 0.15f) s_pad_state.analog_lx = 0.0f;
        if (s_pad_state.analog_ly > -0.15f && s_pad_state.analog_ly < 0.15f) s_pad_state.analog_ly = 0.0f;
    }
}

const PadState* input_get_state(void) {
    return &s_pad_state;
}

void input_cleanup(void) {
    ioPadEnd();
}
