#ifndef LAYOUT_NOTIFY_H
#define LAYOUT_NOTIFY_H

#include "buttons.h"
#include "draw_helpers.h"

#include "../ui.h"

#include "tree/notify.h"

PANEL panel_notify_generic = {
    .type = PANEL_NONE,
    .drawfunc = draw_notification,
    .disabled = 0,
    .child = (PANEL*[]) {
        (PANEL*)&button_notify_one,
        (PANEL*)&button_notify_two,
        (PANEL*)&button_notify_three,

        // (PANEL*)&button_move_notify,
        NULL
    }
};

#endif
