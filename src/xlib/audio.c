// FIXME: Required for UNUSED()
#include "../main.h"

void audio_detect(void) {}

bool audio_init(void *UNUSED(handle)) { return 0; }

bool audio_close(void *UNUSED(handle)) { return 0; }

bool audio_frame(int16_t *UNUSED(buffer)) { return 0; }
