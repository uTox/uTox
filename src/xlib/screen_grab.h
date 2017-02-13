#ifndef XLIB_SCREEN_GRAB_H
#define XLIB_SCREEN_GRAB_H

#include <stdbool.h>

typedef struct {
    int dn_x, dn_y;
    int up_x, up_y;
} GRAB_POS;

void grab_dn(int x, int y);

void grab_up(int x, int y);

GRAB_POS grab_pos(void);

void native_screen_grab_desktop(bool video);

#endif
