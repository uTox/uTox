#if defined(MAIN_H) && !defined(XLIB_MAIN_H)
#error "We should never include main from different platforms."
#endif

#ifndef XLIB_MAIN_H
#define XLIB_MAIN_H
#define MAIN_H

#ifdef HAVE_DBUS
#include "dbus.h"
#endif

#include "../ui/svg.h"

#include <stdint.h>

#include <X11/cursorfont.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/XShm.h>
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

typedef struct native_image NATIVE_IMAGE;
struct native_image {
    // This is really a Picture, but it is just a typedef for XID, and I didn't
    // want to clutter namespace with #include <X11/extensions/Xrender.h> for it.
    XID rgb;
    XID alpha;
};

extern Atom wm_protocols, wm_delete_window;

extern Atom XA_CLIPBOARD, XA_NET_NAME, XA_UTF8_STRING, targets, XA_INCR;
extern Atom XdndAware, XdndEnter, XdndLeave, XdndPosition, XdndStatus, XdndDrop, XdndSelection, XdndDATA, XdndActionCopy;
extern Atom XA_URI_LIST, XA_PNG_IMG;
extern Atom XRedraw;

extern Picture bitmap[BM_ENDMARKER];
extern Cursor  cursors[8];

/* Screen grab vars */
extern uint8_t pointergrab;

extern bool     _redraw;

extern XImage *screen_image;

extern int utox_v4l_fd;

/* dynamically load libgtk */
extern void *libgtk;

extern struct utox_clipboard {
    int  len;
    char data[UINT16_MAX]; // TODO: De-hardcode this value.
} clipboard;

extern struct utox_primary {
    int  len;
    char data[UINT16_MAX]; // TODO: De-hardcode this value.
} primary;

extern struct utox_pastebuf {
    int   len, left;
    Atom  type;
    char *data;
} pastebuf;

Picture ximage_to_picture(XImage *img, const XRenderPictFormat *format);

bool doevent(XEvent *event);

void togglehide(void);

void pasteprimary(void);
void setclipboard(void);
void pastebestformat(const Atom atoms[], size_t len, Atom selection);
void formaturilist(char *out, const char *in, size_t len);
void pastedata(void *data, Atom type, size_t len, bool select);

// Brute Force, the video window we got a close command on (xlib/video.c)
uint16_t find_video_windows(Window w);


// video4linux
bool v4l_init(char *dev_name);
void v4l_close(void);
bool v4l_startread(void);
bool v4l_endread(void);
int v4l_getframe(uint8_t *y, uint8_t *u, uint8_t *v, uint16_t width, uint16_t height);

#endif
