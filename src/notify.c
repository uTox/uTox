#include "main.h"
#include "main_native.h"

#include "window.h"
#include "ui.h"

static uint16_t notification_number = 0;

UTOX_WINDOW *notify_new(void) {
    debug_notice("Notify:\tCreating Notification #%u\n", notification_number);

    const int notify_w = 400;
    const int notify_h = 150;


    const int x = 30;
    const int y = 30 + (20 + notify_h) * notification_number;
    ++notification_number;

    UTOX_WINDOW *w = window_create_notify(x, y, notify_w, notify_h);

    draw_set_curr_win(w);

    return w;
}
