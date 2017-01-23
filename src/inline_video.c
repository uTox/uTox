/** inline_video.c
 *
 * TODO : a lot...
 *
 * We need to resive the video frame, and maybe center...
 *
 * Consider drawing messages under the video?
 * Consider do we want to draw buttons on screen?
 * Consider double click to pop out
 *
 * Consider auto selecting inline video
 */

#include "inline_video.h"

#include "debug.h"
#include "macros.h"
#include "main_native.h"
#include "settings.h"

#include "av/video.h"

#include <stdlib.h>
#include <string.h>

static UTOX_FRAME_PKG current_frame = { 0, 0, 0, 0 };

bool inline_set_frame(uint16_t w, uint16_t h, size_t size, void *img) {
    current_frame.w    = w;
    current_frame.h    = h;
    current_frame.size = size;

    current_frame.img = realloc(current_frame.img, size);
    if (size && current_frame.img) {
        memcpy(current_frame.img, img, size);
        return true;
    }

    current_frame.w    = 0;
    current_frame.h    = 0;
    current_frame.size = 0;
    free(current_frame.img);
    return false;
}

void inline_video_draw(INLINE_VID *UNUSED(p), int x, int y, int width, int height) {
    if (!settings.inline_video) {
        return;
    }

    debug("Inline Video:\tDrawing new frame.\n");

    if (current_frame.img && current_frame.size) {
        draw_inline_image(current_frame.img, current_frame.size,
                          MIN(current_frame.w, width), MIN(current_frame.h, height),
                          x, y + MAIN_TOP_FRAME_THICK);
    }
}

bool inline_video_mmove(INLINE_VID *UNUSED(p), int UNUSED(x), int UNUSED(y), int UNUSED(width), int UNUSED(height),
                        int UNUSED(mx), int UNUSED(my), int UNUSED(dx), int UNUSED(dy)) {
    return 0;
}

bool inline_video_mdown(INLINE_VID *UNUSED(p)) {
    return 0;
}

bool inline_video_mright(INLINE_VID *UNUSED(p)) {
    return 0;
}

bool inline_video_mwheel(INLINE_VID *UNUSED(p), int UNUSED(height), double UNUSED(d), bool UNUSED(smooth)) {
    return 0;
}

bool inline_video_mup(INLINE_VID *UNUSED(p)) {
    return 0;
}

bool inline_video_mleave(INLINE_VID *UNUSED(p)) {
    return 0;
}
