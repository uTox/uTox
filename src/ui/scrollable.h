#ifndef SCROLLABLE_H
#define SCROLLABLE_H

#include "../ui.h"

struct scrollable {
    PANEL panel;

    uint32_t color;
    int      x;
    bool     small;

    double d;
    bool   left, mousedown, mouseover, mouseover2;
    int    content_height;
};

void scroll_draw(SCROLLABLE *s, int x, int y, int width, int height);
int scroll_gety(SCROLLABLE *s, int height);

bool scroll_mmove(SCROLLABLE *s, int x, int y, int width, int height, int mx, int my, int dx, int dy);
bool scroll_mdown(SCROLLABLE *s);
bool scroll_mright(SCROLLABLE *s);
bool scroll_mwheel(SCROLLABLE *s, int height, double delta, bool smooth);
bool scroll_mup(SCROLLABLE *s);
bool scroll_mleave(SCROLLABLE *s);

extern SCROLLABLE scrollbar_flist, scrollbar_friend, scrollbar_group;

#endif
