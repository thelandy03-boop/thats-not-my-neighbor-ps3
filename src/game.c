#include "game.h"
#include "graphics.h"
#include "input.h"
#include <stdio.h>
#include <math.h>

static bool point_in_rect(float px, float py, Rect2D r) {
    return (px >= r.x && px <= (r.x + r.w) && py >= r.y && py <= (r.y + r.h));
}

void game_init(GameContext* ctx) {
    memset(ctx, 0, sizeof(GameContext));
    ctx->state = STATE_OFFICE_BOOTH;
    ctx->running = true;

    // Camera setup (Center at 960, 540)
    ctx->camera.position.x = CAMERA_CENTER_X;
    ctx->camera.position.y = CAMERA_CENTER_Y;
    ctx->camera.target.x   = CAMERA_CENTER_X;
    ctx->camera.target.y   = CAMERA_CENTER_Y;

    // Cursor setup
    ctx->cursor.x = SCREEN_WIDTH / 2.0f;
    ctx->cursor.y = SCREEN_HEIGHT / 2.0f;

    // Security Booth initial state
    ctx->hatch_state = HATCH_CLOSED;
    ctx->hatch_progress = 0.0f;
    ctx->emergency_cover_open = false;
    ctx->alarm_active = false;
    ctx->current_day = 1;

    // Buttons layout on desk (1920x1080 native coordinates)
    // Door Lock / Unlock control panel
    ctx->btn_door_unlock = (DeskButton){ (Rect2D){ 1280, 720, 80, 80 }, false, false, "ABRIR" };
    ctx->btn_door_lock   = (DeskButton){ (Rect2D){ 1380, 720, 80, 80 }, false, false, "CERRAR" };

    // Emergency alarm button & safety cover
    ctx->btn_emergency_cover = (DeskButton){ (Rect2D){ 120, 750, 100, 120 }, false, false, "CUBIERTA" };
    ctx->btn_emergency       = (DeskButton){ (Rect2D){ 130, 760, 80, 100 }, false, false, "ALARMA D.D.D." };

    // Folders on desk
    ctx->btn_folder_list = (DeskButton){ (Rect2D){ 550, 840, 220, 140 }, false, false, "LISTA DEL DIA" };
    ctx->btn_folder_id   = (DeskButton){ (Rect2D){ 820, 840, 220, 140 }, false, false, "DOCUMENTOS ID" };

    // Spawn first visitor
    game_spawn_next_visitor(ctx);
}

void game_spawn_next_visitor(GameContext* ctx) {
    // Pick from legitimate residents (first 20) or special/doppelganger
    ctx->visitor_index = (ctx->processed_count % 20);
    // 25% chance of being a doppelganger
    ctx->visitor_is_doppelganger = ((ctx->processed_count % 4) == 3);
    ctx->visitor_present = true;
    printf("[Game] Next visitor spawned: %s (Doppelganger: %s)\n",
           g_neighbors[ctx->visitor_index].name,
           ctx->visitor_is_doppelganger ? "YES" : "NO");
}

void game_update(GameContext* ctx) {
    const PadState* pad = input_get_state();

    // Exit shortcut: START + SELECT
    if (pad->start && pad->select) {
        ctx->running = false;
        return;
    }

    // 1. DualShock 3 Left Analog Stick Cursor Movement
    const float cursor_speed = 10.0f;
    ctx->cursor.x += pad->analog_lx * cursor_speed;
    ctx->cursor.y += pad->analog_ly * cursor_speed;

    // Bounds clamp (Native 1920x1080)
    if (ctx->cursor.x < 10) ctx->cursor.x = 10;
    if (ctx->cursor.x > SCREEN_WIDTH - 10) ctx->cursor.x = SCREEN_WIDTH - 10;
    if (ctx->cursor.y < 10) ctx->cursor.y = 10;
    if (ctx->cursor.y > SCREEN_HEIGHT - 10) ctx->cursor.y = SCREEN_HEIGHT - 10;

    // 2. Parallax Camera following cursor (Original Godot formula)
    // Smoothly shift camera viewport relative to center
    float target_cam_x = CAMERA_CENTER_X + (ctx->cursor.x - (SCREEN_WIDTH / 2.0f)) * 0.15f;
    float target_cam_y = CAMERA_CENTER_Y + (ctx->cursor.y - (SCREEN_HEIGHT / 2.0f)) * 0.08f;

    if (target_cam_x < CAMERA_MIN_X) target_cam_x = CAMERA_MIN_X;
    if (target_cam_x > CAMERA_MAX_X) target_cam_x = CAMERA_MAX_X;
    if (target_cam_y < CAMERA_MIN_Y) target_cam_y = CAMERA_MIN_Y;
    if (target_cam_y > CAMERA_MAX_Y) target_cam_y = CAMERA_MAX_Y;

    // Smooth camera interpolation
    ctx->camera.position.x += (target_cam_x - ctx->camera.position.x) * 0.1f;
    ctx->camera.position.y += (target_cam_y - ctx->camera.position.y) * 0.1f;

    // 3. Hatch Open/Close Animation State Machine
    const float hatch_anim_speed = 0.04f;
    if (ctx->hatch_state == HATCH_OPENING) {
        ctx->hatch_progress += hatch_anim_speed;
        if (ctx->hatch_progress >= 1.0f) {
            ctx->hatch_progress = 1.0f;
            ctx->hatch_state = HATCH_OPEN;
        }
    } else if (ctx->hatch_state == HATCH_CLOSING) {
        ctx->hatch_progress -= hatch_anim_speed;
        if (ctx->hatch_progress <= 0.0f) {
            ctx->hatch_progress = 0.0f;
            ctx->hatch_state = HATCH_CLOSED;
        }
    }

    // 4. Update Button Hover States
    ctx->btn_door_unlock.hovered     = point_in_rect(ctx->cursor.x, ctx->cursor.y, ctx->btn_door_unlock.rect);
    ctx->btn_door_lock.hovered       = point_in_rect(ctx->cursor.x, ctx->cursor.y, ctx->btn_door_lock.rect);
    ctx->btn_emergency_cover.hovered = point_in_rect(ctx->cursor.x, ctx->cursor.y, ctx->btn_emergency_cover.rect);
    ctx->btn_emergency.hovered       = point_in_rect(ctx->cursor.x, ctx->cursor.y, ctx->btn_emergency.rect);
    ctx->btn_folder_list.hovered     = point_in_rect(ctx->cursor.x, ctx->cursor.y, ctx->btn_folder_list.rect);
    ctx->btn_folder_id.hovered       = point_in_rect(ctx->cursor.x, ctx->cursor.y, ctx->btn_folder_id.rect);

    // 5. DualShock 3 Button Actions & Shortcuts
    // Cross (✖): Click button under cursor
    if (pad->cross && !ctx->cursor_clicking) {
        ctx->cursor_clicking = true;

        if (ctx->btn_door_unlock.hovered && ctx->hatch_state != HATCH_OPEN) {
            ctx->hatch_state = HATCH_OPENING;
            printf("[Booth] Opening window hatch...\n");
        } else if (ctx->btn_door_lock.hovered && ctx->hatch_state != HATCH_CLOSED) {
            ctx->hatch_state = HATCH_CLOSING;
            printf("[Booth] Closing window hatch...\n");
        } else if (ctx->btn_emergency_cover.hovered) {
            ctx->emergency_cover_open = !ctx->emergency_cover_open;
            printf("[Booth] Emergency cover: %s\n", ctx->emergency_cover_open ? "OPEN" : "CLOSED");
        } else if (ctx->emergency_cover_open && ctx->btn_emergency.hovered) {
            ctx->alarm_active = true;
            ctx->state = STATE_EMERGENCY_ALARM;
            printf("[Booth] !!! D.D.D. ALARM TRIGGERED !!!\n");
        } else if (ctx->btn_folder_id.hovered || ctx->btn_folder_list.hovered) {
            ctx->state = STATE_DOCUMENT_INSPECT;
        }
    } else if (!pad->cross) {
        ctx->cursor_clicking = false;
    }

    // Direct DualShock 3 Shortcuts:
    // L1: Fast Open Hatch / R1: Fast Close Hatch
    if (pad->l1 && ctx->hatch_state != HATCH_OPEN) {
        ctx->hatch_state = HATCH_OPENING;
    } else if (pad->r1 && ctx->hatch_state != HATCH_CLOSED) {
        ctx->hatch_state = HATCH_CLOSING;
    }

    // Square (■): Fast Document Inspection
    if (pad->square && ctx->state == STATE_OFFICE_BOOTH) {
        ctx->state = STATE_DOCUMENT_INSPECT;
    }

    // Circle (●): Back / Close document or alarm
    if (pad->circle) {
        if (ctx->state == STATE_DOCUMENT_INSPECT || ctx->state == STATE_EMERGENCY_ALARM) {
            ctx->state = STATE_OFFICE_BOOTH;
        }
    }

    // Triangle (▲): Fast D.D.D. Emergency
    if (pad->triangle && ctx->state == STATE_OFFICE_BOOTH) {
        ctx->emergency_cover_open = true;
        ctx->state = STATE_EMERGENCY_ALARM;
    }
}

void game_render(GameContext* ctx) {
    // Clear screen
    graphics_clear(0x181410FF);

    switch (ctx->state) {
        case STATE_OFFICE_BOOTH: {
            // Parallax offset relative to default center (960, 540)
            int cam_dx = (int)(ctx->camera.position.x - CAMERA_CENTER_X);
            int cam_dy = (int)(ctx->camera.position.y - CAMERA_CENTER_Y);

            // 1. Wall Background & Architecture
            graphics_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x2d2822FF);

            // 2. Security Window Frame (Middle)
            int win_x = 480 - cam_dx;
            int win_y = 120 - cam_dy;
            int win_w = 960;
            int win_h = 560;
            graphics_draw_rect(win_x - 10, win_y - 10, win_w + 20, win_h + 20, 0x111111FF); // Border
            graphics_draw_rect(win_x, win_y, win_w, win_h, 0x080808FF); // Window outside darkness

            // 3. Visitor inside window (when hatch is open)
            if (ctx->hatch_progress > 0.05f && ctx->visitor_present) {
                const NeighborData* visitor = &g_neighbors[ctx->visitor_index];
                int vis_x = win_x + (win_w / 2) - 150;
                int vis_y = win_y + 80;

                // Silhouette / Visitor portrait box
                graphics_draw_rect(vis_x, vis_y, 300, 420, 0x5a4a3aFF);
                graphics_draw_rect(vis_x + 80, vis_y + 40, 140, 160, 0xd4b290FF); // Face placeholder

                // Visitor name tag in window
                char tag[128];
                snprintf(tag, sizeof(tag), "%s %s", visitor->name, visitor->last_name);
                graphics_draw_text(vis_x + 20, vis_y + 430, tag, 0xE0E0E0FF);
            }

            // 4. Security Shutter / Hatch Animation
            // Slide up/down over the window
            int shutter_h = (int)((1.0f - ctx->hatch_progress) * win_h);
            if (shutter_h > 0) {
                graphics_draw_rect(win_x, win_y, win_w, shutter_h, 0x44494eff); // Metal shutter
                // Shutter lines
                for (int sy = win_y + 20; sy < win_y + shutter_h; sy += 30) {
                    graphics_draw_rect(win_x, sy, win_w, 2, 0x222528ff);
                }
            }

            // 5. Security Desk Surface (Foreground)
            int desk_y = 660 - (int)(cam_dy * 1.2f);
            graphics_draw_rect(0, desk_y, SCREEN_WIDTH, SCREEN_HEIGHT - desk_y, 0x221a14FF);
            graphics_draw_rect(0, desk_y, SCREEN_WIDTH, 6, 0x3a2c22FF); // Desk rim

            // 6. Interactive Door Control Panel (Buttons)
            graphics_draw_rect(ctx->btn_door_unlock.rect.x, ctx->btn_door_unlock.rect.y,
                               ctx->btn_door_unlock.rect.w, ctx->btn_door_unlock.rect.h,
                               ctx->btn_door_unlock.hovered ? 0x2e7d32FF : 0x1b5e20FF);
            graphics_draw_text(ctx->btn_door_unlock.rect.x + 12, ctx->btn_door_unlock.rect.y + 30, "ABRIR", 0xFFFFFFFF);

            graphics_draw_rect(ctx->btn_door_lock.rect.x, ctx->btn_door_lock.rect.y,
                               ctx->btn_door_lock.rect.w, ctx->btn_door_lock.rect.h,
                               ctx->btn_door_lock.hovered ? 0xc62828FF : 0x8e0000FF);
            graphics_draw_text(ctx->btn_door_lock.rect.x + 10, ctx->btn_door_lock.rect.y + 30, "CERRAR", 0xFFFFFFFF);

            // 7. Folders on desk
            graphics_draw_rect(ctx->btn_folder_list.rect.x, ctx->btn_folder_list.rect.y,
                               ctx->btn_folder_list.rect.w, ctx->btn_folder_list.rect.h,
                               ctx->btn_folder_list.hovered ? 0xccb077FF : 0x998055FF);
            graphics_draw_text(ctx->btn_folder_list.rect.x + 20, ctx->btn_folder_list.rect.y + 55, "LISTA D.D.D.", 0x111111FF);

            graphics_draw_rect(ctx->btn_folder_id.rect.x, ctx->btn_folder_id.rect.y,
                               ctx->btn_folder_id.rect.w, ctx->btn_folder_id.rect.h,
                               ctx->btn_folder_id.hovered ? 0xddddccFF : 0xaaaa99FF);
            graphics_draw_text(ctx->btn_folder_id.rect.x + 35, ctx->btn_folder_id.rect.y + 55, "PAPELEO ID", 0x111111FF);

            // 8. Emergency Panic Button & Safety Cover
            if (ctx->emergency_cover_open) {
                graphics_draw_rect(ctx->btn_emergency.rect.x, ctx->btn_emergency.rect.y,
                                   ctx->btn_emergency.rect.w, ctx->btn_emergency.rect.h,
                                   ctx->btn_emergency.hovered ? 0xff1744FF : 0xd50000FF);
                graphics_draw_text(ctx->btn_emergency.rect.x + 10, ctx->btn_emergency.rect.y + 40, "D.D.D.", 0xFFFFFFFF);
            } else {
                graphics_draw_rect(ctx->btn_emergency_cover.rect.x, ctx->btn_emergency_cover.rect.y,
                                   ctx->btn_emergency_cover.rect.w, ctx->btn_emergency_cover.rect.h, 0xfbc02dFF);
                graphics_draw_text(ctx->btn_emergency_cover.rect.x + 15, ctx->btn_emergency_cover.rect.y + 45, "PELIGRO", 0x111111FF);
            }

            // HUD Guide
            graphics_draw_text(50, 40, "CABINA D.D.D. #33 | PS3 HOMEBREW EDITION", 0xFFD700FF);
            graphics_draw_text(50, 1020, "[L1/R1] Abrir/Cerrar  [X] Interactuar  [[]] Documentos  [/\\\\] Alarma D.D.D.  [START+SELECT] Salir", 0xAAAAAAFF);
            break;
        }

        case STATE_DOCUMENT_INSPECT: {
            const NeighborData* visitor = &g_neighbors[ctx->visitor_index];

            // Desk dim overlay
            graphics_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x111111DD);

            // ID Document Window
            int doc_x = 320;
            int doc_y = 120;
            int doc_w = 640;
            int doc_h = 420;
            graphics_draw_rect(doc_x, doc_y, doc_w, doc_h, 0xf4eedbFF); // Parchment paper
            graphics_draw_rect(doc_x, doc_y, doc_w, 40, 0x8b1e1eFF);   // Header band
            graphics_draw_text(doc_x + 20, doc_y + 10, "D.D.D. IDENTIFICACION DE RESIDENTE", 0xFFFFFFFF);

            // Photo frame placeholder
            graphics_draw_rect(doc_x + 40, doc_y + 70, 160, 200, 0x444444FF);
            graphics_draw_rect(doc_x + 50, doc_y + 80, 140, 180, 0xd4b290FF);

            // Information fields
            char buf[256];
            snprintf(buf, sizeof(buf), "Nombre: %s", visitor->name);
            graphics_draw_text(doc_x + 230, doc_y + 80, buf, 0x222222FF);

            snprintf(buf, sizeof(buf), "Apellido: %s", visitor->last_name);
            graphics_draw_text(doc_x + 230, doc_y + 120, buf, 0x222222FF);

            snprintf(buf, sizeof(buf), "No. ID: %s", visitor->id_number);
            graphics_draw_text(doc_x + 230, doc_y + 160, buf, 0x222222FF);

            snprintf(buf, sizeof(buf), "Expira: %s", visitor->id_expiration);
            graphics_draw_text(doc_x + 230, doc_y + 200, buf, 0x222222FF);

            snprintf(buf, sizeof(buf), "Depto: %s (%s)", visitor->apt_number_es, visitor->apt_number_en);
            graphics_draw_text(doc_x + 230, doc_y + 240, buf, 0x222222FF);

            snprintf(buf, sizeof(buf), "Ocupacion: %s", visitor->occupation_es);
            graphics_draw_text(doc_x + 40, doc_y + 300, buf, 0x333333FF);

            // Daily verification checklist
            int check_x = 1000;
            int check_y = 120;
            graphics_draw_rect(check_x, check_y, 600, 420, 0xeee8d5FF);
            graphics_draw_text(check_x + 20, check_y + 20, "LISTA DE VERIFICACION D.D.D.", 0x8b1e1eFF);
            graphics_draw_text(check_x + 20, check_y + 80, "Rasgos registrados:", 0x111111FF);
            graphics_draw_text(check_x + 20, check_y + 120, visitor->appearance_es, 0x444444FF);

            graphics_draw_text(doc_x + 200, 600, "Pulsa [O] para Volver a la Cabina", 0xFFD700FF);
            break;
        }

        case STATE_EMERGENCY_ALARM: {
            graphics_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x7f0000FF);
            graphics_draw_text(480, 400, "!!! ALARMA DE EMERGENCIA D.D.D. ACTIVADA !!!", 0xFFFFFFFF);
            graphics_draw_text(520, 480, "Escuadron de Limpieza en Camino...", 0xFFCCCCFF);
            graphics_draw_text(600, 650, "Pulsa [O] para Regresar", 0xFFD700FF);
            break;
        }

        default:
            break;
    }

    // Always render DualShock 3 Pointer Cursor
    graphics_draw_cursor(ctx->cursor.x, ctx->cursor.y, 0xFFFFFFFF);
}

void game_cleanup(GameContext* ctx) {
    printf("[Game] Context cleaned up cleanly.\n");
}
