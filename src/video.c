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

static volatile uint32_t video_active = 0;
static void *video_device;

void utox_video_change_device(void *device) {
    if (device == NULL) {
        video_active = 1;
        utox_video_record_stop(0);
        return;
    }

    if (video_active) {
        video_endread();
        closevideodevice(video_device);
    }

    video_device = device;
    openvideodevice(video_device);

    if (video_active) {
        video_startread();
    } else {
        /* Just grab the new frame size */
        closevideodevice(video_device);
    }

}

void utox_video_record_start(_Bool preview){
    if (video_active++) {
        debug("video already running\n");
        return;
    }
    if (preview) {
        video_preview = 1;
    }
    openvideodevice(video_device);
    video_startread();
    debug("started video\n");
}

void utox_video_record_stop(_Bool preview){
    if (!video_active) {
        debug("video already stopped!\n");
        return;
    }

    video_active--;
    if (video_active) {
        debug("video stream in progress not stopping\n");
        return;
    }

    video_active  = 0;

    if (preview) {
        video_preview = 0;
    }

    video_endread();
    closevideodevice(video_device);
    debug("stopped video\n");
}

void utox_video_thread(void *args) {
    ToxAV *av = args;

    // Add always-present null video input device.
    postmessage(VIDEO_IN_DEVICE_APPEND, STR_VIDEO_IN_NONE, 1, NULL);

    // select a video device (autodectect)
    video_device = video_detect();
    if (video_device) {
        // open the video device to get some info e.g. frame size
        // close it afterwards to not block the device while it is not used
        if (openvideodevice(video_device)) {
            closevideodevice(video_device);
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
                        // debug("Sent video frame to friend %u\n", i);
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
                debug("Err... something really bad happened trying to get this frame, I'm just going to plots now!\n");
                video_endread();
                closevideodevice(video_device);
            }
        }

        yieldcpu(5);
    }

    video_thread_msg = 0;
    utox_video_thread_init = 0;
    debug("UTOX VIDEO:\tClean thread exit!\n");
}
