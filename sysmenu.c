#include "main.h"

enum {
    SYSMENU_NONE,
    SYSMENU_MINI,
    SYSMENU_SIZE,
    SYSMENU_EXIT,
};

static void sysmenu_button(RECT *area, int bm, uint32_t bg_hover, uint32_t bg_down, uint32_t fg_hover, uint32_t fg_down, _Bool active, _Bool down)
{
    if(active) {
        if(down) {
            fillrect(area, bg_down);
            setcolor(bg_down);
            setbkcolor(fg_down);
        } else {
            fillrect(area, bg_hover);
            setcolor(bg_hover);
            setbkcolor(fg_hover);
        }
    } else {
        fillrect(area, COLOR_BG);
        setcolor(COLOR_BG);
        setbkcolor(COLOR_SYSMENU);
    }

    drawbitmap(bm, area->left + 7, area->top + 8, 16, 10);
}

void sysmenu_draw(SYSMENU *s, int x, int y, int width, int height)
{
    RECT r = {x + width - 30, y, x + width, y + 26};


    sysmenu_button(&r, BM_EXIT, RED, RED2, WHITE, WHITE, s->active == SYSMENU_EXIT, s->mdown);
    r.left -= 30;
    r.right -= 30;

    sysmenu_button(&r, maximized ? BM_RESTORE : BM_MAXIMIZE, GRAY, BLUE, 0x333333, WHITE, s->active == SYSMENU_SIZE, s->mdown);
    r.left -= 30;
    r.right -= 30;

    sysmenu_button(&r, BM_MINIMIZE, GRAY, BLUE, 0x333333, WHITE, s->active == SYSMENU_MINI, s->mdown);

    //commitdraw(width - BORDER - 90, BORDER, width - BORDER, BORDER + 26);
}

_Bool sysmenu_mmove(SYSMENU *s, int x, int y, int dy, int width, int height)
{
    uint8_t sm = 0;
    if(inrect(x, y, 0, 0, width, height))
    {
        sm = 1 + x / 30;
    }
    s->mover = sm;

    if(!mdown) {
        if(sm != s->active) {
            s->active = sm;
            return 1;
        }
    } else {
        _Bool md = (sm == s->active);
        if(md != s->mdown) {
            s->mdown = md;
            return 1;
        }
    }

    return 0;
}

_Bool sysmenu_mdown(SYSMENU *s)
{
    if(s->active) {
        if(!s->mdown) {
            s->mdown = 1;
            return 1;
        }
    }

    return 0;
}

_Bool sysmenu_mright(SYSMENU *s)
{
    return 0;
}

_Bool sysmenu_mwheel(SYSMENU *s, int height, double d)
{
    return 0;
}

_Bool sysmenu_mup(SYSMENU *s)
{
    if(s->active == s->mover) {
        switch(s->active) {
        case SYSMENU_EXIT: {
            sysmexit();
            break;
        }

        case SYSMENU_SIZE: {
            sysmsize();
            break;
        }

        case SYSMENU_MINI: {
            sysmmini();
            break;
        }
        }
    }

    s->mdown = 0;

    if(s->active || s->mover) {
        s->active = s->mover;
        return 1;
    }

    return 0;

}

_Bool sysmenu_mleave(SYSMENU *s)
{
    if(s->active) {
        s->active = 0;
        return 1;
    }

    return 0;
}
