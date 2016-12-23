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

typedef struct native_window {
    Window window;
    GC gc;

    Visual *visual;

    Pixmap drawbuf;

    Picture renderpic;
    Picture colorpic;

    XRenderPictFormat *pictformat;

    // TODO should we include ui.h here?
    void *panel;

    int32_t x, y, w, h;

    struct native_window *next;
} UTOX_WINDOW;

struct native_window main_window;
struct native_window tray_window;

struct native_window popup_window;
struct native_window scr_grab_window;

bool window_init(void);

Window *window_create_main(int x, int y, int w, int h, char **argv, int argc);

void window_create_video(void);

UTOX_WINDOW *window_create_notify(int x, int y, int w, int h);

void winodw_create_screen_select();

#endif
