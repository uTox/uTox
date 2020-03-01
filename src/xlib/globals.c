#include "freetype.h"
#include "main.h"
#include "window.h"

/* Globals */

/* freetype.h */
FT_Library ftlib;
FONT       font[16], *sfont;
FcCharSet *charset;
FcFontSet *fs;

bool ft_vert, ft_swap_blue_red;

/* main.h */
Atom wm_protocols, wm_delete_window;

Atom XA_CLIPBOARD, XA_NET_NAME, XA_UTF8_STRING, targets, XA_INCR;
Atom XdndAware, XdndEnter, XdndLeave, XdndPosition, XdndStatus, XdndDrop, XdndSelection, XdndDATA, XdndActionCopy;
Atom XA_URI_LIST, XA_PNG_IMG;
Atom XRedraw;

Picture bitmap[BM_ENDMARKER];
Cursor  cursors[8];

uint8_t pointergrab;

bool     _redraw;

XImage *screen_image;

void *libgtk;

struct utox_clipboard clipboard;

struct utox_primary primary;

struct utox_pastebuf pastebuf;


/* window.h */
Display *display;
Screen  *default_screen;
int     def_screen_num;
Window  root_window;
Visual  *default_visual;

UTOX_WINDOW *curr;

int default_depth;

struct native_window main_window;

struct native_window popup_window;
struct native_window scr_grab_window;

struct native_window tray_pop;
