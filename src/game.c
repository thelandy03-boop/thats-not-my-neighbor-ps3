#include "game.h"
#include "graphics.h"
#include "input.h"
#include <stdio.h>

void game_init(GameContext* ctx) {
    memset(ctx, 0, sizeof(GameContext));
    ctx->state = STATE_TITLE;
    ctx->running = true;
    ctx->cursor.x = SCREEN_WIDTH / 2.0f;
    ctx->cursor.y = SCREEN_HEIGHT / 2.0f;
    ctx->current_day = 1;

    // Default test visitor
    strncpy(ctx->current_visitor.name, "Francis Mosses", sizeof(ctx->current_visitor.name));
    strncpy(ctx->current_visitor.apartment, "F01-01", sizeof(ctx->current_visitor.apartment));
    ctx->current_visitor.is_doppelganger = false;
    ctx->current_visitor.has_valid_id = true;
    ctx->current_visitor.has_entry_permit = true;
}

void game_update(GameContext* ctx) {
    const PadState* pad = input_get_state();

    // Exit check (SELECT + START)
    if (pad->start && pad->select) {
        ctx->running = false;
        return;
    }

    // Cursor movement with Left Stick
    const float cursor_speed = 7.0f;
    ctx->cursor.x += pad->analog_lx * cursor_speed;
    ctx->cursor.y += pad->analog_ly * cursor_speed;

    // Screen bounds clamp
    if (ctx->cursor.x < 0) ctx->cursor.x = 0;
    if (ctx->cursor.x > SCREEN_WIDTH) ctx->cursor.x = SCREEN_WIDTH;
    if (ctx->cursor.y < 0) ctx->cursor.y = 0;
    if (ctx->cursor.y > SCREEN_HEIGHT) ctx->cursor.y = SCREEN_HEIGHT;

    switch (ctx->state) {
        case STATE_TITLE:
            if (pad->cross || pad->start) {
                ctx->state = STATE_DESK_VIEW;
            }
            break;

        case STATE_DESK_VIEW:
            // DualShock 3 shortcuts:
            // Triangle: Open Emergency Alarm (D.D.D.)
            // Square: Check documents / Checklist
            // Circle: Deny Entry / Close hatch
            // Cross: Allow Entry / Open hatch
            if (pad->square) {
                ctx->state = STATE_DOCUMENT_VIEW;
            } else if (pad->triangle) {
                ctx->state = STATE_EMERGENCY_ALARM;
            }
            break;

        case STATE_DOCUMENT_VIEW:
            if (pad->circle) {
                ctx->state = STATE_DESK_VIEW;
            }
            break;

        case STATE_EMERGENCY_ALARM:
            if (pad->circle) {
                ctx->state = STATE_DESK_VIEW;
            }
            break;

        case STATE_PHONE_CALL:
        case STATE_GAME_OVER:
        default:
            break;
    }
}

void game_render(GameContext* ctx) {
    graphics_clear(0x1a1a1aff);

    switch (ctx->state) {
        case STATE_TITLE:
            graphics_draw_text(400, 250, "THAT'S NOT MY NEIGHBOR - PS3 PORT", 0xFF0000FF);
            graphics_draw_text(480, 360, "Press START / CROSS to Begin", 0xFFFFFFFF);
            graphics_draw_text(420, 500, "Port by Homebrew Community", 0x888888FF);
            break;

        case STATE_DESK_VIEW:
            // Security Booth Desk Layout
            graphics_draw_rect(0, 480, SCREEN_WIDTH, 240, 0x332211FF); // Desk surface
            graphics_draw_rect(340, 60, 600, 400, 0x222222FF);         // Security window
            graphics_draw_text(50, 40, "D.D.D. Booth #33 - Security Post", 0xE0E0E0FF);

            // Controls HUD
            graphics_draw_text(50, 660, "[X] Open Door   [O] Close Hatch   [[]] Inspect Docs   [/\\\\] D.D.D. Alarm", 0xAAAAAAFF);
            break;

        case STATE_DOCUMENT_VIEW:
            // ID / Entry Permit Inspection
            graphics_draw_rect(200, 80, 880, 560, 0xEEEEDDFF);
            graphics_draw_text(240, 120, "D.D.D. VERIFICATION DOCUMENT", 0x111111FF);
            graphics_draw_text(240, 180, "Name: Francis Mosses", 0x222222FF);
            graphics_draw_text(240, 220, "Apartment: F01-01", 0x222222FF);
            graphics_draw_text(240, 600, "Press [O] to Return to Desk", 0x555555FF);
            break;

        case STATE_EMERGENCY_ALARM:
            graphics_draw_clear_color:
            graphics_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x880000FF);
            graphics_draw_text(420, 300, "!!! EMERGENCY ALARM TRIGGERED !!!", 0xFFFFFFFF);
            graphics_draw_text(450, 360, "Calling D.D.D. Cleanup Squad...", 0xFFDDDDFF);
            graphics_draw_text(480, 500, "Press [O] to Cancel", 0xFFFFFFFF);
            break;

        default:
            break;
    }

    // Always render cursor pointer
    graphics_draw_cursor(ctx->cursor.x, ctx->cursor.y, 0xFFFFFFFF);
}

void game_cleanup(GameContext* ctx) {
    printf("[Game] Context cleaned up.\n");
}
