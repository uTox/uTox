#define UTOX_DEFAULT_BITRATE_A      32
#define UTOX_DEFAULT_FRAME_A        20
#define UTOX_DEFAULT_SAMPLE_RATE_A  48000
#define UTOX_DEFAULT_AUDIO_CHANNELS 1

/* Check self */
#define UTOX_SENDING_AUDIO(f_number)   ( !!(friend[f_number].call_state_self & TOXAV_FRIEND_CALL_STATE_SENDING_A   ))
#define UTOX_ACCEPTING_AUDIO(f_number) ( !!(friend[f_number].call_state_self & TOXAV_FRIEND_CALL_STATE_ACCEPTING_A ))

/* Check friend */
#define UTOX_AVAILABLE_AUDIO(f_number) ( !!(friend[f_number].call_state_friend & TOXAV_FRIEND_CALL_STATE_SENDING_A ))

/* Check both */
#define UTOX_SEND_AUDIO(f_number)   ( !!(friend[f_number].call_state_self   & TOXAV_FRIEND_CALL_STATE_SENDING_A  ) && \
                                      !!(friend[f_number].call_state_friend & TOXAV_FRIEND_CALL_STATE_ACCEPTING_A) )
#define UTOX_ACCEPT_AUDIO(f_number) ( !!(friend[f_number].call_state_self   & TOXAV_FRIEND_CALL_STATE_ACCEPTING_A ) && \
                                      !!(friend[f_number].call_state_friend & TOXAV_FRIEND_CALL_STATE_SENDING_A)  )

#ifndef AUDIO_FILTERING
    typedef uint8_t Filter_Audio;
#endif

void sourceplaybuffer(int i, const int16_t *data, int samples, uint8_t channels, unsigned int sample_rate);

void toxaudio_postmessage(uint8_t msg, uint32_t param1, uint32_t param2, void *data);

void audio_thread(void *args);
