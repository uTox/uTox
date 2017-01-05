#include "main.h"
#include "main_native.h"

#include "window.h"
#include "ui.h"

static uint16_t notification_number = 0;

UTOX_WINDOW *notify_new(void) {
    debug("Notify:\tCreating Notification #%u\n", notification_number);

    // int notify_x = 20;
    // int notify_y = 20;


    int notify_w = 400;
    int notify_h = 150;

    // RECT rect;
    // SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);

    // int x = rect.right - notify_x - notify_w;
    // int y = rect.top   + notify_y + (notify_y + notify_h) * notification_number;

    int x = 30;
    int y = 30 + (20 + notify_h) * notification_number;
    ++notification_number;
    // TODO wrap at max screen size

    // char pre[128];
    // snprintf(pre, 128, "uTox popup window %u", notification_number);
    // size_t  title_size = strlen(pre) + 1;
    // wchar_t title[title_size];
    // mbstowcs(title, pre, title_size);

    UTOX_WINDOW *w = (UTOX_WINDOW *)window_create_notify(x, y, notify_w, notify_h);

    draw_set_curr_win(w);
    // panel_draw(&panel_notify, 0,0, w->w, w->h);
    // enddraw_notify(0, 0, w->w, w->h);

    return w;
}
