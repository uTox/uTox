#ifndef LAYOUT_CREATE_H
#define LAYOUT_CREATE_H

/* Space keeping file for the macros I want to create to make changing/creating
 * layouts easier and/of faster. */

#define CREATE_BUTTON(n, a, b, w, h) button_##n = {     \
        panel.type      = PANEL_BUTTON,                 \
        panel.x         = a,                            \
        panel.y         = b,                            \
        panel.width     = w,                            \
        panel.height    = h,                            \
    };

#define CREATE_EDIT(n, a, b, w, h) edit_##n = {         \
        panel.type      = PANEL_EDIT,                   \
        panel.x         = a,                            \
        panel.y         = b,                            \
        panel.width     = w,                            \
        panel.height    = h,                            \
    };

#define CREATE_SWITCH(n, a, b, w, h) switch_##n = {     \
        panel.type      = PANEL_SWITCH,                 \
        panel.x         = a,                            \
        panel.y         = b,                            \
        panel.width     = w,                            \
        panel.height    = h,                            \
    };

#define CREATE_DROPDOWN(n, a, b, h, w) dropdown_##n = { \
        panel.type   =  PANEL_DROPDOWN,                 \
        panel.x      =  a,                              \
        panel.y      =  b,                              \
        panel.height =  h,                              \
        panel.width  =  w,                              \
    };

#endif // LAYOUT_CREATE_H
