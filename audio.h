#define UTOX_DEFAULT_AUDIO_BITRATE 32
#define UTOX_DEFAULT_AUDIO_FRAME_DURATION 20
#define UTOX_DEFAULT_AUDIO_SAMPLE_RATE 48000
#define UTOX_DEFAULT_AUDIO_CHANNELS 1

#define UTOX_SENDING_AUDIO(f_number)   ( !!(friend[f_number].call_state_self   & TOXAV_FRIEND_CALL_STATE_SENDING_A   ))
#define UTOX_ACCEPTING_AUDIO(f_number) ( !!(friend[f_number].call_state_friend & TOXAV_FRIEND_CALL_STATE_ACCEPTING_A ))

#ifndef AUDIO_FILTERING
    typedef uint8_t Filter_Audio;
#endif
