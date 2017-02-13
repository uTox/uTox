#include "main.h"

#include "window.h"

#include "../ui.h"

typedef struct {
    int dn_x, dn_y;
    int up_x, up_y;
} GRAB_POS;

GRAB_POS grab;

void grab_dn(int x, int y) {
    grab.dn_x = x;
    grab.dn_y = y;
}

void grab_up(int x, int y) {
    grab.up_x = x;
    grab.up_y = y;
}

GRAB_POS grab_pos(void) {
    return grab;
}

void native_screen_grab_desktop(bool video) {
    pointergrab = 1 + video;
    XGrabPointer(display, main_window.window, False, Button1MotionMask | ButtonPressMask | ButtonReleaseMask, GrabModeAsync,
                 GrabModeAsync, None, cursors[CURSOR_SELECT], CurrentTime);
}
