// dropdown.c
#include "dropdown.h"

#include "../main.h"
#include "../theme.h"

static DROPDOWN *active_dropdown;
static int       active_x, active_y, active_width, active_height;

/* Show selected first, then skip selected */
#define index(d, i) (i == 0 ? d->selected : ((i > d->selected) ? i : i - 1))

// Draw background rectangles for a dropdown
void dropdown_drawactive(void) {
    DROPDOWN *drop = active_dropdown;
    if (!drop) {
        return;
    }

    // load colors for this style
    uint32_t color_bg, color_border, color_aoptbg, color_aopttext, color_text;

    switch (drop->style) {
        case AUXILIARY_STYLE:
            color_bg       = COLOR_BKGRND_AUX;
            color_border   = COLOR_AUX_EDGE_ACTIVE;
            color_aoptbg   = COLOR_AUX_ACTIVEOPTION_BKGRND;
            color_aopttext = COLOR_AUX_ACTIVEOPTION_TEXT;
            color_text     = COLOR_AUX_TEXT;
            break;
        default:
            color_bg       = COLOR_BKGRND_MAIN;
            color_border   = COLOR_EDGE_ACTIVE;
            color_aoptbg   = COLOR_ACTIVEOPTION_BKGRND;
            color_aopttext = COLOR_ACTIVEOPTION_TEXT;
            color_text     = COLOR_MAIN_TEXT;
            break;
    }

    int x = active_x, y = active_y, w = active_width, h = active_height;

    int i, sign = 1;

    // Increase width if needed, so that all menu items fit.
    for (i = 0; i != drop->dropcount; i++) {
        STRING *e        = drop->ondisplay(i, drop);
        int     needed_w = textwidth(e->str, e->length) + SCALE(8);
        if (w < needed_w) {
            w = needed_w;
        }
    }

    if (y + h * drop->dropcount > settings.window_height) {
        // y -= h * (drop->dropcount - 1);
        // sign = -1;
    }
    y -= h * drop->selected;

    draw_rect_fill(x, y, w, h * drop->dropcount, color_bg);
    draw_rect_frame(x, y, w, h * drop->dropcount, color_border);

    if (sign == -1) {
        y += h * (drop->dropcount - 1);
    }

    for (i = 0; i != drop->dropcount; i++) {
        // int j = index(drop, i);
        int     j = i;
        STRING *e = drop->ondisplay(j, drop);
        if (j == drop->over) {
            draw_rect_fill(x + 1, y + 1, w - 2, h - 2, color_aoptbg);
            setcolor(color_aopttext);
        } else {
            setcolor(color_text);
        }
        setfont(FONT_TEXT);
        drawtext(x + SCALE(4), y + SCALE(4), e->str, e->length);

        y += sign * h;
    }
}

// Draw collapsed dropdown
void dropdown_draw(DROPDOWN *d, int x, int y, int width, int height) {
    if (!d->open) {
        // load colors for this style
        uint32_t color_bg, color_border, color_border_h, color_text;

        switch (d->style) {
            case AUXILIARY_STYLE:
                color_bg       = COLOR_BKGRND_AUX;
                color_border   = COLOR_AUX_EDGE_NORMAL;
                color_border_h = COLOR_AUX_EDGE_HOVER;
                color_text     = COLOR_AUX_TEXT;
                break;
            default:
                color_bg       = COLOR_BKGRND_MAIN;
                color_border   = COLOR_EDGE_NORMAL;
                color_border_h = COLOR_EDGE_HOVER;
                color_text     = COLOR_MAIN_TEXT;
                break;
        }

        draw_rect_frame(x, y, width, height, (d->mouseover ? color_border_h : color_border));
        draw_rect_fill(x + 1, y + 1, width - SCALE(2), height - SCALE(2), color_bg);

        if (d->dropcount) {
            setfont(FONT_TEXT);
            setcolor(color_text);
            STRING *text = d->ondisplay(d->selected, d);
            drawtextwidth(x + SCALE(4), width - SCALE(8), y + SCALE(4), text->str, text->length);
        }
    } else {
        active_x      = x;
        active_y      = y;
        active_width  = width;
        active_height = height;
    }
}

bool dropdown_mmove(DROPDOWN *d, int UNUSED(x), int y, int w, int h, int mx, int my, int UNUSED(dx), int UNUSED(dy)) {
    if (d->open) {
        int over = my / h + d->selected;

        if (y + h * d->dropcount > settings.window_height) {
            // over = my > 0 ? 0 : ((-my) / h + 1);
        }

        if (my < 0)
            over--;

        if (over < d->dropcount) {
            // over = index(d, over);
            if (over != d->over) {
                d->over = over;
                return 1;
            }
        }
    } else {
        bool mouseover = inrect(mx, my, 0, 0, w, h);
        if (mouseover != d->mouseover) {
            d->mouseover = mouseover;
            return 1;
        }
    }

    return 0;
}

bool dropdown_mdown(DROPDOWN *d) {
    if (d->mouseover && d->dropcount) {
        d->open         = 1;
        active_dropdown = d;
        return 1;
    }

    return 0;
}

bool dropdown_mright(DROPDOWN *UNUSED(d)) { return 0; }

bool dropdown_mwheel(DROPDOWN *UNUSED(d), int UNUSED(height), double UNUSED(dlta), bool UNUSED(smooth)) { return 0; }

bool dropdown_mup(DROPDOWN *d) {
    if (d->open) {
        d->open         = 0;
        active_dropdown = NULL;
        if (d->over < d->dropcount) {
            d->selected = d->over;
            d->onselect(d->selected, d);
        }
        return 1;
    }

    return 0;
}

bool dropdown_mleave(DROPDOWN *d) {
    if (d->mouseover) {
        d->mouseover = 0;
        return 1;
    }

    return 0;
}

/***** list-based dropdown menu start *****/

// Appends localization-independent menu item.
void list_dropdown_add_hardcoded(DROPDOWN *d, uint8_t *name, void *handle) {
    void *p = realloc(d->userdata, (d->dropcount + 1) * sizeof(DROP_ELEMENT));
    if (!p) {
        return;
    }
    d->userdata = p;

    DROP_ELEMENT *e = &((DROP_ELEMENT *)d->userdata)[d->dropcount++];
    maybe_i18nal_string_set_plain(&e->name, name, strlen((char *)name));
    e->handle = handle;
}

// Appends localized menu item.
void list_dropdown_add_localized(DROPDOWN *d, UTOX_I18N_STR string_id, void *handle) {
    void *p = realloc(d->userdata, (d->dropcount + 1) * sizeof(DROP_ELEMENT));
    if (!p) {
        return;
    }
    d->userdata = p;

    DROP_ELEMENT *e = &((DROP_ELEMENT *)d->userdata)[d->dropcount++];
    maybe_i18nal_string_set_i18nal(&e->name, string_id);
    e->handle = handle;
}

// Clears menu (removes all menu items of a list-based dropdown).
void list_dropdown_clear(DROPDOWN *d) {
    free(d->userdata);
    d->userdata  = NULL;
    d->dropcount = 0;
    d->over      = 0;
    d->selected  = 0;
}

// Generic display function for list-based dropdowns,
// userdata of which is an array of DROP_ELEMENTs.
STRING *list_dropdown_ondisplay(uint16_t i, const DROPDOWN *dm) {
    DROP_ELEMENT *e = &((DROP_ELEMENT *)dm->userdata)[i];
    return maybe_i18nal_string_get(&e->name);
}

/***** list-based dropdown menu end *****/

/***** simple localized dropdown menu start *****/

// Generic display function for simple dropdowns,
// userdata of which is a simple array of UI_STRING_IDs.
STRING *simple_dropdown_ondisplay(uint16_t i, const DROPDOWN *dm) {
    return SPTRFORLANG(LANG, ((UTOX_I18N_STR *)dm->userdata)[i]);
}

/***** simple localized dropdown menu end *****/
