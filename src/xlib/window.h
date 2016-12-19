#ifndef XLIB_WINDOW_H
#define XLIB_WINDOW_H

#include "main.h"

Window window_create_main(Display *display, int screen, int x, int y, int w, int h, char **argv, int argc);

void window_create_video(void);

Window window_create_notify(Display *display, int screen, int x, int y, int w, int h);

void winodw_create_screen_select();

#endif
