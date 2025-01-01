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

bool tray_window_event(XEvent *event);

#ifdef __APPLE__
#include <mach-o/getsect.h>

#define EXTLD(NAME) \
    extern uint8_t _section$__DATA__ ## NAME [];
#define LDVAR(NAME) _section$__DATA__ ## NAME
#define LDLEN(NAME) (getsectbyname("__DATA", "__" #NAME)->size)

#elif (defined __WIN32__)  /* mingw */

#define EXTLD(NAME) \
    extern uint8_t binary_ ## NAME ## _start[]; \
    extern uint8_t binary_ ## NAME ## _end[];
#define LDVAR(NAME) \
    binary_ ## NAME ## _start
#define LDLEN(NAME) \
    ((binary_ ## NAME ## _end) - (binary_ ## NAME ## _start))

#else /* gnu/linux ld */

#define EXTLD(NAME) \
    extern uint8_t _binary_ ## NAME ## _start[]; \
    extern uint8_t _binary_ ## NAME ## _end[];
#define LDVAR(NAME) \
    _binary_ ## NAME ## _start
#define LDLEN(NAME) \
    ((_binary_ ## NAME ## _end) - (_binary_ ## NAME ## _start))

#endif

#endif
