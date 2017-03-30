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

#include <arpa/nameser.h>
#include <ctype.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <netinet/in.h>
#include <pthread.h>
#include <resolv.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>

#include <X11/cursorfont.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/XShm.h>
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define DEFAULT_WIDTH (382 * DEFAULT_SCALE)
#define DEFAULT_HEIGHT (320 * DEFAULT_SCALE)

typedef struct native_image NATIVE_IMAGE;
struct native_image {
    // This is really a Picture, but it is just a typedef for XID, and I didn't
    // want to clutter namespace with #include <X11/extensions/Xrender.h> for it.
    XID rgb;
    XID alpha;
};

Atom wm_protocols, wm_delete_window;

Atom XA_CLIPBOARD, XA_NET_NAME, XA_UTF8_STRING, targets, XA_INCR;
Atom XdndAware, XdndEnter, XdndLeave, XdndPosition, XdndStatus, XdndDrop, XdndSelection, XdndDATA, XdndActionCopy;
Atom XA_URI_LIST, XA_PNG_IMG;
Atom XRedraw;

Picture bitmap[BM_ENDMARKER];
Cursor  cursors[8];

/* Screen grab vars */
uint8_t pointergrab;

bool     havefocus;
bool     _redraw;
uint16_t drawwidth, drawheight;

Window  video_win[32]; // TODO we should allocate this dynamically but this'll work for now
XImage *screen_image;

extern int utox_v4l_fd;

/* dynamiclly load libgtk */
void *libgtk;

struct {
    int  len;
    char data[65536]; // TODO: De-hardcode this value.
} clipboard;

struct {
    int  len;
    char data[65536]; // TODO: De-hardcode this value.
} primary;

struct {
    int   len, left;
    Atom  type;
    char *data;
} pastebuf;

Picture ximage_to_picture(XImage *img, const XRenderPictFormat *format);

bool doevent(XEvent event);

void tray_window_event(XEvent event);
void draw_tray_icon(void);
void togglehide(void);

void pasteprimary(void);
void setclipboard(void);
void pastebestformat(const Atom atoms[], size_t len, Atom selection);
void formaturilist(char *out, const char *in, size_t len);
void pastedata(void *data, Atom type, size_t len, bool select);

// video4linux
bool v4l_init(char *dev_name);
void v4l_close(void);
bool v4l_startread(void);
bool v4l_endread(void);
int v4l_getframe(uint8_t *y, uint8_t *u, uint8_t *v, uint16_t width, uint16_t height);

#endif
