#ifndef UI_H
#define UI_H

#include "../langs/i18n_decls.h"
#include "main_native.h"
#include "sized_string.h"

#include <stdbool.h>

#define DEFAULT_LANG LANG_EN
#define S(x) (ui_gettext(LANG, (STR_##x))->str)
#define SLEN(x) (ui_gettext(LANG, (STR_##x))->length)
#define SPTR(x) (ui_gettext(LANG, (STR_##x)))
/* if UTOX_STR_WIDTH, is giving you a bad size you've probably changed setfont() from the string you're trying to get
 * the size of. Either store the size before changing, or swap it -> run UTOX_STR_WIDTH() -> swap back. */
#define UTOX_STR_WIDTH(x) (textwidth((ui_gettext(LANG, (STR_##x))->str), (ui_gettext(LANG, (STR_##x))->length)))
#define SPTRFORLANG(l, x) (ui_gettext((l), (x)))

typedef enum {
    PANEL_NONE,
    PANEL_MAIN,
    PANEL_MESSAGES,
    PANEL_INLINE_VIDEO,
    PANEL_LIST,
    PANEL_BUTTON,
    PANEL_SWITCH,
    PANEL_DROPDOWN,
    PANEL_EDIT,
    PANEL_SCROLLABLE,
} PANEL_TYPE;

typedef enum {
    MAIN_STYLE,      // white style, used in right side
    AUXILIARY_STYLE, // gray style, used on friends side
} UI_ELEMENT_STYLE;

typedef struct panel      PANEL;
typedef struct scrollable SCROLLABLE;
struct panel {
    PANEL_TYPE type;

    bool disabled;
    int  x, y, width, height;

    SCROLLABLE *content_scroll;

    void (*drawfunc)(int, int, int, int);
    void *object;

    PANEL **child;
};

typedef struct {
    STRING        plain;
    UTOX_I18N_STR i18nal;
} MAYBE_I18NAL_STRING;

void maybe_i18nal_string_set_plain(MAYBE_I18NAL_STRING *, char *str, uint16_t length);
void    maybe_i18nal_string_set_i18nal(MAYBE_I18NAL_STRING *, UTOX_I18N_STR);
STRING *maybe_i18nal_string_get(MAYBE_I18NAL_STRING *);
bool    maybe_i18nal_string_is_valid(MAYBE_I18NAL_STRING *);

// Application-wide language setting
extern UTOX_LANG LANG;

/* uTox panel draw hierarchy. */
extern PANEL panel_root, panel_side_bar, panel_self, panel_quick_buttons, panel_flist, panel_flist_list,
    panel_lower_buttons, panel_main, panel_chat, panel_group, panel_group_chat, panel_group_video, panel_group_settings,
    panel_friend, panel_friend_chat, panel_friend_video, panel_friend_settings, panel_friend_request, panel_overhead,
    panel_splash_page, panel_profile_password, panel_add_friend, panel_settings_master, panel_settings_subheader,
    panel_settings_profile, panel_profile_password_settings, panel_settings_devices, panel_settings_net,
    panel_settings_ui, panel_settings_av, messages_friend, messages_group;

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

char   search_data[128];
double ui_scale;

/* colors */
#define GRAY(x) (((x) << 16) | ((x) << 8) | (x))
#define BLACK 0
#define C_RED RGB(200, 78, 78)
#define C_SCROLL GRAY(209)

/* These are the new defines to help align UI elements, the new ones must use a _top/_bottom/ or _left/_right or
 * _width/_height postfix, and should be used to replace the originals whenever possible.
 * If you're able to replace an original, replace all occurrences, and delete the define. */

/* Left sidebar defines */
#define SIDEBAR_WIDTH (SCALE(230))
/* User badge */
#define SIDEBAR_AVATAR_TOP (SCALE(10))
#define SIDEBAR_AVATAR_LEFT (SCALE(10))
#define SIDEBAR_AVATAR_WIDTH (SCALE(10))
#define SIDEBAR_AVATAR_HEIGHT (SCALE(10))
#define SIDEBAR_NAME_TOP (SCALE(16))
#define SIDEBAR_NAME_LEFT (SCALE(64))
#define SIDEBAR_NAME_WIDTH (SCALE(125))
#define SIDEBAR_NAME_HEIGHT (SCALE(18))
#define SIDEBAR_STATUSMSG_TOP (SCALE(32))
#define SIDEBAR_STATUSMSG_LEFT (SCALE(64))
#define SIDEBAR_STATUSMSG_WIDTH (SCALE(125))
#define SIDEBAR_STATUSMSG_HEIGHT (SCALE(12))
#define SELF_STATUS_ICON_LEFT (SCALE(200))
#define SELF_STATUS_ICON_TOP (SCALE(10))

/* Sidebar buttons and settings */
#define SIDEBAR_FILTER_FRIENDS_TOP (SCALE(60))
#define SIDEBAR_FILTER_FRIENDS_LEFT (SCALE(10))
#define SIDEBAR_FILTER_FRIENDS_WIDTH (SCALE(168))
#define SIDEBAR_FILTER_FRIENDS_HEIGHT (SCALE(12))

/* Roster defines */
#define ROSTER_TOP (SCALE(80))
#define ROSTER_LEFT (SCALE(16))
#define ROSTER_BOTTOM (-SCALE(30))
#define ROSTER_BOX_LEFT (SCALE(8))
#define ROSTER_BOX_HEIGHT (SCALE(50))
#define ROSTER_AVATAR_TOP (SCALE(5))
#define ROSTER_AVATAR_LEFT (SCALE(10))

#define ROSTER_NAME_TOP (SCALE(12))
#define ROSTER_NAME_LEFT (SCALE(60))
#define ROSTER_STATUS_MSG_TOP (SCALE(26))

/* Sidebar Lower search box and setting button */
#define SIDEBAR_SEARCH_TOP (-SCALE(30))
#define SIDEBAR_SEARCH_LEFT (SCALE(0))
#define SIDEBAR_SEARCH_WIDTH (SCALE(199))
#define SIDEBAR_SEARCH_HEIGHT (SCALE(30))

#define SIDEBAR_BUTTON_TOP (-SCALE(30))
#define SIDEBAR_BUTTON_LEFT (SCALE(200))
#define SIDEBAR_BUTTON_WIDTH (SCALE(30))
#define SIDEBAR_BUTTON_HEIGHT (SCALE(30))


/* Main box/Chat box size settings */
#define CHAT_BOX_TOP (SCALE(-52)) /* size of the bottom message box */
#define MAIN_TOP_FRAME_THIN (SCALE(30))
#define MAIN_TOP_FRAME_THICK (SCALE(60))

/* Global UI size settings... */
#define SCROLL_WIDTH (SCALE(8)) // must be divisible by 2
#define FILE_TRANSFER_BOX_HEIGHT (SCALE(28))


/* Main panel defines */
#define MAIN_LEFT (UTOX_SCALE(115)) + 1
#define MAIN_TOP (UTOX_SCALE(30))

/* Legacy defines, instead of using these, you should replace them with something more descriptive */
#define LIST_Y2 (UTOX_SCALE(43))
#define LIST_BUTTON_Y (-UTOX_SCALE(13))
#define MESSAGES_SPACING (UTOX_SCALE(2))
#define MESSAGES_X (UTOX_SCALE(55))
#define TIME_WIDTH (UTOX_SCALE(20))
#define ACTUAL_TIME_WIDTH (UTOX_SCALE(16))
#define NAME_OFFSET (UTOX_SCALE(7))

#endif
