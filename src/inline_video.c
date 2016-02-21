#include "main.h"

void inline_video_draw(INLINE_VID *UNUSED(p), int UNUSED(x), int UNUSED(y), int width, int height){
    return;
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

