#include "main.h"

static vpx_image_t input;

static _Bool openvideodevice(void *handle) {
    // initialize video (will populate video_width and video_height)
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

    // holds the currently selected video device
    void *video_device;
    // Whether a video device has been opened
    _Bool video_device_open = 0;
    // Whether video capturing is active
    _Bool video_active = 0;
    // counts the number of calls/previews that are currently active
    uint16_t video_count = 0;

    // Add always-present null video input device.
    postmessage(VIDEO_IN_DEVICE, STR_VIDEO_IN_NONE, 1, NULL);

    // select a video device (autodectect)
    video_device = video_detect();
    if (video_device) {
        // open the video device to get some info e.g. frame size
        // close it afterwards to not block the device while it is not used
        if (openvideodevice(video_device)) {
            closevideodevice(video_device);
        }
    }

    video_thread_init = 1;

    while (1) {
        if (video_thread_msg) {
            TOX_MSG *m = &video_msg;
            if (!m->msg) {
                break;
            }
            switch (m->msg) {
                // VIDEO_SET: select a different video device
                case VIDEO_SET: {
                    // end capturing if active
                    if (video_active) {
                        video_endread();
                    }

                    // close the currently active video device if any
                    if (video_device_open) {
                        closevideodevice(video_device);
                        video_device_open = 0;
                    }

                    // select the new device
                    video_device = m->data;
                    if(video_device == NULL) {
                        // reset if no device is selected
                        video_device_open = 0;
                        video_active = 0;
                        video_count = 0;
                        video_width = 0;
                        video_height = 0;
                        debug("uToxVID:\tChanged video input to NONE\n");
                        break;
                    }

                    // directly open new device if any calls/previews are currently active
                    if (video_count) {
                        video_device_open = openvideodevice(video_device);
                        if (video_device_open) {
                            video_active = video_startread();
                        } else {
                            // reset if opening failed
                            video_active = 0;
                            video_count  = 0;
                            debug("uToxVID:\tChanging video input device failed: unable to open device.\n");
                            break;
                        }
                    } else {
                        // open the video device to get some info e.g. frame size
                        // close it afterwards to not block the device while it is not used
                        if (openvideodevice(video_device)) {
                            closevideodevice(video_device);
                        }
                        video_active = 0;
                    }

                    debug("uToxVID:\tChanged video input device\n");
                    break;
                }
                // VIDEO_PREVIEW_START: a video preview has been opened
                case VIDEO_PREVIEW_START: {
                    debug("uToxVID:\tStarting video preview\n");
                    video_preview = 1;
                }
                // VIDEO_RECORD_START: a video call has started
                case VIDEO_RECORD_START: {
                    // open device if not opended already
                    if (video_device && !video_device_open) {
                        video_device_open = openvideodevice(video_device);
                    }
                    if (!video_device_open) {
                        debug("uToxVID:\tFailed to start video, no open video device\n");
                        break;
                    }
                    // if video device is successfully opended, start capturing
                    if ( video_width && video_height ) {
                        video_count++;
                        if (video_count && !video_active) {
                            video_active = video_startread();
                        }
                    } else {
                        debug("uToxVID:\tCan't start video for a 0 by 0 frame\n");
                        break;
                    }
                    debug("uToxVID:\tStarting video feed (%i)\n", video_count);
                    break;
                }
                // VIDEO_PREVIEW_STOP: a video preview has ended
                case VIDEO_PREVIEW_STOP: {
                    debug("uToxVID:\tClosing video preview\n");
                    video_preview = 0;
                    postmessage(AV_CLOSE_WINDOW, 0, 0, NULL);
                }
                // VIDEO_RECORD_STOP: a video call has ended
                case VIDEO_RECORD_STOP: {
                    video_count--;
                    debug("uToxVID:\tEnd of video record... (%i)\n", video_count);
                    if (video_count > UTOX_MAX_CALLS) {
                        debug("uToxVID:\tExceeded max calls, abort abort!!\n");
                        video_count = 0;
                    }
                    if (!video_count && video_active) {
                        debug("uToxVID:\tLast video feed close, ending read.\n");
                        video_endread();
                        video_active = 0;
                        // this was the last call/preview, close the device
                        closevideodevice(video_device);
                        video_device_open = 0;
                    }
                    break;
                }
            }
            video_thread_msg = 0;
        }

        if (video_active) {
            // capturing is enabled, capture frames
            int r = video_getframe(utox_video_frame.y, utox_video_frame.u, utox_video_frame.v, utox_video_frame.w, utox_video_frame.h);
            if (r == 1) {
                if (video_preview) {
                    /* Make a copy of the video frame for uTox to display */
                    utox_frame_pkg *frame = malloc(sizeof(*frame));
                    frame->w   = utox_video_frame.w;
                    frame->h   = utox_video_frame.h;
                    frame->img = malloc(utox_video_frame.w * utox_video_frame.h * 4);

                    yuv420tobgr(utox_video_frame.w, utox_video_frame.h,
                                utox_video_frame.y, utox_video_frame.u, utox_video_frame.v,
                                utox_video_frame.w, (utox_video_frame.w / 2), (utox_video_frame.w / 2), frame->img);

                    postmessage(AV_VIDEO_FRAME, 0, 1, (void*)frame);
                }

                int i, active_video_count = 0;
                for (i = 0; i < UTOX_MAX_NUM_FRIENDS; i++) {
                    if (SEND_VIDEO_FRAME(i)) {
                        active_video_count++;
                        TOXAV_ERR_SEND_FRAME error = 0;
                        toxav_video_send_frame(av, friend[i].number, utox_video_frame.w, utox_video_frame.h, utox_video_frame.y, utox_video_frame.u, utox_video_frame.v, &error);

                        if (error) {
                            if (error == TOXAV_ERR_SEND_FRAME_SYNC) {
                                debug("Vid Frame sync error: w=%u h=%u\n", utox_video_frame.w, utox_video_frame.h);
                            } else if (error == TOXAV_ERR_SEND_FRAME_PAYLOAD_TYPE_DISABLED) {
                                debug("ToxAV disagrees with our AV state for friend %u, self %u, friend %u\n",
                                      i, friend[i].call_state_self, friend[i].call_state_friend);
                            } else {
                                debug("toxav_send_video error friend: %i error: %u\n", friend[i].number, error);
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
                video_active = 0;
                video_device_open = 0;
                video_endread();
                closevideodevice(video_device);
            }
        }

        yieldcpu(5);
    }

    if (video_active) {
        video_endread();
    }

    if (video_device_open) {
        closevideodevice(video_device);
    }

    video_thread_msg = 0;
    video_thread_init = 0;
    debug("UTOX VIDEO:\tClean thread exit!\n");
}
