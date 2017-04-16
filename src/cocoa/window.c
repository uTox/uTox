#include "../window.h"

#include <stdbool.h>
#include <stddef.h>

bool native_window_init(void) {
    return true;
}

static UTOX_WINDOW *native_window_create(UTOX_WINDOW *window, char *title, unsigned int class,
                                  int x, int y, int w, int h, int min_width, int min_height,
                                  void *gui_panel, bool override)
{
    return NULL;
}

void native_window_raze(UTOX_WINDOW *window) {
}

UTOX_WINDOW *native_window_create_main(int x, int y, int w, int h, char **argv, int argc) {
    return NULL;
}

void native_window_create_video() {
}

UTOX_WINDOW *native_window_find_notify(void *window) {
    return NULL;
}

UTOX_WINDOW *native_window_create_notify(int x, int y, int w, int h, PANEL* panel) {
    return NULL;
}

static void notify_tween_thread(void *obj) {
}

void native_window_tween(UTOX_WINDOW *win) {
}

void native_window_create_screen_select() {
}

bool native_window_set_target(UTOX_WINDOW *new_win) {
    return true;
}

void native_screen_grab_desktop(bool video) {
}
