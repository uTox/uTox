#include "main.h"

static void utox_filter_audio_kill(Filter_Audio *filter_audio_handle){
    #ifdef AUDIO_FILTERING
    kill_filter_audio(filter_audio_handle);
    #endif
}

static ALCdevice *audio_out_handle, *audio_in_handle;
static void      *audio_out_device, *audio_in_device;
static _Bool      speakers_on, microphone_on;
/* TODO hacky fix. This source list should be a VLA with a way to link sources to friends.
 * NO SRSLY don't leave this like this! */
static ALuint source[UTOX_MAX_NUM_FRIENDS];
static ALuint ringtone;
static ALuint RingBuffer;

void utox_audio_in_device_open(void) {
    if (!audio_in_device) {
        return;
    }
    if (audio_in_device == (void*)1) {
        audio_in_handle = (void*)1;
        return;
    }

    audio_in_handle = alcCaptureOpenDevice(audio_in_device, UTOX_DEFAULT_SAMPLE_RATE_A, AL_FORMAT_MONO16,
                                                            (UTOX_DEFAULT_FRAME_A * UTOX_DEFAULT_SAMPLE_RATE_A * 4)
                                                            / 1000);
}

void utox_audio_in_device_close(void) {
    if (audio_in_handle) {
        if (audio_in_handle == (void*)1) {
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
    if (audio_in_handle) {
        if (audio_in_device == (void*)1) {
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
        microphone_on = 1;
    } else {
        microphone_on = 0;
    }
}

void utox_audio_in_ignore(void) {
    if (audio_in_handle) {
        if (audio_in_handle == (void*)1) {
            audio_close(audio_in_handle);
            microphone_on = 0;
            return;
        }
    alcCaptureStop(audio_in_handle);
    }
    microphone_on = 0;
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

ALCdevice* utox_audio_in_device_get(void) {
    if (audio_in_handle) {
        return audio_in_device;
    } else {
        return NULL;
    }
}

static ALCcontext *context;
void utox_audio_out_device_open(void) {
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
        speakers_on = 0;
        return;
    }
    speakers_on = 1;

    /* Create the buffers for the ringtone */
    alGenSources(1, &ringtone);
    /* Create the buffers for incoming audio */
    alGenSources(countof(source), source);
}

void utox_audio_out_device_close(void) {
    if (!audio_out_handle) {
        return;
    }
    alDeleteSources(countof(source), source);
    alDeleteSources(1, &ringtone);
    alcMakeContextCurrent(NULL);
    alcDestroyContext(context);
    alcCloseDevice(audio_out_handle);
    audio_out_handle = NULL;
    speakers_on = 0;
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

ALCdevice* utox_audio_out_device_get(void) {
    if (audio_out_handle) {
        return audio_out_device;
    } else {
        return NULL;
    }
}

void sourceplaybuffer(int i, const int16_t *data, int samples, uint8_t channels, unsigned int sample_rate) {
    if(!channels || channels > 2) {
        return;
    }

    ALuint bufid;
    ALint processed = 0, queued = 16;
    alGetSourcei(source[i], AL_BUFFERS_PROCESSED, &processed);
    alGetSourcei(source[i], AL_BUFFERS_QUEUED, &queued);
    alSourcei(source[i], AL_LOOPING, AL_FALSE);

    if(processed) {
        ALuint bufids[processed];
        alSourceUnqueueBuffers(source[i], processed, bufids);
        alDeleteBuffers(processed - 1, bufids + 1);
        bufid = bufids[0];
    } else if(queued < 16) {
        alGenBuffers(1, &bufid);
    } else {
        debug("dropped audio frame\n");
        return;
    }

    alBufferData(bufid, (channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16, data, samples * 2 * channels, sample_rate);
    alSourceQueueBuffers(source[i], 1, &bufid);

    // debug("audio frame || samples == %i channels == %u rate == %u \n", samples, channels, sample_rate);

    ALint state;
    alGetSourcei(source[i], AL_SOURCE_STATE, &state);
    if(state != AL_PLAYING) {
        alSourcePlay(source[i]);
        // debug("Starting source %u\n", i);
    }
}

static void utox_init_audio_in(void) {
    const char *audio_in_device_list;
    audio_in_device_list = alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
    if (audio_in_device_list) {
        audio_in_device = (void*)audio_in_device_list;
        debug("uToxAudio:\tinput device list:\n");
        while(*audio_in_device_list) {
            debug("\t%s\n", audio_in_device_list);
            postmessage(AUDIO_IN_DEVICE, UI_STRING_ID_INVALID, 0, (void*)audio_in_device_list);
            audio_in_device_list += strlen(audio_in_device_list) + 1;
        }
    }
    postmessage(AUDIO_IN_DEVICE, STR_AUDIO_IN_NONE, 0, NULL);
    audio_detect(); /* Get audio devices for windows */
}

static void utox_init_audio_out(void) {
    const char *audio_out_device_list;
    if (alcIsExtensionPresent(NULL, "ALC_ENUMERATE_ALL_EXT")) {
        audio_out_device_list = alcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);
    } else {
        audio_out_device_list = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
    }

    if(audio_out_device_list) {
        audio_out_device = audio_out_device_list;
        debug("uToxAudio:\toutput device list:\n");
        while(*audio_out_device_list) {
            debug("\t%s\n", audio_out_device_list);
            postmessage(AUDIO_OUT_DEVICE, 0, 0, (void*)audio_out_device_list);
            audio_out_device_list += strlen(audio_out_device_list) + 1;
        }
    }

    audio_out_handle = alcOpenDevice(audio_out_device);
    if (!audio_out_handle) {
        debug("alcOpenDevice() failed\n");
        return;
    }

    int attrlist[] = {  ALC_FREQUENCY, UTOX_DEFAULT_SAMPLE_RATE_A, ALC_INVALID };

    context = alcCreateContext(audio_out_handle, attrlist);
    if(!alcMakeContextCurrent(context)) {
        debug("alcMakeContextCurrent() failed\n");
        alcCloseDevice(audio_out_handle);
        return;
    }

    ALint error;
    alGetError(); /* clear errors */

    alGenSources((ALuint)1, &ringtone);
    if ((error = alGetError()) != AL_NO_ERROR) {
        debug("uToxAudio:\tError generating source with err %x\n", error);
        return;
    }

    alGenSources(countof(source), source);
    if ((error = alGetError()) != AL_NO_ERROR) {
        debug("uToxAudio:\tError generating source with err %x\n", error);
        return;
    }
}

void postmessage_audio(uint8_t msg, uint32_t param1, uint32_t param2, void *data) {
    while(audio_thread_msg) {
        yieldcpu(1);
    }

    audio_msg.msg = msg;
    audio_msg.param1 = param1;
    audio_msg.param2 = param2;
    audio_msg.data = data;

    audio_thread_msg = 1;
}

void utox_audio_thread(void *args){
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
    utox_init_audio_in();
    utox_audio_in_device_open();
    utox_audio_in_listen();

    /* init Speakers */
    utox_init_audio_out();
    utox_audio_out_device_open();

    alGenBuffers((ALuint)1, &RingBuffer);

    { /* wrapped to keep this data on the stack... I think... */
        float    frequency1  = 441.f;
        float    frequency2  = 882.f;
        int      seconds     = 4;
        unsigned sample_rate = 22050;
        size_t   buf_size    = seconds * sample_rate * 2; //16 bit (2 bytes per sample)
        int16_t *samples = malloc(buf_size * sizeof(int16_t));

        if (!samples) {
            debug("uToxAudio:\tUnable to generate ringtone buffer!\n");
            return;
        }

        /*Generate an electronic ringer sound that quickly alternates between two frequencies*/
        for (int index = 0; index < buf_size; ++index) {
            if ((index / (sample_rate)) % 4 < 2 ) {//4 second ring cycle, first 2 secondsring, the rest(2 seconds) is silence
                if ((index / 1000) % 2 == 1) {
                    samples[index] = 15000 * sin((2.0 * 3.1415926 * frequency1) / sample_rate * index); //5000=amplitude(volume level). It can be from zero to 32700
                } else {
                    samples[index] = 15000 * sin((2.0 * 3.1415926 * frequency2) / sample_rate * index);
                }
            } else {
                samples[index] = 0;
            }
        }

        alBufferData(RingBuffer, AL_FORMAT_MONO16, samples, buf_size, sample_rate);
        free(samples);
    }

    if (ringtone) {
        debug("uToxAudio:\tFirst ringtone is good, going to queue buffers\n");
        if (RingBuffer) {
            alSourcei(ringtone, AL_LOOPING, AL_TRUE);
            alSourcei(ringtone, AL_BUFFER,  RingBuffer);
        } else {
            debug("uToxAudio:\tNo buffer to queue!\n");
        }
    } else {
        debug("uToxAudio:\tFirst ringtone failed... can't queue buffers\n");
    }

    Filter_Audio *f_a = NULL;

    int16_t     *preview_buffer = NULL;
    unsigned int preview_buffer_index = 0;
    _Bool        preview_on = 0;
    #define PREVIEW_BUFFER_SIZE (UTOX_DEFAULT_SAMPLE_RATE_A / 2)

    preview_buffer = calloc(PREVIEW_BUFFER_SIZE, 2);
    preview_buffer_index = 0;

    utox_audio_thread_init = 1;
    while(1) {
        if(audio_thread_msg) {
            TOX_MSG *m = &audio_msg;
            if(!m->msg) {
                break;
            }

            switch (m->msg){
                case AUDIO_START_PREVIEW: {
                    preview_on = 1;
                    break;
                }
                case AUDIO_STOP_PREVIEW: {
                    preview_on = 0;
                    break;
                }
                case AUDIO_PLAY_RINGTONE: {
                    if (audible_notifications_enabled) {
                        debug("starting ringtone!\n");
                        alSourcePlay(ringtone);
                    }
                    break;
                }
                case AUDIO_STOP_RINGTONE: {
                    ALint state;
                    alGetSourcei(ringtone, AL_SOURCE_STATE, &state);
                    if(state == AL_PLAYING) {
                        alSourceStop(ringtone);
                    }
                    break;
                }
            }
            audio_thread_msg = 0;
        }

        // TODO move this code to filter_audio.c
        #ifdef AUDIO_FILTERING
            if (!f_a && audio_filtering_enabled) {
                f_a = new_filter_audio(UTOX_DEFAULT_SAMPLE_RATE_A);
                if (!f_a) {
                    audio_filtering_enabled = 0;
                    debug("filter audio failed\n");
                } else {
                    debug("filter audio on\n");
                }
            } else if (f_a && !audio_filtering_enabled) {
                kill_filter_audio(f_a);
                f_a = NULL;
                debug("filter audio off\n");
            }
        #else
            if (audio_filtering_enabled) {
                audio_filtering_enabled = 0;
            }
        #endif

        _Bool sleep = 1;

        if (microphone_on) {
            ALint samples;
            _Bool frame = 0;
            /* If we have a device_in we're on linux so we can just call OpenAL, otherwise we're on something else so
             * we'll need to call audio_frame() to add to the buffer for us. */
            if (audio_in_handle == (void*)1) {
                frame = audio_frame((void*)buf);
                if (frame) {
                    /* We have an audio frame to use, continue without sleeping. */
                    sleep = 0;
                }
            } else {
                alcGetIntegerv(audio_in_handle, ALC_CAPTURE_SAMPLES, sizeof(samples), &samples);
                if(samples >= perframe) {
                    alcCaptureSamples(audio_in_handle, buf, perframe);
                    frame = 1;
                    if (samples >= perframe * 2) {
                        sleep = 0;
                    }
                }
            }

            #ifdef AUDIO_FILTERING
            #ifdef ALC_LOOPBACK_CAPTURE_SAMPLES
            if (f_a && audio_filtering_enabled) {
                alcGetIntegerv(device_out, ALC_LOOPBACK_CAPTURE_SAMPLES, sizeof(samples), &samples);
                if(samples >= perframe) {
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
                    int ret = filter_audio(f_a, (int16_t*)buf, perframe);

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
                    voice = 0; //PTT is up, send nothing.
                }

                if (preview_on) {
                    if (preview_buffer_index + perframe > PREVIEW_BUFFER_SIZE) {
                        preview_buffer_index = 0;
                    }
                    sourceplaybuffer(0, preview_buffer + preview_buffer_index, perframe, UTOX_DEFAULT_AUDIO_CHANNELS, UTOX_DEFAULT_SAMPLE_RATE_A);
                    if (voice) {
                        memcpy(preview_buffer + preview_buffer_index, buf, perframe * sizeof(int16_t));
                    } else {
                        memset(preview_buffer + preview_buffer_index, 0, perframe * sizeof(int16_t));
                    }
                    preview_buffer_index += perframe;
                }

                if (voice) {
                    int i, active_call_count = 0;
                    for(i = 0; i < UTOX_MAX_NUM_FRIENDS; i++) {
                        if( UTOX_SEND_AUDIO(i) ) {
                            active_call_count++;
                            TOXAV_ERR_SEND_FRAME error = 0;
                            toxav_audio_send_frame(av, friend[i].number, (const int16_t *)buf, perframe, UTOX_DEFAULT_AUDIO_CHANNELS, UTOX_DEFAULT_SAMPLE_RATE_A, &error);
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
                                toxav_group_send_audio(tox, chats[i], (int16_t *)buf, perframe, UTOX_DEFAULT_AUDIO_CHANNELS, UTOX_DEFAULT_SAMPLE_RATE_A);
                            }
                        }
                    }*/
                }
            }
        }

        if (sleep) {
            yieldcpu(5);
        }
    }

    utox_filter_audio_kill(f_a);

    //missing some cleanup ?
    alDeleteSources(1, &ringtone);
    alDeleteSources(countof(source), source);
    alDeleteBuffers(1, &RingBuffer);

    utox_audio_in_device_close();
    utox_audio_out_device_close();

    audio_thread_msg = 0;
    utox_audio_thread_init = 0;
    debug("UTOXAUDIO:\tClean thread exit!\n");
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

        alBufferData(bufid, (channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16, pcm, samples * 2 * channels, sample_rate);
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
