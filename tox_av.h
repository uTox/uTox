
static void av_start(int32_t call_index, void *arg)
{
    int fid = toxav_get_peer_id(arg, call_index, 0);
    _Bool video = toxav_get_peer_transmission_type(arg, call_index, 0) == TypeVideo;

    debug("video for this call: %u\n", video);

    ToxAvCodecSettings settings = av_DefaultSettings;
    if(video && video_width) {
        settings.video_width = video_width;
        settings.video_height = video_height;
    }

    if(toxav_prepare_transmission(arg, call_index, &settings, video) == 0) {
        call[call_index].video = video;
        call[call_index].active = 1;
        if(video) {
            postmessage(FRIEND_CALL_START_VIDEO, fid, call_index, (void*)(settings.video_width | (size_t)settings.video_height << 16));
        } else {
            postmessage(FRIEND_CALL_STATUS, fid, call_index, (void*)CALL_OK);
        }
    } else {
        debug("A/V FAIL!\n");
    }
}

static void callback_av_invite(int32_t call_index, void *arg)
{
    int fid = toxav_get_peer_id(arg, call_index, 0);
    _Bool video = toxav_get_peer_transmission_type(arg, call_index, 0) == TypeVideo;
    postmessage(FRIEND_CALL_STATUS, fid, call_index, (void*)(size_t)(video ? CALL_INVITED_VIDEO : CALL_INVITED));

    debug("A/V Invite (%i)\n", call_index);
}

static void callback_av_start(int32_t call_index, void *arg)
{
    av_start(call_index, arg);

    debug("A/V Start (%i)\n", call_index);
}

#define endcall() \
    int fid = toxav_get_peer_id(arg, call_index, 0); \
    postmessage(FRIEND_CALL_STATUS, fid, call_index, (void*)(size_t)CALL_NONE);

#define stopcall() \
    call[call_index].active = 0; \
    endcall();

static void callback_av_cancel(int32_t call_index, void *arg)
{
    stopcall();

    debug("A/V Cancel (%i)\n", call_index);
}

static void callback_av_reject(int32_t call_index, void *arg)
{
    endcall();

    debug("A/V Reject (%i)\n", call_index);
}

static void callback_av_end(int32_t call_index, void *arg)
{
    stopcall();

    debug("A/V End (%i)\n", call_index);
}

static void callback_av_ringing(int32_t call_index, void *arg)
{
    debug("A/V Ringing (%i)\n", call_index);
}

static void callback_av_starting(int32_t call_index, void *arg)
{
    av_start(call_index, arg);

    debug("A/V Starting (%i)\n", call_index);
}

static void callback_av_ending(int32_t call_index, void *arg)
{
    stopcall();

    debug("A/V Ending (%i)\n", call_index);
}

static void callback_av_error(int32_t call_index, void *arg)
{
    stopcall();

    debug("A/V Error (%i)\n", call_index);
}

static void callback_av_requesttimeout(int32_t call_index, void *arg)
{
    endcall();

    debug("A/V ReqTimeout (%i)\n", call_index);
}

static void callback_av_peertimeout(int32_t call_index, void *arg)
{
    stopcall();

    debug("A/V PeerTimeout (%i)\n", call_index);
}

static void set_av_callbacks(ToxAv *av)
{
    toxav_register_callstate_callback(callback_av_invite, av_OnInvite, av);
    toxav_register_callstate_callback(callback_av_start, av_OnStart, av);
    toxav_register_callstate_callback(callback_av_cancel, av_OnCancel, av);
    toxav_register_callstate_callback(callback_av_reject, av_OnReject, av);
    toxav_register_callstate_callback(callback_av_end, av_OnEnd, av);
    toxav_register_callstate_callback(callback_av_ringing, av_OnRinging, av);
    toxav_register_callstate_callback(callback_av_starting, av_OnStarting, av);
    toxav_register_callstate_callback(callback_av_ending, av_OnEnding, av);
    toxav_register_callstate_callback(callback_av_error, av_OnError, av);
    toxav_register_callstate_callback(callback_av_requesttimeout, av_OnRequestTimeout, av);
    toxav_register_callstate_callback(callback_av_peertimeout, av_OnPeerTimeout, av);
}

uint8_t lbuffer[800 * 600 * 4];

static ALCdevice* alcopencapture(const char *name)
{
    return alcCaptureOpenDevice(name, av_DefaultSettings.audio_sample_rate, AL_FORMAT_MONO16, (av_DefaultSettings.audio_frame_duration * av_DefaultSettings.audio_sample_rate * 4) / 1000);
}

static void sourceplaybuffer(int i, void *buf, int size)
{
    ALuint bufid;
    ALint processed, queued;
    alGetSourcei(source[i], AL_BUFFERS_PROCESSED, &processed);
    alGetSourcei(source[i], AL_BUFFERS_QUEUED, &queued);
    alSourcei(source[i], AL_LOOPING, AL_FALSE);

    if(processed) {
        ALuint bufids[processed];
        alSourceUnqueueBuffers(source[i], processed, bufids);
        alDeleteBuffers(processed - 1, bufids + 1);
        bufid = bufids[0];
    } else if(queued < 8) {
        alGenBuffers(1, &bufid);
    } else {
        debug("dropped audio frame\n");
        return;
    }

    alBufferData(bufid, AL_FORMAT_MONO16, buf, size * 2, av_DefaultSettings.audio_sample_rate);
    alSourceQueueBuffers(source[i], 1, &bufid);

    ALint state;
    alGetSourcei(source[i], AL_SOURCE_STATE, &state);
    if(state != AL_PLAYING) {
        alSourcePlay(source[i]);
        debug("Starting source %u\n", i);
    }
}

static vpx_image_t input;

static _Bool openvideodevice(void *handle)
{
    if(!video_init(handle)) {
        debug("video_init() failed\n");
        return 0;
    }
    vpx_img_alloc(&input, VPX_IMG_FMT_I420, video_width, video_height, 1);
    return 1;
}

static void closevideodevice(void *handle)
{
    video_close(handle);
    vpx_img_free(&input);
}

static void av_thread(void *args)
{
    ToxAv *av = args;
    const char *device_list, *audio_device = NULL, *output_device = NULL;
    void *video_device;
    _Bool audio_preview = 0, video = 0;

    int perframe = (av_DefaultSettings.audio_frame_duration * av_DefaultSettings.audio_sample_rate) / 1000;
    uint8_t buf[perframe * 2], dest[perframe * 2];
    uint8_t video_count = 0, audio_count = 0;
    _Bool video_on = 0, record_on = 0;

    set_av_callbacks(av);

    video_device = video_detect();
    if(video_device) {
        video = openvideodevice(video_device);
    }

    device_list = alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
    if(device_list) {
        audio_device = device_list;
        debug("Input Device List:\n");
        while(*device_list) {
            printf("%s\n", device_list);
            postmessage(NEW_AUDIO_IN_DEVICE, 0, 0, (void*)device_list);
            device_list += strlen(device_list) + 1;
        }
    }

    device_list = alcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);
    if(device_list) {
        output_device = device_list;
        debug("Output Device List:\n");
        while(*device_list) {
            printf("%s\n", device_list);
            postmessage(NEW_AUDIO_OUT_DEVICE, 0, 0, (void*)device_list);
            device_list += strlen(device_list) + 1;
        }
    }

    device_out = alcOpenDevice(output_device);
    if(!device_out) {
        printf("alcOpenDevice() failed\n");
        return;
    }

    context = alcCreateContext(device_out, NULL);
    if(!alcMakeContextCurrent(context)) {
        printf("alcMakeContextCurrent() failed\n");
        alcCloseDevice(device_out);
        return;
    }

    alGenSources(countof(source), source);

    av_thread_run = 1;

    while(1) {
        if(toxav_thread_msg) {
            TOX_MSG *m = &toxav_msg;
            if(!m->msg) {
                break;
            }
            uint8_t msg;
            //uint16_t param1, uint16_t param2;
            void *data;
            msg = m->msg;
            //param1 = m->param1;
            //param2 = m->param2;
            data = m->data;

            switch(msg) {
            case AV_SET_AUDIO_INPUT: {
                audio_device = data;

                if(record_on) {
                    alcCaptureStop(device_in);
                    alcCaptureCloseDevice(device_in);
                }

                if(record_on) {
                    device_in = alcopencapture(audio_device);
                    if(!device_in) {
                        record_on = 0;
                    } else {
                        alcCaptureStart(device_in);
                    }
                }

                debug("set audio in\n");
                break;
            }

            case AV_SET_AUDIO_OUTPUT: {
                ALCdevice *device = alcOpenDevice(output_device);
                if(!device) {
                    printf("alcOpenDevice() failed\n");
                    break;
                }

                ALCcontext *con = alcCreateContext(device, NULL);
                if(!alcMakeContextCurrent(con)) {
                    printf("alcMakeContextCurrent() failed\n");
                    alcCloseDevice(device);
                    break;
                }

                alcDestroyContext(context);
                alcCloseDevice(device_out);
                context = con;
                device_out = device;

                alGenSources(countof(source), source);

                debug("set audio out\n");
                break;
            }

            case AV_SET_VIDEO: {
                if(video_on) {
                    video_endread();
                }

                if(video) {
                    closevideodevice(video_device);
                }

                video_device = data;
                video = openvideodevice(video_device);
                if(video) {
                    if(video_on) {
                        video_on = video_startread();
                    }
                } else {
                    video_on = 0;
                }

                debug("set video\n");
                break;
            }

            case AV_AUDIO_PREVIEW_START: {
                audio_preview = 1;
                /*if(audio_count == 0) {
                    //turn on audio
                }
                audio_count++;*/
                break;
            }

            case AV_AUDIO_PREVIEW_STOP: {
                audio_preview = 0;
                /*audio_count--;
                if(audio_count == 0) {
                    //turn off audio
                }*/
                break;
            }

            case AV_VIDEO_PREVIEW_START: {

                break;
            }

            case AV_VIDEO_PREVIEW_STOP: {

                break;
            }

            }

            toxav_thread_msg = 0;
        }

        if(record_on) {
            ALint samples;
            alcGetIntegerv(device_in, ALC_CAPTURE_SAMPLES, sizeof(samples), &samples);
            if(samples >= perframe) {
                alcCaptureSamples(device_in, buf, perframe);

                if(audio_preview) {
                    sourceplaybuffer(0, buf, perframe);
                }

                int i = 0;
                while(i < MAX_CALLS) {
                    if(call[i].active) {
                        int r;
                        if((r = toxav_prepare_audio_frame(av, i, dest, perframe * 2, (void*)buf, perframe)) < 0) {
                            debug("toxav_prepare_audio_frame error %i\n", r);
                        }

                        if((r = toxav_send_audio(av, i, dest, r)) < 0) {
                            debug("toxav_send_audio error %i %s\n", r, strerror(errno));
                        }
                    }
                    i++;
                }
            }

        }

        if(video_on) {
            if(video_getframe(&input)) {
                if(video_preview) {
                    postmessage(PREVIEW_FRAME, 0, 0, &input);
                }

                int i;
                for(i = 0; i < MAX_CALLS; i++) {
                    if(call[i].active && call[i].video) {
                        int r, len;
                        if((len = toxav_prepare_video_frame(av, i, lbuffer, sizeof(lbuffer), &input)) < 0) {
                            debug("toxav_prepare_video_frame error %i\n", r);
                            continue;
                        }

                        debug("%u\n", len);

                        if((r = toxav_send_video(av, i, (void*)lbuffer, len)) < 0) {
                            debug("toxav_send_video error %i %s\n", r, strerror(errno));
                        }
                    }
                }
            }
        }

        int i, vc = 0, ac = 0;
        for(i = 0; i < MAX_CALLS; i++) {
            if(call[i].active) {
                ac++;
                if(call[i].video) {
                    vpx_image_t *image;
                    if(toxav_recv_video(av, i, &image) == 0) {
                        if(image) {
                            postmessage(FRIEND_VIDEO_FRAME, toxav_get_peer_id(av, i, 0), i, image);
                        }
                    } else {
                        debug("toxav_recv_video() error\n");
                    }
                    vc++;
                }

                int size = toxav_recv_audio(av, i, perframe, (void*)buf);
                if(size > 0) {
                    sourceplaybuffer(i + 1, buf, size);
                }
            }
        }

        ac += audio_preview;
        vc += (video && video_preview);

        if(ac != audio_count) {
            if(ac == 0) {
                if(record_on) {
                    alcCaptureStop(device_in);
                    alcCaptureCloseDevice(device_in);
                    debug("stop\n");
                    record_on = 0;
                }
            } else if(audio_count == 0) {
                device_in = alcopencapture(audio_device);
                if(device_in) {
                    alcCaptureStart(device_in);
                    record_on = 1;
                    debug("start\n");
                }
            }
            audio_count = ac;
        }

        if(vc != video_count) {
            if(vc == 0) {
                if(video_on) {
                    video_endread();
                    video_on = 0;
                }
            } else if(video_count == 0) {
                video_on = video_startread();
            }
            video_count = vc;
        }

        yieldcpu(5);
    }

    //missing some cleanup

    if(device_in) {
        if(record_on) {
            alcCaptureStop(device_in);
        }
        alcCaptureCloseDevice(device_in);
    }

    alcCloseDevice(device_out);
    alcMakeContextCurrent(NULL);
    alcDestroyContext(context);

    av_thread_run = 1;
}
