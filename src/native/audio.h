#ifndef NATIVE_AUDIO_H
#define NATIVE_AUDIO_H

#include <stdbool.h>
#include <stdint.h>

// Audio
void audio_detect(void);
bool audio_init(void *handle);
bool audio_close(void *handle);
bool audio_frame(int16_t *buffer);


#if defined __ANDROID__
void audio_play(int32_t call_index, const int16_t *data, int length, uint8_t channels);
void audio_begin(int32_t call_index);
void audio_end(int32_t call_index);
#endif

#endif
