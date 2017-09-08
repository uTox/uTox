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
#include "settings.h"
#include "ui.h"

#include "av/video.h"

#include "native/image.h"

#include <stdlib.h>
#include <string.h>

static UTOX_FRAME_PKG *preview_frame = NULL;
static UTOX_FRAME_PKG current_frame = { 0, 0, 0, 0 };

bool inline_set_frame_self(UTOX_FRAME_PKG *frame) {
    if (preview_frame) {
        if (preview_frame->img) {
            free(preview_frame->img);
        }
        free(preview_frame);
        preview_frame = NULL;
    }

    if (frame) {
        preview_frame = frame;
        return true;
    }

    return false;
}


bool inline_set_frame_friend(uint16_t w, uint16_t h, size_t size, void *img) {
    current_frame.w    = w;
    current_frame.h    = h;
    current_frame.size = size;

    uint8_t *tmp = realloc(current_frame.img, size);
    if (!size || !tmp) {
        current_frame.w    = 0;
        current_frame.h    = 0;
        current_frame.size = 0;
        tmp ? free(tmp) : free(current_frame.img);

        return false;
    }

    current_frame.img = tmp;
    memcpy(current_frame.img, img, size);
    return true;
}

void inline_video_draw(INLINE_VID *UNUSED(p), int x, int y, int width, int height) {
    if (!settings.inline_video) {
        return;
    }

    LOG_TRACE("Inline Video", "Drawing new frame." );

    if (current_frame.img && current_frame.size) {
        draw_inline_image(current_frame.img, current_frame.size,
                          MIN(current_frame.w, width), MIN(current_frame.h, height),
                          x, y + SCALE(MAIN_TOP_FRAME_THICK + 2));

        // TODO advanced maths to determine best plase to place P-by-P
        x += MIN(current_frame.w, width) + SCALE(2);
        // y += MIN(current_frame.h, height);
    }

    if (preview_frame && preview_frame->size && preview_frame->img) {

        draw_inline_image(preview_frame->img, preview_frame->size,
                          MIN(preview_frame->w, width), MIN(preview_frame->h, height),
                          x, y + SCALE(MAIN_TOP_FRAME_THICK + 2));
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
