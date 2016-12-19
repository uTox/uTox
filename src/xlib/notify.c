#include "main.h"
#include "window.h"

// #include "../main.h"
#include "../ui.h"

struct n_block {
    Window window;

    uint16_t number;

    int main_DC;
    int draw_DC;
    int  mem_DC;

    int draw_BM;
};

static struct n_block list[20] = { 0 };

static void window_set_make(Window window, uint16_t number) {
    for (uint8_t i = 0; i < 20; ++i) {
        if (list[i].number == number) {
            list[i].window = window;
            return;
        }
    }

    for (uint8_t i = 0; i < 20; ++i) {
        if (list[i].number == 0) {
            list[i].window = window;
            list[i].number = number;
            return;
        }
    }
}

static struct n_block *window_set_find(Window window) {
    for (uint8_t i = 0; i < 20; ++i) {
        if (list[i].window == window) {
            return &list[i];
        }
    }
    return NULL;
}

static void redraw_notify(void) {
    debug_error("redraw start\n");
    for (uint8_t i = 0; i < 20; ++i) {
        if (list[i].window) {
            // target_DC = list[i].main_DC;
            // active_DC = list[i].draw_DC;
            // active_BM = list[i].draw_BM;

            // SelectObject(active_DC, active_BM);

            panel_draw(&panel_notify, 0, 0, 400, 150);
            // enddraw_notify(0, 0, 400, 150);
        }
    }
    debug_error("redraw end\n");
}

void enddraw_notify(int x, int y, int width, int height) {
    for (uint8_t i = 0; i < 20; ++i) {
        struct n_block *win = &list[i];
        if (win->window && win->main_DC && win->draw_BM && win->draw_DC) {
            // SelectObject(win->draw_DC, win->draw_BM);
            // BitBlt(win->main_DC, x, y, width, height, win->draw_DC, x, y, SRCCOPY);
        }
    }
}

static uint16_t notification_number = 0;


Window native_notify_new(Window parent, int app_instance) {
    debug("Notify:\tCreating Notification #%u\n", notification_number);

    // int notify_x = 20;
    // int notify_y = 20;

    int x = 20;
    int y = 20;

    int notify_w = 400;
    int notify_h = 150;

    // RECT rect;
    // SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);

    // int x = rect.right - notify_x - notify_w;
    // int y = rect.top   + notify_y + (notify_y + notify_h) * notification_number;

    ++notification_number;

    // char pre[128];
    // snprintf(pre, 128, "uTox popup window %u", notification_number);
    // size_t  title_size = strlen(pre) + 1;
    // wchar_t title[title_size];
    // mbstowcs(title, pre, title_size);

    return window_create_notify(display, screen, x, y, notify_w, notify_h);
    return 0;
}
