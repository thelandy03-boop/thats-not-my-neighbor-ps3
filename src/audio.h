#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>

bool audio_init(void);
void audio_cleanup(void);

void audio_play_sfx(const char* sound_name);
void audio_play_music(const char* music_name, bool loop);
void audio_stop_music(void);

#endif // AUDIO_H
