#include "game.h"
#include "graphics.h"
#include "input.h"
#include "audio.h"
#include <stdio.h>
#include <math.h>

static bool point_in_rect(float px, float py, Rect2D r) {
    return (px >= r.x && px <= (r.x + r.w) && py >= r.y && py <= (r.y + r.h));
}

void game_load_textures(GameContext* ctx) {
    printf("[Game] Loading PNG textures via stb_image...\n");
    ctx->tex_background      = texture_load_from_file("assets/gfx/Background.png");
    ctx->tex_wall            = texture_load_from_file("assets/gfx/Wall.png");
    ctx->tex_panic_window    = texture_load_from_file("assets/gfx/Panic_window.png");
    ctx->tex_cursor          = texture_load_from_file("assets/gfx/Cursor.png");
    ctx->tex_door_control    = texture_load_from_file("assets/gfx/Door_control.png");
    ctx->tex_id_card         = texture_load_from_file("assets/gfx/ID.png");
    ctx->tex_paper           = texture_load_from_file("assets/gfx/Paper.png");
    ctx->tex_todays_list     = texture_load_from_file("assets/gfx/Todays_list.png");
    ctx->tex_ddd_logo        = texture_load_from_file("assets/gfx/DDD_logo.png");
    ctx->tex_phone_base      = texture_load_from_file("assets/gfx/Phone_base.png");
    ctx->tex_phone_roulette  = texture_load_from_file("assets/gfx/Phone_Roulette.png");
    ctx->tex_doppel_hand_l   = texture_load_from_file("assets/gfx/Doppel_Left_Hand.png");
    ctx->tex_doppel_hand_r   = texture_load_from_file("assets/gfx/Doppel_Right_Hand.png");
    ctx->tex_roman_photo     = texture_load_from_file("assets/gfx/neighbors/Char_Roman_Long_Photo.png");
    ctx->tex_francis_photo   = texture_load_from_file("assets/gfx/neighbors/Char_Francis_Long_Photo.png");
}

void game_free_textures(GameContext* ctx) {
    texture_free(ctx->tex_background);
    texture_free(ctx->tex_wall);
    texture_free(ctx->tex_panic_window);
    texture_free(ctx->tex_cursor);
    texture_free(ctx->tex_door_control);
    texture_free(ctx->tex_id_card);
    texture_free(ctx->tex_paper);
    texture_free(ctx->tex_todays_list);
    texture_free(ctx->tex_ddd_logo);
    texture_free(ctx->tex_phone_base);
    texture_free(ctx->tex_phone_roulette);
    texture_free(ctx->tex_doppel_hand_l);
    texture_free(ctx->tex_doppel_hand_r);
    texture_free(ctx->tex_roman_photo);
    texture_free(ctx->tex_francis_photo);
}

void game_init(GameContext* ctx) {
    memset(ctx, 0, sizeof(GameContext));
    ctx->state = STATE_OFFICE_BOOTH;
    ctx->running = true;

    game_load_textures(ctx);

    ctx->camera.position.x = CAMERA_CENTER_X;
    ctx->camera.position.y = CAMERA_CENTER_Y;
    ctx->camera.target.x   = CAMERA_CENTER_X;
    ctx->camera.target.y   = CAMERA_CENTER_Y;

    ctx->cursor.x = SCREEN_WIDTH / 2.0f;
    ctx->cursor.y = SCREEN_HEIGHT / 2.0f;

    ctx->hatch_state = HATCH_CLOSED;
    ctx->hatch_progress = 0.0f;
    ctx->emergency_cover_open = false;
    ctx->alarm_active = false;
    ctx->current_day = 1;

    // Desk buttons
    ctx->btn_door_unlock = (DeskButton){ (Rect2D){ 1280, 720, 80, 80 }, false, false, "ABRIR" };
    ctx->btn_door_lock   = (DeskButton){ (Rect2D){ 1380, 720, 80, 80 }, false, false, "CERRAR" };

    ctx->btn_emergency_cover = (DeskButton){ (Rect2D){ 120, 750, 100, 120 }, false, false, "CUBIERTA" };
    ctx->btn_emergency       = (DeskButton){ (Rect2D){ 130, 760, 80, 100 }, false, false, "ALARMA D.D.D." };

    ctx->btn_folder_list = (DeskButton){ (Rect2D){ 550, 840, 220, 140 }, false, false, "LISTA DEL DIA" };
    ctx->btn_folder_id   = (DeskButton){ (Rect2D){ 820, 840, 220, 140 }, false, false, "DOCUMENTOS ID" };

    // Rotary phone on desk (Left area)
    ctx->btn_phone_desk  = (DeskButton){ (Rect2D){ 260, 780, 200, 200 }, false, false, "TELEFONO" };

    // Document Tabs
    ctx->tab_btn_id    = (DeskButton){ (Rect2D){ 280, 60, 220, 50 }, false, false, "[L1] IDENTIFICACION" };
    ctx->tab_btn_entry = (DeskButton){ (Rect2D){ 520, 60, 220, 50 }, false, false, "SOLICITUD INGRESO" };
    ctx->tab_btn_list  = (DeskButton){ (Rect2D){ 760, 60, 220, 50 }, false, false, "LISTA DEL DIA" };
    ctx->tab_btn_check = (DeskButton){ (Rect2D){ 1000, 60, 220, 50 }, false, false, "[R1] COTEJO D.D.D." };

    // Phone dial rotary buttons (numbers 1 to 9 in circle + 0)
    const float cx = 960.0f;
    const float cy = 560.0f;
    const float r = 180.0f;
    for (int d = 1; d <= 9; d++) {
        float angle = ((float)(d - 1) * 32.0f + 40.0f) * (3.14159f / 180.0f);
        int bx = (int)(cx + cosf(angle) * r) - 30;
        int by = (int)(cy + sinf(angle) * r) - 30;
        char num_str[4];
        snprintf(num_str, sizeof(num_str), "%d", d);
        ctx->phone.dial_buttons[d] = (DeskButton){ (Rect2D){ bx, by, 60, 60 }, false, false, strdup(num_str) };
    }
    // Digit 0 at bottom
    ctx->phone.dial_buttons[0] = (DeskButton){ (Rect2D){ (int)cx - 30, (int)cy + 180, 60, 60 }, false, false, "0" };
    ctx->phone.btn_hangup = (DeskButton){ (Rect2D){ 860, 840, 200, 60 }, false, false, "COLGAR [O]" };
    snprintf(ctx->phone.status_message, sizeof(ctx->phone.status_message), "Levanta el auricular y marca 4 digitos");

    // Game Over buttons
    ctx->btn_restart = (DeskButton){ (Rect2D){ 720, 680, 220, 70 }, false, false, "[X] OTRA VEZ" };
    ctx->btn_quit    = (DeskButton){ (Rect2D){ 980, 680, 220, 70 }, false, false, "[O] SALIR" };

    ctx->active_doc_tab = DOC_TAB_ID;

    game_spawn_next_visitor(ctx);
}

void game_spawn_next_visitor(GameContext* ctx) {
    ctx->visitor_index = (ctx->processed_count % 20);
    ctx->visitor_is_doppelganger = ((ctx->processed_count % 4) == 3);
    ctx->visitor_present = true;

    memset(&ctx->checklist, 0, sizeof(VerificationChecklist));

    const NeighborData* base = &g_neighbors[ctx->visitor_index];
    strncpy(ctx->visitor_docs.displayed_name, base->name, sizeof(ctx->visitor_docs.displayed_name));
    strncpy(ctx->visitor_docs.displayed_last_name, base->last_name, sizeof(ctx->visitor_docs.displayed_last_name));
    strncpy(ctx->visitor_docs.displayed_id_number, base->id_number, sizeof(ctx->visitor_docs.displayed_id_number));
    strncpy(ctx->visitor_docs.displayed_expiration, base->id_expiration, sizeof(ctx->visitor_docs.displayed_expiration));
    strncpy(ctx->visitor_docs.displayed_apartment, base->apt_number_es, sizeof(ctx->visitor_docs.displayed_apartment));
    ctx->visitor_docs.is_on_today_list = true;
    ctx->visitor_docs.has_doppelganger_flaw = ctx->visitor_is_doppelganger;

    if (ctx->visitor_is_doppelganger) {
        ctx->visitor_docs.flaw_type = (ctx->processed_count % 4) + 1;
        switch (ctx->visitor_docs.flaw_type) {
            case 1:
                strncpy(ctx->visitor_docs.displayed_expiration, "05/1952", sizeof(ctx->visitor_docs.displayed_expiration));
                break;
            case 2:
                strncpy(ctx->visitor_docs.displayed_apartment, "P04-09", sizeof(ctx->visitor_docs.displayed_apartment));
                break;
            case 3:
                ctx->visitor_docs.displayed_id_number[3] = '9';
                ctx->visitor_docs.displayed_id_number[4] = '9';
                break;
            case 4:
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

void game_trigger_doppel_attack(GameContext* ctx) {
    ctx->state = STATE_DOPPEL_ATTACK;
    ctx->attack_anim_timer = 0.0f;
    audio_play_sfx(SFX_BITE_ATTACK);
    audio_play_sfx(SFX_WINDOW_BREAK);
    printf("[Game] !!! DOPPELGANGER ATTACK TRIGGERED !!!\n");
}

static void phone_handle_call_evaluation(GameContext* ctx) {
    ctx->phone.in_call = true;
    const NeighborData* visitor = &g_neighbors[ctx->visitor_index];

    if (strcmp(ctx->phone.dialed_digits, "3312") == 0) {
        audio_play_sfx(SFX_PHONE_RING);
        snprintf(ctx->phone.status_message, sizeof(ctx->phone.status_message),
                 "D.D.D. Central: Protocolo activo. Si detectas anomalia, pulsa la alarma roja.");
    } else if (strcmp(ctx->phone.dialed_digits, visitor->phone_number) == 0) {
        audio_play_sfx(SFX_PHONE_RING);
        if (ctx->visitor_is_doppelganger) {
            snprintf(ctx->phone.status_message, sizeof(ctx->phone.status_message),
                     "Residente real responde: 'Hola? Estoy en mi departamento... quien esta en la puerta?!'");
        } else {
            snprintf(ctx->phone.status_message, sizeof(ctx->phone.status_message),
                     "*Linea ocupada / Tono de espera (El residente esta frente a ti)*");
        }
    } else {
        audio_play_sfx(SFX_PHONE_DIAL_TONE);
        snprintf(ctx->phone.status_message, sizeof(ctx->phone.status_message),
                 "*Sin respuesta... Numero no asignado o nadie atiende.*");
    }
}

void game_update(GameContext* ctx) {
    const PadState* pad = input_get_state();

    if (pad->start && pad->select) {
        ctx->running = false;
        return;
    }

    const float cursor_speed = 10.0f;
    ctx->cursor.x += pad->analog_lx * cursor_speed;
    ctx->cursor.y += pad->analog_ly * cursor_speed;

    if (ctx->cursor.x < 10) ctx->cursor.x = 10;
    if (ctx->cursor.x > SCREEN_WIDTH - 10) ctx->cursor.x = SCREEN_WIDTH - 10;
    if (ctx->cursor.y < 10) ctx->cursor.y = 10;
    if (ctx->cursor.y > SCREEN_HEIGHT - 10) ctx->cursor.y = SCREEN_HEIGHT - 10;

    // Camera parallax
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

    // State: Doppelganger Attack Animation Sequence
    if (ctx->state == STATE_DOPPEL_ATTACK) {
        ctx->attack_anim_timer += 0.04f;
        if (ctx->attack_anim_timer >= 2.0f) {
            ctx->doppelgangers_let_in++;
            ctx->state = STATE_GAME_OVER;
        }
        return;
    }

    // Hit-testing
    ctx->btn_door_unlock.hovered     = point_in_rect(ctx->cursor.x, ctx->cursor.y, ctx->btn_door_unlock.rect);
    ctx->btn_door_lock.hovered       = point_in_rect(ctx->cursor.x, ctx->cursor.y, ctx->btn_door_lock.rect);
    ctx->btn_emergency_cover.hovered = point_in_rect(ctx->cursor.x, ctx->cursor.y, ctx->btn_emergency_cover.rect);
    ctx->btn_emergency.hovered       = point_in_rect(ctx->cursor.x, ctx->cursor.y, ctx->btn_emergency.rect);
    ctx->btn_folder_list.hovered     = point_in_rect(ctx->cursor.x, ctx->cursor.y, ctx->btn_folder_list.rect);
    ctx->btn_folder_id.hovered       = point_in_rect(ctx->cursor.x, ctx->cursor.y, ctx->btn_folder_id.rect);
    ctx->btn_phone_desk.hovered      = point_in_rect(ctx->cursor.x, ctx->cursor.y, ctx->btn_phone_desk.rect);

    ctx->tab_btn_id.hovered    = point_in_rect(ctx->cursor.x, ctx->cursor.y, ctx->tab_btn_id.rect);
    ctx->tab_btn_entry.hovered = point_in_rect(ctx->cursor.x, ctx->cursor.y, ctx->tab_btn_entry.rect);
    ctx->tab_btn_list.hovered  = point_in_rect(ctx->cursor.x, ctx->cursor.y, ctx->tab_btn_list.rect);
    ctx->tab_btn_check.hovered = point_in_rect(ctx->cursor.x, ctx->cursor.y, ctx->tab_btn_check.rect);

    ctx->btn_restart.hovered = point_in_rect(ctx->cursor.x, ctx->cursor.y, ctx->btn_restart.rect);
    ctx->btn_quit.hovered    = point_in_rect(ctx->cursor.x, ctx->cursor.y, ctx->btn_quit.rect);

    for (int d = 0; d < 10; d++) {
        ctx->phone.dial_buttons[d].hovered = point_in_rect(ctx->cursor.x, ctx->cursor.y, ctx->phone.dial_buttons[d].rect);
    }
    ctx->phone.btn_hangup.hovered = point_in_rect(ctx->cursor.x, ctx->cursor.y, ctx->phone.btn_hangup.rect);

    // Cross (✖) Click Interactions
    if (pad->cross && !ctx->cursor_clicking) {
        ctx->cursor_clicking = true;

        if (ctx->state == STATE_OFFICE_BOOTH) {
            if (ctx->btn_door_unlock.hovered) {
                // If the visitor was a Doppelganger and player opens the door/hatch -> ATTACK!
                if (ctx->visitor_is_doppelganger) {
                    game_trigger_doppel_attack(ctx);
                } else {
                    ctx->hatch_state = HATCH_OPENING;
                    audio_play_sfx(SFX_WINDOW_SLIDE);
                    // Legitimate visitor passes safely
                    ctx->correct_clears++;
                    ctx->processed_count++;
                    game_spawn_next_visitor(ctx);
                }
            } else if (ctx->btn_door_lock.hovered && ctx->hatch_state != HATCH_CLOSED) {
                ctx->hatch_state = HATCH_CLOSING;
                audio_play_sfx(SFX_WINDOW_SLIDE);
            } else if (ctx->btn_phone_desk.hovered) {
                ctx->state = STATE_PHONE_CALL;
                ctx->phone.digit_count = 0;
                memset(ctx->phone.dialed_digits, 0, sizeof(ctx->phone.dialed_digits));
                snprintf(ctx->phone.status_message, sizeof(ctx->phone.status_message), "Marca 4 digitos...");
                audio_play_sfx(SFX_PHONE_PICKUP);
            } else if (ctx->btn_emergency_cover.hovered) {
                ctx->emergency_cover_open = !ctx->emergency_cover_open;
                audio_play_sfx(SFX_ACCEPT_BUTTON);
            } else if (ctx->emergency_cover_open && ctx->btn_emergency.hovered) {
                ctx->alarm_active = true;
                ctx->state = STATE_EMERGENCY_ALARM;
                audio_play_sfx(SFX_SIREN_ALARM);
                if (ctx->visitor_is_doppelganger) {
                    ctx->caught_doppelgangers++;
                }
            } else if (ctx->btn_folder_id.hovered) {
                ctx->active_doc_tab = DOC_TAB_ID;
                ctx->state = STATE_DOCUMENT_INSPECT;
                audio_play_sfx(SFX_FOLDER_PICKUP);
            } else if (ctx->btn_folder_list.hovered) {
                ctx->active_doc_tab = DOC_TAB_TODAYS_LIST;
                ctx->state = STATE_DOCUMENT_INSPECT;
                audio_play_sfx(SFX_FOLDER_PICKUP);
            }
        } else if (ctx->state == STATE_PHONE_CALL) {
            // Dial numbers 0-9
            for (int d = 0; d < 10; d++) {
                if (ctx->phone.dial_buttons[d].hovered && ctx->phone.digit_count < 4) {
                    ctx->phone.dialed_digits[ctx->phone.digit_count++] = '0' + d;
                    ctx->phone.dialed_digits[ctx->phone.digit_count] = '\0';
                    audio_play_sfx(SFX_PHONE_DIAL);

                    if (ctx->phone.digit_count == 4) {
                        phone_handle_call_evaluation(ctx);
                    }
                    break;
                }
            }

            if (ctx->phone.btn_hangup.hovered) {
                ctx->state = STATE_OFFICE_BOOTH;
                audio_play_sfx(SFX_PHONE_HANGUP);
            }
        } else if (ctx->state == STATE_DOCUMENT_INSPECT) {
            if (ctx->tab_btn_id.hovered) { ctx->active_doc_tab = DOC_TAB_ID; audio_play_sfx(SFX_ACCEPT_BUTTON); }
            else if (ctx->tab_btn_entry.hovered) { ctx->active_doc_tab = DOC_TAB_ENTRY_REQUEST; audio_play_sfx(SFX_ACCEPT_BUTTON); }
            else if (ctx->tab_btn_list.hovered) { ctx->active_doc_tab = DOC_TAB_TODAYS_LIST; audio_play_sfx(SFX_ACCEPT_BUTTON); }
            else if (ctx->tab_btn_check.hovered) { ctx->active_doc_tab = DOC_TAB_CHECKLIST; audio_play_sfx(SFX_ACCEPT_BUTTON); }

            if (ctx->active_doc_tab == DOC_TAB_CHECKLIST) {
                Rect2D r1 = { 400, 240, 40, 40 };
                Rect2D r2 = { 400, 310, 40, 40 };
                Rect2D r3 = { 400, 380, 40, 40 };
                Rect2D r4 = { 400, 450, 40, 40 };
                Rect2D r5 = { 400, 520, 40, 40 };
                Rect2D r6 = { 400, 590, 40, 40 };

                if (point_in_rect(ctx->cursor.x, ctx->cursor.y, r1)) { ctx->checklist.photo_match = !ctx->checklist.photo_match; audio_play_sfx(SFX_PEN_WRITE); }
                else if (point_in_rect(ctx->cursor.x, ctx->cursor.y, r2)) { ctx->checklist.id_number_match = !ctx->checklist.id_number_match; audio_play_sfx(SFX_PEN_WRITE); }
                else if (point_in_rect(ctx->cursor.x, ctx->cursor.y, r3)) { ctx->checklist.not_expired = !ctx->checklist.not_expired; audio_play_sfx(SFX_PEN_WRITE); }
                else if (point_in_rect(ctx->cursor.x, ctx->cursor.y, r4)) { ctx->checklist.entry_permit_valid = !ctx->checklist.entry_permit_valid; audio_play_sfx(SFX_PEN_WRITE); }
                else if (point_in_rect(ctx->cursor.x, ctx->cursor.y, r5)) { ctx->checklist.appearance_match = !ctx->checklist.appearance_match; audio_play_sfx(SFX_PEN_WRITE); }
                else if (point_in_rect(ctx->cursor.x, ctx->cursor.y, r6)) { ctx->checklist.on_today_list = !ctx->checklist.on_today_list; audio_play_sfx(SFX_PEN_WRITE); }
            }
        } else if (ctx->state == STATE_GAME_OVER) {
            if (ctx->btn_restart.hovered) {
                ctx->state = STATE_OFFICE_BOOTH;
                ctx->doppelgangers_let_in = 0;
                ctx->processed_count = 0;
                ctx->correct_clears = 0;
                ctx->caught_doppelgangers = 0;
                game_spawn_next_visitor(ctx);
            } else if (ctx->btn_quit.hovered) {
                ctx->running = false;
            }
        }
    } else if (!pad->cross) {
        ctx->cursor_clicking = false;
    }

    // Circle (●) Button returns to desk
    if (pad->circle) {
        if (ctx->state == STATE_DOCUMENT_INSPECT || ctx->state == STATE_EMERGENCY_ALARM || ctx->state == STATE_PHONE_CALL) {
            ctx->state = STATE_OFFICE_BOOTH;
            audio_play_sfx(SFX_FOLDER_PUT_BACK);
        }
    }
}

void game_render(GameContext* ctx) {
    graphics_clear(0xFF181410);

    switch (ctx->state) {
        case STATE_OFFICE_BOOTH: {
            int cam_dx = (int)(ctx->camera.position.x - CAMERA_CENTER_X);
            int cam_dy = (int)(ctx->camera.position.y - CAMERA_CENTER_Y);

            if (ctx->tex_background) graphics_draw_texture(ctx->tex_background, -cam_dx, -cam_dy);
            else graphics_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0xFF2d2822);

            if (ctx->tex_wall) graphics_draw_texture(ctx->tex_wall, -cam_dx, -cam_dy);

            int win_x = 480 - cam_dx;
            int win_y = 120 - cam_dy;
            int win_w = 960;
            int win_h = 560;

            if (ctx->hatch_progress > 0.05f && ctx->visitor_present) {
                int vis_x = win_x + (win_w / 2) - 150;
                int vis_y = win_y + 80;
                Texture* char_tex = (ctx->visitor_index == 0) ? ctx->tex_roman_photo : ctx->tex_francis_photo;
                if (char_tex) {
                    graphics_draw_texture_scaled(char_tex, vis_x, vis_y, 300, 420);
                } else {
                    graphics_draw_rect(vis_x, vis_y, 300, 420, 0xFF5a4a3a);
                    graphics_draw_rect(vis_x + 80, vis_y + 40, 140, 160, 0xFFd4b290);
                }
                const NeighborData* visitor = &g_neighbors[ctx->visitor_index];
                char tag[128];
                snprintf(tag, sizeof(tag), "%s %s", visitor->name, visitor->last_name);
                graphics_draw_text(vis_x + 20, vis_y + 430, tag, 0xFFE0E0E0);
            }

            int shutter_h = (int)((1.0f - ctx->hatch_progress) * win_h);
            if (shutter_h > 0) {
                if (ctx->tex_panic_window) {
                    graphics_draw_texture_clipped(ctx->tex_panic_window, win_x, win_y, 0, shutter_h);
                } else {
                    graphics_draw_rect(win_x, win_y, win_w, shutter_h, 0xFF44494e);
                }
            }

            // Door control
            if (ctx->tex_door_control) {
                graphics_draw_texture(ctx->tex_door_control, 1260, 680);
            } else {
                graphics_draw_rect(ctx->btn_door_unlock.rect.x, ctx->btn_door_unlock.rect.y,
                                   ctx->btn_door_unlock.rect.w, ctx->btn_door_unlock.rect.h,
                                   ctx->btn_door_unlock.hovered ? 0xFF2e7d32 : 0xFF1b5e20);
                graphics_draw_rect(ctx->btn_door_lock.rect.x, ctx->btn_door_lock.rect.y,
                                   ctx->btn_door_lock.rect.w, ctx->btn_door_lock.rect.h,
                                   ctx->btn_door_lock.hovered ? 0xFFc62828 : 0xFF8e0000);
            }

            // Phone on desk
            if (ctx->tex_phone_base) {
                graphics_draw_texture_scaled(ctx->tex_phone_base, ctx->btn_phone_desk.rect.x, ctx->btn_phone_desk.rect.y, 200, 200);
            } else {
                graphics_draw_rect(ctx->btn_phone_desk.rect.x, ctx->btn_phone_desk.rect.y, 200, 200, 0xFF1c1c1c);
            }
            graphics_draw_text(ctx->btn_phone_desk.rect.x + 30, ctx->btn_phone_desk.rect.y + 80, "TELEFONO", 0xFFFFFFFF);

            // Papers
            graphics_draw_rect(ctx->btn_folder_list.rect.x, ctx->btn_folder_list.rect.y, 220, 140, 0xFF998055);
            graphics_draw_text(ctx->btn_folder_list.rect.x + 20, ctx->btn_folder_list.rect.y + 55, "LISTA D.D.D.", 0xFF111111);

            graphics_draw_rect(ctx->btn_folder_id.rect.x, ctx->btn_folder_id.rect.y, 220, 140, 0xFFaaaa99);
            graphics_draw_text(ctx->btn_folder_id.rect.x + 35, ctx->btn_folder_id.rect.y + 55, "PAPELEO ID", 0xFF111111);

            // Emergency Panic Button
            if (ctx->emergency_cover_open) {
                graphics_draw_rect(ctx->btn_emergency.rect.x, ctx->btn_emergency.rect.y,
                                   ctx->btn_emergency.rect.w, ctx->btn_emergency.rect.h,
                                   ctx->btn_emergency.hovered ? 0xFFff1744 : 0xFFd50000);
                graphics_draw_text(ctx->btn_emergency.rect.x + 10, ctx->btn_emergency.rect.y + 40, "D.D.D.", 0xFFFFFFFF);
            } else {
                graphics_draw_rect(ctx->btn_emergency_cover.rect.x, ctx->btn_emergency_cover.rect.y,
                                   ctx->btn_emergency_cover.rect.w, ctx->btn_emergency_cover.rect.h, 0xFFfbc02d);
                graphics_draw_text(ctx->btn_emergency_cover.rect.x + 15, ctx->btn_emergency_cover.rect.y + 45, "PELIGRO", 0xFF111111);
            }

            if (ctx->tex_ddd_logo) graphics_draw_texture_scaled(ctx->tex_ddd_logo, 60, 40, 120, 120);

            graphics_draw_text(200, 50, "CABINA D.D.D. #33 | PS3 HOMEBREW EDITION", 0xFFFFD700);
            graphics_draw_text(50, 1020, "[L1/R1] Abrir/Cerrar  [X] Interactuar  [[]] Documentos  [/\\\\] Alarma D.D.D.", 0xFFAAAAAA);
            break;
        }

        case STATE_PHONE_CALL: {
            graphics_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0xEE111111);

            int px = 640;
            int py = 80;
            int pw = 640;
            int ph = 860;
            graphics_draw_rect(px, py, pw, ph, 0xFF242220);
            graphics_draw_rect(px, py, pw, 60, 0xFF8b1e1e);
            graphics_draw_text(px + 120, py + 18, "TELEFONO ANALOGO D.D.D. - MARCADO", 0xFFFFFFFF);

            // Dial display screen
            graphics_draw_rect(px + 80, py + 100, pw - 160, 70, 0xFF080808);
            char dial_screen[64];
            snprintf(dial_screen, sizeof(dial_screen), "NUMERO: %s", ctx->phone.dialed_digits[0] ? ctx->phone.dialed_digits : "----");
            graphics_draw_text(px + 100, py + 125, dial_screen, 0xFF00FF66);

            // Rotary wheel background
            if (ctx->tex_phone_roulette) {
                graphics_draw_texture_scaled(ctx->tex_phone_roulette, px + 100, py + 220, 440, 440);
            } else {
                graphics_draw_rect(px + 120, py + 240, 400, 400, 0xFF151515);
            }

            // Dial buttons 0-9
            for (int d = 0; d < 10; d++) {
                DeskButton* btn = &ctx->phone.dial_buttons[d];
                graphics_draw_rect(btn->rect.x, btn->rect.y, btn->rect.w, btn->rect.h,
                                   btn->hovered ? 0xFFc49a45 : 0xFFe0e0e0);
                graphics_draw_text(btn->rect.x + 22, btn->rect.y + 18, btn->label, 0xFF111111);
            }

            // Status message from dispatcher / line
            graphics_draw_text(px + 40, py + 720, ctx->phone.status_message, 0xFFFFD700);

            // Hang up button
            graphics_draw_rect(ctx->phone.btn_hangup.rect.x, ctx->phone.btn_hangup.rect.y,
                               ctx->phone.btn_hangup.rect.w, ctx->phone.btn_hangup.rect.h,
                               ctx->phone.btn_hangup.hovered ? 0xFFd32f2f : 0xFF8b1e1e);
            graphics_draw_text(ctx->phone.btn_hangup.rect.x + 40, ctx->phone.btn_hangup.rect.y + 20, "COLGAR [O]", 0xFFFFFFFF);
            break;
        }

        case STATE_DOPPEL_ATTACK: {
            // Screen flashes blood red
            graphics_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0xCC7f0000);

            // Jumpscare Hands reach and scale up to camera
            float scale = 1.0f + (ctx->attack_anim_timer * 1.5f);
            int hand_w = (int)(240 * scale);
            int hand_h = (int)(320 * scale);

            if (ctx->tex_doppel_hand_l) {
                graphics_draw_texture_scaled(ctx->tex_doppel_hand_l, 400 - (int)(scale * 80), 300, hand_w, hand_h);
            }
            if (ctx->tex_doppel_hand_r) {
                graphics_draw_texture_scaled(ctx->tex_doppel_hand_r, 1100 + (int)(scale * 40), 300, hand_w, hand_h);
            }

            graphics_draw_text(680, 200, "!!! NO ES TU VECINO !!!", 0xFFFFFFFF);
            break;
        }

        case STATE_GAME_OVER: {
            graphics_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0xFF080606);

            graphics_draw_text(580, 240, "HAS SIDO ELIMINADO POR UN DOPPELGANGER", 0xFFd32f2f);
            graphics_draw_text(680, 320, "RESUMEN FINAL DEL D.D.D.", 0xFFFFFFFF);

            char stat1[128], stat2[128], stat3[128];
            snprintf(stat1, sizeof(stat1), "Doppels que ingresaron: %d", ctx->doppelgangers_let_in);
            snprintf(stat2, sizeof(stat2), "Doppels capturados: %d", ctx->caught_doppelgangers);
            snprintf(stat3, sizeof(stat3), "Residentes autorizados correctamente: %d", ctx->correct_clears);

            graphics_draw_text(640, 420, stat1, 0xFFff8a80);
            graphics_draw_text(640, 470, stat2, 0xFFb9f6ca);
            graphics_draw_text(640, 520, stat3, 0xFFFFFFFF);

            graphics_draw_text(820, 590, "RANGO: F", 0xFFd50000);

            // Restart & Quit buttons
            graphics_draw_rect(ctx->btn_restart.rect.x, ctx->btn_restart.rect.y,
                               ctx->btn_restart.rect.w, ctx->btn_restart.rect.h,
                               ctx->btn_restart.hovered ? 0xFF2e7d32 : 0xFF1b5e20);
            graphics_draw_text(ctx->btn_restart.rect.x + 35, ctx->btn_restart.rect.y + 25, "[X] OTRA VEZ", 0xFFFFFFFF);

            graphics_draw_rect(ctx->btn_quit.rect.x, ctx->btn_quit.rect.y,
                               ctx->btn_quit.rect.w, ctx->btn_quit.rect.h,
                               ctx->btn_quit.hovered ? 0xFFc62828 : 0xFF8e0000);
            graphics_draw_text(ctx->btn_quit.rect.x + 55, ctx->btn_quit.rect.y + 25, "[O] SALIR", 0xFFFFFFFF);
            break;
        }

        case STATE_DOCUMENT_INSPECT: {
            const NeighborData* visitor = &g_neighbors[ctx->visitor_index];
            graphics_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0xEE12100d);

            DeskButton* tabs[4] = { &ctx->tab_btn_id, &ctx->tab_btn_entry, &ctx->tab_btn_list, &ctx->tab_btn_check };
            for (int t = 0; t < 4; t++) {
                uint32_t bg = (ctx->active_doc_tab == (DocTab)t) ? 0xFF8b1e1e : (tabs[t]->hovered ? 0xFF554433 : 0xFF332820);
                graphics_draw_rect(tabs[t]->rect.x, tabs[t]->rect.y, tabs[t]->rect.w, tabs[t]->rect.h, bg);
                graphics_draw_text(tabs[t]->rect.x + 15, tabs[t]->rect.y + 15, tabs[t]->label, 0xFFFFFFFF);
            }

            int main_x = 280;
            int main_y = 130;
            int main_w = 1360;
            int main_h = 760;

            if (ctx->active_doc_tab == DOC_TAB_ID) {
                if (ctx->tex_id_card) {
                    graphics_draw_texture_scaled(ctx->tex_id_card, main_x, main_y, main_w, main_h);
                } else {
                    graphics_draw_rect(main_x, main_y, main_w, main_h, 0xFFf5f0e1);
                    graphics_draw_rect(main_x, main_y, main_w, 60, 0xFF8b1e1e);
                }

                Texture* photo = (ctx->visitor_index == 0) ? ctx->tex_roman_photo : ctx->tex_francis_photo;
                if (photo) {
                    graphics_draw_texture_scaled(photo, main_x + 60, main_y + 120, 260, 320);
                } else {
                    graphics_draw_rect(main_x + 60, main_y + 120, 260, 320, 0xFFd4b290);
                }

                char buf[256];
                snprintf(buf, sizeof(buf), "NOMBRE: %s", ctx->visitor_docs.displayed_name);
                graphics_draw_text(main_x + 380, main_y + 140, buf, 0xFF111111);

                snprintf(buf, sizeof(buf), "APELLIDO: %s", ctx->visitor_docs.displayed_last_name);
                graphics_draw_text(main_x + 380, main_y + 200, buf, 0xFF111111);

                snprintf(buf, sizeof(buf), "NO. IDENTIFICACION: %s", ctx->visitor_docs.displayed_id_number);
                graphics_draw_text(main_x + 380, main_y + 260, buf, 0xFF111111);

                snprintf(buf, sizeof(buf), "FECHA EXPIRACION: %s", ctx->visitor_docs.displayed_expiration);
                graphics_draw_text(main_x + 380, main_y + 320, buf, 0xFF111111);

                snprintf(buf, sizeof(buf), "APARTAMENTO: %s (%s)", ctx->visitor_docs.displayed_apartment, visitor->apt_number_en);
                graphics_draw_text(main_x + 380, main_y + 380, buf, 0xFF111111);

                snprintf(buf, sizeof(buf), "OCUPACION: %s", visitor->occupation_es);
                graphics_draw_text(main_x + 60, main_y + 490, buf, 0xFF222222);

            } else if (ctx->active_doc_tab == DOC_TAB_ENTRY_REQUEST) {
                graphics_draw_rect(main_x, main_y, main_w, main_h, 0xFFf0e6d2);
                graphics_draw_rect(main_x, main_y, main_w, 60, 0xFF2e4a62);
                graphics_draw_text(main_x + 30, main_y + 18, "SOLICITUD DE INGRESO - DEPARTAMENTO DE DETECCION", 0xFFFFFFFF);

                char buf[256];
                snprintf(buf, sizeof(buf), "Solicitante: %s %s", ctx->visitor_docs.displayed_name, ctx->visitor_docs.displayed_last_name);
                graphics_draw_text(main_x + 80, main_y + 140, buf, 0xFF111111);

                snprintf(buf, sizeof(buf), "Destino: Apartamento %s", ctx->visitor_docs.displayed_apartment);
                graphics_draw_text(main_x + 80, main_y + 210, buf, 0xFF111111);

                graphics_draw_text(main_x + 80, main_y + 280, "Motivo de Ingreso: Regreso a su residencia habitual tras jornada laboral.", 0xFF222222);
                graphics_draw_text(main_x + 80, main_y + 350, "Estado de Autorizacion: APROBADO PREVIAMENTE POR D.D.D.", 0xFF1b5e20);

            } else if (ctx->active_doc_tab == DOC_TAB_TODAYS_LIST) {
                if (ctx->tex_todays_list) {
                    graphics_draw_texture_scaled(ctx->tex_todays_list, main_x, main_y, main_w, main_h);
                } else {
                    graphics_draw_rect(main_x, main_y, main_w, main_h, 0xFFe8e4d8);
                    graphics_draw_rect(main_x, main_y, main_w, 60, 0xFF3d352e);
                }

                for (int i = 0; i < 6; i++) {
                    int n_idx = (ctx->processed_count + i) % 20;
                    const NeighborData* n = &g_neighbors[n_idx];
                    char line[256];
                    snprintf(line, sizeof(line), "%d. %s %s  -  Depto: %s  -  Tel: %s",
                             i + 1, n->name, n->last_name, n->apt_number_es, n->phone_number);

                    uint32_t col = (ctx->visitor_docs.is_on_today_list && n_idx == ctx->visitor_index) ? 0xFF8b1e1e : 0xFF111111;
                    graphics_draw_text(main_x + 80, main_y + 160 + (i * 65), line, col);
                }

            } else if (ctx->active_doc_tab == DOC_TAB_CHECKLIST) {
                graphics_draw_rect(main_x, main_y, main_w, main_h, 0xFFf7f5eb);
                graphics_draw_rect(main_x, main_y, main_w, 60, 0xFF8b1e1e);
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
                    graphics_draw_rect(main_x + 60, cy, 40, 40, 0xFF332820);
                    if (checks[c]) {
                        graphics_draw_rect(main_x + 68, cy + 8, 24, 24, 0xFF2e7d32);
                    }
                    graphics_draw_text(main_x + 120, cy + 8, items[c], 0xFF111111);
                }

                graphics_draw_text(main_x + 60, main_y + 570, "Rasgos en Archivo:", 0xFF8b1e1e);
                graphics_draw_text(main_x + 60, main_y + 610, visitor->appearance_es, 0xFF333333);
            }

            graphics_draw_text(main_x + 350, 930, "[L1/R1] Cambiar Documento   [X] Marcar   [O] Volver al Despacho", 0xFFFFD700);
            break;
        }

        case STATE_EMERGENCY_ALARM: {
            graphics_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0xFF7f0000);
            graphics_draw_text(480, 400, "!!! ALARMA DE EMERGENCIA D.D.D. ACTIVADA !!!", 0xFFFFFFFF);
            graphics_draw_text(520, 480, "Escuadron de Limpieza en Camino...", 0xFFFFCCCC);
            graphics_draw_text(600, 650, "Pulsa [O] para Regresar", 0xFFFFD700);
            break;
        }

        default:
            break;
    }

    if (ctx->tex_cursor) {
        graphics_draw_texture(ctx->tex_cursor, (int)ctx->cursor.x - 16, (int)ctx->cursor.y - 16);
    } else {
        graphics_draw_cursor(ctx->cursor.x, ctx->cursor.y, 0xFFFFFFFF);
    }
}

void game_cleanup(GameContext* ctx) {
    game_free_textures(ctx);
    printf("[Game] Cleaned up textures and context.\n");
}
