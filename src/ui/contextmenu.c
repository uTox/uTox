#include "contextmenu.h"

#include "draw.h"
#include "../theme.h"

// FIXME: Required for UNUSED()
#include "../main.h"

static CONTEXTMENU context_menu;

#define CONTEXT_WIDTH (SCALE(120))
#define CONTEXT_HEIGHT (SCALE(24))

static void calculate_pos_and_width(CONTEXTMENU *b, int *x, int *w) {
    uint8_t i;

    *x = b->x;
    *w = b->width;

    // Increase width if needed, so that all menu items fit.
    for (i = 0; i < b->count; i++) {
        STRING *name     = b->ondisplay(i, b);
        int     needed_w = textwidth(name->str, name->length) + SCALE(8);
        if (*w < needed_w) {
            *w = needed_w;
        }
    }

    // Push away from the right border to fit.
    if (*x + *w >= settings.window_width) {
        *x -= *w;
    }
}

void contextmenu_draw(void) {
    CONTEXTMENU *b = &context_menu;
    if (!b->open) {
        return;
    }
    setfont(FONT_TEXT);

    int x, w, active_h;
    calculate_pos_and_width(b, &x, &w);

    draw_rect_fill(x, b->y, w, b->height, COLOR_BKGRND_MAIN);
    active_h = b->y + b->over * CONTEXT_HEIGHT;
    draw_rect_fill(x, active_h, w, CONTEXT_HEIGHT, COLOR_ACTIVEOPTION_BKGRND);

    int i;
    for (i = 0; i != b->count; i++) {
        // Ensure that font is set before calculating position and width.
        STRING *name = b->ondisplay(i, b);
        setcolor((active_h == b->y + i * CONTEXT_HEIGHT) ? COLOR_ACTIVEOPTION_TEXT : COLOR_MAIN_TEXT);
        drawtext(x + SCALE(4), b->y + SCALE(4) + i * CONTEXT_HEIGHT, name->str, name->length);
    }

    draw_rect_frame(x, b->y, w, b->height, COLOR_EDGE_ACTIVE);
}

bool contextmenu_mmove(int mx, int my, int UNUSED(dx), int UNUSED(dy)) {
    CONTEXTMENU *b = &context_menu;

    if (!b->open) {
        return 0;
    }

    cursor = CURSOR_NONE;

    // Ensure that font is set before calculating position and width.
    setfont(FONT_TEXT);
    setcolor(COLOR_BKGRND_MAIN);

    int x, w;
    calculate_pos_and_width(b, &x, &w);

    bool mouseover = inrect(mx, my, x, b->y, w, b->height);
    if (!mouseover) {
        if (b->over != 0xFF) {
            b->over = 0xFF;
            return 1;
        }
        return 0;
    }

    uint8_t over = (my - b->y) / CONTEXT_HEIGHT;
    if (over >= b->count) {
        over = 0xFF;
    }

    if (over != b->over) {
        b->over = over;
        return 1;
    }

    return 0;
}

bool contextmenu_mdown(void) {
    CONTEXTMENU *b = &context_menu;

    if (!b->open) {
        return 0;
    }

    if (b->over != 0xFF) {
        b->down = b->over;
    } else {
        b->open = 0;
    }
    return 1;
}

bool contextmenu_mup(void) {
    CONTEXTMENU *b = &context_menu;

    if (!b->open) {
        return 0;
    }

    if (b->over == b->down) {
        b->onselect(b->over);
        b->open = 0;
        return 1;
    }

    return 0;
}

bool contextmenu_mleave(void) {
    CONTEXTMENU *b = &context_menu;

    if (!b->open) {
        return 0;
    }

    if (b->over != 0xFF) {
        b->over = 0xFF;
        return 1;
    }

    return 0;
}

void contextmenu_new_ex(uint8_t count, void *userdata, void (*onselect)(uint8_t),
                        STRING *(*ondisplay)(uint8_t, const CONTEXTMENU *)) {
    CONTEXTMENU *b = &context_menu;

    b->y      = mouse.y;
    b->height = CONTEXT_HEIGHT * count;
    if (b->y + b->height >= settings.window_height) {
        b->y -= b->height;
    }
    b->x     = mouse.x;
    b->width = CONTEXT_WIDTH;

    b->open      = 1;
    b->count     = count;
    b->over      = 0xFF;
    b->onselect  = onselect;
    b->ondisplay = ondisplay;
    b->userdata  = userdata;
}

static STRING *contextmenu_localized_ondisplay(uint8_t i, const CONTEXTMENU *cm) {
    return SPTRFORLANG(LANG, ((UTOX_I18N_STR *)cm->userdata)[i]);
}

void contextmenu_new(uint8_t count, UTOX_I18N_STR *menu_string_ids, void (*onselect)(uint8_t)) {
    contextmenu_new_ex(count, menu_string_ids, onselect, contextmenu_localized_ondisplay);
}
