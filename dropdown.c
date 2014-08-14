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

    int x = active_x, y = active_y, w = active_width, h = active_height;

    setfont(FONT_TEXT);
    setcolor(COLOR_TEXT);

    int i, sign = 1;

    if(y + h * b->dropcount > height) {
        y -= h * (b->dropcount - 1);
        sign = -1;
    }

    drawrect(x, y, x + w, y + h * b->dropcount, WHITE);
    framerect(x, y, x + w, y + h * b->dropcount, BLUE);

    if(sign == -1) {
        y += h * (b->dropcount - 1);
    }

    for(i = 0; i != b->dropcount; i++) {
        int j = index(b, i);
        DROP_ELEMENT *e = &b->drop[j];
        if(j == b->over) {
            drawrectw(x + 1, y + 1, w - 2, h - 2, C_GRAY);
        }
        drawtextwidth(x + 2 * SCALE, w - 4 * SCALE, y + 2 * SCALE, e->name, strlen((char*)e->name));

        y += sign * h;
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
            drawtextwidth(x + 2 * SCALE, width - 4 * SCALE, y + 2 * SCALE, e->name, strlen((char*)e->name));
        }
    } else {
        active_x = x;
        active_y = y;
        active_width = width;
        active_height = height;
    }
}

_Bool dropdown_mmove(DROPDOWN *b, int x, int y, int w, int h, int mx, int my, int dx, int dy)
{
    if(b->open) {
        int over = my / h;
        if(y + h * b->dropcount > height) {
            over = my > 0 ? 0 : ((-my) / h + 1);
        }
        if(over < b->dropcount) {
            over = index(b, over);
            if(over != b->over) {
                b->over = over;
                return 1;
            }
        }
    } else {
        _Bool mouseover = inrect(mx, my, 0, 0, w, h);
        if(mouseover != b->mouseover) {
            b->mouseover = mouseover;
            return 1;
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
        if(b->over < b->dropcount) {
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
