#include "main.h"

/** TODO : a lot...
 *
 * We need to resive the video frame, and maybe center...
 *
 * Consider drawing messages under the video?
 * Consider do we want to draw buttons on screen?
 * Consider double click to pop out
 *
 * Consider auto selecting inline video
 */

void inline_video_draw(INLINE_VID *UNUSED(p), int x, int y, int width, int height){
    if (!settings.inline_video) {
        return;
    }

    debug("Inline Video:\tDrawing new frame.\n");

    if (current_frame && current_frame->img && current_frame->size) {
        draw_inline_image(current_frame->img, current_frame->size,
                          current_frame->w, current_frame->h,
                          x, y + MAIN_TOP_FRAME_THICK);
    }
}

_Bool inline_video_mmove(INLINE_VID *UNUSED(p), int UNUSED(x), int UNUSED(y), int UNUSED(width), int UNUSED(height), int UNUSED(mx), int UNUSED(my), int UNUSED(dx), int UNUSED(dy)) {
    return 0;
}

_Bool inline_video_mdown(INLINE_VID *UNUSED(p)) {
    return 0;
}

_Bool inline_video_mright(INLINE_VID *UNUSED(p)) {
    return 0;
}

_Bool inline_video_mwheel(INLINE_VID *UNUSED(p), int UNUSED(height), double UNUSED(d), _Bool UNUSED(smooth)) {
    return 0;
}

_Bool inline_video_mup(INLINE_VID *UNUSED(p)) {
    return 0;
}

_Bool inline_video_mleave(INLINE_VID *UNUSED(p)) {
    return 0;
}

