#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include <X11/Xatom.h>
#include <X11/X.h>
#include <X11/cursorfont.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <X11/extensions/Xrender.h>

#include "freetype.h"

#include <X11/extensions/XShm.h>
#include <sys/shm.h>

#define _GNU_SOURCE
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <pthread.h>
#include <unistd.h>
#include <locale.h>
#include <dlfcn.h>

#ifdef __APPLE__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#define DEFAULT_WIDTH (382 * DEFAULT_SCALE)
#define DEFAULT_HEIGHT (320 * DEFAULT_SCALE)

#include <netinet/in.h>

#ifdef __APPLE__
#include <arpa/nameser_compat.h>
#else
#include <arpa/nameser.h>
#endif

#include <resolv.h>

#include <errno.h>

#define debug(...) printf(__VA_ARGS__)

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
} UTOX_NATIVE_IMAGE;

#define UTOX_NATIVE_IMAGE_IS_VALID(x) (None != (x))
#define UTOX_NATIVE_IMAGE_HAS_ALPHA(x) (None != (x->alpha))


/* Main window */
Display  *display;
int      screen;
int		 depth;
Window   root, window;
GC       gc;
Colormap cmap;
Visual   *visual;
Pixmap   drawbuf;
Picture  renderpic;
Picture  colorpic;
_Bool    hidden;
XRenderPictFormat *pictformat;

/* Tray icon window */
Window   tray_window;
Pixmap   trayicon_drawbuf;
Picture  trayicon_renderpic;
GC       trayicon_gc;
uint32_t tray_width, tray_height;

Picture bitmap[BM_ENDMARKER];
Cursor cursors[8];

Atom wm_protocols, wm_delete_window;

uint32_t scolor;

Atom XA_CLIPBOARD, XA_NET_NAME, XA_UTF8_STRING, targets, XA_INCR;
Atom XdndAware, XdndEnter, XdndLeave, XdndPosition, XdndStatus, XdndDrop, XdndSelection, XdndDATA, XdndActionCopy;
Atom XA_URI_LIST, XA_PNG_IMG;
Atom XRedraw;

Screen *scr;

/* Screen grab vars */
uint8_t pointergrab;
int grabx, graby, grabpx, grabpy;
GC grabgc;

XSizeHints *xsh;

_Bool havefocus;
_Bool _redraw;
uint16_t drawwidth, drawheight;

Window video_win[MAX_NUM_FRIENDS];
XImage *screen_image;

extern int utox_v4l_fd;

/* dynamiclly load libgtk */
void *libgtk;


/* pointers to dynamically loaded libs */
_Bool utox_portable;

struct {
    int len;
    char data[65536]; //TODO: De-hardcode this value.
} clipboard;

struct {
    int len;
    char data[65536]; //TODO: De-hardcode this value.
} primary;

struct {
    int len, left;
    Atom type;
    void *data;
} pastebuf;

Picture ximage_to_picture(XImage *img, const XRenderPictFormat *format);

_Bool doevent(XEvent event);

void tray_window_event(XEvent event);
void draw_tray_icon(void);
void togglehide(void);

void pasteprimary(void);
void setclipboard(void);
void pastebestformat(const Atom atoms[], int len, Atom selection);
void formaturilist(char *out, const char *in, int len);
void pastedata(void *data, Atom type, int len, _Bool select);

// video4linux
_Bool v4l_init(char *dev_name);
void v4l_close(void);
_Bool v4l_startread(void);
_Bool v4l_endread(void);
int v4l_getframe(uint8_t *y, uint8_t *u, uint8_t *v, uint16_t width, uint16_t height);
