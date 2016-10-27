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
#include "freetype.h"

#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xrender.h>
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


#define DEFAULT_WIDTH (382 * DEFAULT_SCALE)
#define DEFAULT_HEIGHT (320 * DEFAULT_SCALE)

#define debug(...) ((settings.verbose >= VERB_TEENAGE_GIRL) ? printf(__VA_ARGS__) : (0))
#define debug_info(...) ((settings.verbose >= VERB_NEW_ADHD_MEDS) ? printf(__VA_ARGS__) : (0))
#define debug_notice(...) ((settings.verbose >= VERB_CONCERNED_PARENT) ? printf(__VA_ARGS__) : (0))
/* debug_warning needed! */
#define debug_error(...) ((settings.verbose >= VERB_JANICE_ACCOUNTING) ? printf(__VA_ARGS__) : (0))

#define RGB(r, g, b) (((r) << 16) | ((g) << 8) | (b))

#define KEY_BACK XK_BackSpace
#define KEY_RETURN XK_Return
#define KEY_LEFT XK_Left
#define KEY_RIGHT XK_Right
#define KEY_TAB XK_Tab
#define KEY_LEFT_TAB XK_ISO_Left_Tab
#define KEY_DEL XK_Delete
#define KEY_END XK_End
#define KEY_HOME XK_Home
#define KEY_UP XK_Up
#define KEY_DOWN XK_Down
#define KEY_PAGEUP XK_Page_Up
#define KEY_PAGEDOWN XK_Page_Down

typedef struct utox_native_image {
    // This is really a Picture, but it is just a typedef for XID, and I didn't
    // want to clutter namespace with #include <X11/extensions/Xrender.h> for it.
    XID rgb;
    XID alpha;
} NATIVE_IMAGE;

#define NATIVE_IMAGE_IS_VALID(x) (None != (x))
#define NATIVE_IMAGE_HAS_ALPHA(x) (None != (x->alpha))

/* Main window */
Display *          display;
int                screen;
int                xwin_depth;
Window             root, window;
GC                 gc;
Colormap           cmap;
Visual *           visual;
Pixmap             drawbuf;
Picture            renderpic;
Picture            colorpic;
bool               hidden;
XRenderPictFormat *pictformat;

/* Tray icon window */
Window   tray_window;
Pixmap   trayicon_drawbuf;
Picture  trayicon_renderpic;
GC       trayicon_gc;
uint32_t tray_width, tray_height;

Picture bitmap[BM_ENDMARKER];
Cursor  cursors[8];

Atom wm_protocols, wm_delete_window;

uint32_t scolor;

Atom XA_CLIPBOARD, XA_NET_NAME, XA_UTF8_STRING, targets, XA_INCR;
Atom XdndAware, XdndEnter, XdndLeave, XdndPosition, XdndStatus, XdndDrop, XdndSelection, XdndDATA, XdndActionCopy;
Atom XA_URI_LIST, XA_PNG_IMG;
Atom XRedraw;

Screen *scr;

/* Screen grab vars */
uint8_t pointergrab;
int     grabx, graby, grabpx, grabpy;
GC      grabgc;

XSizeHints *xsh;

bool     havefocus;
bool     _redraw;
uint16_t drawwidth, drawheight;

Window  video_win[32]; // TODO we should allocate this dynamiclly but this'll work for now
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
    void *data;
} pastebuf;

Picture ximage_to_picture(XImage *img, const XRenderPictFormat *format);

bool doevent(XEvent event);

void tray_window_event(XEvent event);
void draw_tray_icon(void);
void togglehide(void);

void pasteprimary(void);
void setclipboard(void);
void pastebestformat(const Atom atoms[], int len, Atom selection);
void formaturilist(char *out, const char *in, int len);
void pastedata(void *data, Atom type, int len, bool select);

// video4linux
bool v4l_init(char *dev_name);
void v4l_close(void);
bool v4l_startread(void);
bool v4l_endread(void);
int v4l_getframe(uint8_t *y, uint8_t *u, uint8_t *v, uint16_t width, uint16_t height);

#endif
