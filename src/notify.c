#include "notify.h"

#include "main.h"
#include "debug.h"

#include "draw.h"
#include "ui.h"
#include "window.h"

#include "layout/notify.h"

static uint16_t notification_number = 0;

UTOX_WINDOW *notify_new(NOTIFY_TYPE type) {
    LOG_NOTE("Notifier", "Notify:\tCreating Notification #%u\n", notification_number);

    const int notify_w = 400;
    const int notify_h = 150;


    const int x = 30;
    const int y = 30 + (20 + notify_h) * notification_number;
    ++notification_number;

    PANEL *panel;
    switch (type) {
        case NOTIFY_TYPE_NONE: {
            return NULL;
        }
        case NOTIFY_TYPE_MSG: {
            panel = &panel_notify_generic;
            break;
        }
        case NOTIFY_TYPE_CALL:
        case NOTIFY_TYPE_CALL_VIDEO: {
            panel = &panel_notify_generic; // TODO create a video call panel type
            break;
        }
    }

    UTOX_WINDOW *w = window_create_notify(x, y, notify_w, notify_h, panel);

    native_window_set_target(w);

    return w;
}
