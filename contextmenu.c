#include "main.h"

static CONTEXTMENU context_menu;

#define CONTEXT_WIDTH (SCALE * 60)
#define CONTEXT_HEIGHT (SCALE * 12)

void contextmenu_draw(void)
{
    CONTEXTMENU *b = &context_menu;
    if(!b->open) {
        return;
    }

    setfont(FONT_TEXT);
    setcolor(COLOR_TEXT);

    drawrectw(b->x, b->y, b->width, b->height, WHITE);
    drawrectw(b->x, b->y + b->over * CONTEXT_HEIGHT, b->width, CONTEXT_HEIGHT, C_GRAY);

    int i;
    for(i = 0; i != b->count; i++) {
        drawtext(b->x + SCALE * 2, b->y + SCALE * 2 + i * CONTEXT_HEIGHT, b->names[i], strlen((char*)b->names[i]));
    }

    framerect(b->x, b->y, b->x + b->width, b->y + b->height, BLUE);
}

_Bool contextmenu_mmove(int mx, int my, int UNUSED(dx), int UNUSED(dy))
{
    CONTEXTMENU *b = &context_menu;

    if(!b->open) {
        return 0;
    }

    cursor = CURSOR_NONE;

    _Bool mouseover = inrect(mx, my, b->x, b->y, b->width, b->height);
    if(!mouseover) {
        if(b->over != 0xFF) {
            b->over = 0xFF;
            return 1;
        }
        return 0;
    }

    uint8_t over = (my - b->y) / CONTEXT_HEIGHT;
    if(over >= b->count) {
        over = 0xFF;
    }

    if(over != b->over) {
        b->over = over;
        return 1;
    }

    return 0;
}

_Bool contextmenu_mdown(void)
{
    CONTEXTMENU *b = &context_menu;

    if(!b->open) {
        return 0;
    }

    if(b->over != 0xFF) {
        b->down = b->over;
    } else {
        b->open = 0;
    }
    return 1;
}

_Bool contextmenu_mup(void)
{
    CONTEXTMENU *b = &context_menu;

    if(!b->open) {
        return 0;
    }

    if(b->over == b->down) {
        b->onselect(b->over);
        b->open = 0;
        return 1;
    }

    return 0;
}

_Bool contextmenu_mleave(void)
{
    CONTEXTMENU *b = &context_menu;

    if(!b->open) {
        return 0;
    }

    if(b->over != 0xFF) {
        b->over = 0xFF;
        return 1;
    }

    return 0;
}

void contextmenu_new(uint8_t **names, uint8_t count, void (*onselect)(uint8_t))
{
    CONTEXTMENU *b = &context_menu;

    b->y = mouse.y;
    b->height = CONTEXT_HEIGHT * count;
    if(b->y + b->height >= height) {
        b->y -= b->height;
    }
    b->x = mouse.x;
    b->width = CONTEXT_WIDTH;
    if(b->x + b->width >= width) {
        b->x -= b->width;
    }

    b->open = 1;
    b->count = count;
    b->over = 0xFF;
    b->onselect = onselect;
    memcpy(b->names, names, sizeof(uint8_t*) * count);
}
