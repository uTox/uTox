/* toxav thread messages (sent from the client thread to the audio or video thread) */

/* utox av thread commands */
enum {
    UTOXAV_KILL,
    UTOXAV_START_CALL,
    UTOXAV_END_CALL,
    UTOXAV_START_PREVIEW,
    UTOXAV_END_PREVIEW,

};

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
    // kill the video thread
    VIDEO_KILL,
    // start a video call
    VIDEO_RECORD_START,
    // end a video call
    VIDEO_RECORD_STOP,
    // set a new video device
    VIDEO_SET,
    // start a video preview
    VIDEO_PREVIEW_START,
    // stop a video preview
    VIDEO_PREVIEW_STOP,
};

void toxav_postmessage(uint8_t msg, uint32_t param1, uint32_t param2, void *data);

void toxav_thread(void *args);

void utox_av_local_disconnect(ToxAV *av, int32_t friend_number);

void utox_av_local_call_control(ToxAV *av, uint32_t friend_number, TOXAV_CALL_CONTROL control);

void set_av_callbacks(ToxAV *av);

// void callback_av_group_audio(Tox *tox, int groupnumber, int peernumber, const int16_t *pcm, unsigned int samples,
//                                     uint8_t channels, unsigned int sample_rate, void *userdata);
// void group_av_peer_add(GROUPCHAT *g, int peernumber);
// void group_av_peer_remove(GROUPCHAT *g, int peernumber);
