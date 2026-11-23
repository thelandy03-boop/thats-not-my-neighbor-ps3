#ifndef GAME_H
#define GAME_H

#include "common.h"

typedef enum {
    STATE_TITLE,
    STATE_DESK_VIEW,
    STATE_DOCUMENT_VIEW,
    STATE_PHONE_CALL,
    STATE_EMERGENCY_ALARM,
    STATE_GAME_OVER
} GameState;

typedef struct {
    char name[64];
    char apartment[8];
    bool is_doppelganger;
    bool has_valid_id;
    bool has_entry_permit;
} Visitor;

typedef struct {
    GameState state;
    bool running;
    Vector2D cursor;
    int current_day;
    int cleared_visitors;
    int caught_doppelgangers;
    Visitor current_visitor;
} GameContext;

void game_init(GameContext* ctx);
void game_update(GameContext* ctx);
void game_render(GameContext* ctx);
void game_cleanup(GameContext* ctx);

#endif // GAME_H
