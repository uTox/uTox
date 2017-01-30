#include "notify.h"

#include "../ui.h"
#include "../logging_native.h"

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


static void btn_notify_one_mup(void) {
    debug_error("Button 1 pressed\n");
}

BUTTON button_notify_one = {
    // .bm  = BM_SBUTTON,
    .update = button_setcolors_success,
    .on_mup = btn_notify_one_mup,
    .nodraw = false,
};

static void btn_notify_two_mup(void) {
    debug_error("Button 2 pressed\n");
}

BUTTON button_notify_two = {
    // .bm  = BM_SBUTTON,
    .update = button_setcolors_success,
    .on_mup = btn_notify_two_mup,
    .nodraw = false,
};

static void btn_notify_three_mup(void) {
    debug_error("Button 3 pressed\n");
}
BUTTON button_notify_three = {
    // .bm  = BM_SBUTTON,
    .update = button_setcolors_success,
    .on_mup = btn_notify_three_mup,
    .nodraw = false,
};
