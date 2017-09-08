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
#include "native/thread.h"

#include <stdlib.h>
#include <string.h>

static UTOX_FRAME_PKG *preview_frame = NULL;
static UTOX_FRAME_PKG *current_frame = NULL;

/* Xlib is VERY threaded, so we need to make our own copy of the data we're going to draw
 * I know memcpy isn't ideal here, but perfect is the enemy of done. I'd love to see a better
 * implementation here! Please try to better me on this!
 */
static void clone_frame(UTOX_FRAME_PKG *dst, UTOX_FRAME_PKG *src) {
    if (!src->size) {
        return;
    }

    if (dst->size != src->size) {
        void *img = realloc(dst->img, src->size);
        if (!img) {
            LOG_FATAL_ERR(EXIT_MALLOC ,"Inline Image", "Unable to malloc for this image");
        }

        dst->img  = img;
    }

    dst->w    = src->w;
    dst->h    = src->h;
    dst->size = src->size;
    memcpy(dst->img, src->img, dst->size);
}


bool inline_set_frame_self(UTOX_FRAME_PKG *frame) {
    if (!settings.inline_video) {
        return false;
    }

    if (!preview_frame) {
        preview_frame = calloc(1, sizeof *preview_frame);
        if (!preview_frame) {
            LOG_FATAL_ERR(EXIT_MALLOC,"Inline Image", "Unable to calloc for our preview");
        }
    }

    if (frame) {
        clone_frame(preview_frame, frame);
        return true;
    } else {
        free(preview_frame->img);
        preview_frame->img = NULL;
        // TODO free(preview_frame); // We can't free this without friend ownership else DOS exploit
    }

    return false;
}


bool inline_set_frame_friend(UTOX_FRAME_PKG *frame) {
    if (!current_frame) {
        current_frame = calloc(1, sizeof *current_frame);
        if (!current_frame) {
            LOG_FATAL_ERR(EXIT_MALLOC,"Inline Image", "Unable to calloc for our preview");
        }
    }

    if (frame) {
        clone_frame(current_frame, frame);
        return true;
    } else {
        free(current_frame->img);
        current_frame->img = NULL;
        // TODO free(current_frame); // We can't free this without friend ownership else DOS exploit
    }

    return false;
}

void inline_video_draw(INLINE_VID *UNUSED(p), int x, int y, int width, int height) {
    LOG_TRACE("Inline Video", "Drawing new frame." );
    if (!settings.inline_video) {
        return;
    }

    if (current_frame && current_frame->img && current_frame->size) {
        draw_inline_image(current_frame->img, current_frame->size,
                          MIN(current_frame->w, width), MIN(current_frame->h, height),
                          x, y + SCALE(MAIN_TOP_FRAME_THICK + 2));

        // TODO advanced maths to determine best plase to place P-by-P
        x += MIN(current_frame->w, width) + SCALE(2);
        // y += MIN(current_frame->h, height);
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
