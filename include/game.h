#ifndef GAME_H
#define GAME_H

#include "common.h"
#include "graphics.h"
#include "neighbors_data.h"

#define CAMERA_CENTER_X     960.0f
#define CAMERA_CENTER_Y     540.0f
#define CAMERA_MIN_X        780.0f
#define CAMERA_MAX_X        1148.0f
#define CAMERA_MIN_Y        480.0f
#define CAMERA_MAX_Y        595.0f

typedef enum {
    STATE_TITLE,
    STATE_OFFICE_BOOTH,
    STATE_DOCUMENT_INSPECT,
    STATE_PHONE_CALL,
    STATE_DOPPEL_ATTACK,
    STATE_EMERGENCY_ALARM,
    STATE_GAME_OVER,
    STATE_VICTORY_SUMMARY
} GameState;

typedef enum {
    HATCH_CLOSED,
    HATCH_OPENING,
    HATCH_OPEN,
    HATCH_CLOSING
} HatchState;

typedef enum {
    DOC_TAB_ID,
    DOC_TAB_ENTRY_REQUEST,
    DOC_TAB_TODAYS_LIST,
    DOC_TAB_CHECKLIST
} DocTab;

typedef struct {
    Vector2D position;
    Vector2D target;
} Camera;

typedef struct {
    Rect2D rect;
    bool hovered;
    bool pressed;
    const char* label;
} DeskButton;

typedef struct {
    bool photo_match;
    bool id_number_match;
    bool not_expired;
    bool entry_permit_valid;
    bool appearance_match;
    bool on_today_list;
} VerificationChecklist;

typedef struct {
    char displayed_name[64];
    char displayed_last_name[64];
    char displayed_id_number[32];
    char displayed_expiration[16];
    char displayed_apartment[16];
    bool is_on_today_list;
    bool has_doppelganger_flaw;
    int flaw_type;
} CurrentVisitorDocs;

typedef struct {
    char dialed_digits[8];
    int digit_count;
    bool in_call;
    float call_timer;
    char status_message[128];
    DeskButton dial_buttons[10];
    DeskButton btn_hangup;
} PhoneContext;

typedef struct {
    GameState state;
    bool running;
    Camera camera;
    Vector2D cursor;
    bool cursor_clicking;

    // Document inspection
    DocTab active_doc_tab;
    VerificationChecklist checklist;
    CurrentVisitorDocs visitor_docs;

    // Rotary Phone Context
    PhoneContext phone;

    // Security Booth State
    HatchState hatch_state;
    float hatch_progress;
    bool emergency_cover_open;
    bool alarm_active;

    // Visitor
    int visitor_index;
    bool visitor_is_doppelganger;
    bool visitor_present;

    // Attack sequence
    float attack_anim_timer;

    // Desk interactive buttons
    DeskButton btn_door_unlock;
    DeskButton btn_door_lock;
    DeskButton btn_emergency;
    DeskButton btn_emergency_cover;
    DeskButton btn_folder_list;
    DeskButton btn_folder_id;
    DeskButton btn_phone_desk;

    // Document tabs
    DeskButton tab_btn_id;
    DeskButton tab_btn_entry;
    DeskButton tab_btn_list;
    DeskButton tab_btn_check;

    // Game Over buttons
    DeskButton btn_restart;
    DeskButton btn_quit;

    // Textures
    Texture* tex_background;
    Texture* tex_wall;
    Texture* tex_panic_window;
    Texture* tex_cursor;
    Texture* tex_door_control;
    Texture* tex_id_card;
    Texture* tex_paper;
    Texture* tex_todays_list;
    Texture* tex_ddd_logo;
    Texture* tex_phone_base;
    Texture* tex_phone_roulette;
    Texture* tex_doppel_hand_l;
    Texture* tex_doppel_hand_r;
    Texture* tex_roman_photo;
    Texture* tex_francis_photo;

    // Game Statistics / Rank
    int current_day;
    int processed_count;
    int correct_clears;
    int caught_doppelgangers;
    int doppelgangers_let_in;
} GameContext;

void game_init(GameContext* ctx);
void game_load_textures(GameContext* ctx);
void game_free_textures(GameContext* ctx);
void game_update(GameContext* ctx);
void game_render(GameContext* ctx);
void game_spawn_next_visitor(GameContext* ctx);
void game_trigger_doppel_attack(GameContext* ctx);
void game_cleanup(GameContext* ctx);

#endif // GAME_H
