#include "main.h"

static vpx_image_t input;
static _Bool utox_open_video_device(void *handle) {
    // initialize video (will populate video_width and video_height)
    if (handle == (void*)1) {
        if (!video_init((void*)1)){
            debug("uToxVideo:\tvideo_init() failed for desktop\n");
            return 0;
        }
    } else {
        if(!video_init(*(void**)handle)) {
            debug("uToxVideo:\tvideo_init() failed webcam\n");
            return 0;
        }
    }
    vpx_img_alloc(&input, VPX_IMG_FMT_I420, video_width, video_height, 1);
    utox_video_frame.y = input.planes[0];
    utox_video_frame.u = input.planes[1];
    utox_video_frame.v = input.planes[2];
    utox_video_frame.w = input.d_w;
    utox_video_frame.h = input.d_h;
    debug("uToxVideo:\tvideo init done!\n");

    return 1;
}

static void utox_close_video_device(void *handle) {
    if (handle >= (void*)2) {
        video_close(*(void**)handle);
        vpx_img_free(&input);
    }
}

static          void    *video_device[16]     = {NULL}; /* TODO; magic number */
static          int16_t  video_device_count   = 0;
static volatile uint32_t video_device_current = 0;
static volatile _Bool    video_active         = 0;

void utox_video_append_device(void *device, _Bool localized, void *name, _Bool default_) {
    video_device[video_device_count++] = device;

    if (localized) {
        // Device name is localized with name containing UI_STRING_ID.
        // device is device handle pointer.
        list_dropdown_add_localized(&dropdown_video, name, device);
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

void utox_video_change_device(uint16_t device_number) {
    if (!device_number){
        video_device_current = 0;
        video_active         = 0;
        debug("uToxVideo:\tDisabled Video device (none)\n");
        return;
    }

    if (video_active) {
        video_endread();
        utox_close_video_device(video_device[video_device_current]);
    }

    video_device_current = device_number;

    utox_open_video_device(video_device[device_number]);

    if (video_active) {
        video_startread();
    } else {
        /* Just grab the new frame size */
        utox_close_video_device(video_device[video_device_current]);
    }

}

void utox_video_record_start(_Bool preview){
    if (video_active) {
        debug("uToxVideo:\tvideo already running\n");
        return;
    }

    if (!video_device_current) {
        debug("uToxVideo:\tNot starting device None\n");
        return;
    }

    if (preview)
        video_preview = 1;

    utox_open_video_device(video_device[video_device_current]);
    video_startread();
    video_active = 1;
    debug("uToxVideo:\tstarted video\n");
}

void utox_video_record_stop(_Bool preview){
    if (!video_active) {
        debug("uToxVideo:\tvideo already stopped!\n");
        return;
    }
    video_active  = 0;

    if (preview) {
        video_preview = 0;
    }

    video_endread();
    utox_close_video_device(video_device[video_device_current]);
    debug("uToxVideo:\tstopped video\n");
}

void utox_video_thread(void *args) {
    ToxAV *av = args;

    // Add always-present null video input device.
    utox_video_append_device(NULL, 1, STR_VIDEO_IN_NONE, 1);

    // select a video device (autodectect)
    video_device_current = native_video_detect();

    if (video_device_current) {
        // open the video device to get some info e.g. frame size
        // close it afterwards to not block the device while it is not used
        if (utox_open_video_device(video_device[video_device_current])) {
            utox_close_video_device(video_device[video_device_current]);
        }
    }

    utox_video_thread_init = 1;

    while (1) {
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
                            if (i >= UTOX_MAX_CALLS){
                                debug("uToxVideo:\tTrying to send video frame to too many peers. Please report this bug!\n");
                                break;
                            }
                        }
                    }
                }
            } else if(r == -1) {
                debug("uToxVideo:\tErr... something really bad happened trying to get this frame, I'm just going to plots now!\n");
                video_endread();
                utox_close_video_device(video_device);
            }
        }

        yieldcpu(5);
    }

    video_thread_msg = 0;
    utox_video_thread_init = 0;
    debug("uToxVideo:\tClean thread exit!\n");
}
