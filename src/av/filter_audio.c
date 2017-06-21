#include "filter_audio.h"

#include "audio.h"

#include "../settings.h"

#include <stdbool.h>

#ifdef AUDIO_FILTERING
#include <filter_audio.h>
#endif

Filter_Audio *f_a = NULL;

bool filter_audio_check(void) {
#ifdef AUDIO_FILTERING
    if (!f_a && settings.audiofilter_enabled) {
        f_a = new_filter_audio(UTOX_DEFAULT_SAMPLE_RATE_A);
        if (!f_a) {
            LOG_INFO("Filter Audio", "filter audio failed" );
            return false;
        }
        LOG_INFO("Filter Audio", "filter audio on" );
    } else if (f_a && !settings.audiofilter_enabled) { //no return is needed for this one because its already false
        kill_filter_audio(f_a);
        f_a = NULL;
        LOG_INFO("Filter Audio", "filter audio off" );
    }
    return settings.audiofilter_enabled; //if there is no change return the current value
#else
    return false;
#endif
}
