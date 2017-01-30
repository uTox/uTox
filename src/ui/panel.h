#ifndef UI_PANEL_H
#define UI_PANEL_H

#include <stdbool.h>

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

typedef struct panel PANEL;
typedef struct scrollable SCROLLABLE;

struct panel {
    PANEL_TYPE type;

    bool disabled;
    int  x, y, width, height;

    SCROLLABLE *content_scroll;

    void (*drawfunc)(int, int, int, int);
    void *object;

    PANEL **child;
};

#endif // UI_PANEL_H
