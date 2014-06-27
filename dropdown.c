#include "main.h"

static DROPDOWN *active;
static int active_x, active_y, active_width, active_height;

#define index(b, i) (i == 0 ? b->selected : ((i > b->selected) ? i : i - 1))

void dropdown_drawactive(void)
{
    DROPDOWN *b = active;
    if(!b) {
        return;
    }

    int x = active_x, y = active_y, width = active_width, height = active_height;

    setfont(FONT_TEXT);
    setcolor(COLOR_TEXT);
    framerect(x, y, x + width, y + height * b->dropcount, BLUE);
    drawrect(x + 1, y + 1, x + width - 1, y + height * b->dropcount - 1, WHITE);
    int i;
    for(i = 0; i != b->dropcount; i++) {
        int j = index(b, i);
        DROP_ELEMENT *e = &b->drop[j];
        if(j == b->over) {
            drawrectw(x + 1, y + 1 + i * height, width - 2, height - 2, C_GRAY);
        }
        drawtext(x + 2 * SCALE, y + 2 * SCALE + i * height, e->name, strlen((char*)e->name));
    }
}

void dropdown_draw(DROPDOWN *b, int x, int y, int width, int height)
{
    if(!b->open) {
        framerect(x, y, x + width, y + height, (b->mouseover ? C_GRAY2 : C_GRAY));
        drawrect(x + 1, y + 1, x + width - 1, y + height - 1, WHITE);

        if(b->dropcount) {
            setfont(FONT_TEXT);
            setcolor(COLOR_TEXT);
            DROP_ELEMENT *e = &b->drop[b->selected];
            drawtext(x + 2 * SCALE, y + 2 * SCALE, e->name, strlen((char*)e->name));
        }
    } else {
        active_x = x;
        active_y = y;
        active_width = width;
        active_height = height;
    }
}

_Bool dropdown_mmove(DROPDOWN *b, int x, int y, int width, int height, int mx, int my, int dy)
{
    _Bool mouseover = inrect(mx, my, 0, 0, width, height);
    if(mouseover != b->mouseover) {
        b->mouseover = mouseover;
        return 1;
    }

    if(b->open) {
        int over = my / height;
        if(over < b->dropcount) {
            over = index(b, over);
            if(over != b->over) {
                b->over = over;
                return 1;
            }
        }
    }

    return 0;
}

_Bool dropdown_mdown(DROPDOWN *b)
{
    if(b->mouseover && b->dropcount) {
        b->open = 1;
        active = b;
        return 1;
    }

    return 0;
}

_Bool dropdown_mright(DROPDOWN *b)
{
    return 0;
}

_Bool dropdown_mwheel(DROPDOWN *b, int height, double d)
{
    return 0;
}

_Bool dropdown_mup(DROPDOWN *b)
{
    if(b->open) {
        b->open = 0;
        active = NULL;
        if(b->over != b->selected && b->over < b->dropcount) {
            b->selected = b->over;
            b->onselect(b->drop[b->over].handle);
        }
        return 1;
    }

    return 0;
}

_Bool dropdown_mleave(DROPDOWN *b)
{
    if(b->mouseover) {
        b->mouseover = 0;
        return 1;
    }

    return 0;
}

void dropdown_add(DROPDOWN *b, uint8_t *name, void *handle)
{
    void *p = realloc(b->drop, (b->dropcount + 1) * sizeof(DROP_ELEMENT));
    if(!p) {
        return;
    }

    b->drop = p;
    DROP_ELEMENT *e = &b->drop[b->dropcount++];
    e->name = name;
    e->handle = handle;
}
