#ifndef GAME_H
#define GAME_H

#include "common.h"
#include "neighbors_data.h"

// Camera / Parallax bounds from original game (Office.gd)
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
    STATE_EMERGENCY_ALARM,
    STATE_PAUSE,
    STATE_GAME_OVER
} GameState;

typedef enum {
    HATCH_CLOSED,
    HATCH_OPENING,
    HATCH_OPEN,
    HATCH_CLOSING
} HatchState;

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
    GameState state;
    bool running;
    Camera camera;
    Vector2D cursor;
    bool cursor_clicking;

    // Security Booth State
    HatchState hatch_state;
    float hatch_progress; // 0.0 (closed) to 1.0 (open)
    bool emergency_cover_open;
    bool alarm_active;

    // Current visitor data
    int visitor_index;
    bool visitor_is_doppelganger;
    bool visitor_present;

    // Interactive buttons
    DeskButton btn_door_unlock;
    DeskButton btn_door_lock;
    DeskButton btn_emergency;
    DeskButton btn_emergency_cover;
    DeskButton btn_folder_list;
    DeskButton btn_folder_id;

    // Stats
    int current_day;
    int processed_count;
    int correct_clears;
    int caught_doppelgangers;
} GameContext;

void game_init(GameContext* ctx);
void game_update(GameContext* ctx);
void game_render(GameContext* ctx);
void game_spawn_next_visitor(GameContext* ctx);
void game_cleanup(GameContext* ctx);

#endif // GAME_H
