#include "common.h"
#include "graphics.h"
#include "input.h"
#include "game.h"

int main(int argc, char* argv[]) {
    printf("[TNMN-PS3] Initializing That's Not My Neighbor on PS3...\n");

    if (!graphics_init()) {
        printf("[TNMN-PS3] Failed to initialize graphics subsystem.\n");
        return -1;
    }

    if (!input_init()) {
        printf("[TNMN-PS3] Failed to initialize input subsystem.\n");
        graphics_cleanup();
        return -1;
    }

    GameContext game;
    game_init(&game);

    while (game.running) {
        input_poll();
        game_update(&game);
        game_render(&game);
        graphics_flip();
    }

    game_cleanup(&game);
    input_cleanup();
    graphics_cleanup();

    printf("[TNMN-PS3] Exited cleanly.\n");
    return 0;
}
