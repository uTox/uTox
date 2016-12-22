#ifndef LAYOUT_ROOT_H
#define LAYOUT_ROOT_H

#include "draw_helpers.h"

#include "../ui.h"

PANEL panel_notify = {
    .type = PANEL_NONE,
    .drawfunc = draw_notification,
    .disabled = 0,
    .child = (PANEL*[]) {
        NULL
    }
};

#endif
