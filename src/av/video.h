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

#endif
