/* draws an image in the style of an avatar at within rect (x,y,targetwidth,targetheight)
 * this means: resize the image while keeping proportion so that the dimension(width or height) that has the smallest rational difference to the targetdimension becomes exactly targetdimension, then
 * crop the image so it fits in the (x,y,targetwidth,targetheight) rect, and
 * set the position if a dimension is too large so it's centered on the middle
 *
 * first argument is the image to draw, width and height are the width and height of the input image
 */
void draw_avatar_image(UTOX_NATIVE_IMAGE *image, int x, int y, uint32_t width, uint32_t height, uint32_t targetwidth, uint32_t targetheight);

enum
{
    PANEL_NONE,
    PANEL_MAIN,
    PANEL_MESSAGES,
    PANEL_LIST,
    PANEL_BUTTON,
    PANEL_DROPDOWN,
    PANEL_EDIT,
    PANEL_SCROLLABLE,
};

typedef enum {
    MAIN_STYLE, // white style, used in right side
    AUXILIARY_STYLE, // gray style, used on friends side
} UI_ELEMENT_STYLE;

typedef struct scrollable SCROLLABLE;
typedef struct edit EDIT;
typedef struct panel PANEL;
typedef struct button BUTTON;
typedef struct messages MESSAGES;
struct panel
{
    uint8_t type;
    _Bool disabled;
    int x, y, width, height;
    SCROLLABLE *content_scroll;
    void (*drawfunc)(int, int, int, int);
    PANEL **child;
};

enum
{
    ADDF_NONE,
    ADDF_SENT,
    ADDF_DISCOVER,
    ADDF_BADNAME,
    ADDF_NONAME,
    ADDF_TOOLONG, //if message length is too long.
    ADDF_NOMESSAGE, //if no message (message length must be >= 1 byte).
    ADDF_OWNKEY, //if user's own key.
    ADDF_ALREADYSENT, //if friend request already sent or already a friend.
    ADDF_UNKNOWN, //for unknown error.
    ADDF_BADCHECKSUM, //if bad checksum in address.
    ADDF_SETNEWNOSPAM, //if the friend was already there but the nospam was different.
    ADDF_NOMEM, //if increasing the friend list size fails.
};

extern PANEL panel_root, panel_search_filter, panel_quick_buttons;
extern PANEL panel_chat, panel_friend_chat, panel_friend_request, panel_group_chat;
extern PANEL panel_overhead, panel_add_friend, panel_settings_master, panel_change_profile;
extern MESSAGES messages_friend, messages_group;
extern EDIT edit_name, edit_status, edit_add_id, edit_add_msg, edit_msg, edit_msg_group, edit_search, edit_proxy_ip, edit_proxy_port;
extern SCROLLABLE scrollbar_roster;
extern BUTTON button_add_new_contact, button_settings, button_transfer;

typedef struct {
  STRING plain;
  UI_STRING_ID i18nal;
} MAYBE_I18NAL_STRING;

void maybe_i18nal_string_set_plain(MAYBE_I18NAL_STRING*, char_t *str, STRING_IDX length);
void maybe_i18nal_string_set_i18nal(MAYBE_I18NAL_STRING*, UI_STRING_ID);
STRING* maybe_i18nal_string_get(MAYBE_I18NAL_STRING*);
_Bool maybe_i18nal_string_is_valid(MAYBE_I18NAL_STRING*);

#define DEFAULT_LANG LANG_EN
#define S(x) (ui_gettext(LANG, (STR_##x))->str)
#define SLEN(x) (ui_gettext(LANG, (STR_##x))->length)
#define SPTR(x) (ui_gettext(LANG, (STR_##x)))
/* if UTOX_STR_WIDTH, is giving you a bad size you've probably changed setfont() from the string you're trying to get
 * the size of. Either store the size before changing, or swap it -> run UTOX_STR_WIDTH() -> swap back. */
#define UTOX_STR_WIDTH(x) (textwidth((ui_gettext(LANG, (STR_##x))->str), (ui_gettext(LANG, (STR_##x))->length)))
#define SPTRFORLANG(l,x) (ui_gettext((l), (x)))
// Application-wide language setting
extern UI_LANG_ID LANG;

void ui_scale(uint8_t scale);
void ui_size(int width, int height);

void ui_mouseleave(void);

void panel_draw(PANEL *p, int x, int y, int width, int height);

_Bool panel_mmove(PANEL *p, int x, int y, int width, int height, int mx, int my, int dx, int dy);
void panel_mdown(PANEL *p);
_Bool panel_dclick(PANEL *p, _Bool triclick);
_Bool panel_mright(PANEL *p);
_Bool panel_mwheel(PANEL *p, int x, int y, int width, int height, double d);
_Bool panel_mup(PANEL *p);
_Bool panel_mleave(PANEL *p);

#define GRAY(x) (((x) << 16) | ((x) << 8) | (x))

#define BLACK 0

/* search
 */
uint8_t SEARCH;
char search_data[128];

/* metrics
 */
uint8_t SCALE;

/* These are the new defines to help align UI elements, the new ones must use a _top/_bottom/ or _left/_right or
 * _width/_height postfix, and should be used to replace the originals whenever possible.
 * If you're able to replace an original, replace all occurrences, and delete the define. */

/* Left sidebar defines */
#define SIDEBAR_WIDTH                   (111 * SCALE)

/* User badge */
#define SIDEBAR_AVATAR_TOP              (5  * SCALE)
#define SIDEBAR_AVATAR_LEFT             (5  * SCALE)
#define SIDEBAR_AVATAR_WIDTH            (5  * SCALE)
#define SIDEBAR_AVATAR_HEIGHT           (5  * SCALE)
#define SIDEBAR_NAME_TOP                (8  * SCALE)
#define SIDEBAR_NAME_LEFT               (32 * SCALE)
#define SIDEBAR_NAME_WIDTH              (95 * SCALE)
#define SIDEBAR_NAME_HEIGHT             (9  * SCALE)
#define SIDEBAR_STATUSMSG_TOP           (16 * SCALE)
#define SIDEBAR_STATUSMSG_LEFT          (32 * SCALE)
#define SIDEBAR_STATUSMSG_WIDTH         (95 * SCALE)
#define SIDEBAR_STATUSMSG_HEIGHT        (6  * SCALE)
#define SELF_STATUS_X                   (96 * SCALE)
#define SELF_STATUS_Y                   (5  * SCALE)

/* Sidebar buttons and settings */
#define SIDEBAR_SEARCH_TOP              (31 * SCALE)
#define SIDEBAR_SEARCH_LEFT             (21 * SCALE)
#define SIDEBAR_SEARCH_WIDTH            (83 * SCALE)
#define SIDEBAR_SEARCH_HEIGHT           (12 * SCALE)

#define SIDEBAR_FILTER_FRIENDS_TOP      (48 * SCALE)
#define SIDEBAR_FILTER_FRIENDS_LEFT     (5  * SCALE)
#define SIDEBAR_FILTER_FRIENDS_WIDTH    (84 * SCALE)
#define SIDEBAR_FILTER_FRIENDS_HEIGHT   (6  * SCALE)

#define SIDEBAR_MENU_BUTTON_TOP         (30 * SCALE)
#define SIDEBAR_MENU_BUTTON_LEFT        (0  * SCALE)
#define SIDEBAR_MENU_BUTTON_WIDTH       (20 * SCALE)
#define SIDEBAR_MENU_BUTTON_HEIGHT      (16 * SCALE)

#define SIDEBAR_BUTTON_TOP              (30 * SCALE)
#define SIDEBAR_BUTTON_LEFT             (27 * SCALE)
#define SIDEBAR_BUTTON_WIDTH            (27 * SCALE)
#define SIDEBAR_BUTTON_HEIGHT           (16 * SCALE)

/* Roster defines */
#define ROSTER_TOP                      (57 * SCALE)
#define ROSTER_LEFT                     (8  * SCALE)
#define ROSTER_BOTTOM                   (-1 * SCALE)
#define ROSTER_BOX_LEFT                 (4  * SCALE)
#define ROSTER_BOX_HEIGHT               (25 * SCALE)
#define ROSTER_AVATAR_TOP               (5  * SCALE / 2)
#define ROSTER_AVATAR_LEFT              (4  + 5 * SCALE / 2)

#define ROSTER_NAME_TOP                 (6  * SCALE)
#define ROSTER_NAME_LEFT                (30 * SCALE)
#define ROSTER_STATUS_MSG_TOP           (13 * SCALE)

/* Main box/Chat box size settings */
#define CHAT_BOX_TOP                    (-26 * SCALE) /* size of the bottom message box */
#define MAIN_TOP_FRAME_THIN             ( 15 * SCALE)
#define MAIN_TOP_FRAME_THICK            ( 30 * SCALE)

/* Global UI size settings... */
#define SCROLL_WIDTH                    (4   * SCALE) //must be divisible by 2
#define FILE_TRANSFER_BOX_HEIGHT        (26  * SCALE)

/* Legacy defines, instead of using these, you should replace them with something more descriptive */
/* Main panel defines */
#define MAIN_LEFT                       (111 * SCALE) + 1

#define LIST_Y                          (30 * SCALE)
#define LIST_Y2                         (43 * SCALE)


#define LIST_BUTTON_Y                   (-13 * SCALE)


#define MESSAGES_SPACING                (SCALE * 2)
#define MESSAGES_X                      (55 * SCALE)
#define TIME_WIDTH                      (16 * SCALE)
#define NAME_OFFSET                     (7 * SCALE)



/* main */
//#define MAIN_X
//#define MAIN_Y LIST_Y

/* colors */
#define C_RED                   RGB(200, 78, 78)
#define C_SCROLL                GRAY(209)
