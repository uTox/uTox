#ifndef LAYOUT_CREATE_H
#define LAYOUT_CREATE_H

/* Space keeping file for the macros I want to create to make changing/creating
 * layouts easier and/of faster. */

/* These macros are broken right now.
 * TODO: Fix them. */

/*
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
*/

#define CREATE_BUTTON(n, a, b, w, h)            \
    button_##n.panel.type     = PANEL_BUTTON;   \
    button_##n.panel.x        = a;              \
    button_##n.panel.y        = b;              \
    button_##n.panel.width    = w;              \
    button_##n.panel.height   = h;

#define CREATE_EDIT(n, a, b, w, h)              \
    edit_##n.panel.type       = PANEL_EDIT;     \
    edit_##n.panel.x          = a;              \
    edit_##n.panel.y          = b;              \
    edit_##n.panel.width      = w;              \
    edit_##n.panel.height     = h;

#define CREATE_SWITCH(n, a, b, w, h)            \
    switch_##n.panel.type     = PANEL_SWITCH;   \
    switch_##n.panel.x        = a;              \
    switch_##n.panel.y        = b;              \
    switch_##n.panel.width    = w;              \
    switch_##n.panel.height   = h;

#define CREATE_DROPDOWN(n, a, b, h, w)          \
    dropdown_##n.panel.type   = PANEL_DROPDOWN; \
    dropdown_##n.panel.x      = a;              \
    dropdown_##n.panel.y      = b;              \
    dropdown_##n.panel.height = h;              \
    dropdown_##n.panel.width  = w;

#endif // LAYOUT_CREATE_H
