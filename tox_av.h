// TODO replace this with something else.
struct _utox_av_settings {
    uint32_t v_bitrate, max_v_w, max_v_h;
    uint32_t a_bitrate, a_frame_duration, a_sample_rate, a_channels;
};

struct _utox_av_settings utox_av_settings = {
    500, 1280, 720, 32000, 20, 48000, 1
};


static void av_start(int32_t call_index, void *arg){
/*    ToxAVCSettings peer_settings;

    int fid = toxav_get_peer_id(arg, call_index, 0);
    toxav_get_peer_csettings(arg, call_index, 0, &peer_settings);

    _Bool video = (peer_settings.call_type == av_TypeVideo);

    debug("video for this call: %u\n", video);

    if(toxav_prepare_transmission(arg, call_index, 1) == 0) {
        if(video) {
            postmessage(FRIEND_CALL_VIDEO, fid, call_index, (void*)(640 | (size_t)480 << 16));
        } else {
            postmessage(FRIEND_CALL_STATUS, fid, call_index, (void*)CALL_OK);
        }
    } else {
        debug("A/V FAIL!\n");
    }
*/}

static void utox_av_incoming_call(ToxAV *av, uint32_t friend_number, bool audio, bool video, void *UNUSED(userdata)){
    debug("A/V Invite (%u)\n", friend_number);
    /*int fid = toxav_get_peer_id(arg, call_index, 0);

    ToxAVCSettings peer_settings;
    toxav_get_peer_csettings(arg, call_index, 0, &peer_settings);
    _Bool video = (peer_settings.call_type == av_TypeVideo);
    */
    postmessage(FRIEND_CALL_STATUS, friend_number, friend_number, (void*)(size_t)(video ? CALL_INVITED_VIDEO : CALL_INVITED));
    toxaudio_postmessage(AUDIO_PLAY_RINGTONE, friend_number, 0, NULL);

}

static void callback_av_start(void *arg, int32_t call_index, void *UNUSED(userdata)){
    av_start(call_index, arg);
    toxaudio_postmessage(AUDIO_STOP_RINGTONE, call_index, 0, NULL);

    debug("A/V Start (%i)\n", call_index);
}

static void utox_av_end(ToxAV *av, int32_t friend_number){
    debug("A/V End (%i)\n", friend_number);

    TOXAV_ERR_CALL_CONTROL error = 0;
    toxav_call_control(av, friend_number, TOXAV_CALL_CONTROL_CANCEL, &error);
    if (error) {
        debug("unhanded error in utox_av_end\n");
    }
    postmessage(FRIEND_CALL_STATUS, friend_number, 0, (void*)(size_t)CALL_NONE);
    toxaudio_postmessage(AUDIO_STOP_RINGTONE, friend_number, 0, NULL);
}

static void callback_av_ringing(ToxAV *av, int32_t friend_number){
    debug("A/V Ringing (%i)\n", friend_number);
}

static void callback_av_requesttimeout(ToxAV *av, int32_t friend_number){
    debug("A/V ReqTimeout (%i)\n", friend_number);

    TOXAV_ERR_CALL_CONTROL error = 0;
    toxav_call_control(av, friend_number, TOXAV_CALL_CONTROL_CANCEL, &error);
    if (error) {
        debug("unhanded error in callback_av_requesttimeout\n");
    }
    postmessage(FRIEND_CALL_STATUS, friend_number, 0, (void*)(size_t)CALL_NONE);
    toxaudio_postmessage(AUDIO_STOP_RINGTONE, friend_number, 0, NULL);
}

static void callback_av_peertimeout(ToxAV *av, int32_t friend_number){
    debug("A/V PeerTimeout (%i)\n", friend_number);

    TOXAV_ERR_CALL_CONTROL error = 0;
    toxav_call_control(av, friend_number, TOXAV_CALL_CONTROL_CANCEL, &error);
    if (error) {
        debug("unhanded error in callback_av_peertimeout\n");
    }
    postmessage(FRIEND_CALL_STATUS, friend_number, 0, (void*)(size_t)CALL_NONE);
    toxaudio_postmessage(AUDIO_STOP_RINGTONE, friend_number, 0, NULL);
}

static void callback_av_selfmediachange(void *arg, int32_t friend_number, void *UNUSED(userdata)){
    debug("A/V SelfMediachange (%i)\n", friend_number);
}

static void callback_av_peermediachange(void *arg, int32_t friend_number, void *UNUSED(userdata)){
    /*ToxAVCSettings settings;
    toxav_get_peer_csettings(arg, friend_number, 0, &settings);
    int fid = toxav_get_peer_id(arg, friend_number, 0);

    postmessage(FRIEND_CALL_MEDIACHANGE, fid, friend_number, (settings.call_type == av_TypeVideo) ? (void*)1 : NULL);
    debug("A/V PeerMediachange (%i)\n", friend_number);*/
}

uint8_t lbuffer[800 * 600 * 4]; //needs to be always large enough for encoded frames

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

static void video_thread(void *args)
{
    ToxAV *av = args;

    void *video_device;
    _Bool video = 0;
    uint8_t video_count = 0;
    _Bool video_on = 0;
    _Bool call[MAX_CALLS] = {0}, preview = 0, newinput = 1;

    // Add always-present null video input device.
    postmessage(NEW_VIDEO_DEVICE, STR_VIDEO_IN_NONE, 1, NULL);

    video_device = video_detect();
    if(video_device) {
        video = openvideodevice(video_device);
    }

    video_thread_init = 1;

    while(1) {
        if(video_thread_msg) {
            TOX_MSG *m = &video_msg;
            if(!m->msg) {
                break;
            }

            switch(m->msg) {
            case VIDEO_SET: {
                if(video_on) {
                    video_endread();
                }

                if(video) {
                    closevideodevice(video_device);
                }

                video_device = m->data;
                if(video_device == NULL) {
                    video = 0;
                    video_on = 0;
                    if(m->param1) {
                        goto VIDEO_PREVIEW_END;
                    } else {
                        break;
                    }
                }

                video = openvideodevice(video_device);
                if (video) {
                    if (video_count) {
                        video_on = video_startread();
                    }
                } else {
                    video_on = 0;
                }

                newinput = 1;
                break;
            }

            case VIDEO_PREVIEW_START: {
                preview = 1;
                video_count++;
                if (video && !video_on) {
                    video_on = video_startread();
                }
                break;
            }

            case VIDEO_CALL_START: {
                call[m->param1] = 1;
                video_count++;
                if (video && !video_on) {
                    video_on = video_startread();
                }
                break;
            }

            VIDEO_PREVIEW_END:
            case VIDEO_PREVIEW_END: {
                debug("preview end %u\n", video_count);
                preview = 0;
                video_count--;
                if (!video_count && video_on) {
                    video_endread();
                    video_on = 0;
                }
                break;
            }

            case VIDEO_CALL_END: {
                if (!call[m->param1]) {
                    break;
                }
                call[m->param1] = 0;
                video_count--;
                if (!video_count && video_on) {
                    video_endread();
                    video_on = 0;
                }
                break;
            }
            }

            video_thread_msg = 0;
        }

        if (video_on) {
            uint16_t width = 0, height = 0;
            uint8_t *y = NULL, *u = NULL, *v = NULL;
            int r = video_getframe(y, u, v, width, height);
            if(r == 1) {
                if(preview) {
                    /* Make a copy of the video frame for uTox to display */
                    uint8_t *img_data = malloc(width * height * 4);
                    yuv420tobgr(width, height, y, u, v, width, width / 2, width / 2, img_data);

                    postmessage(PREVIEW_FRAME + newinput, width, height, img_data);
                    newinput = 0;
                }

                int i;
                for(i = 0; i < MAX_CALLS; i++) {
                    if( (friend[i].call_state        | TOXAV_FRIEND_CALL_STATE_SENDING_V   ) &&
                        (friend[i].call_state_friend | TOXAV_FRIEND_CALL_STATE_RECEIVING_V )) {
                        // TODO get the yuv from utox?
                        TOXAV_ERR_SEND_FRAME error = 0;
                        toxav_video_send_frame(av, friend[i].number, width, height, y, u, v, &error);
                        // bool toxav_video_send_frame(ToxAV *toxAV, uint32_t friend_number,
                        // uint16_t width, uint16_t height, const uint8_t *y, const uint8_t *u, const uint8_t *v, *error);
                        if (error) {
                            debug("toxav_send_video error %i %u\n", friend[i].number, error);
                        }
                    }
                }
            } else if(r == -1) {
                video_on = 0;
                video = 0;
                video_endread();
                closevideodevice(video_device);
            }
        }

        yieldcpu(5);
    }

    if (video_on) {
        video_endread();
    }

    if (video) {
        closevideodevice(video_device);
    }

    video_thread_msg = 0;
    video_thread_init = 0;
}

#ifndef NATIVE_ANDROID_AUDIO
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

static ALCdevice *device_out, *device_in;
static ALCcontext *context;
static ALuint source[MAX_CALLS];

static ALCdevice* alcopencapture(void *handle){
    if(!handle) {
        return NULL;
    }

    if(handle == (void*)1) {
        return handle;
    }

    return alcCaptureOpenDevice(handle, utox_av_settings.a_sample_rate, AL_FORMAT_STEREO16, (utox_av_settings.a_frame_duration * utox_av_settings.a_sample_rate * 4) / 1000);
    // return alcCaptureOpenDevice(handle, av_DefaultSettings.audio_sample_rate, AL_FORMAT_MONO16, ((av_DefaultSettings.audio_frame_duration * av_DefaultSettings.audio_sample_rate * 4) / 1000) * av_DefaultSettings.audio_channels);
}

static void alccapturestart(void *handle){
    if(handle == (void*)1) {
        audio_init(handle);
        return;
    }
    alcCaptureStart(handle);
}

static void alccapturestop(void *handle)
{
    if(handle == (void*)1) {
        audio_close(handle);
        return;
    }

    alcCaptureStop(handle);
}

static void alccaptureclose(void *handle)
{
    if(handle == (void*)1) {
        return;
    }

    alcCaptureCloseDevice(handle);
}

static void sourceplaybuffer(int i, const int16_t *data, int samples, uint8_t channels, unsigned int sample_rate)
{
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

    ALint state;
    alGetSourcei(source[i], AL_SOURCE_STATE, &state);
    if(state != AL_PLAYING) {
        alSourcePlay(source[i]);
        debug("Starting source %u\n", i);
    }
}

static void audio_thread(void *args){
    ToxAV *av = args;
    const char *device_list, *output_device = NULL;
    void *audio_device = NULL;

    _Bool call[MAX_CALLS] = {0}, preview = 0;
    _Bool groups_audio[MAX_NUM_GROUPS] = {0};

    int perframe = (utox_av_settings.a_frame_duration * utox_av_settings.a_sample_rate) / 1000;
    uint8_t buf[perframe * 2 * utox_av_settings.a_channels], dest[perframe * 2 * utox_av_settings.a_channels];
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
    if(device_list) {
        audio_device = (void*)device_list;
        debug("Input Device List:\n");
        while(*device_list) {
            debug("%s\n", device_list);
            postmessage(NEW_AUDIO_IN_DEVICE, UI_STRING_ID_INVALID, 0, (void*)device_list);
            device_list += strlen(device_list) + 1;
        }
    }

    postmessage(NEW_AUDIO_IN_DEVICE, STR_AUDIO_IN_NONE, 0, NULL);
    audio_detect();

    if (alcIsExtensionPresent(NULL, "ALC_ENUMERATE_ALL_EXT"))
        device_list = alcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);
    else
        device_list = alcGetString(NULL, ALC_DEVICE_SPECIFIER);

    if(device_list) {
        output_device = device_list;
        debug("Output Device List:\n");
        while(*device_list) {
            debug("%s\n", device_list);
            postmessage(NEW_AUDIO_OUT_DEVICE, 0, 0, (void*)device_list);
            device_list += strlen(device_list) + 1;
        }
    }

    device_out = alcOpenDevice(output_device);
    if(!device_out) {
        debug("alcOpenDevice() failed\n");
        return;
    }

    int attrlist[] = {  ALC_FREQUENCY, utox_av_settings.a_sample_rate,
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
    #define PREVIEW_BUFFER_SIZE (utox_av_settings.a_sample_rate / 2)

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
                        debug("start\n");
                    }
                }
                break;
            }

            case AUDIO_CALL_START: {
                call[m->param1] = 1;
                audio_count++;
                if(!record_on) {
                    device_in = alcopencapture(audio_device);
                    if(device_in) {
                        alccapturestart(device_in);
                        record_on = 1;
                        debug("start\n");
                    }
                }
                break;
            }

            case GROUP_AUDIO_CALL_START: {
                audio_count++;
                groups_audio[m->param1] = 1;
                if(!record_on) {
                    device_in = alcopencapture(audio_device);
                    if(device_in) {
                        alccapturestart(device_in);
                        record_on = 1;
                        debug("start\n");
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
                    debug("stop\n");
                }
                break;
            }

            case AUDIO_CALL_END: {
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
                f_a = new_filter_audio(utox_av_settings.a_sample_rate);
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
            if(device_in == (void*)1) {
                frame = audio_frame((void*)buf);
                if (frame) {
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
                    set_echo_delay_ms(f_a, utox_av_settings.a_frame_duration);
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

                if(preview) {
                    if (preview_buffer_index + perframe > PREVIEW_BUFFER_SIZE) {
                        preview_buffer_index = 0;
                    }

                    sourceplaybuffer(0, preview_buffer + preview_buffer_index, perframe, utox_av_settings.a_channels, utox_av_settings.a_sample_rate);
                    if (voice) {
                        memcpy(preview_buffer + preview_buffer_index, buf, perframe * sizeof(int16_t));
                    } else {
                        memset(preview_buffer + preview_buffer_index, 0, perframe * sizeof(int16_t));
                    }

                    preview_buffer_index += perframe;
                }

                if (voice) {
                    int i;
                    for(i = 0; i < MAX_CALLS; i++) {
                        if((friend[i].call_state | TOXAV_FRIEND_CALL_STATE_SENDING_V) && (friend[i].call_state_friend | TOXAV_FRIEND_CALL_STATE_RECEIVING_V)) {
                            TOXAV_ERR_SEND_FRAME error = 0;
                            // bool toxav_audio_send_frame(ToxAV *toxAV, uint32_t friend_number, const int16_t *pcm, size_t sample_count, uint8_t channels, uint32_t sampling_rate, TOXAV_ERR_SEND_FRAME *error);
                            toxav_audio_send_frame(av, friend[i].number, (const uint16_t *)buf, samples, utox_av_settings.a_channels, perframe, &error);
                            if (error) {
                                debug("toxav_send_audio error %i %i\n", friend[i].number, error);
                            }
                        }
                    }

                    Tox *tox = toxav_get_tox(av);
                    uint32_t num_chats = tox_count_chatlist(tox);

                    if (num_chats != 0) {
                        int32_t chats[num_chats];
                        uint32_t max = tox_get_chatlist(tox, chats, num_chats);
                        for (i = 0; i < max; ++i) {
                            if (groups_audio[chats[i]]) {
                                toxav_group_send_audio(tox, chats[i], (int16_t *)buf, perframe, utox_av_settings.a_channels, utox_av_settings.a_sample_rate);
                            }
                        }
                    }
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


void toxaudio_postmessage(uint8_t msg, uint32_t param1, uint32_t param2, void *data)
{
    while(audio_thread_msg) {
        yieldcpu(1);
    }

    audio_msg.msg = msg;
    audio_msg.param1 = param1;
    audio_msg.param2 = param2;
    audio_msg.data = data;

    audio_thread_msg = 1;
}

void callback_av_group_audio(Tox *tox, int groupnumber, int peernumber, const int16_t *pcm, unsigned int samples,
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

void group_av_peer_add(GROUPCHAT *g, int peernumber)
{
    alGenSources(1, &g->source[peernumber]);
}

void group_av_peer_remove(GROUPCHAT *g, int peernumber)
{
    alDeleteSources(1, &g->source[peernumber]);
}

#else
    /* android audio functions */
    static void audio_thread(void *args){}

    void toxaudio_postmessage(uint8_t msg, uint32_t param1, uint32_t param2, void *data)    {
        switch(msg) {
        case AUDIO_SET_INPUT: {;
            break;
        }

        case AUDIO_SET_OUTPUT: {
            break;
        }

        case AUDIO_PREVIEW_START: {
            break;
        }

        case AUDIO_CALL_START: {
            audio_begin(param1);
            break;
        }

        case AUDIO_PREVIEW_END: {
            break;
        }

        case AUDIO_CALL_END: {
            audio_end(param1);
            break;
        }

        }
    }
#endif

static void toxav_thread(void *args)
{
    ToxAV *av = args;

    toxav_thread_init = 1;

    debug("Toxav thread init\n");
    while (1) {
        if(toxav_thread_msg) {
            TOX_MSG *msg = &toxav_msg;
            if(msg->msg == TOXAV_KILL) {
                break;
            }
            /*
            switch(m->msg) {
            }*/
            toxav_thread_msg = 0;
        }

        toxav_iterate(av);
        yieldcpu(toxav_iteration_interval(av));
    }

    debug("Toxav thread die\n");
    toxav_thread_msg = 0;
    toxav_thread_init = 0;
}

/** responds to a audio frame call back from toxav
 *
 * Moving this here might break Android, if you know this commit compiles and runs on android, remove this line!
 */
static void utox_av_incoming_frame_a(ToxAV *av, int32_t friend_number, int16_t *pcm, size_t sample_count,
                                     uint8_t channels, uint32_t sample_rate, void *userdata){
    #ifdef NATIVE_ANDROID_AUDIO
    audio_play(friend_number, pcm, sample_count, channels);
    #else
    sourceplaybuffer(friend_number, pcm, sample_count, channels, sample_rate);
    #endif
}


static void utox_av_incoming_frame_v(ToxAV *toxAV, uint32_t friend_number, uint16_t width, uint16_t height,
                                        const uint8_t *y, const uint8_t *u, const uint8_t *v,
                                        int32_t ystride, int32_t ustride, int32_t vstride, void *user_data){
    /* copy the vpx_image */
    /* 4 bits for the H*W, then a pixel for each color * size */
    uint16_t *img_data = malloc(4 + width * height * 4);
    img_data[0] = width;
    img_data[1] = height;
    yuv420tobgr(width, height, y, u, v, ystride, ustride, vstride, (void*)&img_data[2]);

    postmessage(FRIEND_VIDEO_FRAME, friend_number, friend_number, img_data);
}



/** respond to a Audio Video state change call back from toxav */
static void utox_callback_av_change_state(ToxAV *av, uint32_t friend_number, uint32_t state, void *userdata){
    if ( state == 1 ) {
        // handle error
        utox_av_end(av, friend_number);
        debug("ToxAV:\tChange state error, send bug report\n");
        return;
    } else if ( state == 2 ) {
        utox_av_end(av, friend_number);
        debug("ToxAV:\tCall ended with friend_number %u.\n", friend_number);
    }

    int TOXAV_FRIEND_CALL_STATE_SENDING_A = 4;
    int TOXAV_FRIEND_CALL_STATE_SENDING_V = 8;
    int TOXAV_FRIEND_CALL_STATE_RECIVING_A = 16;
    int TOXAV_FRIEND_CALL_STATE_RECIVING_V = 32;

    int state_audio = (state | (TOXAV_FRIEND_CALL_STATE_SENDING_A | TOXAV_FRIEND_CALL_STATE_RECIVING_A));
    int state_video = (state | (TOXAV_FRIEND_CALL_STATE_SENDING_V | TOXAV_FRIEND_CALL_STATE_RECIVING_V));

    if (friend[friend_number].call_state ^ state_audio) {
        // do change
        // start audio
        // stop audio
    }

    if (friend[friend_number].call_state ^ state_video) {
        // start video
        // stop video
    }
}

static void set_av_callbacks(ToxAV *av){
    toxav_callback_call(av, &utox_av_incoming_call, NULL);

    toxav_callback_call_state(av, &utox_callback_av_change_state, NULL);

    // toxav_register_callstate_callback(av, callback_av_start, av_OnStart, NULL);

    // toxav_register_callstate_callback(av, callback_av_ringing, av_OnRinging, NULL);

    // toxav_register_callstate_callback(av, callback_av_requesttimeout, av_OnRequestTimeout, NULL);
    // toxav_register_callstate_callback(av, callback_av_peertimeout, av_OnPeerTimeout, NULL);
    // toxav_register_callstate_callback(av, callback_av_selfmediachange, av_OnSelfCSChange, NULL);
    // toxav_register_callstate_callback(av, callback_av_peermediachange, av_OnPeerCSChange, NULL);

    // toxav_register_audio_callback(av, callback_av_audio, NULL);
    // toxav_register_video_callback(av, callback_av_video, NULL);
    toxav_callback_audio_receive_frame(av, &utox_av_incoming_frame_a, NULL);
    toxav_callback_video_receive_frame(av, &utox_av_incoming_frame_v, NULL);
}
