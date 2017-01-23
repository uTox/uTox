#include "main.h"

#include "window.h"
#include "../main.h" // cursors

void native_screen_grab_desktop(bool video) {
    pointergrab = 1 + video;
    XGrabPointer(display, main_window.window, False, Button1MotionMask | ButtonPressMask | ButtonReleaseMask, GrabModeAsync,
                 GrabModeAsync, None, cursors[CURSOR_SELECT], CurrentTime);
}
