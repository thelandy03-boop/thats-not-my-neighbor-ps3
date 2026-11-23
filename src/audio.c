#include "audio.h"
#include <stdio.h>
#include <string.h>

bool audio_init(void) {
    printf("[Audio] Audio subsystem initialized.\n");
    return true;
}

void audio_play_music(const char* filename, bool loop) {
    printf("[Audio] Playing BGM: %s (Loop: %s)\n", filename, loop ? "YES" : "NO");
}

void audio_stop_music(void) {
    printf("[Audio] BGM stopped.\n");
}

void audio_play_sfx(SoundEffect sfx) {
    const char* sfx_names[] = {
        "SFX_Window.wav",
        "SFX_Window_Break.wav",
        "SFX_Phone_Ring.ogg",
        "SFX_Pick_Up_Phone.wav",
        "SFX_Pick_Up_Folder.wav",
        "SFX_Put_Back_Folder.wav",
        "SFX_Write.wav",
        "SFX_Accept.wav",
        "SFX_Siren.ogg"
    };
    if (sfx >= 0 && sfx <= SFX_SIREN_ALARM) {
        printf("[Audio] Triggering SFX: %s\n", sfx_names[sfx]);
    }
}

void audio_cleanup(void) {
    printf("[Audio] Audio subsystem cleaned up.\n");
}
