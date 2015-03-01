#include "main.h"

static CONTEXTMENU context_menu;

#define CONTEXT_WIDTH (SCALE * 60)
#define CONTEXT_HEIGHT (SCALE * 12)

extern int COLOUR_BACKGROUND;
extern int COLOUR_FOREGROUND;

static void calculate_pos_and_width(CONTEXTMENU *b, int *x, int *w) {
    uint8_t i;

    *x = b->x;
    *w = b->width;

    // Increase width if needed, so that all menu items fit.
    for(i = 0; i < b->count; i++) {
        STRING *name = b->ondisplay(i, b);
        int needed_w = textwidth(name->str, name->length) + 4 * SCALE;
        if(*w < needed_w) {
            *w = needed_w;
        }
    }

    // Push away from the right border to fit.
    if(*x + *w >= utox_window_width) {
        *x -= *w;
    }
}

void contextmenu_draw(void)
{
    CONTEXTMENU *b = &context_menu;
    if(!b->open) {
        return;
    }

    // Ensure that font is set before calculating position and width.
    setfont(FONT_TEXT);
    setcolor(COLOUR_FOREGROUND);

    int x, w;
    calculate_pos_and_width(b, &x, &w);

    drawrectw(x, b->y, w, b->height, COLOUR_BACKGROUND);
    drawrectw(x, b->y + b->over * CONTEXT_HEIGHT, w, CONTEXT_HEIGHT, C_GRAY);

    int i;
    for(i = 0; i != b->count; i++) {
        STRING *name = b->ondisplay(i, b);
        drawtext(x + SCALE * 2, b->y + SCALE * 2 + i * CONTEXT_HEIGHT, name->str, name->length);
    }

    framerect(x, b->y, x + w, b->y + b->height, BLUE);
}

_Bool contextmenu_mmove(int mx, int my, int UNUSED(dx), int UNUSED(dy))
{
    CONTEXTMENU *b = &context_menu;

    if(!b->open) {
        return 0;
    }

    cursor = CURSOR_NONE;

    // Ensure that font is set before calculating position and width.
    setfont(FONT_TEXT);
    setcolor(COLOUR_FOREGROUND);

    int x, w;
    calculate_pos_and_width(b, &x, &w);

    _Bool mouseover = inrect(mx, my, x, b->y, w, b->height);
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

void contextmenu_new_ex(uint8_t count, void *userdata, void (*onselect)(uint8_t), STRING* (*ondisplay)(uint8_t, const CONTEXTMENU*))
{
    CONTEXTMENU *b = &context_menu;

    b->y = mouse.y;
    b->height = CONTEXT_HEIGHT * count;
    if(b->y + b->height >= utox_window_height) {
        b->y -= b->height;
    }
    b->x = mouse.x;
    b->width = CONTEXT_WIDTH;

    b->open = 1;
    b->count = count;
    b->over = 0xFF;
    b->onselect = onselect;
    b->ondisplay = ondisplay;
    b->userdata = userdata;
}

static STRING* contextmenu_localized_ondisplay(uint8_t i, const CONTEXTMENU* cm)
{
    return SPTRFORLANG(LANG, ((UI_STRING_ID*) cm->userdata)[i]);
}

void contextmenu_new(uint8_t count, UI_STRING_ID* menu_string_ids, void (*onselect)(uint8_t)) {
    contextmenu_new_ex(count, menu_string_ids, onselect, contextmenu_localized_ondisplay);
}
