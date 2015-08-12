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
    uint8_t  *y, *u, *v;
    uint16_t w, h;
} utox_av_video_frame;

utox_av_video_frame utox_video_frame;

_Bool openvideodevice(void *handle);
void closevideodevice(void *handle);
