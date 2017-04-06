#ifndef NATIVE_AUDIO_H
#define NATIVE_AUDIO_H

#include <stdbool.h>
#include <stdint.h>

// Audio
void audio_detect(void);
bool audio_init(void *handle);
bool audio_close(void *handle);
bool audio_frame(int16_t *buffer);

#endif
