#ifndef XLIB_WINDOW_H
#define XLIB_WINDOW_H

#include "../window.h"

#include <X11/cursorfont.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/XShm.h>
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <inttypes.h>
#include <stdbool.h>

Display *display;
Screen  *default_screen;
int     def_screen_num;
Window  root_window;
Visual  *default_visual;


// TODO move
UTOX_WINDOW *curr;

int default_depth;

struct native_window {
    struct utox_window _; // Global struct shared across all platforms

    Window window;
    GC gc;

    Visual *visual;

    Pixmap drawbuf;

    Picture renderpic;
    Picture colorpic;

    XRenderPictFormat *pictformat;

};

struct native_window main_window;

struct native_window popup_window;
struct native_window scr_grab_window;

bool native_window_init(void);

UTOX_WINDOW *native_window_create_main(int x, int y, int w, int h, char **argv, int argc);

UTOX_WINDOW *native_window_create_video(int x, int y, int w, int h);

UTOX_WINDOW *native_window_find_notify(void *window);

UTOX_WINDOW *native_window_create_notify(int x, int y, int w, int h, void *panel);

void window_set_focus(UTOX_WINDOW *win);

void native_window_tween(UTOX_WINDOW *win);

void native_window_create_screen_select();

#endif
