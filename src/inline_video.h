#ifndef INLINE_VIDEO_H
#define INLINE_VIDEO_H

#include "ui.h"

#include <inttypes.h>
#include <stdbool.h>

typedef struct inline_vid { PANEL panel; } INLINE_VID;

bool inline_set_frame(uint16_t w, uint16_t h, size_t size, void *img);

void inline_video_draw(INLINE_VID *p, int x, int y, int width, int height);

bool inline_video_mmove(INLINE_VID *p, int x, int y, int width, int height, int mx, int my, int dx, int dy);

bool inline_video_mdown(INLINE_VID *p);

bool inline_video_mright(INLINE_VID *p);

bool inline_video_mwheel(INLINE_VID *p, int height, double d, bool smooth);

bool inline_video_mup(INLINE_VID *p);

bool inline_video_mleave(INLINE_VID *p);

#endif
