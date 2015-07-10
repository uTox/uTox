#ifndef AUDIO_FILTERING
    typedef uint8_t Filter_Audio;
#endif


void utox_filter_audio_kill(){
#ifdef AUDIO_FILTERING
    kill_filter_audio(f_a);
#endif
}
