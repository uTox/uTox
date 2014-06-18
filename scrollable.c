#include "main.h"

void scroll_draw(SCROLLABLE *s, int x, int y, int width, int height)
{
    uint32_t c = s->content_height;
    uint32_t h = height, m, dy;

    if(h >= c) {
        m = h;
        dy = 0;
    }
    else {
        m = (h * h) / c;
        double d = (h - m);
        dy = (s->d * d) + 0.5;
        if(m <= 10) {
            m = 10;
        }
    }

    y += dy;
    x += s->x;

    if(!s->left) {
        x += width - SCROLL_WIDTH;
    }

    drawalpha(BM_SCROLLHALFTOP, x, y, SCROLL_WIDTH, SCROLL_WIDTH / 2, s->color);

    y += SCROLL_WIDTH / 2;
    int y2 = y + m - SCROLL_WIDTH, x2 = x + SCROLL_WIDTH;
    //drawrect(x, y, x + 1, y2, GRAY(46)); //GRAY(37)
    drawrect(x + 0, y, x2 - 0, y2, s->color);
    //drawrect(x2 - 1, y, x2, y2, GRAY(46)); alpha lines?

    drawalpha(BM_SCROLLHALFBOT, x, y2, SCROLL_WIDTH, SCROLL_WIDTH / 2, s->color);
}

int scroll_gety(SCROLLABLE *s, int height)
{
    uint32_t c = s->content_height;

    if(c > height)
    {
        return (s->d * (double)(c - height)) + 0.5;
    }

    return 0;
}

_Bool scroll_mmove(SCROLLABLE *s, int x, int y, int dy, int width, int height)
{
    _Bool draw = 0;

    _Bool hit = inrect(x, y, s->left ? SCROLL_WIDTH : (width - SCROLL_WIDTH), 0, width, height);
    if(s->mouseover != hit)
    {
        s->mouseover = hit;
        draw = 1;
    }

    s->mouseover2 = inrect(x, y, 0, 0, width, height);

    if(s->mousedown)
    {
        uint32_t c = s->content_height;
        uint32_t h = height;

        if(c > h)
        {
            uint32_t m = (h * h) / c;
            double d = (h - m);

            s->d = ((s->d * d) + (double)dy) / d;

            if(s->d < 0.0)
            {
                s->d = 0.0;
            }
            else if(s->d >= 1.0)
            {
                s->d = 1.0;
            }

            draw = 1;
        }
    }

    return draw;
}

_Bool scroll_mdown(SCROLLABLE *s)
{
    if(s->mouseover)
    {
        s->mousedown = 1;
        return 1;
    }

    return 0;
}

_Bool scroll_mright(SCROLLABLE *s)
{
    return 0;
}

_Bool scroll_mwheel(SCROLLABLE *s, int height, double d)
{
    if(s->mouseover2)
    {
        uint32_t c = s->content_height;
        uint32_t h = height;

        if(c > h)
        {
            uint32_t m = (h * h) / c;
            double dd = (h - m);

            s->d -= 16.0 * d / dd;;

            if(s->d < 0.0)
            {
                s->d = 0.0;
            }
            else if(s->d >= 1.0)
            {
                s->d = 1.0;
            }

            return 1;
        }
    }

    return 0;
}

_Bool scroll_mup(SCROLLABLE *s)
{
    if(s->mousedown)
    {
        s->mousedown = 0;
        return 1;
    }

    return 0;
}

_Bool scroll_mleave(SCROLLABLE *s)
{
    if(s->mouseover)
    {
        s->mouseover = 0;
        return 1;
    }

    s->mouseover2 = 0;

    return 0;
}
