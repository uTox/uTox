#include "video.h"

#include "utox_av.h"

#include "../friend.h"
#include "../debug.h"
#include "../macros.h"
#include "../self.h"
#include "../settings.h"
#include "../tox.h"
#include "../utox.h"

#include "../native/thread.h"
#include "../native/video.h"

#include <tox/toxav.h>

#include <pthread.h>
#include <vpx/vpx_codec.h>
#include <vpx/vpx_image.h>

#include "../main.h" // video/screen super globals

static void *   video_device[16]     = { NULL }; /* TODO; magic number */
static int16_t  video_device_count   = 0;
static uint32_t video_device_current = 0;
static bool     video_active         = false;

static utox_av_video_frame utox_video_frame;

static bool video_device_status = true;

static vpx_image_t input;

static pthread_mutex_t video_thread_lock;


static bool video_device_init(void *handle) {
    // initialize video (will populate video_width and video_height)
    if (handle == (void *)1) {
        if (!native_video_init((void *)1)) {
            LOG_TRACE("native_video_init() failed for desktop" );
            return false;
        }
    } else {
        if (!handle || !native_video_init(*(void **)handle)) {
            LOG_TRACE("native_video_init() failed webcam" );
            return false;
        }
    }
    vpx_img_alloc(&input, VPX_IMG_FMT_I420, video_width, video_height, 1);
    utox_video_frame.y = input.planes[0];
    utox_video_frame.u = input.planes[1];
    utox_video_frame.v = input.planes[2];
    utox_video_frame.w = input.d_w;
    utox_video_frame.h = input.d_h;

    LOG_NOTE("video init done!" );
    video_device_status = true;

    return true;
}

static void close_video_device(void *handle) {
    if (handle >= (void *)2) {
        native_video_close(*(void **)handle);
        vpx_img_free(&input);
    }
    video_device_status = false;
}

static bool video_device_start(void) {
    if (video_device_status) {
        native_video_startread();
        video_active = true;
        return true;
    }
    video_active = false;
    return false;
}

static bool video_device_stop(void) {
    if (video_device_status) {
        native_video_endread();
        video_active = false;
        return true;
    }
    video_active = false;
    return false;
}

#include "../ui/dropdown.h"
#include "../layout/settings.h" // TODO move?
void utox_video_append_device(void *device, bool localized, void *name, bool default_) {
    video_device[video_device_count++] = device;

    if (localized) {
        // Device name is localized with name containing UTOX_I18N_STR.
        // device is device handle pointer.
        dropdown_list_add_localized(&dropdown_video, (UTOX_I18N_STR)name, device);
    } else {
        // Device name is a hardcoded string.
        // device is a pointer to a buffer, that contains device handle pointer,
        // followed by device name string.
        dropdown_list_add_hardcoded(&dropdown_video, name, *(void **)device);
    }

    /* TODO remove all default settings */
    // default == true, if this device will be chosen by video detecting code.
    if (default_) {
        dropdown_video.selected = dropdown_video.over = (dropdown_video.dropcount - 1);
    }
}

bool utox_video_change_device(uint16_t device_number) {
    pthread_mutex_lock(&video_thread_lock);

    static bool _was_active = false;

    if (!device_number) {
        video_device_current = 0;
        if (video_active) {
            video_device_stop();
            close_video_device(video_device[video_device_current]);
            if (settings.video_preview) {
                settings.video_preview = false;
                postmessage_utox(AV_CLOSE_WINDOW, 0, 0, NULL);
            }
        }
        LOG_TRACE("Disabled Video device (none)" );
        pthread_mutex_unlock(&video_thread_lock);
        return false;
    }

    if (video_active) {
        _was_active = true;
        video_device_stop();
        close_video_device(video_device[video_device_current]);
    } else {
        _was_active = false;
    }

    video_device_current = device_number;

    video_device_init(video_device[device_number]);

    if (_was_active) {
        LOG_TRACE("Trying to restart video with new device..." );
        if (!video_device_start()) {
            LOG_ERR("Error, unable to start new device...");
            if (settings.video_preview) {
                settings.video_preview = false;
                postmessage_utox(AV_CLOSE_WINDOW, 0, 0, NULL);
            }

            pthread_mutex_unlock(&video_thread_lock);
            return false;
        }
        pthread_mutex_unlock(&video_thread_lock);
        return true;
    } else {
        /* Just grab the new frame size */
        close_video_device(video_device[video_device_current]);
    }
    pthread_mutex_unlock(&video_thread_lock);
    return false;
}

bool utox_video_start(bool preview) {
    if (video_active) {
        LOG_NOTE("video already running" );
        return true;
    }

    if (!video_device_current) {
        LOG_NOTE("Not starting device None" );
        return false;
    }

    if (preview) {
        settings.video_preview = true;
    }

    if (video_device_init(video_device[video_device_current]) && video_device_start()) {
        video_active = true;
        LOG_NOTE("started video" );
        return true;
    }

    LOG_ERR("Unable to start video.");
    return false;
}

bool utox_video_stop(bool UNUSED(preview)) {
    if (!video_active) {
        LOG_TRACE("video already stopped!" );
        return false;
    }

    video_active           = false;
    settings.video_preview = false;
    postmessage_utox(AV_CLOSE_WINDOW, 0, 0, NULL);

    video_device_stop();
    close_video_device(video_device[video_device_current]);
    LOG_TRACE("stopped video" );
    return true;
}

void postmessage_video(uint8_t msg, uint32_t param1, uint32_t param2, void *data) {
    while (video_thread_msg) {
        yieldcpu(1);
    }

    video_msg.msg    = msg;
    video_msg.param1 = param1;
    video_msg.param2 = param2;
    video_msg.data   = data;

    video_thread_msg = 1;
}

void utox_video_thread(void *args) {
    ToxAV *av = args;

    pthread_mutex_init(&video_thread_lock, NULL);

    // Add always-present null video input device.
    utox_video_append_device(NULL, 1, (void *)STR_VIDEO_IN_NONE, 1);

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
            const int r = native_video_getframe(utox_video_frame.y, utox_video_frame.u, utox_video_frame.v,
                                                utox_video_frame.w, utox_video_frame.h);
            if (r == 1) {
                if (settings.video_preview) {
                    /* Make a copy of the video frame for uTox to display */
                    UTOX_FRAME_PKG *frame = malloc(sizeof(UTOX_FRAME_PKG));
                    frame->w              = utox_video_frame.w;
                    frame->h              = utox_video_frame.h;
                    frame->img            = malloc(utox_video_frame.w * utox_video_frame.h * 4);

                    yuv420tobgr(utox_video_frame.w, utox_video_frame.h, utox_video_frame.y, utox_video_frame.u,
                                utox_video_frame.v, utox_video_frame.w, (utox_video_frame.w / 2),
                                (utox_video_frame.w / 2), frame->img);

                    postmessage_utox(AV_VIDEO_FRAME, UINT16_MAX, 1, (void *)frame);
                }

                size_t active_video_count = 0;
                for (size_t i = 0; i < self.friend_list_count; i++) {
                    if (SEND_VIDEO_FRAME(i)) {
                        LOG_TRACE("sending video frame to friend %lu" , i);
                        active_video_count++;
                        TOXAV_ERR_SEND_FRAME error = 0;
                        toxav_video_send_frame(av, get_friend(i)->number, utox_video_frame.w, utox_video_frame.h,
                                               utox_video_frame.y, utox_video_frame.u, utox_video_frame.v, &error);
                        // LOG_TRACE("Sent video frame to friend %u" , i);
                        if (error) {
                            if (error == TOXAV_ERR_SEND_FRAME_SYNC) {
                                LOG_ERR("Vid Frame sync error: w=%u h=%u", utox_video_frame.w, utox_video_frame.h);
                            } else if (error == TOXAV_ERR_SEND_FRAME_PAYLOAD_TYPE_DISABLED) {
                                LOG_ERR("uToxVideo",
                                    "ToxAV disagrees with our AV state for friend %lu, self %u, friend %u",
                                        i, get_friend(i)->call_state_self, get_friend(i)->call_state_friend);
                            } else {
                                LOG_ERR("toxav_send_video error friend: %i error: %u",
                                        get_friend(i)->number, error);
                            }
                        } else {
                            if (active_video_count >= UTOX_MAX_CALLS) {
                                LOG_ERR("Trying to send video frame to too many peers. Please report this bug!");
                                break;
                            }
                        }
                    }
                }
            } else if (r == -1) {
                LOG_ERR("Err... something really bad happened trying to get this frame, I'm just going "
                            "to plots now!");
                video_device_stop();
                close_video_device(video_device);
            }

            pthread_mutex_unlock(&video_thread_lock);
            yieldcpu(40); /* 60fps = 16.666ms || 25 fps = 40ms || the data quality is SO much better at 25... */
            continue;     /* We're running video, so don't sleep for an extra 100 ms */
        }

        yieldcpu(100);
    }

    video_device_count   = 0;
    video_device_current = 0;
    video_active         = false;

    for (uint8_t i = 0; i < 16; ++i) {
        video_device[i] = NULL;
    }

    video_thread_msg       = 0;
    utox_video_thread_init = 0;
    LOG_TRACE("Clean thread exit!");
}

void yuv420tobgr(uint16_t width, uint16_t height, const uint8_t *y, const uint8_t *u, const uint8_t *v,
                 unsigned int ystride, unsigned int ustride, unsigned int vstride, uint8_t *out) {
    for (unsigned long int i = 0; i < height; ++i) {
        for (unsigned long int j = 0; j < width; ++j) {
            uint8_t *point = out + 4 * ((i * width) + j);
            int       t_y   = y[((i * ystride) + j)];
            const int t_u   = u[(((i / 2) * ustride) + (j / 2))];
            const int t_v   = v[(((i / 2) * vstride) + (j / 2))];
            t_y            = t_y < 16 ? 16 : t_y;

            const int r = (298 * (t_y - 16) + 409 * (t_v - 128) + 128) >> 8;
            const int g = (298 * (t_y - 16) - 100 * (t_u - 128) - 208 * (t_v - 128) + 128) >> 8;
            const int b = (298 * (t_y - 16) + 516 * (t_u - 128) + 128) >> 8;

            point[2] = r > 255 ? 255 : r < 0 ? 0 : r;
            point[1] = g > 255 ? 255 : g < 0 ? 0 : g;
            point[0] = b > 255 ? 255 : b < 0 ? 0 : b;
            point[3] = ~0;
        }
    }
}

void yuv422to420(uint8_t *plane_y, uint8_t *plane_u, uint8_t *plane_v, uint8_t *input, uint16_t width, uint16_t height) {
    const uint8_t *end = input + width * height * 2;
    while (input != end) {
        uint8_t *line_end = input + width * 2;
        while (input != line_end) {
            *plane_y++ = *input++;
            *plane_v++ = *input++;
            *plane_y++ = *input++;
            *plane_u++ = *input++;
        }

        line_end = input + width * 2;
        while (input != line_end) {
            *plane_y++ = *input++;
            input++; // u
            *plane_y++ = *input++;
            input++; // v
        }
    }
}

static uint8_t rgb_to_y(int r, int g, int b) {
    const int y = ((9798 * r + 19235 * g + 3736 * b) >> 15);
    return y > 255 ? 255 : y < 0 ? 0 : y;
}

static uint8_t rgb_to_u(int r, int g, int b) {
    const int u = ((-5538 * r + -10846 * g + 16351 * b) >> 15) + 128;
    return u > 255 ? 255 : u < 0 ? 0 : u;
}

static uint8_t rgb_to_v(int r, int g, int b) {
    const int v = ((16351 * r + -13697 * g + -2664 * b) >> 15) + 128;
    return v > 255 ? 255 : v < 0 ? 0 : v;
}

void bgrtoyuv420(uint8_t *plane_y, uint8_t *plane_u, uint8_t *plane_v, uint8_t *rgb, uint16_t width, uint16_t height) {
    uint8_t *p;
    uint8_t  r, g, b;

    for (uint16_t y = 0; y != height; y += 2) {
        p = rgb;
        for (uint16_t x = 0; x != width; x++) {
            b          = *rgb++;
            g          = *rgb++;
            r          = *rgb++;
            *plane_y++ = rgb_to_y(r, g, b);
        }

        for (uint16_t x = 0; x != width / 2; x++) {
            b          = *rgb++;
            g          = *rgb++;
            r          = *rgb++;
            *plane_y++ = rgb_to_y(r, g, b);

            b          = *rgb++;
            g          = *rgb++;
            r          = *rgb++;
            *plane_y++ = rgb_to_y(r, g, b);

            b = ((int)b + (int)*(rgb - 6) + (int)*p + (int)*(p + 3) + 2) / 4;
            p++;
            g = ((int)g + (int)*(rgb - 5) + (int)*p + (int)*(p + 3) + 2) / 4;
            p++;
            r = ((int)r + (int)*(rgb - 4) + (int)*p + (int)*(p + 3) + 2) / 4;
            p++;

            *plane_u++ = rgb_to_u(r, g, b);
            *plane_v++ = rgb_to_v(r, g, b);

            p += 3;
        }
    }
}

void bgrxtoyuv420(uint8_t *plane_y, uint8_t *plane_u, uint8_t *plane_v, uint8_t *rgb, uint16_t width, uint16_t height) {
    uint8_t *p;
    uint8_t  r, g, b;

    for (uint16_t y = 0; y != height; y += 2) {
        p = rgb;
        for (uint16_t x = 0; x != width; x++) {
            b = *rgb++;
            g = *rgb++;
            r = *rgb++;
            rgb++;

            *plane_y++ = rgb_to_y(r, g, b);
        }

        for (uint16_t x = 0; x != width / 2; x++) {
            b = *rgb++;
            g = *rgb++;
            r = *rgb++;
            rgb++;

            *plane_y++ = rgb_to_y(r, g, b);

            b = *rgb++;
            g = *rgb++;
            r = *rgb++;
            rgb++;

            *plane_y++ = rgb_to_y(r, g, b);

            b = ((int)b + (int)*(rgb - 8) + (int)*p + (int)*(p + 4) + 2) / 4;
            p++;
            g = ((int)g + (int)*(rgb - 7) + (int)*p + (int)*(p + 4) + 2) / 4;
            p++;
            r = ((int)r + (int)*(rgb - 6) + (int)*p + (int)*(p + 4) + 2) / 4;
            p++;
            p++;

            *plane_u++ = rgb_to_u(r, g, b);
            *plane_v++ = rgb_to_v(r, g, b);

            p += 4;
        }
    }
}

void scale_rgbx_image(uint8_t *old_rgbx, uint16_t old_width, uint16_t old_height, uint8_t *new_rgbx, uint16_t new_width,
                      uint16_t new_height) {
    for (int y = 0; y != new_height; y++) {
        const int y0 = y * old_height / new_height;
        for (int x = 0; x != new_width; x++) {
            const int x0 = x * old_width / new_width;

            const int a         = x + y * new_width;
            const int b         = x0 + y0 * old_width;
            new_rgbx[a * 4]     = old_rgbx[b * 4];
            new_rgbx[a * 4 + 1] = old_rgbx[b * 4 + 1];
            new_rgbx[a * 4 + 2] = old_rgbx[b * 4 + 2];
        }
    }
}
