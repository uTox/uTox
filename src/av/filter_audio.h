#ifndef FILTER_AUDIO_H
#define FILTER_AUDIO_H

#include <stdint.h>

#ifdef AUDIO_FILTERING
#include <filter_audio.h>
#else
typedef uint8_t Filter_Audio;
#endif

extern Filter_Audio *f_a;

void filter_audio_check(void);

#endif
