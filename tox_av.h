
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

static void av_thread(void *args)
{
    ToxAv *av = args;
    ALuint *p, *end;
    const char *device_list;
    _Bool record, video;
    vpx_image_t input;

    set_av_callbacks(av);

    if(!(video = video_init())) {
        debug("no video\n");
    } else {
        vpx_img_alloc(&input, VPX_IMG_FMT_I420, video_width, video_height, 1);
    }

    device_list = alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
    if(device_list) {
        debug("Input Device List:\n");
        while(*device_list) {
            printf("%s\n", device_list);
            device_list += strlen(device_list) + 1;
        }
    }

    device_list = alcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);
    if(device_list) {
        debug("Output Device List:\n");
        while(*device_list) {
            printf("%s\n", device_list);
            device_list += strlen(device_list) + 1;
        }
    }

    device_out = alcOpenDevice(NULL);
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

    device_in = alcCaptureOpenDevice(NULL, av_DefaultSettings.audio_sample_rate, AL_FORMAT_MONO16, (av_DefaultSettings.audio_frame_duration * av_DefaultSettings.audio_sample_rate * 4) / 1000);
    if(!device_in) {
        printf("no audio input, disabling audio input\n");
        record = 0;
    } else {
        alcCaptureStart(device_in);
        record = 1;
    }

    alListener3f(AL_POSITION, 0.0, 0.0, 0.0);
    alListener3f(AL_VELOCITY, 0.0, 0.0, 0.0);

    alGenSources(countof(source), source);
    p = source;
    end = p + countof(source);
    while(p != end) {
        ALuint s = *p++;
        alSourcef(s, AL_PITCH, 1.0);
        alSourcef(s, AL_GAIN, 1.0);
        alSource3f(s, AL_POSITION, 0.0, 0.0, 0.0);
        alSource3f(s, AL_VELOCITY, 0.0, 0.0, 0.0);
        alSourcei(s, AL_LOOPING, AL_FALSE);
    }

    int perframe = (av_DefaultSettings.audio_frame_duration * av_DefaultSettings.audio_sample_rate) / 1000;
    uint8_t buf[perframe * 2], dest[perframe * 2];
    uint8_t video_count = 0;
    _Bool video_on = 0;

    av_thread_run = 1;

    while(av_thread_run) {
        if(record) {
            ALint samples;
            alcGetIntegerv(device_in, ALC_CAPTURE_SAMPLES, sizeof(samples), &samples);
            if(samples >= perframe) {
                alcCaptureSamples(device_in, buf, perframe);

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

        int i, vc = 0;
        for(i = 0; i < MAX_CALLS; i++) {
            if(call[i].active) {
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
                    ALuint bufid;
                    ALint processed;
                    alGetSourcei(source[i], AL_BUFFERS_PROCESSED, &processed);

                    if(processed) {
                        alSourceUnqueueBuffers(source[i], 1, &bufid);
                    } else {
                        alGenBuffers(1, &bufid);
                    }

                    alBufferData(bufid, AL_FORMAT_MONO16, buf, size * 2, av_DefaultSettings.audio_sample_rate);
                    alSourceQueueBuffers(source[i], 1, &bufid);

                    ALint state;;
                    alGetSourcei(source[i], AL_SOURCE_STATE, &state);
                    if(state != AL_PLAYING) {
                        alSourcePlay(source[i]);
                        debug("Starting source %u\n", i);
                    }
                }
            }
        }

        vc += video_preview;

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

    if(record) {
        alcCaptureStop(device_in);
        alcCaptureCloseDevice(device_in);
    }

    alcCloseDevice(device_out);
    alcMakeContextCurrent(NULL);
    alcDestroyContext(context);

    av_thread_run = 1;
}
