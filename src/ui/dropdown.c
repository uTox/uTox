#include "dropdown.h"

#include "draw.h"

#include "../macros.h"
#include "../settings.h"
#include "../theme.h"

#include <stdlib.h>
#include <string.h>

/* Cap how many rows an open menu may show before scrolling. */
#define DROPDOWN_MAX_VISIBLE 8

static DROPDOWN *active_dropdown;
static int       active_x, active_y, active_width, active_height;

static int dropdown_visible_rows(const DROPDOWN *d, int row_h) {
    if (!d || d->dropcount == 0 || row_h <= 0) {
        return 0;
    }

    int space = (int)settings.window_height - active_y;
    if (space < row_h) {
        space = row_h;
    }

    int max_fit = space / row_h;
    if (max_fit > DROPDOWN_MAX_VISIBLE) {
        max_fit = DROPDOWN_MAX_VISIBLE;
    }

    return (d->dropcount < (uint16_t)max_fit) ? (int)d->dropcount : max_fit;
}

static void dropdown_clamp_scroll(DROPDOWN *d, int visible) {
    if (!d) {
        return;
    }
    if (visible <= 0 || d->dropcount <= (uint16_t)visible) {
        d->scroll = 0;
        return;
    }

    uint16_t max_scroll = (uint16_t)(d->dropcount - (uint16_t)visible);
    if (d->scroll > max_scroll) {
        d->scroll = max_scroll;
    }
}

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

    // Increase width if needed, so that all menu items fit.
    if (!drop->ondisplay) {
        return;
    }

    for (int i = 0; i != drop->dropcount; i++) {
        STRING *e        = drop->ondisplay(i, drop);
        int     needed_w = textwidth(e->str, e->length) + SCALE(8);
        if (w < needed_w) {
            w = needed_w;
        }
    }

    int visible = dropdown_visible_rows(drop, h);
    dropdown_clamp_scroll(drop, visible);
    if (visible <= 0 || h <= 0) {
        return;
    }

    int menu_h = h * visible;

    draw_rect_fill(x, y, w, menu_h, color_bg);
    draw_rect_frame(x, y, w, menu_h, color_border);

    for (int i = 0; i < visible; i++) {
        int     j = (int)drop->scroll + i;
        STRING *e = drop->ondisplay((uint16_t)j, drop);
        if (j == drop->over) {
            draw_rect_fill(x + 1, y + 1, w - 2, h - 2, color_aoptbg);
            setcolor(color_aopttext);
        } else {
            setcolor(color_text);
        }
        setfont(FONT_TEXT);
        drawtext(x + SCALE(4), y + SCALE(4), e->str, e->length);

        y += h;
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
        draw_rect_fill(x + 1, y + 1, width - 2, height - 2, color_bg);

        if (d->dropcount && d->ondisplay && d->selected < d->dropcount) {
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

bool dropdown_mmove(DROPDOWN *d, int UNUSED(x), int UNUSED(y), int w, int h, int mx, int my, int UNUSED(dx),
                    int UNUSED(dy)) {
    if (d->open) {
        int visible = dropdown_visible_rows(d, h);
        dropdown_clamp_scroll(d, visible);

        bool mouseover = (visible > 0 && h > 0) && inrect(mx, my, 0, 0, w, h * visible);

        if (d->mouseover != mouseover) {
            d->mouseover = mouseover;
        }

        if (mouseover) {
            d->skip_mup = true;
        } else {
            d->skip_mup = false;
            return true;
        }

        int over = my / h + (int)d->scroll;
        if (over >= 0 && over < d->dropcount && over != d->over) {
            d->over = (uint16_t)over;
            return true;
        }
    } else {
        bool mouseover = inrect(mx, my, 0, 0, w, h);
        if (d->mouseover != mouseover) {
            d->mouseover = mouseover;
            return true;
        }
    }

    return false;
}

bool dropdown_mdown(DROPDOWN *d) {
    if (d->mouseover && d->dropcount) {
        if (!d->open) {
            d->open         = true;
            active_dropdown = d;
            /* Prefer current selection at the top; clamp happens on draw/move. */
            d->scroll = d->selected;
            d->over   = d->selected;
        }
        return true;
    }

    if (d->skip_mup) {
        return dropdown_close(d);
    }

    return false;
}

bool dropdown_close(DROPDOWN *d) {
    d->open         = false;
    d->scroll       = 0;
    active_dropdown = NULL;
    return true;
}

bool dropdown_mright(DROPDOWN *UNUSED(d)) {
    return false;
}

bool dropdown_mwheel(DROPDOWN *d, int height, double dlta, bool UNUSED(smooth)) {
    if (!d || !d->open || d->dropcount == 0 || height <= 0) {
        return false;
    }

    int visible = dropdown_visible_rows(d, height);
    if (visible <= 0 || d->dropcount <= (uint16_t)visible) {
        return false;
    }

    int delta_rows = (int)dlta;
    if (delta_rows == 0) {
        if (dlta > 0) {
            delta_rows = 1;
        } else if (dlta < 0) {
            delta_rows = -1;
        } else {
            return false;
        }
    }

    int max_scroll = (int)d->dropcount - visible;
    int new_scroll = (int)d->scroll - delta_rows;
    if (new_scroll < 0) {
        new_scroll = 0;
    } else if (new_scroll > max_scroll) {
        new_scroll = max_scroll;
    }

    if ((uint16_t)new_scroll == d->scroll) {
        return false;
    }

    d->scroll = (uint16_t)new_scroll;
    return true;
}

bool dropdown_mup(DROPDOWN *d) {
    if (d->open) {
        if (!d->mouseover) {
            return dropdown_close(d);
        }

        if (d->skip_mup) {
            d->skip_mup = false;
            dropdown_close(d);

            if (d->over < d->dropcount) {
                d->selected = d->over;
                if (d->onselect) {
                    d->onselect(d->selected, d);
                }
            }

            return true;
        } else {
            d->skip_mup = true;
        }

        return false;
    }

    return false;
}

bool dropdown_mleave(DROPDOWN *d) {
    if (d->mouseover) {
        d->mouseover = false;
        return true;
    }

    return false;
}

/***** list-based dropdown menu start *****/

// Appends localization-independent menu item.
void dropdown_list_add_hardcoded(DROPDOWN *d, char *name, void *handle) {
    if (!d || !name) {
        return;
    }
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
void dropdown_list_add_localized(DROPDOWN *d, UTOX_I18N_STR string_id, void *handle) {
    if (!d) {
        return;
    }
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
void dropdown_list_clear(DROPDOWN *d) {
    if (!d) {
        return;
    }
    free(d->userdata);
    d->userdata  = NULL;
    d->dropcount = 0;
    d->over      = false;
    d->selected  = 0;
    d->scroll    = 0;
}

// Generic display function for list-based dropdowns,
// userdata of which is an array of DROP_ELEMENTs.
STRING *dropdown_list_ondisplay(uint16_t i, const DROPDOWN *dm) {
    static STRING empty = { .str = "", .length = 0 };
    if (!dm || !dm->userdata || i >= dm->dropcount) {
        return &empty;
    }
    DROP_ELEMENT *e = &((DROP_ELEMENT *)dm->userdata)[i];
    return maybe_i18nal_string_get(&e->name);
}

/***** list-based dropdown menu end *****/

/***** simple localized dropdown menu start *****/

// Generic display function for simple dropdowns,
// userdata of which is a simple array of UI_STRING_IDs.
STRING *simple_dropdown_ondisplay(uint16_t i, const DROPDOWN *dm) {
    static STRING empty = { .str = "", .length = 0 };
    if (!dm || !dm->userdata) {
        return &empty;
    }
    return SPTRFORLANG(settings.language, ((UTOX_I18N_STR *)dm->userdata)[i]);
}

/***** simple localized dropdown menu end *****/
