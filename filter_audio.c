#include "main.h"

void utox_filter_audio_kill(Filter_Audio filter_audio_handle){
    #ifdef AUDIO_FILTERING
    kill_filter_audio(filter_audio_handle);
    #endif
}
