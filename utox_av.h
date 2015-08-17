/* toxav thread messages (sent from the client thread) */
enum {
    AUDIO_KILL,
    AUDIO_START,
    AUDIO_END,
    AUDIO_SET_INPUT,
    AUDIO_SET_OUTPUT,
    AUDIO_PREVIEW_START,
    AUDIO_PREVIEW_END,
    AUDIO_PLAY_RINGTONE,
    AUDIO_STOP_RINGTONE,
    GROUP_AUDIO_CALL_START,
    GROUP_AUDIO_CALL_END,
};

enum {
    VIDEO_KILL,
    VIDEO_START,
    VIDEO_END,
    VIDEO_SET,
    VIDEO_PREVIEW_START,
    VIDEO_PREVIEW_END,
};

void set_av_callbacks(ToxAV *av);

void toxaudio_postmessage(uint8_t msg, uint32_t param1, uint32_t param2, void *data);

void audio_thread(void *args);

void toxav_thread(void *args);

void utox_av_local_disconnect(ToxAV *av, int32_t friend_number);

void callback_av_group_audio(Tox *tox, int groupnumber, int peernumber, const int16_t *pcm, unsigned int samples,
                                    uint8_t channels, unsigned int sample_rate, void *userdata);

// void group_av_peer_add(GROUPCHAT *g, int peernumber);
// void group_av_peer_remove(GROUPCHAT *g, int peernumber);
