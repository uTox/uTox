#include "main.h"

#ifdef __APPLE__
    #include <OpenAL/al.h>
    #include <OpenAL/alc.h>
#else
    #include <AL/al.h>
    #include <AL/alc.h>

    #ifdef AUDIO_FILTERING
        #include <AL/alext.h>
    #endif
    /* include for compatibility with older versions of OpenAL */
    #ifndef ALC_ALL_DEVICES_SPECIFIER
        #include <AL/alext.h>
    #endif
#endif

#ifdef AUDIO_FILTERING
    #include <filter_audio.h>
#endif

static void utox_filter_audio_kill(Filter_Audio *filter_audio_handle){
    #ifdef AUDIO_FILTERING
    kill_filter_audio(filter_audio_handle);
    #endif
}

static ALCdevice *device_out, *device_in;
static ALCcontext *context;
static ALuint source[MAX_CALLS];

static ALCdevice* alcopencapture(void *handle) {
    if(!handle) {
        return NULL;
    }

    if(handle == (void*)1) {
        return handle;
    }

    return alcCaptureOpenDevice(handle, UTOX_DEFAULT_AUDIO_SAMPLE_RATE, AL_FORMAT_MONO16, (UTOX_DEFAULT_AUDIO_FRAME_DURATION * UTOX_DEFAULT_AUDIO_SAMPLE_RATE * 4) / 1000);
    // return alcCaptureOpenDevice(handle, av_DefaultSettings.audio_sample_rate, AL_FORMAT_STERIO16, ((av_DefaultSettings.audio_frame_duration * av_DefaultSettings.audio_sample_rate * 4) / 1000) * av_DefaultSettings.audio_channels);
}

static void alccapturestart(void *handle) {
    if(handle == (void*)1) {
        audio_init(handle);
        return;
    }
    alcCaptureStart(handle);
}

static void alccapturestop(void *handle) {
    if(handle == (void*)1) {
        audio_close(handle);
        return;
    }

    alcCaptureStop(handle);
}

static void alccaptureclose(void *handle) {
    if(handle == (void*)1) {
        return;
    }

    alcCaptureCloseDevice(handle);
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

void audio_thread(void *args){
    ToxAV *av = args;
    const char *device_list, *output_device = NULL;
    void *audio_device = NULL;

    _Bool call[MAX_CALLS] = {0}, preview = 0;
    _Bool groups_audio[MAX_NUM_GROUPS] = {0};

    int perframe = (UTOX_DEFAULT_AUDIO_FRAME_DURATION * UTOX_DEFAULT_AUDIO_SAMPLE_RATE) / 1000;
    uint8_t buf[perframe * 2 * UTOX_DEFAULT_AUDIO_CHANNELS]; //, dest[perframe * 2 * UTOX_DEFAULT_AUDIO_CHANNELS];
    memset(buf, 0, sizeof(buf));

    uint8_t audio_count = 0;
    _Bool record_on = 0;
    #ifdef AUDIO_FILTERING
    debug("Audio Filtering");
    #ifdef ALC_LOOPBACK_CAPTURE_SAMPLES
    debug(" and Echo cancellation");
    #endif
    debug(" enabled in this build\n");
    #endif

    debug("frame size: %u\n", perframe);

    device_list = alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
    if (device_list) {
        audio_device = (void*)device_list;
        debug("Input Device List:\n");
        while(*device_list) {
            debug("%s\n", device_list);
            postmessage(AUDIO_IN_DEVICE, UI_STRING_ID_INVALID, 0, (void*)device_list);
            device_list += strlen(device_list) + 1;
        }
    }

    postmessage(AUDIO_IN_DEVICE, STR_AUDIO_IN_NONE, 0, NULL);
    audio_detect();

    if (alcIsExtensionPresent(NULL, "ALC_ENUMERATE_ALL_EXT")) {
        device_list = alcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);
    } else {
        device_list = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
    }

    if(device_list) {
        output_device = device_list;
        debug("Output Device List:\n");
        while(*device_list) {
            debug("%s\n", device_list);
            postmessage(AUDIO_OUT_DEVICE, 0, 0, (void*)device_list);
            device_list += strlen(device_list) + 1;
        }
    }

    device_out = alcOpenDevice(output_device);
    if(!device_out) {
        debug("alcOpenDevice() failed\n");
        return;
    }

    int attrlist[] = {  ALC_FREQUENCY, UTOX_DEFAULT_AUDIO_SAMPLE_RATE,
                        ALC_INVALID };

    context = alcCreateContext(device_out, attrlist);
    if(!alcMakeContextCurrent(context)) {
        debug("alcMakeContextCurrent() failed\n");
        alcCloseDevice(device_out);
        return;
    }

    alGenSources(countof(source), source);

    static ALuint ringSrc[MAX_CALLS];
    alGenSources(MAX_CALLS, ringSrc);

    /* Create buffer to store samples */
    ALuint RingBuffer;
    alGenBuffers(1, &RingBuffer);

    {
        float frequency1 = 441.f;
        float frequency2 = 882.f;
        int seconds = 4;
        unsigned sample_rate = 22050;
        size_t buf_size = seconds * sample_rate * 2; //16 bit (2 bytes per sample)
        int16_t *samples = malloc(buf_size * sizeof(int16_t));
        if (!samples)
            return;

        /*Generate an electronic ringer sound that quickly alternates between two frequencies*/
        int index = 0;
        for(index = 0; index < buf_size; ++index) {
            if ((index / (sample_rate)) % 4 < 2 ) {//4 second ring cycle, first 2 secondsring, the rest(2 seconds) is silence
                if((index / 1000) % 2 == 1) {
                    samples[index] = 5000 * sin((2.0 * 3.1415926 * frequency1) / sample_rate * index); //5000=amplitude(volume level). It can be from zero to 32700
                } else {
                    samples[index] = 5000 * sin((2.0 * 3.1415926 * frequency2) / sample_rate * index);
                }
            } else {
                samples[index] = 0;
            }
        }

        alBufferData(RingBuffer, AL_FORMAT_MONO16, samples, buf_size, sample_rate);
        free(samples);
    }

    {
        unsigned int i;
        for (i = 0; i < MAX_CALLS; ++i) {
            alSourcei(ringSrc[i], AL_LOOPING, AL_TRUE);
            alSourcei(ringSrc[i], AL_BUFFER, RingBuffer);
        }
    }
    Filter_Audio *f_a = NULL;

    audio_thread_init = 1;

    int16_t *preview_buffer = NULL;
    unsigned int preview_buffer_index = 0;
    #define PREVIEW_BUFFER_SIZE (UTOX_DEFAULT_AUDIO_SAMPLE_RATE / 2)

    while(1) {
        if(audio_thread_msg) {
            TOX_MSG *m = &audio_msg;
            if(!m->msg) {
                break;
            }

            switch(m->msg) {
            case AUDIO_SET_INPUT: {
                audio_device = m->data;

                if(record_on) {
                    alccapturestop(device_in);
                    alccaptureclose(device_in);
                }

                if(audio_count) {
                    device_in = alcopencapture(audio_device);
                    if(!device_in) {
                        record_on = 0;
                    } else {
                        alccapturestart(device_in);
                        record_on = 1;
                    }
                }

                debug("set audio in\n");
                break;
            }

            case AUDIO_SET_OUTPUT: {
                output_device = m->data;

                ALCdevice *device = alcOpenDevice(output_device);
                if(!device) {
                    debug("alcOpenDevice() failed\n");
                    break;
                }

                ALCcontext *con = alcCreateContext(device, NULL);
                if(!alcMakeContextCurrent(con)) {
                    debug("alcMakeContextCurrent() failed\n");
                    alcCloseDevice(device);
                    break;
                }

                alcDestroyContext(context);
                alcCloseDevice(device_out);
                context = con;
                device_out = device;

                alGenSources(countof(source), source);
                alGenSources(MAX_CALLS, ringSrc);

                Tox *tox = toxav_get_tox(av);
                uint32_t num_chats = tox_count_chatlist(tox);

                if (num_chats != 0) {
                    int32_t chats[num_chats];
                    uint32_t max = tox_get_chatlist(tox, chats, num_chats);

                    unsigned int i;
                    for (i = 0; i < max; ++i) {
                        if (tox_group_get_type(tox, chats[i]) == TOX_GROUPCHAT_TYPE_AV) {
                            GROUPCHAT *g = &group[chats[i]];
                            alGenSources(g->peers, g->source);
                        }
                    }
                }

                debug("set audio out\n");
                break;
            }

            case AUDIO_PREVIEW_START: {
                preview = 1;
                audio_count++;
                preview_buffer = calloc(PREVIEW_BUFFER_SIZE, 2);
                preview_buffer_index = 0;
                if(!record_on) {
                    device_in = alcopencapture(audio_device);
                    if(device_in) {
                        alccapturestart(device_in);
                        record_on = 1;
                        debug("Starting Audio Preview\n");
                    }
                }
                break;
            }

            case AUDIO_START: {
                audio_count++;
                if(!record_on) {
                    device_in = alcopencapture(audio_device);
                    if(device_in) {
                        alccapturestart(device_in);
                        record_on = 1;
                        debug("Starting Audio Call\n");
                    }
                }
                break;
            }

            case GROUP_AUDIO_CALL_START: {
                break; // TODO, new groups API
                audio_count++;
                groups_audio[m->param1] = 1;
                if(!record_on) {
                    device_in = alcopencapture(audio_device);
                    if(device_in) {
                        alccapturestart(device_in);
                        record_on = 1;
                        debug("Starting Audio GroupCall\n");
                    }
                }
                break;
            }

            case AUDIO_PREVIEW_END: {
                preview = 0;
                audio_count--;
                free(preview_buffer);
                preview_buffer = NULL;
                if(!audio_count && record_on) {
                    alccapturestop(device_in);
                    alccaptureclose(device_in);
                    record_on = 0;
                    debug("Audio Preview Stopped\n");
                }
                break;
            }

            case AUDIO_END: {
                if(!call[m->param1]) {
                    break;
                }
                call[m->param1] = 0;
                audio_count--;
                if(!audio_count && record_on) {
                    alccapturestop(device_in);
                    alccaptureclose(device_in);
                    record_on = 0;
                    debug("stop\n");
                }
                break;
            }

            case GROUP_AUDIO_CALL_END: {
                break; // TODO, new groups API
                if(!groups_audio[m->param1]) {
                    break;
                }
                audio_count--;
                groups_audio[m->param1] = 0;
                if(!audio_count && record_on) {
                    alccapturestop(device_in);
                    alccaptureclose(device_in);
                    record_on = 0;
                    debug("stop\n");
                }
                break;
            }

            case AUDIO_PLAY_RINGTONE: {
                if(!audible_notifications_enabled) {
                    break;
                }

                alSourcePlay(ringSrc[m->param1]);
                break;
            }

            case AUDIO_STOP_RINGTONE: {
                ALint state;
                alGetSourcei(ringSrc[m->param1], AL_SOURCE_STATE, &state);
                if(state == AL_PLAYING) {
                    alSourceStop(ringSrc[m->param1]);
                }
                break;
            }
            }

            audio_thread_msg = 0;
        }

        // TODO move this code to filter_audio.c
        #ifdef AUDIO_FILTERING
            if (!f_a && audio_filtering_enabled) {
                f_a = new_filter_audio(UTOX_DEFAULT_AUDIO_SAMPLE_RATE);
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

        if(record_on) {
            ALint samples;
            _Bool frame = 0;
            /* If we have a device_in we're on linux so we can just call OpenAL, otherwise we're on something else so
             * we'll need to call audio_frame() to add to the buffer for us. */
            if (device_in == (void*)1) {
                frame = audio_frame((void*)buf);
                if (frame) {
                    /* We have an audio frame to use, continue without sleeping. */
                    sleep = 0;
                }
            } else {
                alcGetIntegerv(device_in, ALC_CAPTURE_SAMPLES, sizeof(samples), &samples);
                if(samples >= perframe) {
                    alcCaptureSamples(device_in, buf, perframe);
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
                    alcCaptureSamplesLoopback(device_out, buffer, perframe);
                    pass_audio_output(f_a, buffer, perframe);
                    set_echo_delay_ms(f_a, UTOX_DEFAULT_AUDIO_FRAME_DURATION);
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

                if (preview) {
                    if (preview_buffer_index + perframe > PREVIEW_BUFFER_SIZE) {
                        preview_buffer_index = 0;
                    }

                    sourceplaybuffer(0, preview_buffer + preview_buffer_index, perframe, UTOX_DEFAULT_AUDIO_CHANNELS, UTOX_DEFAULT_AUDIO_SAMPLE_RATE);
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
                            toxav_audio_send_frame(av, friend[i].number, (const int16_t *)buf, perframe, UTOX_DEFAULT_AUDIO_CHANNELS, UTOX_DEFAULT_AUDIO_SAMPLE_RATE, &error);
                            if (error) {
                                debug("toxav_send_audio error friend == %i, error ==  %i\n", i, error);
                            } else {
                                // debug("Send a frame to friend %i\n",i);
                                if (i >= UTOX_MAX_CALLS) {
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
                                toxav_group_send_audio(tox, chats[i], (int16_t *)buf, perframe, UTOX_DEFAULT_AUDIO_CHANNELS, UTOX_DEFAULT_AUDIO_SAMPLE_RATE);
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
    alDeleteSources(MAX_CALLS, ringSrc);
    alDeleteSources(countof(source), source);
    alDeleteBuffers(1, &RingBuffer);

    if(device_in) {
        if(record_on) {
            alcCaptureStop(device_in);
        }
        alcCaptureCloseDevice(device_in);
    }

    alcMakeContextCurrent(NULL);
    alcDestroyContext(context);
    alcCloseDevice(device_out);

    audio_thread_msg = 0;
    audio_thread_init = 0;
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
