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
    ctx->btn_door_unlock = (DeskButton){ (Rect2D){ 1280, 720, 80, 80 }, false, false, "ABRIR" };
    ctx->btn_door_lock   = (DeskButton){ (Rect2D){ 1380, 720, 80, 80 }, false, false, "CERRAR" };

    ctx->btn_emergency_cover = (DeskButton){ (Rect2D){ 120, 750, 100, 120 }, false, false, "CUBIERTA" };
    ctx->btn_emergency       = (DeskButton){ (Rect2D){ 130, 760, 80, 100 }, false, false, "ALARMA D.D.D." };

    ctx->btn_folder_list = (DeskButton){ (Rect2D){ 550, 840, 220, 140 }, false, false, "LISTA DEL DIA" };
    ctx->btn_folder_id   = (DeskButton){ (Rect2D){ 820, 840, 220, 140 }, false, false, "DOCUMENTOS ID" };

    // Document Tab navigation buttons
    ctx->tab_btn_id    = (DeskButton){ (Rect2D){ 280, 60, 220, 50 }, false, false, "[L1] IDENTIFICACION" };
    ctx->tab_btn_entry = (DeskButton){ (Rect2D){ 520, 60, 220, 50 }, false, false, "SOLICITUD INGRESO" };
    ctx->tab_btn_list  = (DeskButton){ (Rect2D){ 760, 60, 220, 50 }, false, false, "LISTA DEL DIA" };
    ctx->tab_btn_check = (DeskButton){ (Rect2D){ 1000, 60, 220, 50 }, false, false, "[R1] COTEJO D.D.D." };

    ctx->active_doc_tab = DOC_TAB_ID;

    // Spawn first visitor
    game_spawn_next_visitor(ctx);
}

void game_spawn_next_visitor(GameContext* ctx) {
    ctx->visitor_index = (ctx->processed_count % 20);
    // 25% chance of being a doppelganger
    ctx->visitor_is_doppelganger = ((ctx->processed_count % 4) == 3);
    ctx->visitor_present = true;

    // Reset verification checklist for new visitor
    memset(&ctx->checklist, 0, sizeof(VerificationChecklist));

    const NeighborData* base = &g_neighbors[ctx->visitor_index];
    strncpy(ctx->visitor_docs.displayed_name, base->name, sizeof(ctx->visitor_docs.displayed_name));
    strncpy(ctx->visitor_docs.displayed_last_name, base->last_name, sizeof(ctx->visitor_docs.displayed_last_name));
    strncpy(ctx->visitor_docs.displayed_id_number, base->id_number, sizeof(ctx->visitor_docs.displayed_id_number));
    strncpy(ctx->visitor_docs.displayed_expiration, base->id_expiration, sizeof(ctx->visitor_docs.displayed_expiration));
    strncpy(ctx->visitor_docs.displayed_apartment, base->apt_number_es, sizeof(ctx->visitor_docs.displayed_apartment));
    ctx->visitor_docs.is_on_today_list = true;
    ctx->visitor_docs.has_doppelganger_flaw = ctx->visitor_is_doppelganger;

    // If it's a Doppelganger, plant an authentic flaw
    if (ctx->visitor_is_doppelganger) {
        ctx->visitor_docs.flaw_type = (ctx->processed_count % 4) + 1;
        switch (ctx->visitor_docs.flaw_type) {
            case 1: // Expired ID flaw
                strncpy(ctx->visitor_docs.displayed_expiration, "05/1952", sizeof(ctx->visitor_docs.displayed_expiration));
                break;
            case 2: // Wrong Apartment
                strncpy(ctx->visitor_docs.displayed_apartment, "P04-09", sizeof(ctx->visitor_docs.displayed_apartment));
                break;
            case 3: // Corrupted ID Number
                ctx->visitor_docs.displayed_id_number[3] = '9';
                ctx->visitor_docs.displayed_id_number[4] = '9';
                break;
            case 4: // Not on today's list
                ctx->visitor_docs.is_on_today_list = false;
                break;
        }
    } else {
        ctx->visitor_docs.flaw_type = 0;
    }

    printf("[Game] Spawned: %s %s (Doppelganger: %s, Flaw: %d)\n",
           base->name, base->last_name,
           ctx->visitor_is_doppelganger ? "YES" : "NO",
           ctx->visitor_docs.flaw_type);
}

void game_update(GameContext* ctx) {
    const PadState* pad = input_get_state();

    if (pad->start && pad->select) {
        ctx->running = false;
        return;
    }

    // Cursor movement
    const float cursor_speed = 10.0f;
    ctx->cursor.x += pad->analog_lx * cursor_speed;
    ctx->cursor.y += pad->analog_ly * cursor_speed;

    if (ctx->cursor.x < 10) ctx->cursor.x = 10;
    if (ctx->cursor.x > SCREEN_WIDTH - 10) ctx->cursor.x = SCREEN_WIDTH - 10;
    if (ctx->cursor.y < 10) ctx->cursor.y = 10;
    if (ctx->cursor.y > SCREEN_HEIGHT - 10) ctx->cursor.y = SCREEN_HEIGHT - 10;

    // Camera parallax (in booth state)
    if (ctx->state == STATE_OFFICE_BOOTH) {
        float target_cam_x = CAMERA_CENTER_X + (ctx->cursor.x - (SCREEN_WIDTH / 2.0f)) * 0.15f;
        float target_cam_y = CAMERA_CENTER_Y + (ctx->cursor.y - (SCREEN_HEIGHT / 2.0f)) * 0.08f;

        if (target_cam_x < CAMERA_MIN_X) target_cam_x = CAMERA_MIN_X;
        if (target_cam_x > CAMERA_MAX_X) target_cam_x = CAMERA_MAX_X;
        if (target_cam_y < CAMERA_MIN_Y) target_cam_y = CAMERA_MIN_Y;
        if (target_cam_y > CAMERA_MAX_Y) target_cam_y = CAMERA_MAX_Y;

        ctx->camera.position.x += (target_cam_x - ctx->camera.position.x) * 0.1f;
        ctx->camera.position.y += (target_cam_y - ctx->camera.position.y) * 0.1f;
    }

    // Shutter animation
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

    // Button hit-testing
    ctx->btn_door_unlock.hovered     = point_in_rect(ctx->cursor.x, ctx->cursor.y, ctx->btn_door_unlock.rect);
    ctx->btn_door_lock.hovered       = point_in_rect(ctx->cursor.x, ctx->cursor.y, ctx->btn_door_lock.rect);
    ctx->btn_emergency_cover.hovered = point_in_rect(ctx->cursor.x, ctx->cursor.y, ctx->btn_emergency_cover.rect);
    ctx->btn_emergency.hovered       = point_in_rect(ctx->cursor.x, ctx->cursor.y, ctx->btn_emergency.rect);
    ctx->btn_folder_list.hovered     = point_in_rect(ctx->cursor.x, ctx->cursor.y, ctx->btn_folder_list.rect);
    ctx->btn_folder_id.hovered       = point_in_rect(ctx->cursor.x, ctx->cursor.y, ctx->btn_folder_id.rect);

    ctx->tab_btn_id.hovered    = point_in_rect(ctx->cursor.x, ctx->cursor.y, ctx->tab_btn_id.rect);
    ctx->tab_btn_entry.hovered = point_in_rect(ctx->cursor.x, ctx->cursor.y, ctx->tab_btn_entry.rect);
    ctx->tab_btn_list.hovered  = point_in_rect(ctx->cursor.x, ctx->cursor.y, ctx->tab_btn_list.rect);
    ctx->tab_btn_check.hovered = point_in_rect(ctx->cursor.x, ctx->cursor.y, ctx->tab_btn_check.rect);

    // Cross (✖) Click interaction
    if (pad->cross && !ctx->cursor_clicking) {
        ctx->cursor_clicking = true;

        if (ctx->state == STATE_OFFICE_BOOTH) {
            if (ctx->btn_door_unlock.hovered && ctx->hatch_state != HATCH_OPEN) {
                ctx->hatch_state = HATCH_OPENING;
            } else if (ctx->btn_door_lock.hovered && ctx->hatch_state != HATCH_CLOSED) {
                ctx->hatch_state = HATCH_CLOSING;
            } else if (ctx->btn_emergency_cover.hovered) {
                ctx->emergency_cover_open = !ctx->emergency_cover_open;
            } else if (ctx->emergency_cover_open && ctx->btn_emergency.hovered) {
                ctx->alarm_active = true;
                ctx->state = STATE_EMERGENCY_ALARM;
            } else if (ctx->btn_folder_id.hovered) {
                ctx->active_doc_tab = DOC_TAB_ID;
                ctx->state = STATE_DOCUMENT_INSPECT;
            } else if (ctx->btn_folder_list.hovered) {
                ctx->active_doc_tab = DOC_TAB_TODAYS_LIST;
                ctx->state = STATE_DOCUMENT_INSPECT;
            }
        } else if (ctx->state == STATE_DOCUMENT_INSPECT) {
            // Tab clicks
            if (ctx->tab_btn_id.hovered) ctx->active_doc_tab = DOC_TAB_ID;
            else if (ctx->tab_btn_entry.hovered) ctx->active_doc_tab = DOC_TAB_ENTRY_REQUEST;
            else if (ctx->tab_btn_list.hovered) ctx->active_doc_tab = DOC_TAB_TODAYS_LIST;
            else if (ctx->tab_btn_check.hovered) ctx->active_doc_tab = DOC_TAB_CHECKLIST;

            // Checklist checkboxes toggles (Checklist Tab)
            if (ctx->active_doc_tab == DOC_TAB_CHECKLIST) {
                Rect2D r1 = { 400, 240, 40, 40 };
                Rect2D r2 = { 400, 310, 40, 40 };
                Rect2D r3 = { 400, 380, 40, 40 };
                Rect2D r4 = { 400, 450, 40, 40 };
                Rect2D r5 = { 400, 520, 40, 40 };
                Rect2D r6 = { 400, 590, 40, 40 };

                if (point_in_rect(ctx->cursor.x, ctx->cursor.y, r1)) ctx->checklist.photo_match = !ctx->checklist.photo_match;
                else if (point_in_rect(ctx->cursor.x, ctx->cursor.y, r2)) ctx->checklist.id_number_match = !ctx->checklist.id_number_match;
                else if (point_in_rect(ctx->cursor.x, ctx->cursor.y, r3)) ctx->checklist.not_expired = !ctx->checklist.not_expired;
                else if (point_in_rect(ctx->cursor.x, ctx->cursor.y, r4)) ctx->checklist.entry_permit_valid = !ctx->checklist.entry_permit_valid;
                else if (point_in_rect(ctx->cursor.x, ctx->cursor.y, r5)) ctx->checklist.appearance_match = !ctx->checklist.appearance_match;
                else if (point_in_rect(ctx->cursor.x, ctx->cursor.y, r6)) ctx->checklist.on_today_list = !ctx->checklist.on_today_list;
            }
        }
    } else if (!pad->cross) {
        ctx->cursor_clicking = false;
    }

    // Shortcuts:
    // L1/R1 in document view switches tabs, in booth opens/closes hatch
    if (ctx->state == STATE_DOCUMENT_INSPECT) {
        if (pad->l1) {
            if (ctx->active_doc_tab > 0) ctx->active_doc_tab--;
        } else if (pad->r1) {
            if (ctx->active_doc_tab < DOC_TAB_CHECKLIST) ctx->active_doc_tab++;
        }
    } else if (ctx->state == STATE_OFFICE_BOOTH) {
        if (pad->l1 && ctx->hatch_state != HATCH_OPEN) ctx->hatch_state = HATCH_OPENING;
        else if (pad->r1 && ctx->hatch_state != HATCH_CLOSED) ctx->hatch_state = HATCH_CLOSING;
    }

    if (pad->square && ctx->state == STATE_OFFICE_BOOTH) {
        ctx->state = STATE_DOCUMENT_INSPECT;
    }

    if (pad->circle) {
        if (ctx->state == STATE_DOCUMENT_INSPECT || ctx->state == STATE_EMERGENCY_ALARM) {
            ctx->state = STATE_OFFICE_BOOTH;
        }
    }

    if (pad->triangle && ctx->state == STATE_OFFICE_BOOTH) {
        ctx->emergency_cover_open = true;
        ctx->state = STATE_EMERGENCY_ALARM;
    }
}

void game_render(GameContext* ctx) {
    graphics_clear(0x181410FF);

    switch (ctx->state) {
        case STATE_OFFICE_BOOTH: {
            int cam_dx = (int)(ctx->camera.position.x - CAMERA_CENTER_X);
            int cam_dy = (int)(ctx->camera.position.y - CAMERA_CENTER_Y);

            // Wall Background
            graphics_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x2d2822FF);

            // Window Frame
            int win_x = 480 - cam_dx;
            int win_y = 120 - cam_dy;
            int win_w = 960;
            int win_h = 560;
            graphics_draw_rect(win_x - 10, win_y - 10, win_w + 20, win_h + 20, 0x111111FF);
            graphics_draw_rect(win_x, win_y, win_w, win_h, 0x080808FF);

            // Visitor
            if (ctx->hatch_progress > 0.05f && ctx->visitor_present) {
                const NeighborData* visitor = &g_neighbors[ctx->visitor_index];
                int vis_x = win_x + (win_w / 2) - 150;
                int vis_y = win_y + 80;
                graphics_draw_rect(vis_x, vis_y, 300, 420, 0x5a4a3aFF);
                graphics_draw_rect(vis_x + 80, vis_y + 40, 140, 160, 0xd4b290FF);

                char tag[128];
                snprintf(tag, sizeof(tag), "%s %s", visitor->name, visitor->last_name);
                graphics_draw_text(vis_x + 20, vis_y + 430, tag, 0xE0E0E0FF);
            }

            // Security Shutter
            int shutter_h = (int)((1.0f - ctx->hatch_progress) * win_h);
            if (shutter_h > 0) {
                graphics_draw_rect(win_x, win_y, win_w, shutter_h, 0x44494eff);
                for (int sy = win_y + 20; sy < win_y + shutter_h; sy += 30) {
                    graphics_draw_rect(win_x, sy, win_w, 2, 0x222528ff);
                }
            }

            // Desk Surface
            int desk_y = 660 - (int)(cam_dy * 1.2f);
            graphics_draw_rect(0, desk_y, SCREEN_WIDTH, SCREEN_HEIGHT - desk_y, 0x221a14FF);
            graphics_draw_rect(0, desk_y, SCREEN_WIDTH, 6, 0x3a2c22FF);

            // Door Buttons
            graphics_draw_rect(ctx->btn_door_unlock.rect.x, ctx->btn_door_unlock.rect.y,
                               ctx->btn_door_unlock.rect.w, ctx->btn_door_unlock.rect.h,
                               ctx->btn_door_unlock.hovered ? 0x2e7d32FF : 0x1b5e20FF);
            graphics_draw_text(ctx->btn_door_unlock.rect.x + 12, ctx->btn_door_unlock.rect.y + 30, "ABRIR", 0xFFFFFFFF);

            graphics_draw_rect(ctx->btn_door_lock.rect.x, ctx->btn_door_lock.rect.y,
                               ctx->btn_door_lock.rect.w, ctx->btn_door_lock.rect.h,
                               ctx->btn_door_lock.hovered ? 0xc62828FF : 0x8e0000FF);
            graphics_draw_text(ctx->btn_door_lock.rect.x + 10, ctx->btn_door_lock.rect.y + 30, "CERRAR", 0xFFFFFFFF);

            // Folders
            graphics_draw_rect(ctx->btn_folder_list.rect.x, ctx->btn_folder_list.rect.y,
                               ctx->btn_folder_list.rect.w, ctx->btn_folder_list.rect.h,
                               ctx->btn_folder_list.hovered ? 0xccb077FF : 0x998055FF);
            graphics_draw_text(ctx->btn_folder_list.rect.x + 20, ctx->btn_folder_list.rect.y + 55, "LISTA D.D.D.", 0x111111FF);

            graphics_draw_rect(ctx->btn_folder_id.rect.x, ctx->btn_folder_id.rect.y,
                               ctx->btn_folder_id.rect.w, ctx->btn_folder_id.rect.h,
                               ctx->btn_folder_id.hovered ? 0xddddccFF : 0xaaaa99FF);
            graphics_draw_text(ctx->btn_folder_id.rect.x + 35, ctx->btn_folder_id.rect.y + 55, "PAPELEO ID", 0x111111FF);

            // Emergency Panic Button
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

            graphics_draw_text(50, 40, "CABINA D.D.D. #33 | PS3 HOMEBREW EDITION", 0xFFD700FF);
            graphics_draw_text(50, 1020, "[L1/R1] Abrir/Cerrar  [X] Interactuar  [[]] Inspeccionar Documentos  [/\\\\] Alarma D.D.D.", 0xAAAAAAFF);
            break;
        }

        case STATE_DOCUMENT_INSPECT: {
            const NeighborData* visitor = &g_neighbors[ctx->visitor_index];

            // Background dimming
            graphics_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x12100dEE);

            // Top Navigation Tabs
            DeskButton* tabs[4] = { &ctx->tab_btn_id, &ctx->tab_btn_entry, &ctx->tab_btn_list, &ctx->tab_btn_check };
            for (int t = 0; t < 4; t++) {
                uint32_t bg = (ctx->active_doc_tab == (DocTab)t) ? 0x8b1e1eFF : (tabs[t]->hovered ? 0x554433FF : 0x332820FF);
                graphics_draw_rect(tabs[t]->rect.x, tabs[t]->rect.y, tabs[t]->rect.w, tabs[t]->rect.h, bg);
                graphics_draw_text(tabs[t]->rect.x + 15, tabs[t]->rect.y + 15, tabs[t]->label, 0xFFFFFFFF);
            }

            int main_x = 280;
            int main_y = 130;
            int main_w = 1360;
            int main_h = 760;

            if (ctx->active_doc_tab == DOC_TAB_ID) {
                // Official ID Card Document
                graphics_draw_rect(main_x, main_y, main_w, main_h, 0xf5f0e1FF); // Paper background
                graphics_draw_rect(main_x, main_y, main_w, 60, 0x8b1e1eFF);
                graphics_draw_text(main_x + 30, main_y + 18, "D.D.D. IDENTIFICACION OFICIAL DE RESIDENTE", 0xFFFFFFFF);

                // Photo frame
                graphics_draw_rect(main_x + 60, main_y + 120, 260, 320, 0x222222FF);
                graphics_draw_rect(main_x + 70, main_y + 130, 240, 300, 0xd4b290FF);
                graphics_draw_text(main_x + 120, main_y + 260, "[FOTO]", 0x555555FF);

                // Information fields
                char buf[256];
                snprintf(buf, sizeof(buf), "NOMBRE: %s", ctx->visitor_docs.displayed_name);
                graphics_draw_text(main_x + 380, main_y + 140, buf, 0x111111FF);

                snprintf(buf, sizeof(buf), "APELLIDO: %s", ctx->visitor_docs.displayed_last_name);
                graphics_draw_text(main_x + 380, main_y + 200, buf, 0x111111FF);

                snprintf(buf, sizeof(buf), "NO. IDENTIFICACION: %s", ctx->visitor_docs.displayed_id_number);
                graphics_draw_text(main_x + 380, main_y + 260, buf, 0x111111FF);

                snprintf(buf, sizeof(buf), "FECHA EXPIRACION: %s", ctx->visitor_docs.displayed_expiration);
                graphics_draw_text(main_x + 380, main_y + 320, buf, 0x111111FF);

                snprintf(buf, sizeof(buf), "APARTAMENTO: %s (%s)", ctx->visitor_docs.displayed_apartment, visitor->apt_number_en);
                graphics_draw_text(main_x + 380, main_y + 380, buf, 0x111111FF);

                snprintf(buf, sizeof(buf), "OCUPACION: %s", visitor->occupation_es);
                graphics_draw_text(main_x + 60, main_y + 490, buf, 0x222222FF);

                // Official Seal Stamp
                graphics_draw_rect(main_x + 950, main_y + 420, 180, 180, 0x99222244);
                graphics_draw_text(main_x + 970, main_y + 500, "SELLO D.D.D.", 0x8b1e1eFF);

            } else if (ctx->active_doc_tab == DOC_TAB_ENTRY_REQUEST) {
                // Entry Request Permit
                graphics_draw_rect(main_x, main_y, main_w, main_h, 0xf0e6d2FF);
                graphics_draw_rect(main_x, main_y, main_w, 60, 0x2e4a62FF);
                graphics_draw_text(main_x + 30, main_y + 18, "SOLICITUD DE INGRESO - DEPARTAMENTO DE DETECCION", 0xFFFFFFFF);

                char buf[256];
                snprintf(buf, sizeof(buf), "Solicitante: %s %s", ctx->visitor_docs.displayed_name, ctx->visitor_docs.displayed_last_name);
                graphics_draw_text(main_x + 80, main_y + 140, buf, 0x111111FF);

                snprintf(buf, sizeof(buf), "Destino: Apartamento %s", ctx->visitor_docs.displayed_apartment);
                graphics_draw_text(main_x + 80, main_y + 210, buf, 0x111111FF);

                graphics_draw_text(main_x + 80, main_y + 280, "Motivo de Ingreso: Regreso a su residencia habitual tras jornada laboral.", 0x222222FF);
                graphics_draw_text(main_x + 80, main_y + 350, "Estado de Autorizacion: APROBADO PREVIAMENTE POR D.D.D.", 0x1b5e20FF);

                // D.D.D. Verification stamp box
                graphics_draw_rect(main_x + 850, main_y + 400, 240, 160, 0x2e4a6233);
                graphics_draw_text(main_x + 880, main_y + 470, "FIRMA D.D.D. OK", 0x2e4a62FF);

            } else if (ctx->active_doc_tab == DOC_TAB_TODAYS_LIST) {
                // Today's Approved List
                graphics_draw_rect(main_x, main_y, main_w, main_h, 0xe8e4d8FF);
                graphics_draw_rect(main_x, main_y, main_w, 60, 0x3d352eFF);
                graphics_draw_text(main_x + 30, main_y + 18, "LISTA DE INQUILINOS PROGRAMADOS PARA HOY", 0xFFFFFFFF);

                graphics_draw_text(main_x + 60, main_y + 100, "Los siguientes residentes estan registrados para retornar hoy:", 0x333333FF);

                // Show 6 entries on today's list
                for (int i = 0; i < 6; i++) {
                    int n_idx = (ctx->processed_count + i) % 20;
                    const NeighborData* n = &g_neighbors[n_idx];
                    char line[256];
                    snprintf(line, sizeof(line), "%d. %s %s  -  Depto: %s  -  Tel: %s",
                             i + 1, n->name, n->last_name, n->apt_number_es, n->phone_number);

                    // Highlight current visitor if present
                    uint32_t col = (ctx->visitor_docs.is_on_today_list && n_idx == ctx->visitor_index) ? 0x8b1e1eFF : 0x111111FF;
                    graphics_draw_text(main_x + 80, main_y + 160 + (i * 65), line, col);
                }

                if (!ctx->visitor_docs.is_on_today_list) {
                    graphics_draw_text(main_x + 80, main_y + 600, "(!) ATENCION: El visitante NO aparece en la lista autorizada de hoy.", 0xc62828FF);
                }

            } else if (ctx->active_doc_tab == DOC_TAB_CHECKLIST) {
                // Verification Checklist
                graphics_draw_rect(main_x, main_y, main_w, main_h, 0xf7f5ebFF);
                graphics_draw_rect(main_x, main_y, main_w, 60, 0x8b1e1eFF);
                graphics_draw_text(main_x + 30, main_y + 18, "LISTA DE COTEJO D.D.D. (PROTOCOLO ANTI-DOPPELGANGER)", 0xFFFFFFFF);

                const char* items[6] = {
                    "1. La foto coincide con el rostro de la ventana",
                    "2. El numero de ID es valido y coincide con los registros",
                    "3. La identificacion NO esta expirada",
                    "4. Posee Solicitud de Ingreso con sello oficial",
                    "5. Los rasgos fisicos coinciden con el archivo del inquilino",
                    "6. El nombre figura en la lista autorizada de hoy"
                };

                bool checks[6] = {
                    ctx->checklist.photo_match,
                    ctx->checklist.id_number_match,
                    ctx->checklist.not_expired,
                    ctx->checklist.entry_permit_valid,
                    ctx->checklist.appearance_match,
                    ctx->checklist.on_today_list
                };

                for (int c = 0; c < 6; c++) {
                    int cy = main_y + 110 + (c * 70);
                    // Checkbox
                    graphics_draw_rect(main_x + 60, cy, 40, 40, 0x332820FF);
                    if (checks[c]) {
                        graphics_draw_rect(main_x + 68, cy + 8, 24, 24, 0x2e7d32FF); // Checked mark
                    }
                    graphics_draw_text(main_x + 120, cy + 8, items[c], 0x111111FF);
                }

                // Reference appearance from database
                graphics_draw_text(main_x + 60, main_y + 570, "Rasgos en Archivo:", 0x8b1e1eFF);
                graphics_draw_text(main_x + 60, main_y + 610, visitor->appearance_es, 0x333333FF);
            }

            // Bottom controls guide
            graphics_draw_text(main_x + 350, 930, "[L1/R1] Cambiar Documento   [X] Marcar/Desmarcar   [O] Volver al Despacho", 0xFFD700FF);
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

    // DualShock 3 Pointer
    graphics_draw_cursor(ctx->cursor.x, ctx->cursor.y, 0xFFFFFFFF);
}

void game_cleanup(GameContext* ctx) {
    printf("[Game] Context cleaned up cleanly.\n");
}
