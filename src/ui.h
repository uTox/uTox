#ifndef UI_H
#define UI_H

#include "../langs/i18n_decls.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct native_image NATIVE_IMAGE;

#define DEFAULT_LANG LANG_EN // TODO does this belong here?
#define S(x) (ui_gettext(LANG, (STR_##x))->str)
#define SLEN(x) (ui_gettext(LANG, (STR_##x))->length)
#define SPTR(x) (ui_gettext(LANG, (STR_##x)))

/* if UTOX_STR_WIDTH, is giving you a bad size you've probably changed setfont() from the string you're trying to get
 * the size of. Either store the size before changing, or swap it -> run UTOX_STR_WIDTH() -> swap back. */
#define UTOX_STR_WIDTH(x) (textwidth((ui_gettext(LANG, (STR_##x))->str), (ui_gettext(LANG, (STR_##x))->length)))
#define SPTRFORLANG(l, x) (ui_gettext((l), (x)))


// TODO: Create ui_native headers or something.
// This is hard to read. I know. I'm sorry.
// This is to stop a circular dependency between svg.c and xlib/main.h.
#if defined __WIN32__ || defined _WIN32 || defined __CYGWIN__
// Windows supplies its own RGB function.
#include <windows.h>  // TODO, don't do this
#elif defined __ANDROID__
#define RGB(r, g, b) ((r) | ((g) << 8) | ((b) << 16))
#elif defined __OBJC__
// xlib and cocoa use the same format for this, but I left both cases here
// in case I want to use this #ifdef construct elsewhere.
#define RGB(r, g, b) (((r) << 16) | ((g) << 8) | (b))
#else
#define RGB(r, g, b) (((r) << 16) | ((g) << 8) | (b))
#endif

// Mouse stuff
enum {
    CURSOR_NONE,
    CURSOR_TEXT,
    CURSOR_HAND,
    CURSOR_SELECT,
    CURSOR_ZOOM_IN,
    CURSOR_ZOOM_OUT,
};

struct utox_mouse {
    int x, y;
} mouse;

uint8_t cursor;
bool mdown;

enum {
    FONT_TEXT,
    FONT_TITLE,
    FONT_SELF_NAME,
    FONT_STATUS,
    FONT_LIST_NAME,
    FONT_MISC,

    FONT_END,
};

typedef enum {
    MAIN_STYLE,      // white style, used in right side
    AUXILIARY_STYLE, // gray style, used on friends side
} UI_ELEMENT_STYLE;

typedef struct panel PANEL;
typedef struct scrollable SCROLLABLE;

typedef struct maybe_i18nal_string {
    STRING        plain;
    UTOX_I18N_STR i18nal;
} MAYBE_I18NAL_STRING;

void maybe_i18nal_string_set_plain(MAYBE_I18NAL_STRING *, char *str, uint16_t length);
void    maybe_i18nal_string_set_i18nal(MAYBE_I18NAL_STRING *, UTOX_I18N_STR);
STRING *maybe_i18nal_string_get(MAYBE_I18NAL_STRING *);
bool    maybe_i18nal_string_is_valid(MAYBE_I18NAL_STRING *);

// Application-wide language setting
extern UTOX_LANG LANG;

/* draws an image in the style of an avatar at within rect (x,y,targetwidth,targetheight)
 * this means: resize the image while keeping proportion so that the dimension(width or height) that has the smallest
 * rational difference to the targetdimension becomes exactly targetdimension, then
 * crop the image so it fits in the (x,y,targetwidth,targetheight) rect, and
 * set the position if a dimension is too large so it's centered on the middle
 *
 * first argument is the image to draw, width and height are the width and height of the input image
 */
void draw_avatar_image(NATIVE_IMAGE *image, int x, int y, uint32_t width, uint32_t height, uint32_t targetwidth,
                       uint32_t targetheight);

void ui_set_scale(uint8_t scale);
void ui_rescale(uint8_t scale);
void ui_size(int width, int height);

void ui_mouseleave(void);

void panel_draw(PANEL *p, int x, int y, int width, int height);

bool panel_mmove(PANEL *p, int x, int y, int width, int height, int mx, int my, int dx, int dy);
void panel_mdown(PANEL *p);
bool panel_dclick(PANEL *p, bool triclick);
bool panel_mright(PANEL *p);
bool panel_mwheel(PANEL *p, int x, int y, int width, int height, double d, bool smooth);
bool panel_mup(PANEL *p);
bool panel_mleave(PANEL *p);

char search_data[1024]; // TODO this is NOT where this belongs

double ui_scale;

#define SCALE(x) (((int)((ui_scale / 10.0) * ((double)x))) ?: 1)
#define UI_FSCALE(x) (((ui_scale / 10.0) * ((double)x)) ?: 1)

#define drawstr(x, y, i) drawtext(x, y, S(i), SLEN(i))
#define drawstr_getwidth(x, y, str) drawtext_getwidth(x, y, (char *)str, sizeof(str) - 1)
#define strwidth(x) textwidth((char *)x, sizeof(x) - 1)

/* colors */
#define GRAY(x) (((x) << 16) | ((x) << 8) | (x))
#define BLACK 0
#define C_RED RGB(200, 78, 78)
#define C_SCROLL GRAY(209)

/* These are the new defines to help align UI elements, the new ones must use a _top/_bottom/ or _left/_right or
 * _width/_height postfix, and should be used to replace the originals whenever possible.
 * If you're able to replace an original, replace all occurrences, and delete the define. */

/* Left sidebar defines */
#define SIDEBAR_WIDTH SCALE(230)
/* User badge */
#define SIDEBAR_AVATAR_TOP SCALE(10)
#define SIDEBAR_AVATAR_LEFT SCALE(10)
#define SIDEBAR_AVATAR_WIDTH SCALE(10)
#define SIDEBAR_AVATAR_HEIGHT SCALE(10)
#define SIDEBAR_NAME_TOP SCALE(16)
#define SIDEBAR_NAME_LEFT SCALE(64)
#define SIDEBAR_NAME_WIDTH SCALE(125)
#define SIDEBAR_NAME_HEIGHT SCALE(18)
#define SIDEBAR_STATUSMSG_TOP SCALE(32)
#define SIDEBAR_STATUSMSG_LEFT SCALE(64)
#define SIDEBAR_STATUSMSG_WIDTH SCALE(125)
#define SIDEBAR_STATUSMSG_HEIGHT SCALE(12)
#define SELF_STATUS_ICON_LEFT SCALE(200)
#define SELF_STATUS_ICON_TOP SCALE(10)

/* Sidebar buttons and settings */
#define SIDEBAR_FILTER_FRIENDS_TOP SCALE(60)
#define SIDEBAR_FILTER_FRIENDS_LEFT SCALE(10)
#define SIDEBAR_FILTER_FRIENDS_WIDTH SCALE(168)
#define SIDEBAR_FILTER_FRIENDS_HEIGHT SCALE(12)

/* Roster defines */
#define ROSTER_TOP SCALE(80)
#define ROSTER_LEFT SCALE(16)
#define ROSTER_BOTTOM SCALE(-30)
#define ROSTER_BOX_LEFT SCALE(8)
#define ROSTER_BOX_HEIGHT SCALE(50)
#define ROSTER_AVATAR_TOP SCALE(5)
#define ROSTER_AVATAR_LEFT SCALE(10)

#define ROSTER_NAME_TOP SCALE(12)
#define ROSTER_NAME_LEFT SCALE(60)
#define ROSTER_STATUS_MSG_TOP SCALE(26)

/* Sidebar Lower search box and setting button */
#define SIDEBAR_SEARCH_TOP SCALE(-30)
#define SIDEBAR_SEARCH_LEFT SCALE(0)
#define SIDEBAR_SEARCH_WIDTH SCALE(199)
#define SIDEBAR_SEARCH_HEIGHT SCALE(30)

#define SIDEBAR_BUTTON_TOP SCALE(-30)
#define SIDEBAR_BUTTON_LEFT SCALE(200)
#define SIDEBAR_BUTTON_WIDTH SCALE(30)
#define SIDEBAR_BUTTON_HEIGHT SCALE(30)


/* Main box/Chat box size settings */
#define CHAT_BOX_TOP SCALE(-52) /* size of the bottom message box */
#define MAIN_TOP_FRAME_THIN SCALE(30)
#define MAIN_TOP_FRAME_THICK SCALE(60)

/* Global UI size settings... */
#define SCROLL_WIDTH SCALE(8) // must be divisible by 2
#define FILE_TRANSFER_BOX_HEIGHT SCALE(28)


/* Main panel defines */
#define MAIN_LEFT SCALE(230)
#define MAIN_TOP SCALE(60)

/* Legacy defines, instead of using these, you should replace them with something more descriptive */
#define LIST_Y2 SCALE(86)
#define LIST_BUTTON_Y SCALE(-26)
#define MESSAGES_SPACING SCALE(4)
#define MESSAGES_X SCALE(110)
#define TIME_WIDTH SCALE(45)
#define ACTUAL_TIME_WIDTH SCALE(32)
#define NAME_OFFSET SCALE(14)

#endif
