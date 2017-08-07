#ifndef UI_PANEL_H
#define UI_PANEL_H

#include <stdbool.h>

#include "../typedefs.h"

typedef enum {
    PANEL_NONE,
    PANEL_MAIN,
    PANEL_MESSAGES,
    PANEL_INLINE_VIDEO,
    PANEL_LIST,
    PANEL_BUTTON,
    PANEL_SWITCH,
    PANEL_DROPDOWN,
    PANEL_EDIT,
    PANEL_SCROLLABLE,
} PANEL_TYPE;

typedef void ui_draw_cb(int x, int y, int w, int h);
typedef void ui_update_cb(int width, int height, int scale);

struct panel {
    PANEL_TYPE type;

    bool disabled;
    int  x, y, width, height;

    SCROLLABLE *content_scroll;

    ui_draw_cb *drawfunc;
    ui_update_cb *update;
    void *object;

    PANEL **child;
};

#endif // UI_PANEL_H
