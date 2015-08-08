void set_av_callbacks(ToxAV *av);

void toxaudio_postmessage(uint8_t msg, uint32_t param1, uint32_t param2, void *data);

void video_thread(void *args);
void audio_thread(void *args);

void toxav_thread(void *args);


void callback_av_group_audio(Tox *tox, int groupnumber, int peernumber, const int16_t *pcm, unsigned int samples,
                                    uint8_t channels, unsigned int sample_rate, void *userdata);

// void group_av_peer_add(GROUPCHAT *g, int peernumber);
// void group_av_peer_remove(GROUPCHAT *g, int peernumber);

