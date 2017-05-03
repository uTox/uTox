#include "window.h"

#include "native/window.h"

void window_raze(UTOX_WINDOW *window) {
    native_window_raze(window);
}

void window_create_video(int x, int y, int w, int h) {
    native_window_create_video(x, y, w, h);
}

UTOX_WINDOW *window_find_notify(void *window) {
    return native_window_find_notify(window);
}

UTOX_WINDOW *window_create_notify(int x, int y, int w, int h, PANEL *panel) {
    return native_window_create_notify(x, y, w, h, panel);
}

void window_tween(UTOX_WINDOW *win) {
    native_window_tween(win);
}

void window_create_screen_select(void) {
    native_window_create_screen_select();
}

