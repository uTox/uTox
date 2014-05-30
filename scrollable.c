#include "main.h"

static _Bool scroll_hit(SCROLLABLE *s, int x, int y)
{
    x -= (s->x + s->width);
    if(x < -SCROLL_WIDTH || x >= 0)
    {
        return 0;
    }

    y -= s->y;
    if(y < 0 || y >= s->height)
    {
        return 0;
    }

    return 1;
}

static _Bool scroll_area(SCROLLABLE *s, int x, int y)
{
    x -= s->x;
    if(x < 0 || x >= s->width)
    {
        return 0;
    }

    y -= s->y;
    if(y < 0 || y >= s->height)
    {
        return 0;
    }

    return 1;
}


void scroll_draw(SCROLLABLE *s)
{
    uint32_t c = s->content_height;
    uint32_t h = s->height;

    if(h >= c)
    {
        return;
    }

    RECT r = {s->x + s->width - SCROLL_WIDTH, s->y, s->x + s->width, s->y + s->height};

    fillrect(&r, s->mouseover ? GRAY : WHITE);

    uint32_t m = (h * h) / c;
    double d = (h - m);
    uint32_t y = (s->d * d) + 0.5;

    r.top += y;
    r.bottom = r.top + m;


    fillrect(&r, s->mouseover ? GRAY3 : GRAY2);

    commitdraw(r.left, s->y, SCROLL_WIDTH, s->height);
}

int scroll_gety(SCROLLABLE *s)
{
    uint32_t c = s->content_height;
    uint32_t h = s->height;

    if(c > h)
    {
        return (s->d * (double)(c - h)) + 0.5;
    }

    return 0;
}

void scroll_mousemove(SCROLLABLE *s, int x, int y, int dy)
{
    _Bool hit = scroll_hit(s, x, y);
    if(s->mouseover != hit)
    {
        s->mouseover = hit;
        scroll_draw(s);//note: avoid double-draw
    }

    if(s->mousedown)
    {
        uint32_t c = s->content_height;
        uint32_t h = s->height;

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

            s->onscroll();
        }
    }
}

void scroll_mousedown(SCROLLABLE *s)
{
    if(s->mouseover)
    {
        s->mousedown = 1;
        scroll_draw(s);
    }
}

void scroll_mouseup(SCROLLABLE *s)
{
    if(s->mousedown)
    {
        s->mousedown = 0;
        scroll_draw(s);
    }
}

void scroll_mouseleave(SCROLLABLE *s)
{
    if(s->mouseover)
    {
        s->mouseover = 0;
        scroll_draw(s);
    }
}

void scroll_mousewheel(SCROLLABLE *s, int x, int y, double d)
{
    if(scroll_area(s, x, y))
    {
        uint32_t c = s->content_height;
        uint32_t h = s->height;

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

            s->onscroll();
        }
    }
}
