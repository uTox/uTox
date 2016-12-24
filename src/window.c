#include "window.h"

#include "main.h"

#include "main_native.h"

bool window_init(void) {
    return native_window_init();
}

void window_raze(UTOX_WINDOW *window) {
    native_window_raze(window);
}

void window_create_video() {
    native_window_create_video();
}

UTOX_WINDOW *window_find_notify(void *window) {
    return native_window_find_notify(window);
}

UTOX_WINDOW *window_create_notify(int x, int y, int w, int h) {
    return native_window_create_notify(x, y, w, h);
}

void window_tween(void) {
    native_window_tween();
}

void window_create_screen_select(void) {
    native_window_create_screen_select();
}

