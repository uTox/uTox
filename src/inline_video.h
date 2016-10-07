#ifndef INLINE_VIDEO_H
#define INLINE_VIDEO_H

typedef struct inline_vid {
    PANEL panel;

} INLINE_VID;

void inline_video_draw(INLINE_VID *UNUSED(p), int UNUSED(x), int UNUSED(y), int width, int height);

_Bool inline_video_mmove(INLINE_VID *UNUSED(p), int UNUSED(x), int UNUSED(y), int UNUSED(width), int UNUSED(height), int UNUSED(mx), int UNUSED(my), int UNUSED(dx), int UNUSED(dy));

_Bool inline_video_mdown(INLINE_VID *UNUSED(p));

_Bool inline_video_mright(INLINE_VID *UNUSED(p));

_Bool inline_video_mwheel(INLINE_VID *UNUSED(p), int UNUSED(height), double UNUSED(d), _Bool UNUSED(smooth));

_Bool inline_video_mup(INLINE_VID *UNUSED(p));

_Bool inline_video_mleave(INLINE_VID *UNUSED(p));

#endif
