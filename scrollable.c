#include "main.h"

void scroll_draw(SCROLLABLE *s, int x, int y, int width, int height)
{
    uint32_t c = s->content_height;
    uint32_t h = height, m, dy;
    int scroll_width = 0;
    if (s->small) {
        scroll_width = SCROLL_WIDTH / 2;
    } else {
        scroll_width = SCROLL_WIDTH;
    }

    if(h >= c) {
        // If h(eight) > c(ontent height), don't draw anything.
        return;
    } else {
        m = (h * h) / c;
        double d = (h - m);
        dy = (s->d * d) + 0.5;
    }

    y += dy;
    x += s->x;


    if(!s->left) {
        x += width - scroll_width;
    }

    drawalpha(s->small ? BM_SCROLLHALFTOP_SMALL : BM_SCROLLHALFTOP, x, y, scroll_width, scroll_width /2, s->color);

    y += scroll_width / 2;
    int y2 = y + m - scroll_width;
    if(scroll_width > m) {
        y2 = y;
    }
    drawrect(x, y, x + scroll_width, y2, s->color);

    drawalpha(s->small ? BM_SCROLLHALFBOT_SMALL : BM_SCROLLHALFBOT, x, y2, scroll_width, scroll_width /2, s->color);
}

int scroll_gety(SCROLLABLE *s, int height) {
    int c = s->content_height;

    if (c > height) {
        return (s->d * (double)(c - height)) + 0.5;
    }

    return 0;
}

_Bool scroll_mmove(SCROLLABLE *s, int UNUSED(px), int UNUSED(py), int width, int height, int x, int y, int UNUSED(dx), int dy)
{
    _Bool draw = 0;

    _Bool hit = inrect(x, y, s->left ? 0 : (width - SCROLL_WIDTH), 0, SCROLL_WIDTH, height);
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

_Bool scroll_mright(SCROLLABLE *UNUSED(s))
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
