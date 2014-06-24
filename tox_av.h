
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

#ifdef V4L

#define CLEAR(x) memset(&(x), 0, sizeof(x))

static int xioctl(int fh, int request, void *arg)
{
    int r;

    do {
        r = ioctl(fh, request, arg);
    } while (-1 == r && EINTR == errno);

    return r;
}

static _Bool video_init(char *dev_name)
{
    struct stat st;
    if (-1 == stat(dev_name, &st)) {
        debug("Cannot identify '%s': %d, %s\n", dev_name, errno, strerror(errno));
        return 0;
    }

    if (!S_ISCHR(st.st_mode)) {
        debug("%s is no device\n", dev_name);
        return 0;
    }

    fd = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

    if (-1 == fd) {
        debug("Cannot open '%s': %d, %s\n", dev_name, errno, strerror(errno));
        return 0;
    }

    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    unsigned int min;

    if(-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
        if (EINVAL == errno) {
            debug("%s is no V4L2 device\n", dev_name);
        } else {
            debug("VIDIOC_QUERYCAP error %d, %s\n", errno, strerror(errno));
        }
        return 0;
    }

    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        debug("%s is no video capture device\n", dev_name);
        return 0;
    }

    if(!(cap.capabilities & V4L2_CAP_STREAMING)) {
        debug("%s does not support streaming i/o\n", dev_name);
        return 0;
    }

    /* Select video input, video standard and tune here. */
    CLEAR(cropcap);

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; /* reset to default */

        if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
            switch (errno) {
            case EINVAL:
                /* Cropping not supported. */
                break;
            default:
                /* Errors ignored. */
                break;
            }
        }
    } else {
        /* Errors ignored. */
    }

    v4lconvert_data = v4lconvert_create(fd);

    CLEAR(fmt);

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if(-1 == xioctl(fd, VIDIOC_G_FMT, &fmt)) {
        debug("VIDIOC_S_FMT error %d, %s\n", errno, strerror(errno));
        return 0;
    }

    /*if(fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_YUYV) {
        debug("Unsupported video format: %u %u %u %u\n", fmt.fmt.pix.width, fmt.fmt.pix.height, fmt.fmt.pix.pixelformat, fmt.fmt.pix.field);
    }*/

    video_width = fmt.fmt.pix.width;
    video_height = fmt.fmt.pix.height;
    dest_fmt.fmt.pix.width = fmt.fmt.pix.width;
    dest_fmt.fmt.pix.height = fmt.fmt.pix.height;
    debug("Video size: %u %u\n", video_width, video_height);



    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
        fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
        fmt.fmt.pix.sizeimage = min;


    /* part 3*/
    uint32_t buffer_size = fmt.fmt.pix.sizeimage;
    struct v4l2_requestbuffers req;

    CLEAR(req);

    req.count  = 4;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_USERPTR;

    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            debug("%s does not support user pointer i/o\n", dev_name);
        } else {
            debug("VIDIOC_REQBUFS error %d, %s\n", errno, strerror(errno));
        }
        return 0;
    }

    buffers = calloc(4, sizeof(*buffers));

    if (!buffers) {
        debug("Out of memory\n");
        return 0;
    }

    for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
        buffers[n_buffers].length = buffer_size;
        buffers[n_buffers].start = malloc(buffer_size);

        if (!buffers[n_buffers].start) {
            debug("Out of memory\n");
            return 0;
        }
    }

    unsigned int i;
    enum v4l2_buf_type type;

    for (i = 0; i < n_buffers; ++i) {
        struct v4l2_buffer buf;

        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_USERPTR;
        buf.index = i;
        buf.m.userptr = (unsigned long)buffers[i].start;
        buf.length = buffers[i].length;

        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf)) {
            debug("VIDIOC_QBUF error %d, %s\n", errno, strerror(errno));
            return 0;
        }
    }
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(fd, VIDIOC_STREAMON, &type)) {
        debug("VIDIOC_STREAMON error %d, %s\n", errno, strerror(errno));
        return 0;
    }

    vpx_img_alloc(&input, VPX_IMG_FMT_I420, video_width, video_height, 1);

    return 1;
}

uint8_t lbuffer[800 * 600 * 4];

static _Bool read_frame(ToxAv *av)
{
    struct v4l2_buffer buf;
    unsigned int i;

    CLEAR(buf);

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_USERPTR;

    if (-1 == ioctl(fd, VIDIOC_DQBUF, &buf)) {
        switch (errno) {
        case EINTR:
        case EAGAIN:
            return 0;

        case EIO:
        /* Could ignore EIO, see spec. */

        /* fall through */

        default:
            debug("VIDIOC_DQBUF error %d, %s\n", errno, strerror(errno));
            return 0;

        }
    }

    for (i = 0; i < n_buffers; ++i)
        if (buf.m.userptr == (unsigned long)buffers[i].start
                && buf.length == buffers[i].length)
            break;

    if(i >= n_buffers) {
        debug("fatal error\n");
        return 0;
    }

    if(fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV) {
        yuv422to420(input.planes[0], input.planes[1], input.planes[2], (void*)buf.m.userptr, video_width, video_height);
    } else {
        v4lconvert_convert(v4lconvert_data, &fmt, &dest_fmt, (void*)buf.m.userptr, fmt.fmt.pix.sizeimage, input.planes[0], (video_width * video_height * 3) / 2);
    }

    for(i = 0; i < MAX_CALLS; i++) {
        if(call[i].active && call[i].video) {
            int r, len;
            if((len = toxav_prepare_video_frame(av, i, lbuffer, sizeof(lbuffer), &input)) < 0) {
                debug("toxav_prepare_video_frame error %i\n", r);
                continue;
            }

            if((r = toxav_send_video(av, i, (void*)lbuffer, len)) < 0) {
                debug("toxav_send_video error %i %s\n", r, strerror(errno));
            } else {
                debug("sent frame %u\n", len);
            }
        }
    }

    if (-1 == xioctl(fd, VIDIOC_QBUF, &buf)) {
        debug("VIDIOC_DQBUF error %d, %s\n", errno, strerror(errno));
    }

    return 1;
}

#else
_Bool video_init(char *dev_name)
{
    return 0;
}

_Bool read_frame(ToxAv *av)
{
    return 0;
}

#endif

static void av_thread(void *args)
{
    ToxAv *av = args;
    ALuint *p, *end;
    const char *device_list;
    _Bool record, video;

    set_av_callbacks(av);

    if(!(video = video_init("/dev/video0"))) {
        if(!(video = video_init("/dev/video1"))) {
            debug("no video\n");
        }
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

    device_in = alcCaptureOpenDevice(NULL, av_DefaultSettings.audio_sample_rate, AL_FORMAT_MONO16, (av_DefaultSettings.audio_frame_duration * av_DefaultSettings.audio_sample_rate * 2) / 1000);
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

        if(video) {
            if(read_frame(av)) {
                //debug("video frame\n");
            }
        }

        int i;
        for(i = 0; i < MAX_CALLS; i++) {
            if(call[i].active) {
                int size;
                vpx_image_t *image;
                if(toxav_recv_video(av, i, &image) == 0) {
                    if(image) {
                        postmessage(FRIEND_VIDEO_FRAME, toxav_get_peer_id(av, i, 0), i, image);
                    }
                }

                size = toxav_recv_audio(av, i, perframe, (void*)buf);
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
