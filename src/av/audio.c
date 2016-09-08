#include "../main.h"

static void utox_filter_audio_kill(Filter_Audio *filter_audio_handle) {
#ifdef AUDIO_FILTERING
    kill_filter_audio(filter_audio_handle);
#endif
}

static ALCdevice *audio_out_handle, *audio_in_handle;
static void *     audio_out_device, *audio_in_device;
static _Bool      speakers_on, microphone_on;
static int16_t    speakers_count, microphone_count;
/* TODO hacky fix. This source list should be a VLA with a way to link sources to friends.
 * NO SRSLY don't leave this like this! */
static ALuint ringtone, preview, notifytone;

static ALuint RingBuffer, ToneBuffer;

void utox_audio_in_device_open(void) {
    if (!audio_in_device) {
        return;
    }
    if (audio_in_device == (void *)1) {
        audio_in_handle = (void *)1;
        return;
    }

    audio_in_handle = alcCaptureOpenDevice(audio_in_device, UTOX_DEFAULT_SAMPLE_RATE_A, AL_FORMAT_MONO16,
                                           (UTOX_DEFAULT_FRAME_A * UTOX_DEFAULT_SAMPLE_RATE_A * 4) / 1000);
}

void utox_audio_in_device_close(void) {
    if (audio_in_handle) {
        if (audio_in_handle == (void *)1) {
            audio_in_handle = NULL;
            microphone_on   = 0;
            return;
        }
        if (microphone_on) {
            alcCaptureStop(audio_in_handle);
        }
        alcCaptureCloseDevice(audio_in_handle);
    }
    audio_in_handle = NULL;
    microphone_on   = 0;
}

void utox_audio_in_listen(void) {
    if (microphone_on) {
        microphone_count++;
        return;
    }

    if (audio_in_handle) {
        if (audio_in_device == (void *)1) {
            audio_init(audio_in_handle);
            return;
        }
        alcCaptureStart(audio_in_handle);
    } else if (audio_in_device) {
        /* Unable to get handle, try to open it again. */
        utox_audio_in_device_open();
        if (audio_in_handle) {
            alcCaptureStart(audio_in_handle);
        } else {
            debug("uToxAudio:\tUnable to listen to device!\n");
        }
    }

    if (audio_in_handle) {
        microphone_on    = 1;
        microphone_count = 1;
    } else {
        microphone_on    = 0;
        microphone_count = 1;
    }
}

void utox_audio_in_ignore(void) {
    if (--microphone_count > 0 || !microphone_on) {
        return;
    }

    if (audio_in_handle) {
        if (audio_in_handle == (void *)1) {
            audio_close(audio_in_handle);
            microphone_on    = 0;
            microphone_count = 0;
            return;
        }
        alcCaptureStop(audio_in_handle);
    }

    microphone_on    = 0;
    microphone_count = 0;
}

void utox_audio_in_device_set(ALCdevice *new_device) {
    if (new_device) {
        audio_in_device = new_device;
        debug("uToxAudio:\tAudio in device changed.\n");
    } else {
        audio_in_device = NULL;
        audio_in_handle = NULL;
        debug("uToxAudio:\tAudio out device set to null.\n");
    }
}

ALCdevice *utox_audio_in_device_get(void) {
    if (audio_in_handle) {
        return audio_in_device;
    } else {
        return NULL;
    }
}

static ALCcontext *context;

void utox_audio_out_device_open(void) {
    if (speakers_on) {
        speakers_count++;
        return;
    }

    audio_out_handle = alcOpenDevice(audio_out_device);
    if (!audio_out_handle) {
        debug("alcOpenDevice() failed\n");
        speakers_on = 0;
        return;
    }

    context = alcCreateContext(audio_out_handle, NULL);
    if (!alcMakeContextCurrent(context)) {
        debug("alcMakeContextCurrent() failed\n");
        alcCloseDevice(audio_out_handle);
        audio_out_handle = NULL;
        speakers_on      = 0;
        return;
    }
    speakers_on = 1;
    speakers_count++;

    ALint error;
    alGetError(); /* clear errors */

    /* Create the buffers for the ringtone */
    alGenSources((ALuint)1, &preview);
    if ((error = alGetError()) != AL_NO_ERROR) {
        debug("uToxAudio:\tError generating source with err %x\n", error);
        return;
    }
    /* Create the buffers for incoming audio */
    alGenSources((ALuint)1, &ringtone);
    if ((error = alGetError()) != AL_NO_ERROR) {
        debug("uToxAudio:\tError generating source with err %x\n", error);
        return;
    }
    alGenSources((ALuint)1, &notifytone);
    if ((error = alGetError()) != AL_NO_ERROR) {
        debug("uToxAudio:\tError generating source with err %x\n", error);
        return;
    }
}

void utox_audio_out_device_close(void) {
    if (!audio_out_handle) {
        return;
    }

    if (--speakers_count > 0 || !speakers_on) {
        return;
    }

    alDeleteSources((ALuint)1, &preview);
    alDeleteSources((ALuint)1, &ringtone);
    alDeleteSources((ALuint)1, &notifytone);
    alcMakeContextCurrent(NULL);
    alcDestroyContext(context);
    alcCloseDevice(audio_out_handle);
    audio_out_handle = NULL;
    speakers_on      = 0;
    speakers_count   = 0;
}

void utox_audio_out_device_set(ALCdevice *new_device) {
    if (new_device) {
        audio_out_device = new_device;
        debug("uToxAudio:\tAudio out device changed.\n");
    } else {
        audio_out_device = NULL;
        audio_out_handle = NULL;
        debug("uToxAudio:\tAudio in device set to null.\n");
    }
}

ALCdevice *utox_audio_out_device_get(void) {
    if (audio_out_handle) {
        return audio_out_device;
    } else {
        return NULL;
    }
}

void sourceplaybuffer(unsigned int f, const int16_t *data, int samples, uint8_t channels, unsigned int sample_rate) {
    if (!channels || channels > 2) {
        return;
    }

    ALuint source;
    if (f >= UTOX_MAX_NUM_FRIENDS) {
        source = preview;
    } else {
        source = friend[f].audio_dest;
    }

    ALuint bufid;
    ALint  processed = 0, queued = 16;
    alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);
    alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);
    alSourcei(source, AL_LOOPING, AL_FALSE);

    if (processed) {
        ALuint bufids[processed];
        alSourceUnqueueBuffers(source, processed, bufids);
        alDeleteBuffers(processed - 1, bufids + 1);
        bufid = bufids[0];
    } else if (queued < 16) {
        alGenBuffers(1, &bufid);
    } else {
        debug("dropped audio frame\n");
        return;
    }

    alBufferData(bufid, (channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16, data, samples * 2 * channels,
                 sample_rate);
    alSourceQueueBuffers(source, 1, &bufid);

    // debug("audio frame || samples == %i channels == %u rate == %u \n", samples, channels, sample_rate);

    ALint state;
    alGetSourcei(source, AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING) {
        alSourcePlay(source);
        // debug("Starting source %u\n", i);
    }
}

static void audio_in_init(void) {
    const char *audio_in_device_list;
    audio_in_device_list = alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
    if (audio_in_device_list) {
        audio_in_device = (void *)audio_in_device_list;
        debug("uToxAudio:\tinput device list:\n");
        while (*audio_in_device_list) {
            debug("\t%s\n", audio_in_device_list);
            postmessage(AUDIO_IN_DEVICE, UI_STRING_ID_INVALID, 0, (void *)audio_in_device_list);
            audio_in_device_list += strlen(audio_in_device_list) + 1;
        }
    }
    postmessage(AUDIO_IN_DEVICE, STR_AUDIO_IN_NONE, 0, NULL);
    audio_detect(); /* Get audio devices for windows */
}

static void audio_out_init(void) {
    const char *audio_out_device_list;
    if (alcIsExtensionPresent(NULL, "ALC_ENUMERATE_ALL_EXT")) {
        audio_out_device_list = alcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);
    } else {
        audio_out_device_list = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
    }

    if (audio_out_device_list) {
        audio_out_device = (void *)audio_out_device_list;
        debug("uToxAudio:\toutput device list:\n");
        while (*audio_out_device_list) {
            debug("\t%s\n", audio_out_device_list);
            postmessage(AUDIO_OUT_DEVICE, 0, 0, (void *)audio_out_device_list);
            audio_out_device_list += strlen(audio_out_device_list) + 1;
        }
    }

    audio_out_handle = alcOpenDevice(audio_out_device);
    if (!audio_out_handle) {
        debug("alcOpenDevice() failed\n");
        return;
    }

    int attrlist[] = {ALC_FREQUENCY, UTOX_DEFAULT_SAMPLE_RATE_A, ALC_INVALID};

    context = alcCreateContext(audio_out_handle, attrlist);
    if (!alcMakeContextCurrent(context)) {
        debug("alcMakeContextCurrent() failed\n");
        alcCloseDevice(audio_out_handle);
        return;
    }

    // ALint error;
    // alGetError(); /* clear errors */

    // alGenSources((ALuint)1, &ringtone);
    // if ((error = alGetError()) != AL_NO_ERROR) {
    //     debug("uToxAudio:\tError generating source with err %x\n", error);
    //     return;
    // }

    // alGenSources((ALuint)1, &preview);
    // if ((error = alGetError()) != AL_NO_ERROR) {
    //     debug("uToxAudio:\tError generating source with err %x\n", error);
    //     return;
    // }

    alcCloseDevice(audio_out_handle);
}

static void audio_source_init(ALuint *source) {
    ALint error;
    alGetError();
    alGenSources((ALuint)1, source);
    if ((error = alGetError()) != AL_NO_ERROR) {
        debug("uToxAudio:\tError generating source with err %x\n", error);
    }
}

static void audio_source_term(ALuint *source) { alDeleteSources((ALuint)1, source); }

#define fade_step_out() (1 - ((double)(index % (sample_rate / notes_per_sec)) / (sample_rate / notes_per_sec)))
#define fade_step_in() (((double)(index % (sample_rate / notes_per_sec)) / (sample_rate / notes_per_sec)))

#define gen_note_raw(x, a) ((a * base_amplitude) * (sin((t * x) * index / sample_rate)))
#define gen_note_num(x, a) ((a * base_amplitude) * (sin((t * notes[x].freq) * index / sample_rate)))

#define gen_note_num_fade(x, a)                                                                                        \
    ((a * base_amplitude * fade_step_out()) * (sin((t * notes[x].freq) * index / sample_rate)))
#define gen_note_num_fade_in(x, a)                                                                                     \
    ((a * base_amplitude * fade_step_in()) * (sin((t * notes[x].freq) * index / sample_rate)))

// clang-format off
enum {
    NOTE_none,
    NOTE_c3_sharp,
    NOTE_g3,
    NOTE_b3,
    NOTE_c4,
    NOTE_a4,
    NOTE_b4,
    NOTE_e4,
    NOTE_f4,
    NOTE_c5,
    NOTE_d5,
    NOTE_e5,
    NOTE_f5,
    NOTE_g5,
    NOTE_a5,
    NOTE_c6_sharp,
    NOTE_e6,
};

static struct {
    uint8_t note;
    double  freq;
} notes[] = {
    {NOTE_none,         1           }, /* Can't be 0 or openal will skip this note/time */
    {NOTE_c3_sharp,     138.59      },
    {NOTE_g3,           196.00      },
    {NOTE_b3,           246.94      },
    {NOTE_c4,           261.63      },
    {NOTE_a4,           440.f       },
    {NOTE_b4,           493.88      },
    {NOTE_e4,           329.63      },
    {NOTE_f4,           349.23      },
    {NOTE_c5,           523.25      },
    {NOTE_d5,           587.33      },
    {NOTE_e5,           659.25      },
    {NOTE_f5,           698.46      },
    {NOTE_g5,           783.99      },
    {NOTE_a5,           880.f       },
    {NOTE_c6_sharp,     1108.73     },
    {NOTE_e6,           1318.51     },
};

static struct melodys { /* C99 6.7.8/10 uninitialized arithmetic types are 0 this is what we want. */
    uint8_t count;
    uint8_t volume;
    uint8_t fade;
    uint8_t notes[8];
} normal_ring[16] = {
    {1, 14, 1, {NOTE_f5,        }},
    {1, 14, 1, {NOTE_f5,        }},
    {1, 14, 1, {NOTE_f5,        }},
    {1, 14, 1, {NOTE_c6_sharp,  }},
    {1, 14, 0, {NOTE_c5,        }},
    {1, 14, 1, {NOTE_c5,        }},
    {0, 0, 0,  {0,  }},
}, friend_offline[4] = {
    {1, 14, 1, {NOTE_c4, }},
    {1, 14, 1, {NOTE_g3, }},
    {1, 14, 1, {NOTE_g3, }},
    {0, 0, 0,  {0, }},
}, friend_online[4] = {
    {1, 14, 0, {NOTE_g3, }},
    {1, 14, 1, {NOTE_g3, }},
    {1, 14, 1, {NOTE_a4, }},
    {1, 14, 1, {NOTE_b4, }},
}, friend_new_msg[8] = {
    {1, 0, 0,  {0, }}, /* 3/8 sec of silence for spammy friends */
    {1, 0, 0,  {0, }},
    {1, 0, 0,  {0, }},
    {1, 9,  0, {NOTE_g5, }},
    {1, 9,  1, {NOTE_g5, }},
    {1, 12, 1, {NOTE_a4, }},
    {1, 10, 1, {NOTE_a4, }},
    {1, 0, 0,  {0, }},
};
// clang-format on

typedef struct melodys MELODY;

static void generate_melody(MELODY melody[], uint32_t seconds, uint32_t notes_per_sec, ALuint *target) {
    ALint error;
    alGetError(); /* clear errors */

    alGenBuffers((ALuint)1, target);

    if ((error = alGetError()) != AL_NO_ERROR) {
        debug("uToxAudio:\tError generating buffer with err %i\n", error);
        return;
    }

    uint32_t sample_rate    = 22000;
    uint32_t base_amplitude = 1000;
    double   t              = 6.283185307179586476925286766559;

    size_t   buf_size = seconds * sample_rate * 2; // 16 bit (2 bytes per sample)
    int16_t *samples  = calloc(buf_size, sizeof(int16_t));

    if (!samples) {
        debug("uToxAudio:\tUnable to generate ringtone buffer!\n");
        return;
    }

    int position = 0;
    for (uint64_t index = 0; index < buf_size; ++index) {
        /* index / sample rate `mod` seconds. will give you full second long notes
         * you can change the length each tone is played by changing notes_per_sec
         * but you'll need to add additional case to cover the entire span of time */
        position = ((index / (sample_rate / notes_per_sec)) % (seconds * notes_per_sec));

        for (int i = 0; i < melody[position].count; ++i) {
            if (melody[position].fade) {
                samples[index] += gen_note_num_fade(melody[position].notes[i], melody[position].volume);
            } else {
                samples[index] += gen_note_num(melody[position].notes[i], melody[position].volume);
            }
        }
    }

    alBufferData(*target, AL_FORMAT_MONO16, samples, buf_size, sample_rate);
    free(samples);
}

static void generate_tone_call_ringtone() { generate_melody(normal_ring, 4, 4, &RingBuffer); }

static void generate_tone_friend_offline() { generate_melody(friend_offline, 1, 4, &ToneBuffer); }

static void generate_tone_friend_online() { generate_melody(friend_online, 1, 4, &ToneBuffer); }

static void generate_tone_friend_new_msg() { generate_melody(friend_new_msg, 1, 8, &ToneBuffer); }

void postmessage_audio(uint8_t msg, uint32_t param1, uint32_t param2, void *data) {
    while (audio_thread_msg) {
        yieldcpu(1);
    }

    audio_msg.msg    = msg;
    audio_msg.param1 = param1;
    audio_msg.param2 = param2;
    audio_msg.data   = data;

    audio_thread_msg = 1;
}

void utox_audio_thread(void *args) {
    time_t close_device_in = 0;
    time_t currtime        = 0;

    ToxAV *av = args;

#ifdef AUDIO_FILTERING
    debug("uToxAudio:\tAudio Filtering");
#ifdef ALC_LOOPBACK_CAPTURE_SAMPLES
    debug(" and Echo cancellation");
#endif
    debug(" enabled in this build\n");
#endif
    //_Bool call[MAX_CALLS] = {0}, preview = 0;
    //_Bool groups_audio[MAX_NUM_GROUPS] = {0};

    int     perframe = (UTOX_DEFAULT_FRAME_A * UTOX_DEFAULT_SAMPLE_RATE_A) / 1000;
    uint8_t buf[perframe * 2 * UTOX_DEFAULT_AUDIO_CHANNELS]; //, dest[perframe * 2 * UTOX_DEFAULT_AUDIO_CHANNELS];
    memset(buf, 0, sizeof(buf));

    debug("uToxAudio:\tframe size: %u\n", perframe);

    /* init Microphone */
    audio_in_init();
    // utox_audio_in_device_open();
    // utox_audio_in_listen();

    /* init Speakers */
    audio_out_init();
    // utox_audio_out_device_open();
    // utox_audio_out_device_close();

    Filter_Audio *f_a = NULL;

    int16_t *    preview_buffer       = NULL;
    unsigned int preview_buffer_index = 0;
    _Bool        preview_on           = 0;
#define PREVIEW_BUFFER_SIZE (UTOX_DEFAULT_SAMPLE_RATE_A / 2)

    preview_buffer       = calloc(PREVIEW_BUFFER_SIZE, 2);
    preview_buffer_index = 0;

    utox_audio_thread_init = 1;
    while (1) {
        time(&currtime);
        if (audio_thread_msg) {
            TOX_MSG *m = &audio_msg;
            if (!m->msg) {
                break;
            }

            switch (m->msg) {
                case UTOXAUDIO_START_FRIEND: {
                    FRIEND *f = &friend[m->param1];
                    if (!f->audio_dest) {
                        utox_audio_out_device_open();
                        audio_source_init(&f->audio_dest);
                    }
                    break;
                }
                case UTOXAUDIO_STOP_FRIEND: {
                    FRIEND *f = &friend[m->param1];
                    if (f->audio_dest) {
                        audio_source_term(&f->audio_dest);
                        f->audio_dest = 0;
                        utox_audio_out_device_close();
                    }
                    break;
                }
                case UTOXAUDIO_START_PREVIEW: {
                    utox_audio_out_device_open();
                    preview_on = 1;
                    break;
                }
                case UTOXAUDIO_STOP_PREVIEW: {
                    preview_on = 0;
                    utox_audio_out_device_close();
                    break;
                }
                case UTOXAUDIO_PLAY_RINGTONE: {
                    if (settings.ringtone_enabled && self.status != USER_STATUS_DO_NOT_DISTURB) {
                        debug_info("uToxAudio:\tGoing to start ringtone!\n");

                        utox_audio_out_device_open();

                        generate_tone_call_ringtone();

                        alSourcei(ringtone, AL_LOOPING, AL_TRUE);
                        alSourcei(ringtone, AL_BUFFER, RingBuffer);

                        alSourcePlay(ringtone);
                    }
                    break;
                }
                case UTOXAUDIO_STOP_RINGTONE: {
                    debug_info("uToxAudio:\tGoing to stop ringtone!\n");
                    alSourceStop(ringtone);
                    utox_audio_out_device_close();
                    break;
                }
                case UTOXAUDIO_PLAY_NOTIFICATION: {
                    if (settings.ringtone_enabled && self.status == USER_STATUS_AVAILABLE) {
                        debug_info("uToxAudio:\tGoing to start notification tone!\n");

                        if (close_device_in <= currtime) {
                            utox_audio_out_device_open();
                        }

                        switch (m->param1) {
                            case NOTIFY_TONE_FRIEND_ONLINE: {
                                generate_tone_friend_online();
                                break;
                            }
                            case NOTIFY_TONE_FRIEND_OFFLINE: {
                                generate_tone_friend_offline();
                                break;
                            }
                            case NOTIFY_TONE_FRIEND_NEW_MSG: {
                                generate_tone_friend_new_msg();
                                break;
                            }
                        }

                        alSourcei(notifytone, AL_LOOPING, AL_FALSE);
                        alSourcei(notifytone, AL_BUFFER, ToneBuffer);

                        alSourcePlay(notifytone);

                        time(&close_device_in);
                        close_device_in += 4;
                        debug_info("uToxAudio:\tclose device set!\n");
                    }
                    break;
                }
                case UTOXAUDIO_STOP_NOTIFICATION: {

                    break;
                }
            }
            audio_thread_msg = 0;

            if (close_device_in && close_device_in <= currtime) {
                debug_info("uToxAudio:\tclose device triggered!\n");
                utox_audio_out_device_close();
                close_device_in = 0;
            }
        }

// TODO move this code to filter_audio.c
#ifdef AUDIO_FILTERING
        if (!f_a && settings.audiofilter_enabled) {
            f_a = new_filter_audio(UTOX_DEFAULT_SAMPLE_RATE_A);
            if (!f_a) {
                settings.audiofilter_enabled = 0;
                debug("filter audio failed\n");
            } else {
                debug("filter audio on\n");
            }
        } else if (f_a && !settings.audiofilter_enabled) {
            kill_filter_audio(f_a);
            f_a = NULL;
            debug("filter audio off\n");
        }
#else
        if (settings.audiofilter_enabled) {
            settings.audiofilter_enabled = 0;
        }
#endif

        _Bool sleep = 1;

        if (microphone_on) {
            ALint samples;
            _Bool frame = 0;
            /* If we have a device_in we're on linux so we can just call OpenAL, otherwise we're on something else so
             * we'll need to call audio_frame() to add to the buffer for us. */
            if (audio_in_handle == (void *)1) {
                frame = audio_frame((void *)buf);
                if (frame) {
                    /* We have an audio frame to use, continue without sleeping. */
                    sleep = 0;
                }
            } else {
                alcGetIntegerv(audio_in_handle, ALC_CAPTURE_SAMPLES, sizeof(samples), &samples);
                if (samples >= perframe) {
                    alcCaptureSamples(audio_in_handle, buf, perframe);
                    frame = 1;
                    if (samples >= perframe * 2) {
                        sleep = 0;
                    }
                }
            }

#ifdef AUDIO_FILTERING
#ifdef ALC_LOOPBACK_CAPTURE_SAMPLES
            if (f_a && settings.audiofilter_enabled) {
                alcGetIntegerv(audio_out_device, ALC_LOOPBACK_CAPTURE_SAMPLES, sizeof(samples), &samples);
                if (samples >= perframe) {
                    int16_t buffer[perframe];
                    alcCaptureSamplesLoopback(audio_out_handle, buffer, perframe);
                    pass_audio_output(f_a, buffer, perframe);
                    set_echo_delay_ms(f_a, UTOX_DEFAULT_FRAME_A);
                    if (samples >= perframe * 2) {
                        sleep = 0;
                    }
                }
            }
#endif
#endif

            if (frame) {
                _Bool voice = 1;
#ifdef AUDIO_FILTERING
                if (f_a) {
                    int ret = filter_audio(f_a, (int16_t *)buf, perframe);

                    if (ret == -1) {
                        debug("filter audio error\n");
                    }

                    if (ret == 0) {
                        voice = 0;
                    }
                }
#endif

                /* If push to talk, we don't have to do anything */
                if (!check_ptt_key()) {
                    voice = 0; // PTT is up, send nothing.
                }

                if (preview_on) {
                    if (preview_buffer_index + perframe > PREVIEW_BUFFER_SIZE) {
                        preview_buffer_index = 0;
                    }
                    sourceplaybuffer(UTOX_MAX_NUM_FRIENDS, preview_buffer + preview_buffer_index, perframe,
                                     UTOX_DEFAULT_AUDIO_CHANNELS, UTOX_DEFAULT_SAMPLE_RATE_A);
                    if (voice) {
                        memcpy(preview_buffer + preview_buffer_index, buf, perframe * sizeof(int16_t));
                    } else {
                        memset(preview_buffer + preview_buffer_index, 0, perframe * sizeof(int16_t));
                    }
                    preview_buffer_index += perframe;
                }

                if (voice) {
                    int i, active_call_count = 0;
                    for (i = 0; i < UTOX_MAX_NUM_FRIENDS; i++) {
                        if (UTOX_SEND_AUDIO(i)) {
                            active_call_count++;
                            TOXAV_ERR_SEND_FRAME error = 0;
                            // debug("uToxAudio:\tSending audio frame!\n");
                            toxav_audio_send_frame(av, friend[i].number, (const int16_t *)buf, perframe,
                                                   UTOX_DEFAULT_AUDIO_CHANNELS, UTOX_DEFAULT_SAMPLE_RATE_A, &error);
                            if (error) {
                                debug("toxav_send_audio error friend == %i, error ==  %i\n", i, error);
                            } else {
                                // debug("Send a frame to friend %i\n",i);
                                if (active_call_count >= UTOX_MAX_CALLS) {
                                    debug("We're calling more peers than allowed by UTOX_MAX_CALLS, This is a bug\n");
                                    break;
                                }
                            }
                        }
                    }

                    // TODO REMOVED until new groups api can be implemented.
                    /*Tox *tox = toxav_get_tox(av);
                    uint32_t num_chats = tox_count_chatlist(tox);

                    if (num_chats != 0) {
                        int32_t chats[num_chats];
                        uint32_t max = tox_get_chatlist(tox, chats, num_chats);
                        for (i = 0; i < max; ++i) {
                            if (groups_audio[chats[i]]) {
                                toxav_group_send_audio(tox, chats[i], (int16_t *)buf, perframe,
                                                       UTOX_DEFAULT_AUDIO_CHANNELS, UTOX_DEFAULT_SAMPLE_RATE_A);
                            }
                        }
                    }*/
                }
            }
        }

        if (sleep) {
            yieldcpu(50);
        }
    }

    utox_filter_audio_kill(f_a);

    // missing some cleanup ?
    alDeleteSources(1, &ringtone);
    alDeleteSources(1, &preview);
    alDeleteBuffers(1, &RingBuffer);

    utox_audio_in_device_close();
    utox_audio_out_device_close();

    audio_thread_msg       = 0;
    utox_audio_thread_init = 0;
    debug("uToxAudio:\tClean thread exit!\n");
}

// COMMENTED OUT FOR NEW GC
/*void callback_av_group_audio(Tox *tox, int groupnumber, int peernumber, const int16_t *pcm, unsigned int samples,
                                    uint8_t channels, unsigned int sample_rate, void *userdata)
{
    GROUPCHAT *g = &group[groupnumber];

    uint64_t time = get_time();

    if (time - g->last_recv_audio[peernumber] > (uint64_t)1 * 1000 * 1000 * 1000) {
        postmessage(GROUP_UPDATE, groupnumber, peernumber, NULL);
    }

    g->last_recv_audio[peernumber] = time;

    if(!channels || channels > 2 || g->muted) {
        return;
    }

    ALuint bufid;
    ALint processed = 0, queued = 16;
    alGetSourcei(g->source[peernumber], AL_BUFFERS_PROCESSED, &processed);
    alGetSourcei(g->source[peernumber], AL_BUFFERS_QUEUED, &queued);
    alSourcei(g->source[peernumber], AL_LOOPING, AL_FALSE);

    if(processed) {
        ALuint bufids[processed];
        alSourceUnqueueBuffers(g->source[peernumber], processed, bufids);
        alDeleteBuffers(processed - 1, bufids + 1);
        bufid = bufids[0];
    } else if(queued < 16) {
        alGenBuffers(1, &bufid);
    } else {
        debug("dropped audio frame %i %i\n", groupnumber, peernumber);
        return;
    }

    alBufferData(bufid, (channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16, pcm, samples * 2 * channels,
                    sample_rate);
    alSourceQueueBuffers(g->source[peernumber], 1, &bufid);

    ALint state;
    alGetSourcei(g->source[peernumber], AL_SOURCE_STATE, &state);
    if(state != AL_PLAYING) {
        alSourcePlay(g->source[peernumber]);
        debug("Starting source %i %i\n", groupnumber, peernumber);
    }
}

void group_av_peer_add(GROUPCHAT *g, int peernumber) {
    alGenSources(1, &g->source[peernumber]);
}

void group_av_peer_remove(GROUPCHAT *g, int peernumber) {
    alDeleteSources(1, &g->source[peernumber]);
}
*/
