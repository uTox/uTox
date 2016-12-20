#ifndef XLIB_WINDOW_H
#define XLIB_WINDOW_H

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

int default_depth;

struct xlib_window {
    Window window;
    GC gc;

    Visual *visual;

    Pixmap drawbuf;

    Picture renderpic;
    Picture colorpic;

    XRenderPictFormat *pictformat;

    uint32_t x, y, w, h;
};

struct xlib_window main_window;
struct xlib_window tray_window;

struct xlib_window popup_window;
struct xlib_window scr_grab_window;

bool window_init(void);

Window window_create_main(int x, int y, int w, int h, char **argv, int argc);

void window_create_video(void);

Window window_create_notify(int x, int y, int w, int h);

void winodw_create_screen_select();

#endif
