#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>

typedef enum {
    SFX_WINDOW_SLIDE,
    SFX_WINDOW_BREAK,
    SFX_PHONE_RING,
    SFX_PHONE_PICKUP,
    SFX_PHONE_HANGUP,
    SFX_PHONE_DIAL,
    SFX_PHONE_DIAL_TONE,
    SFX_FOLDER_PICKUP,
    SFX_FOLDER_PUT_BACK,
    SFX_PEN_WRITE,
    SFX_ACCEPT_BUTTON,
    SFX_SIREN_ALARM,
    SFX_BITE_ATTACK
} SoundEffect;

bool audio_init(void);
void audio_play_music(const char* filename, bool loop);
void audio_stop_music(void);
void audio_play_sfx(SoundEffect sfx);
void audio_cleanup(void);

#endif // AUDIO_H
