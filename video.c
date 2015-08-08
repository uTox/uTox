#include "main.h"

static vpx_image_t input;

_Bool openvideodevice(void *handle) {
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

void closevideodevice(void *handle) {
    video_close(handle);
    vpx_img_free(&input);
}
