#define UTOX_DEFAULT_VIDEO_BITRATE 3000
#define UTOX_DEFAULT_VIDEO_WIDTH   1280
#define UTOX_DEFAULT_VIDEO_HEIGHT  720

#define UTOX_SENDING_VIDEO(f_number)   ( !!(friend[f_number].call_state_self   & TOXAV_FRIEND_CALL_STATE_SENDING_V   ))
#define UTOX_ACCEPTING_VIDEO(f_number) ( !!(friend[f_number].call_state_friend & TOXAV_FRIEND_CALL_STATE_ACCEPTING_V ))

#define UTOX_SEND_VIDEO(f_number)   ( !!(friend[f_number].call_state_self   & TOXAV_FRIEND_CALL_STATE_SENDING_V  ) && \
                                      !!(friend[f_number].call_state_friend & TOXAV_FRIEND_CALL_STATE_ACCEPTING_V) )

#define UTOX_ACCEPT_VIDEO(f_number) ( !!(friend[f_number].call_state_self   & TOXAV_FRIEND_CALL_STATE_ACCEPTING_V ) && \
                                      !!(friend[f_number].call_state_friend & TOXAV_FRIEND_CALL_STATE_SENDING_V)  )


typedef struct UTOX_AV_VIDEO_FRAME {
    uint16_t w, h;
    uint8_t  *y, *u, *v;
} utox_av_video_frame;

typedef struct UTOX_FRAME_PKG {
    uint16_t w, h;
    void *img;
} utox_frame_pkg;

utox_av_video_frame utox_video_frame;

void toxvideo_postmessage(uint8_t msg, uint32_t param1, uint32_t param2, void *data);

void video_thread(void *args);
