#define UTOX_DEFAULT_BITRATE_V  5000
#define UTOX_MIN_BITRATE_VIDEO  512
#define UTOX_DEFAULT_VID_WIDTH  1280
#define UTOX_DEFAULT_VID_HEIGHT 720

/* Check self */
#define SELF_SEND_VIDEO(f_number)        (!!(friend[f_number].call_state_self   & TOXAV_FRIEND_CALL_STATE_SENDING_V  ))
#define SELF_ACCEPT_VIDEO(f_number)      (!!(friend[f_number].call_state_self   & TOXAV_FRIEND_CALL_STATE_ACCEPTING_V))
/* Check friend */
#define FRIEND_SENDING_VIDEO(f_number)   (!!(friend[f_number].call_state_friend & TOXAV_FRIEND_CALL_STATE_SENDING_V  ))
#define FRIEND_ACCEPTING_VIDEO(f_number) (!!(friend[f_number].call_state_friend & TOXAV_FRIEND_CALL_STATE_ACCEPTING_V))

/* Check both */
#define SEND_VIDEO_FRAME(f_number)   (!!(friend[f_number].call_state_self   & TOXAV_FRIEND_CALL_STATE_SENDING_V  ) && \
                                      !!(friend[f_number].call_state_friend & TOXAV_FRIEND_CALL_STATE_ACCEPTING_V)    )

#define ACCEPT_VIDEO_FRAME(f_number) (!!(friend[f_number].call_state_self   & TOXAV_FRIEND_CALL_STATE_ACCEPTING_V) && \
                                      !!(friend[f_number].call_state_friend & TOXAV_FRIEND_CALL_STATE_SENDING_V)      )


typedef struct UTOX_AV_VIDEO_FRAME {
    uint16_t w, h;
    uint8_t  *y, *u, *v;
} utox_av_video_frame;

typedef struct UTOX_FRAME_PKG {
    uint16_t w, h;
    void *img;
} utox_frame_pkg;

utox_av_video_frame utox_video_frame;

void utox_video_change_device(void *device);
void utox_video_record_start(void);
void utox_video_record_stop(void);

void utox_video_thread(void *args);

