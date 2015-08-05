#define UTOX_DEFAULT_VIDEO_BITRATE 3000
#define UTOX_DEFAULT_VIDEO_WIDTH   1280
#define UTOX_DEFAULT_VIDEO_HEIGHT  720

typedef struct UTOX_AV_VIDEO_FRAME {
    uint8_t  *y, *u, *v;
    uint16_t w, h;
} utox_av_video_frame;
