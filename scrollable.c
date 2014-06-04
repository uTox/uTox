#include "main.h"

void scroll_draw(SCROLLABLE *s, int x, int y, int width, int height)
{
    uint32_t c = s->content_height;
    uint32_t h = height;

    if(h >= c)
    {
        return;
    }

    RECT r = {x + width - SCROLL_WIDTH, y, x + width, y + height};

    fillrect(&r, s->mouseover ? GRAY : WHITE);

    uint32_t m = (h * h) / c;
    double d = (h - m);
    uint32_t dy = (s->d * d) + 0.5;

    r.top += dy;
    r.bottom = r.top + m;


    fillrect(&r, s->mouseover ? GRAY3 : GRAY2);
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

    _Bool hit = inrect(x, y, width - SCROLL_WIDTH, 0, width, height);
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
