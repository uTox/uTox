#include "main.h"

void button_draw(BUTTON *b, int x, int y, int width, int height)
{
    uint32_t color = b->mousedown ? b->c3 : (b->mouseover ? b->c2 : b->c1);
    if(b->bm) {
        drawalpha(b->bm, x, y, width, height, color);
    } else {
        drawrectw(x, y, width, height, b->disabled ? LIST_MAIN : color);

        //setfont(FONT_TEXT_LARGE);
        //setcolor(b->mouseover ? 0x222222 : 0x555555);
        //drawtext(x + 5, y, b->text, b->text_length);
    }

    if(b->bm2) {
        int bx = width / 2 - b->bw / 2, by = height / 2 - b->bh / 2;
        drawalpha(b->bm2, x + bx, y + by, b->bw, b->bh, WHITE);
    }

    if(b->str) {
        setfont(FONT_SELF_NAME);
        setcolor(WHITE);
        drawtext(x + 3 * SCALE, y + SCALE, (uint8_t*)b->str, strlen(b->str));
    }
}

_Bool button_mmove(BUTTON *b, int x, int y, int dy, int width, int height)
{
    _Bool mouseover = inrect(x, y, 0, 0, width, height);
    if(mouseover != b->mouseover) {
        b->mouseover = mouseover;
        return 1;
    }

    return 0;
}

_Bool button_mdown(BUTTON *b)
{
    if(!b->mousedown && b->mouseover) {
        b->mousedown = 1;
        return 1;
    }

    return 0;
}

_Bool button_mright(BUTTON *b)
{
    return 0;
}

_Bool button_mwheel(BUTTON *b, int height, double d)
{
    return 0;
}

_Bool button_mup(BUTTON *b)
{
    if(b->mousedown) {
        if(b->mouseover) {
            b->onpress();
        }

        b->mousedown = 0;
        return 1;
    }

    return 0;
}

_Bool button_mleave(BUTTON *b)
{
    if(b->mouseover) {
        b->mouseover = 0;
        return 1;
    }

    return 0;
}
