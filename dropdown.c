#include "main.h"

static DROPDOWN *active;
static int active_x, active_y, active_width, active_height;

extern int COLOUR_MAIN_FOREGROUND;
extern int COLOUR_MAIN_BACKGROUND;
extern int COLOUR_EDGE_BACKGROUND;
extern int COLOUR_DROPDOWN_ACTIVE_BACKGROUND;
extern int COLOUR_DROPDOWN_ACTIVE_FOREGROUND;

#define index(b, i) (i == 0 ? b->selected : ((i > b->selected) ? i : i - 1))

// Draw background recktangles for a dropdown
void dropdown_drawactive(void)
{
    DROPDOWN *b = active;
    if(!b) {
        return;
    }

    int x = active_x, y = active_y, w = active_width, h = active_height;

    int i, sign = 1;

    // Increase width if needed, so that all menu items fit.
    for(i = 0; i != b->dropcount; i++) {
        STRING* e = b->ondisplay(i, b);
        int needed_w = textwidth(e->str, e->length) + 4 * SCALE;
        if(w < needed_w) {
            w = needed_w;
        }
    }

    if(y + h * b->dropcount > utox_window_height) {
        y -= h * (b->dropcount - 1);
        sign = -1;
    }

    drawrect(x, y, x + w, y + h * b->dropcount, COLOUR_MAIN_BACKGROUND);
    framerect(x, y, x + w, y + h * b->dropcount, COLOUR_EDGE_BACKGROUND);

    if(sign == -1) {
        y += h * (b->dropcount - 1);
    }

    for(i = 0; i != b->dropcount; i++) {
        int j = index(b, i);
        STRING* e = b->ondisplay(j, b);
        if(j == b->over) {
            drawrectw(x + 1, y + 1, w - 2, h - 2, COLOUR_DROPDOWN_ACTIVE_BACKGROUND);
            setcolor(COLOUR_MAIN_BACKGROUND);
        } else {
            setcolor(COLOUR_MAIN_FOREGROUND);
        }
        setfont(FONT_TEXT);
        drawtext(x + 2 * SCALE, y + 2 * SCALE, e->str, e->length);

        y += sign * h;
    }
}

// Draw collapsed dropdown
void dropdown_draw(DROPDOWN *b, int x, int y, int width, int height)
{
    if(!b->open) {
        framerect(x, y, x + width, y + height, (b->mouseover ? C_GRAY2 : C_GRAY));
        drawrect(x + 1, y + 1, x + width - 1, y + height - 1, COLOUR_MAIN_BACKGROUND);

        if(b->dropcount) {
            setfont(FONT_TEXT);
            setcolor(COLOUR_MAIN_FOREGROUND);
            STRING* e = b->ondisplay(b->selected, b);
            drawtextwidth(x + 2 * SCALE, width - 4 * SCALE, y + 2 * SCALE, e->str, e->length);
        }
    } else {
        active_x = x;
        active_y = y;
        active_width = width;
        active_height = height;
    }
}

_Bool dropdown_mmove(DROPDOWN *b, int UNUSED(x), int y, int w, int h, int mx, int my, int UNUSED(dx), int UNUSED(dy))
{
    if(b->open) {
        int over = my / h;
        if(y + h * b->dropcount > utox_window_height) {
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

_Bool dropdown_mright(DROPDOWN *UNUSED(b))
{
    return 0;
}

_Bool dropdown_mwheel(DROPDOWN *UNUSED(b), int UNUSED(height), double UNUSED(d))
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
            b->onselect(b->selected, b);
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

/***** list-based dropdown menu start *****/

// Appends localization-independent menu item.
void list_dropdown_add_hardcoded(DROPDOWN *b, uint8_t* name, void *handle)
{
    void *p = realloc(b->userdata, (b->dropcount + 1) * sizeof(DROP_ELEMENT));
    if(!p) {
        return;
    }
    b->userdata = p;

    DROP_ELEMENT *e = &((DROP_ELEMENT*)b->userdata)[b->dropcount++];
    maybe_i18nal_string_set_plain(&e->name, name, strlen((char*)name));
    e->handle = handle;
}

// Appends localized menu item.
void list_dropdown_add_localized(DROPDOWN *b, UI_STRING_ID string_id, void *handle)
{
    void *p = realloc(b->userdata, (b->dropcount + 1) * sizeof(DROP_ELEMENT));
    if(!p) {
        return;
    }
    b->userdata = p;

    DROP_ELEMENT *e = &((DROP_ELEMENT*)b->userdata)[b->dropcount++];
    maybe_i18nal_string_set_i18nal(&e->name, string_id);
    e->handle = handle;
}

// Clears menu (removes all menu items of a list-based dropdown).
void list_dropdown_clear(DROPDOWN *b)
{
    free(b->userdata);
    b->userdata = NULL;
    b->dropcount = 0;
    b->over = 0;
    b->selected = 0;
}

// Generic display function for list-based dropdowns,
// userdata of which is an array of DROP_ELEMENTs.
STRING* list_dropdown_ondisplay(uint16_t i, const DROPDOWN* dm)
{
    DROP_ELEMENT *e = &((DROP_ELEMENT*) dm->userdata)[i];
    return maybe_i18nal_string_get(&e->name);
}

/***** list-based dropdown menu end *****/

/***** simple localized dropdown menu start *****/

// Generic display function for simple dropdowns,
// userdata of which is a simple array of UI_STRING_IDs.
STRING* simple_dropdown_ondisplay(uint16_t i, const DROPDOWN* dm)
{
    return SPTRFORLANG(LANG, ((UI_STRING_ID*) dm->userdata)[i]);
}

/***** simple localized dropdown menu end *****/
