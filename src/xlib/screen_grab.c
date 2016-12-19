#include "main.h"

void native_screen_grab_desktop(bool video) {
    pointergrab = 1 + video;
    XGrabPointer(display, window, False, Button1MotionMask | ButtonPressMask | ButtonReleaseMask, GrabModeAsync,
                 GrabModeAsync, None, cursors[CURSOR_SELECT], CurrentTime);
}
