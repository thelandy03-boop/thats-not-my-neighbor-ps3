#include "input.h"
#include "common.h"

#if __has_include(<io/pad.h>)
#include <io/pad.h>
#elif __has_include(<pad.h>)
#include <pad.h>
#else
// PSL1GHT binary-compatible DualShock 3 structure definitions
typedef struct _pad_data {
    int len;
    uint16_t button[4];
    float ANA_R_H;
    float ANA_R_V;
    float ANA_L_H;
    float ANA_L_V;
} padData;

typedef struct _pad_info {
    int max;
    int connected;
    uint32_t info;
    uint16_t vendor_id[7];
    uint16_t product_id[7];
    uint8_t status[7];
} padInfo;

#define PAD_CTRL_SELECT     0x0001
#define PAD_CTRL_START      0x0008
#define PAD_CTRL_UP         0x0010
#define PAD_CTRL_RIGHT      0x0020
#define PAD_CTRL_DOWN       0x0040
#define PAD_CTRL_LEFT       0x0080

#define PAD_CTRL_L2         0x0001
#define PAD_CTRL_R2         0x0002
#define PAD_CTRL_L1         0x0004
#define PAD_CTRL_R1         0x0008
#define PAD_CTRL_TRIANGLE   0x0010
#define PAD_CTRL_CIRCLE     0x0020
#define PAD_CTRL_CROSS      0x0040
#define PAD_CTRL_SQUARE     0x0080

int ioPadInit(int max) __attribute__((weak));
int ioPadEnd(void) __attribute__((weak));
int ioPadGetInfo(padInfo *info) __attribute__((weak));
int ioPadGetData(int port, padData *data) __attribute__((weak));
#endif

static PadState s_pad_state;
static padInfo s_pad_info;
static padData s_pad_data;

bool input_init(void) {
    memset(&s_pad_state, 0, sizeof(PadState));
    if (ioPadInit) {
        ioPadInit(7);
    }
    return true;
}

void input_poll(void) {
    if (!ioPadGetInfo || !ioPadGetData) return;

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

        // Stick values from PSL1GHT padData
        s_pad_state.analog_lx = s_pad_data.ANA_L_H;
        s_pad_state.analog_ly = s_pad_data.ANA_L_V;
        s_pad_state.analog_rx = s_pad_data.ANA_R_H;
        s_pad_state.analog_ry = s_pad_data.ANA_R_V;

        if (s_pad_state.analog_lx > -0.15f && s_pad_state.analog_lx < 0.15f) s_pad_state.analog_lx = 0.0f;
        if (s_pad_state.analog_ly > -0.15f && s_pad_state.analog_ly < 0.15f) s_pad_state.analog_ly = 0.0f;
    }
}

const PadState* input_get_state(void) {
    return &s_pad_state;
}

void input_cleanup(void) {
    if (ioPadEnd) {
        ioPadEnd();
    }
}
