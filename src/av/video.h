#ifndef VIDEO_H
#define VIDEO_H

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

#define UTOX_DEFAULT_BITRATE_V 5000
#define UTOX_MIN_BITRATE_VIDEO 512
#define UTOX_DEFAULT_VID_WIDTH 1280
#define UTOX_DEFAULT_VID_HEIGHT 720

/* Check self */
#define SELF_SEND_VIDEO(f_number) (!!(friend[f_number].call_state_self & TOXAV_FRIEND_CALL_STATE_SENDING_V))
#define SELF_ACCEPT_VIDEO(f_number) (!!(friend[f_number].call_state_self & TOXAV_FRIEND_CALL_STATE_ACCEPTING_V))
/* Check friend */
#define FRIEND_SENDING_VIDEO(f_number) (!!(friend[f_number].call_state_friend & TOXAV_FRIEND_CALL_STATE_SENDING_V))
#define FRIEND_ACCEPTING_VIDEO(f_number) (!!(friend[f_number].call_state_friend & TOXAV_FRIEND_CALL_STATE_ACCEPTING_V))

/* Check both */
#define SEND_VIDEO_FRAME(f_number)                                            \
    (!!(friend[f_number].call_state_self & TOXAV_FRIEND_CALL_STATE_SENDING_V) \
     && !!(friend[f_number].call_state_friend & TOXAV_FRIEND_CALL_STATE_ACCEPTING_V))

#define ACCEPT_VIDEO_FRAME(f_number)                                            \
    (!!(friend[f_number].call_state_self & TOXAV_FRIEND_CALL_STATE_ACCEPTING_V) \
     && !!(friend[f_number].call_state_friend & TOXAV_FRIEND_CALL_STATE_SENDING_V))


typedef struct UTOX_AV_VIDEO_FRAME {
    uint16_t w, h;
    uint8_t *y, *u, *v;
} utox_av_video_frame;

typedef struct utox_frame_pkg {
    uint16_t w, h;
    size_t   size;
    void *   img;
} UTOX_FRAME_PKG;

utox_av_video_frame utox_video_frame;

void utox_video_append_device(void *device, bool localized, void *name, bool default_);

bool utox_video_change_device(uint16_t i);

bool utox_video_start(bool preview);
bool utox_video_stop(bool preview);

void utox_video_thread(void *args);

void postmessage_video(uint8_t msg, uint32_t param1, uint32_t param2, void *data);


// Color format conversion functions

void yuv420tobgr(uint16_t width, uint16_t height, const uint8_t *y, const uint8_t *u, const uint8_t *v,
                 unsigned int ystride, unsigned int ustride, unsigned int vstride, uint8_t *out);
void yuv422to420(uint8_t *plane_y, uint8_t *plane_u, uint8_t *plane_v, uint8_t *input, uint16_t width, uint16_t height);
void bgrtoyuv420(uint8_t *plane_y, uint8_t *plane_u, uint8_t *plane_v, uint8_t *rgb, uint16_t width, uint16_t height);
void bgrxtoyuv420(uint8_t *plane_y, uint8_t *plane_u, uint8_t *plane_v, uint8_t *rgb, uint16_t width, uint16_t height);

// TODO: Documentation.
void scale_rgbx_image(uint8_t *old_rgbx, uint16_t old_width, uint16_t old_height, uint8_t *new_rgbx, uint16_t new_width,
                      uint16_t new_height);


#endif
