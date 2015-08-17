#define UTOX_DEFAULT_AUDIO_BITRATE 32
#define UTOX_DEFAULT_AUDIO_FRAME_DURATION 20
#define UTOX_DEFAULT_AUDIO_SAMPLE_RATE 48000
#define UTOX_DEFAULT_AUDIO_CHANNELS 1

#define UTOX_SENDING_AUDIO(f_number)   ( !!(friend[f_number].call_state_self   & TOXAV_FRIEND_CALL_STATE_SENDING_A   ))
#define UTOX_ACCEPTING_AUDIO(f_number) ( !!(friend[f_number].call_state_friend & TOXAV_FRIEND_CALL_STATE_ACCEPTING_A ))

#define UTOX_SEND_AUDIO(f_number)   ( !!(friend[f_number].call_state_self   & TOXAV_FRIEND_CALL_STATE_SENDING_A  ) && \
                                      !!(friend[f_number].call_state_friend & TOXAV_FRIEND_CALL_STATE_ACCEPTING_A) )

#define UTOX_ACCEPT_AUDIO(f_number) ( !!(friend[f_number].call_state_self   & TOXAV_FRIEND_CALL_STATE_ACCEPTING_A ) && \
                                      !!(friend[f_number].call_state_friend & TOXAV_FRIEND_CALL_STATE_SENDING_A)  )

#ifndef AUDIO_FILTERING
    typedef uint8_t Filter_Audio;
#endif

void sourceplaybuffer(int i, const int16_t *data, int samples, uint8_t channels, unsigned int sample_rate);

void audio_thread(void *args);
