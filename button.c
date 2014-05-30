#include "main.h"

static _Bool inbutton(BUTTON *b, int x, int y)
{
    x -= b->x;
    y -= b->y;
    return (x >= 0 && x < b->width && y >= 0 && y < b->height);
}

void button_draw(BUTTON *b)
{
    RECT frame = {b->x, b->y, b->x + b->width, b->y + b->height};
    framerect(&frame, BLACK);

    RECT area = {b->x + 1, b->y + 1, b->x + b->width - 1, b->y + b->height - 1};
    fillrect(&area, b->mouseover ? BUTTON_AREA_HIGHLIGHT : BUTTON_AREA);

    setfont(FONT_BUTTON);
    setcolor(b->mouseover ? 0x222222 : 0x555555);

    drawtext(b->x + 5, b->y, b->text, b->text_length);

    commitdraw(b->x, b->y, b->width, b->height);
}

void button_mousemove(BUTTON *b, int x, int y)
{
    _Bool mouseover = inbutton(b, x, y);
    if(mouseover != b->mouseover) {
        b->mouseover = mouseover;
        b->onredraw();
    }
}

void button_mousedown(BUTTON *b, int x, int y)
{
    if(inbutton(b, x, y)) {
        if(!b->mousedown) {
            b->mousedown = 1;
            b->onredraw();
        }
    }
}

void button_mouseup(BUTTON *b)
{
    if(b->mousedown) {
        if(b->mouseover) {
            b->onpress();
        }

        b->mousedown = 0;
        b->onredraw();
    }
}

void button_mouseleave(BUTTON *b)
{
    if(b->mouseover) {
        b->mouseover = 0;
        b->onredraw();
    }
}
