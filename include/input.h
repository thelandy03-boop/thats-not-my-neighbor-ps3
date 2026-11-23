#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool cross;
    bool circle;
    bool triangle;
    bool square;
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
} PadState;

bool input_init(void);
void input_poll(void);
const PadState* input_get_state(void);
bool input_is_pressed(uint32_t button_mask);
void input_cleanup(void);

#endif // INPUT_H
