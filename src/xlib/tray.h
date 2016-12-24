#ifndef XLIB_TRAY_H
#define XLIB_TRAY_H

#include <X11/X.h>
#include <X11/Xlib.h>

#define SYSTEM_TRAY_REQUEST_DOCK 0
#define SYSTEM_TRAY_BEGIN_MESSAGE 1
#define SYSTEM_TRAY_CANCEL_MESSAGE 2

#include "tray.h"

#include "main.h"
#include "window.h"

#include <string.h>
#include <inttypes.h>

extern uint8_t _binary_icons_utox_128x128_png_start;
extern size_t  _binary_icons_utox_128x128_png_size;


void send_message(Display *dpy, /* display */
                  Window w, /* sender (tray window) */
                  long message, /* message opcode */
                  long data1, /* message data 1 */
                  long data2, /* message data 2 */
                  long data3 /* message data 3 */);

void draw_tray_icon(void);

void create_tray_icon(void);

void destroy_tray_icon(void);

void tray_window_event(XEvent event);

#endif
