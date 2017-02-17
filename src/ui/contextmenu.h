#ifndef UI_CONTEXTMENU_H
#define UI_CONTEXTMENU_H

#include "../ui.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct contextmenu {
    int     x, y, width, height;
    bool    open;
    uint8_t count, over, down;
    void (*onselect)(uint8_t);
    STRING *(*ondisplay)(uint8_t, const struct contextmenu *);
    void *userdata;
} CONTEXTMENU;

void contextmenu_draw(void);
bool contextmenu_mmove(int mx, int my, int dx, int dy);
bool contextmenu_mdown(void);
bool contextmenu_mup(void);
bool contextmenu_mleave(void);

void contextmenu_new(uint8_t count, const UTOX_I18N_STR *menu_string_ids, void (*onselect)(uint8_t));
void contextmenu_new_ex(uint8_t count, const void *userdata, void (*onselect)(uint8_t),
                        STRING *(*ondisplay)(uint8_t, const CONTEXTMENU *));

#endif
