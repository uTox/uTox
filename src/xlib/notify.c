#include "main.h"

#include "window.h"
#include "drawing.h"

// #include "../main.h"
#include "../ui.h"

UTOX_WINDOW list[32];

static void window_set_make(Window window) {
    for (uint8_t i = 0; i < 32; ++i) {
        if (list[i].window == 0) {
            list[i].window = window;
            return;
        }
    }
}

static UTOX_WINDOW *window_set_find(Window window) {
    for (uint8_t i = 0; i < 32; ++i) {
        if (list[i].window == window) {
            return &list[i];
        }
    }
    return NULL;
}

static void redraw_notify(void) {
    debug_error("notify redraw start\n");
    for (uint8_t i = 0; i < 32; ++i) {
        if (list[i].window) {
            draw_window_set(&list[i]);

            panel_draw(&panel_notify, 0, 0, 400, 150);
            // enddraw_notify(0, 0, 400, 150);
        }
    }
    debug_error("redraw end\n");
}

void enddraw_notify(int x, int y, int width, int height) {
    for (uint8_t i = 0; i < 32; ++i) {
        UTOX_WINDOW *win = &list[i];
        if (win->window && win->drawbuf && win->gc) {
            // SelectObject(win->draw_DC, win->draw_BM);
            // BitBlt(win->main_DC, x, y, width, height, win->draw_DC, x, y, SRCCOPY);
        }
    }
}

static uint16_t notification_number = 0;


Window native_notify_new(void) {
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

    UTOX_WINDOW *w = window_create_notify(x, y, notify_w, notify_h);

    draw_window_set(w);
    panel_draw(&panel_notify, 0,0, w->w, w->h);
    enddraw_notify(0, 0, w->w, w->h);

    return w->window;
}

static void notify_tween_thread(void *obj) {
    UTOX_WINDOW *target = obj;

    if (!target) {
        return;
    }

    XEvent ev = {
        .xclient = {
            .type         = ClientMessage,
            .display      = display,
            .window       = target->window,
            .message_type = XRedraw,
            .format       = 8,
            .data = {
                .s = { 0, 0 }
            }
        }
    };

    while (target->y) {
        XMoveWindow(display, target->window, target->x, target->y--);
        enddraw_notify(0, 0, 400, 150);
        XSendEvent(display, target->window, 0, 0, &ev);
        XFlush(display);
        yieldcpu(1);
    }
}

static UTOX_WINDOW *focus;
void notify_set_focus(UTOX_WINDOW *win) {
    focus = win;
}

void native_notify_tween(void) {
    thread(notify_tween_thread, focus);
}
