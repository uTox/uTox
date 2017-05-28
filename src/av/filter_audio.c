#include "filter_audio.h"

#include "audio.h"

#include "../settings.h"

#ifdef AUDIO_FILTERING
#include <filter_audio.h>
#endif

Filter_Audio *f_a = NULL;

void filter_audio_check(void) {
#ifdef AUDIO_FILTERING
    if (!f_a && settings.audiofilter_enabled) {
        f_a = new_filter_audio(UTOX_DEFAULT_SAMPLE_RATE_A);
        if (!f_a) {
            settings.audiofilter_enabled = false;
            LOG_TRACE("Filter Audio", "filter audio failed" );
        } else {
            LOG_TRACE("Filter Audio", "filter audio on" );
        }
    } else if (f_a && !settings.audiofilter_enabled) {
        kill_filter_audio(f_a);
        f_a = NULL;
        LOG_TRACE("Filter Audio", "filter audio off" );
    }
#else
    if (settings.audiofilter_enabled) {
        settings.audiofilter_enabled = false;
    }
#endif
}
