#ifndef DROPDOWN_H
#define DROPDOWN_H

#include <inttypes.h>
#include <stdbool.h>

#include "../ui.h"

// userdata of list-based dropdown consists of these records
typedef struct {
    MAYBE_I18NAL_STRING name;
    void *              handle;
} DROP_ELEMENT;

typedef struct dropdown {
    PANEL    panel;
    bool     mouseover, open;
    uint16_t dropcount, selected, over;

    void (*onselect)(uint16_t, const struct dropdown *);
    STRING *(*ondisplay)(uint16_t, const struct dropdown *);

    UI_ELEMENT_STYLE style;

    void *userdata;
} DROPDOWN;

void dropdown_drawactive(void);

void dropdown_draw(DROPDOWN *b, int x, int y, int width, int height);
bool dropdown_mmove(DROPDOWN *b, int x, int y, int width, int height, int mx, int my, int dx, int dy);
bool dropdown_mdown(DROPDOWN *b);
bool dropdown_mright(DROPDOWN *b);
bool dropdown_mwheel(DROPDOWN *b, int height, double d, bool smooth);
bool dropdown_mup(DROPDOWN *b);
bool dropdown_mleave(DROPDOWN *b);

STRING *simple_dropdown_ondisplay(uint16_t, const DROPDOWN *);

STRING *dropdown_list_ondisplay(uint16_t i, const DROPDOWN *dm);
void dropdown_list_add_hardcoded(DROPDOWN *d, char *name, void *handle);
void dropdown_list_add_localized(DROPDOWN *d, UTOX_I18N_STR string_id, void *handle);
void dropdown_list_clear(DROPDOWN *);

#endif
