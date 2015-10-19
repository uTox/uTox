#include "main.h"

static vpx_image_t input;

static _Bool openvideodevice(void *handle) {
    if(!video_init(handle)) {
        debug("video_init() failed\n");
        return 0;
    }
    vpx_img_alloc(&input, VPX_IMG_FMT_I420, video_width, video_height, 1);
    utox_video_frame.y = input.planes[0];
    utox_video_frame.u = input.planes[1];
    utox_video_frame.v = input.planes[2];
    utox_video_frame.w = input.d_w;
    utox_video_frame.h = input.d_h;
    return 1;
}

static void closevideodevice(void *handle) {
    video_close(handle);
    vpx_img_free(&input);
}

void toxvideo_postmessage(uint8_t msg, uint32_t param1, uint32_t param2, void *data) {
    while(video_thread_msg) {
        yieldcpu(1);
    }

    video_msg.msg = msg;
    video_msg.param1 = param1;
    video_msg.param2 = param2;
    video_msg.data = data;

    video_thread_msg = 1;
}

void video_thread(void *args) {
    ToxAV *av = args;

    void *video_device;
    _Bool video = 0;
    uint8_t video_count = 0;
    _Bool video_on = 0;
    _Bool preview = 0, newinput = 1;

    // Add always-present null video input device.
    postmessage(VIDEO_IN_DEVICE, STR_VIDEO_IN_NONE, 1, NULL);

    video_device = video_detect();
    if(video_device) {
        video = openvideodevice(video_device);
    }

    video_thread_init = 1;

    while(1) {
        if(video_thread_msg) {
            TOX_MSG *m = &video_msg;
            if (!m->msg) {
                break;
            }
            switch(m->msg) {
                case VIDEO_SET: {
                    if (video_on) {
                        video_endread();
                    }

                    if (video) {
                        closevideodevice(video_device);
                    }

                    video_device = m->data;
                    if(video_device == NULL) {
                        video = 0;
                        video_on = 0;
                        if (!m->param1) {
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
                    m->param1--;
                }
                case VIDEO_START: {
                    STRING *s = SPTR(WINDOW_TITLE_VIDEO_PREVIEW);
                    video_begin(m->param1 + 1, s->str, s->length, video_width, video_height);
                    video_count++;
                    if (video && !video_on) {
                        video_on = video_startread();
                    }
                    break;
                }
                case VIDEO_PREVIEW_END: {
                    debug("preview end %u\n", video_count);
                    preview = 0;
                    video_count--;
                    if (!video_count && video_on) {
                        video_endread();
                        video_on = 0;
                    }
                    video_end(0);
                    break;
                }
                case VIDEO_END: {
                    video_count--;
                    if (!video_count && video_on) {
                        video_endread();
                        video_on = 0;
                    }
                    video_end(m->param1 + 1);
                    break;
                }
            }
            video_thread_msg = 0;
        }

        if (video_on) {
            int r = video_getframe(utox_video_frame.y, utox_video_frame.u, utox_video_frame.v, utox_video_frame.w, utox_video_frame.h);
            if (r == 1) {
                if (preview) {
                    /* Make a copy of the video frame for uTox to display */
                    utox_frame_pkg *frame = malloc(sizeof(*frame));
                    frame->w   = utox_video_frame.w;
                    frame->h   = utox_video_frame.h;
                    frame->img = malloc(utox_video_frame.w * utox_video_frame.h * 4);

                    yuv420tobgr(utox_video_frame.w, utox_video_frame.h,
                                utox_video_frame.y, utox_video_frame.u, utox_video_frame.v,
                                utox_video_frame.w, (utox_video_frame.w / 2), (utox_video_frame.w / 2), frame->img);

                    postmessage(AV_VIDEO_FRAME, 0, 0, (void*)frame);
                    newinput = 0;
                }

                int i, active_video_count = 0;
                for (i = 0; i < UTOX_MAX_NUM_FRIENDS; i++) {
                    if (UTOX_SEND_VIDEO(i)) {
                        active_video_count++;
                        TOXAV_ERR_SEND_FRAME error = 0;
                        toxav_video_send_frame(av, friend[i].number, utox_video_frame.w, utox_video_frame.h, utox_video_frame.y, utox_video_frame.u, utox_video_frame.v, &error);

                        if (error) {
                            debug("toxav_send_video error %i %u\n", friend[i].number, error);
                            if (error == 4) {
                                debug("w and h %u and %u\n", utox_video_frame.w, utox_video_frame.h);
                            }
                        } else {
                            if (i >= UTOX_MAX_CALLS){
                                debug("Trying to send video frame to too many peers. Please report this bug!\n");
                                break;
                            }
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
    debug("UTOX VIDEO:\tClean thread exit!\n");
}
