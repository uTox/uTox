#ifndef XLIB_TRAY_H
#define XLIB_TRAY_H

#include "main.h"
#include "window.h"

#include <string.h>
#include <inttypes.h>

#include <X11/X.h>
#include <X11/Xlib.h>

// TODO fine the correct header for these, or consider an enum
#define SYSTEM_TRAY_REQUEST_DOCK 0
#define SYSTEM_TRAY_BEGIN_MESSAGE 1
#define SYSTEM_TRAY_CANCEL_MESSAGE 2

void create_tray_icon(void);

void destroy_tray_icon(void);

bool tray_window_event(XEvent event);

void tray_drop_init(PANEL *p);

#endif
