#include "common.h"
#include "graphics.h"
#include "input.h"
#include "audio.h"
#include "game.h"
#include <unistd.h>

#if __has_include(<sys/process.h>)
#include <sys/process.h>
SYS_PROCESS_PARAM(1001, 0x200000)
#endif

// Static allocation to avoid stack overflow on Cell OS
static GameContext s_game;

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

    if (!audio_init()) {
        printf("[TNMN-PS3] Failed to initialize audio subsystem.\n");
    }

    // Start background atmosphere audio
    audio_play_music("Music_Dark.ogg", true);

    game_init(&s_game);

    while (s_game.running) {
        input_poll();
        game_update(&s_game);
        game_render(&s_game);
        graphics_flip();
        usleep(1000);
    }

    game_cleanup(&s_game);
    audio_stop_music();
    audio_cleanup();
    input_cleanup();
    graphics_cleanup();

    printf("[TNMN-PS3] Exited cleanly.\n");
    return 0;
}
