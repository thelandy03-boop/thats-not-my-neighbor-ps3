#include "audio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if defined(__CELLOS_LV2__) || defined(_PS3)
#include <audio/audio.h>
#endif

static bool s_audio_initialized = false;
static char s_current_music[128] = {0};

static const char* audio_resolve_path(const char* filename, char* buffer, size_t buf_size) {
    if (!filename) return NULL;

    FILE* f = fopen(filename, "rb");
    if (f) {
        fclose(f);
        return filename;
    }

    snprintf(buffer, buf_size, "usrdir/assets/audio/%s", filename);
    f = fopen(buffer, "rb");
    if (f) {
        fclose(f);
        return buffer;
    }

    snprintf(buffer, buf_size, "/app_home/usrdir/assets/audio/%s", filename);
    f = fopen(buffer, "rb");
    if (f) {
        fclose(f);
        return buffer;
    }

    snprintf(buffer, buf_size, "/dev_hdd0/game/TNMN00001/USRDIR/assets/audio/%s", filename);
    f = fopen(buffer, "rb");
    if (f) {
        fclose(f);
        return buffer;
    }

    return filename;
}

bool audio_init(void) {
    if (s_audio_initialized) return true;

    printf("[Audio] Initializing Cell Audio subsystem...\n");

#if defined(__CELLOS_LV2__) || defined(_PS3)
    int ret = audioInit();
    if (ret != 0) {
        printf("[Audio] Warning: audioInit returned %d. Continuing in silent mode.\n", ret);
    }
#endif

    s_audio_initialized = true;
    printf("[Audio] Audio subsystem initialized.\n");
    return true;
}

void audio_play_sfx(const char* sound_name) {
    if (!sound_name || sound_name[0] == '\0') return;

    char resolved[512];
    const char* path = audio_resolve_path(sound_name, resolved, sizeof(resolved));

    printf("[Audio] SFX Triggered: %s (Path: %s)\n", sound_name, path);
    // On PS3 PSL1GHT hardware, sound events can be dispatched or streamed to audioPort
}

void audio_play_music(const char* music_name, bool loop) {
    if (!music_name || music_name[0] == '\0') return;

    char resolved[512];
    const char* path = audio_resolve_path(music_name, resolved, sizeof(resolved));

    strncpy(s_current_music, music_name, sizeof(s_current_music) - 1);
    printf("[Audio] BGM Play: %s (Loop: %s, Path: %s)\n",
           music_name, loop ? "YES" : "NO", path);
}

void audio_stop_music(void) {
    if (s_current_music[0] != '\0') {
        printf("[Audio] BGM Stopped: %s\n", s_current_music);
        s_current_music[0] = '\0';
    }
}

void audio_cleanup(void) {
    if (!s_audio_initialized) return;

    audio_stop_music();

#if defined(__CELLOS_LV2__) || defined(_PS3)
    audioQuit();
#endif

    s_audio_initialized = false;
    printf("[Audio] Audio subsystem cleaned up.\n");
}
