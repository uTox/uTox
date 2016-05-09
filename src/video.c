#include "main.h"

static void    *video_device[16]     = {NULL}; /* TODO; magic number */
static int16_t  video_device_count   = 0;
static uint32_t video_device_current = 0;
static _Bool    video_active         = 0;

static _Bool    video_device_status    = 1;

static vpx_image_t input;

static pthread_mutex_t video_thread_lock;


static _Bool video_device_init(void *handle) {
    // initialize video (will populate video_width and video_height)
    if (handle == (void*)1) {
        if (!native_video_init((void*)1)){
            debug("uToxVideo:\tnative_video_init() failed for desktop\n");
            return 0;
        }
    } else {
        if (!handle || !native_video_init(*(void**)handle)) {
            debug("uToxVideo:\tnative_video_init() failed webcam\n");
            return 0;
        }
    }
    vpx_img_alloc(&input, VPX_IMG_FMT_I420, video_width, video_height, 1);
    utox_video_frame.y = input.planes[0];
    utox_video_frame.u = input.planes[1];
    utox_video_frame.v = input.planes[2];
    utox_video_frame.w = input.d_w;
    utox_video_frame.h = input.d_h;

    debug_notice("uToxVideo:\tvideo init done!\n");
    video_device_status = 1;

    return 1;
}

static void close_video_device(void *handle) {
    if (handle >= (void*)2) {
        native_video_close(*(void**)handle);
        vpx_img_free(&input);
    }
    video_device_status = 0;
}

static _Bool video_device_start(void){
    if (video_device_status) {
        native_video_startread();
        video_active = 1;
        return 1;
    }
    video_active = 0;
    return 0;
}

static _Bool video_device_stop(void){
    if (video_device_status) {
        native_video_endread();
        video_active = 0;
        return 1;
    }
    video_active = 0;
    return 0;
}


void utox_video_append_device(void *device, _Bool localized, void *name, _Bool default_) {
    video_device[video_device_count++] = device;

    if (localized) {
        // Device name is localized with name containing UI_STRING_ID.
        // device is device handle pointer.
        list_dropdown_add_localized(&dropdown_video, (UI_STRING_ID)name, device);
    } else {
        // Device name is a hardcoded string.
        // device is a pointer to a buffer, that contains device handle pointer,
        // followed by device name string.
        list_dropdown_add_hardcoded(&dropdown_video, name, *(void**)device);
    }

    /* TODO remove all default settings */
    // default == true, if this device will be chosen by video detecting code.
    if (default_) {
        dropdown_video.selected = dropdown_video.over = (dropdown_video.dropcount - 1);
    }

}

_Bool utox_video_change_device(uint16_t device_number) {
    pthread_mutex_lock(&video_thread_lock);

    static _Bool _was_active = 0;

    if (!device_number){
        video_device_current = 0;
        if (video_active) {
            video_device_stop();
            close_video_device(video_device[video_device_current]);
            if (settings.video_preview) {
                settings.video_preview = 0;
                postmessage(AV_CLOSE_WINDOW, 0, 0, NULL);
            }
        }
        debug("uToxVideo:\tDisabled Video device (none)\n");
        pthread_mutex_unlock(&video_thread_lock);
        return 0;
    }

    if (video_active) {
        _was_active = 1;
        video_device_stop();
        close_video_device(video_device[video_device_current]);
    } else {
        _was_active = 0;
    }

    video_device_current = device_number;

    video_device_init(video_device[device_number]);

    if (_was_active) {
        debug("uToxVideo:\tTrying to restart video with new device...\n");
        if (!video_device_start()) {
            debug_error("uToxVideo:\tError, unable to start new device...\n");
            if (settings.video_preview) {
                settings.video_preview = 0;
                postmessage(AV_CLOSE_WINDOW, 0, 0, NULL);
            }

            pthread_mutex_unlock(&video_thread_lock);
            return 0;
        }
        pthread_mutex_unlock(&video_thread_lock);
        return 1;
    } else {
        /* Just grab the new frame size */
        close_video_device(video_device[video_device_current]);
    }
    pthread_mutex_unlock(&video_thread_lock);
    return 0;
}

_Bool utox_video_start(_Bool preview){
    if (video_active) {
        debug_notice("uToxVideo:\tvideo already running\n");
        return 1;
    }

    if (!video_device_current) {
        debug_notice("uToxVideo:\tNot starting device None\n");
        return 0;
    }

    if (preview) {
        settings.video_preview = 1;
    }

    if (video_device_init(video_device[video_device_current])
        && video_device_start()) {
        video_active = 1;
        debug_notice("uToxVideo:\tstarted video\n");
        return 1;
    }

    debug_error("uToxVideo:\tUnable to start video.\n");
    return 0;
}

_Bool utox_video_stop(_Bool preview){
    if (!video_active) {
        debug("uToxVideo:\tvideo already stopped!\n");
        return 0;
    }

    video_active  = 0;
    settings.video_preview = 0;
    postmessage(AV_CLOSE_WINDOW, 0, 0, NULL);

    video_device_stop();
    close_video_device(video_device[video_device_current]);
    debug("uToxVideo:\tstopped video\n");
    return 1;
}

void postmessage_video(uint8_t msg, uint32_t param1, uint32_t param2, void *data) {
    while(video_thread_msg) {
        yieldcpu(1);
    }

    video_msg.msg = msg;
    video_msg.param1 = param1;
    video_msg.param2 = param2;
    video_msg.data = data;

    video_thread_msg = 1;
}

void utox_video_thread(void *args) {
    ToxAV *av = args;

    pthread_mutex_init(&video_thread_lock, NULL);

    // Add always-present null video input device.
    utox_video_append_device(NULL, 1, (void*)STR_VIDEO_IN_NONE, 1);

    // select a video device (autodectect)
    video_device_current = native_video_detect();

    if (video_device_current) {
        // open the video device to get some info e.g. frame size
        // close it afterwards to not block the device while it is not used
        if (video_device_init(video_device[video_device_current])) {
            close_video_device(video_device[video_device_current]);
        }
    }

    utox_video_thread_init = 1;

    while (1) {
        if (video_thread_msg) {
            TOX_MSG *m = &video_msg;
            if (!m->msg || m->msg == UTOXVIDEO_KILL) {
                break;
            }
        }

        if (video_active) {
            pthread_mutex_lock(&video_thread_lock);
            // capturing is enabled, capture frames
            int r = native_video_getframe(utox_video_frame.y, utox_video_frame.u, utox_video_frame.v, utox_video_frame.w, utox_video_frame.h);
            if (r == 1) {
                if (settings.video_preview) {
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

                uint32_t i, active_video_count = 0;
                for (i = 0; i < UTOX_MAX_NUM_FRIENDS; i++) {
                    if (SEND_VIDEO_FRAME(i)) {
                        debug("sending to friend %u", i);
                        active_video_count++;
                        TOXAV_ERR_SEND_FRAME error = 0;
                        toxav_video_send_frame(av, friend[i].number, utox_video_frame.w, utox_video_frame.h, utox_video_frame.y, utox_video_frame.u, utox_video_frame.v, &error);
                        // debug("uToxVideo:\tSent video frame to friend %u\n", i);
                        if (error) {
                            if (error == TOXAV_ERR_SEND_FRAME_SYNC) {
                                debug("uToxVideo:\tVid Frame sync error: w=%u h=%u\n", utox_video_frame.w, utox_video_frame.h);
                            } else if (error == TOXAV_ERR_SEND_FRAME_PAYLOAD_TYPE_DISABLED) {
                                debug("uToxVideo:\tToxAV disagrees with our AV state for friend %u, self %u, friend %u\n",
                                      i, friend[i].call_state_self, friend[i].call_state_friend);
                            } else {
                                debug("uToxVideo:\ttoxav_send_video error friend: %i error: %u\n", friend[i].number, error);
                            }
                        } else {
                            if (active_video_count >= UTOX_MAX_CALLS){
                                debug("uToxVideo:\tTrying to send video frame to too many peers. Please report this bug!\n");
                                break;
                            }
                        }
                    }
                }
            } else if(r == -1) {
                debug("uToxVideo:\tErr... something really bad happened trying to get this frame, I'm just going to plots now!\n");
                video_device_stop();
                close_video_device(video_device);
            }

            pthread_mutex_unlock(&video_thread_lock);
            yieldcpu(40); /* 60fps = 16.666ms || 25 fps = 40ms || the data quality is SO much better at 25... */
            continue; /* We're running video, so don't sleep for and extra 100 */
        }

        yieldcpu(100);
    }

    video_thread_msg = 0;
    utox_video_thread_init = 0;
    debug("uToxVideo:\tClean thread exit!\n");
}
