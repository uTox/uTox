/* draws an image in the style of an avatar at within rect (x,y,targetwidth,targetheight)
 * this means: resize the image while keeping proportion so that the dimension(width or height) that has the smallest rational difference to the targetdimension becomes exactly targetdimension, then
 * crop the image so it fits in the (x,y,targetwidth,targetheight) rect, and
 * set the position if a dimension is too large so it's centered on the middle
 *
 * first argument is the image to draw, width and height are the width and height of the input image
 */
void draw_avatar_image(UTOX_NATIVE_IMAGE *image, int x, int y, uint32_t width, uint32_t height, uint32_t targetwidth, uint32_t targetheight);

enum {
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

enum {
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
extern PANEL panel_overhead, panel_add_friend, panel_settings_master, panel_change_profile, panel_profile_password;
extern MESSAGES messages_friend, messages_group;
extern EDIT edit_name, edit_status, edit_add_id, edit_add_msg, edit_msg, edit_msg_group, edit_search, edit_proxy_ip, edit_proxy_port, edit_profile_password;
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

void ui_set_scale(uint8_t scale);
void ui_size(int width, int height);

void ui_mouseleave(void);

void panel_draw(PANEL *p, int x, int y, int width, int height);

_Bool panel_mmove(PANEL *p, int x, int y, int width, int height, int mx, int my, int dx, int dy);
void panel_mdown(PANEL *p);
_Bool panel_dclick(PANEL *p, _Bool triclick);
_Bool panel_mright(PANEL *p);
_Bool panel_mwheel(PANEL *p, int x, int y, int width, int height, double d, _Bool smooth);
_Bool panel_mup(PANEL *p);
_Bool panel_mleave(PANEL *p);

#define GRAY(x) (((x) << 16) | ((x) << 8) | (x))

#define BLACK 0

char search_data[128];

/* metrics
 */
float ui_scale;

/* These are the new defines to help align UI elements, the new ones must use a _top/_bottom/ or _left/_right or
 * _width/_height postfix, and should be used to replace the originals whenever possible.
 * If you're able to replace an original, replace all occurrences, and delete the define. */


/* Left sidebar defines */
#define SIDEBAR_WIDTH                      (UTOX_SCALE(115 ))
/* User badge */
    #define SIDEBAR_AVATAR_TOP              (UTOX_SCALE(5  ))
    #define SIDEBAR_AVATAR_LEFT             (UTOX_SCALE(5  ))
    #define SIDEBAR_AVATAR_WIDTH            (UTOX_SCALE(5  ))
    #define SIDEBAR_AVATAR_HEIGHT           (UTOX_SCALE(5  ))
    #define SIDEBAR_NAME_TOP                (UTOX_SCALE(8  ))
    #define SIDEBAR_NAME_LEFT               (UTOX_SCALE(32 ))
    #define SIDEBAR_NAME_WIDTH              (UTOX_SCALE(95 ))
    #define SIDEBAR_NAME_HEIGHT             (UTOX_SCALE(9  ))
    #define SIDEBAR_STATUSMSG_TOP           (UTOX_SCALE(16 ))
    #define SIDEBAR_STATUSMSG_LEFT          (UTOX_SCALE(32 ))
    #define SIDEBAR_STATUSMSG_WIDTH         (UTOX_SCALE(95 ))
    #define SIDEBAR_STATUSMSG_HEIGHT        (UTOX_SCALE(6  ))
    #define SELF_STATUS_ICON_LEFT           (UTOX_SCALE(96 ))
    #define SELF_STATUS_ICON_TOP            (UTOX_SCALE(5  ))

/* Sidebar buttons and settings */
    #define SIDEBAR_FILTER_FRIENDS_TOP      (UTOX_SCALE(30 ))
    #define SIDEBAR_FILTER_FRIENDS_LEFT     ( UTOX_SCALE(5 ))
    #define SIDEBAR_FILTER_FRIENDS_WIDTH    (UTOX_SCALE(84 ))
    #define SIDEBAR_FILTER_FRIENDS_HEIGHT   ( UTOX_SCALE(6 ))

/* Roster defines */
    #define ROSTER_TOP                      ( UTOX_SCALE(39 ))
    #define ROSTER_LEFT                     (  UTOX_SCALE(8 ))
    #define ROSTER_BOTTOM                   (-UTOX_SCALE(15 ))
    #define ROSTER_BOX_LEFT                 (  UTOX_SCALE(4 ))
    #define ROSTER_BOX_HEIGHT               ( UTOX_SCALE(25 ))
    #define ROSTER_AVATAR_TOP               (  UTOX_SCALE(5 ) / 2)
    #define ROSTER_AVATAR_LEFT              (  4 + UTOX_SCALE(5 ) / 2)

    #define ROSTER_NAME_TOP                 (UTOX_SCALE(6  ))
    #define ROSTER_NAME_LEFT                (UTOX_SCALE(30 ))
    #define ROSTER_STATUS_MSG_TOP           (UTOX_SCALE(13 ))

/* Sidebar Lower search box and setting button */
    #define SIDEBAR_SEARCH_TOP              (-UTOX_SCALE(15 ))
    #define SIDEBAR_SEARCH_LEFT             (  UTOX_SCALE(0 ))
    #define SIDEBAR_SEARCH_WIDTH            (UTOX_SCALE(100 ))
    #define SIDEBAR_SEARCH_HEIGHT           ( UTOX_SCALE(15 ))

    #define SIDEBAR_BUTTON_TOP              (-UTOX_SCALE(15 ))
    #define SIDEBAR_BUTTON_LEFT             (UTOX_SCALE(100 ))
    #define SIDEBAR_BUTTON_WIDTH            ( UTOX_SCALE(15 ))
    #define SIDEBAR_BUTTON_HEIGHT           ( UTOX_SCALE(15 ))


/* Main box/Chat box size settings */
#define CHAT_BOX_TOP                    (-UTOX_SCALE(26 )) /* size of the bottom message box */
#define MAIN_TOP_FRAME_THIN             ( UTOX_SCALE(15 ))
#define MAIN_TOP_FRAME_THICK            ( UTOX_SCALE(30 ))

/* Global UI size settings... */
#define SCROLL_WIDTH                    (UTOX_SCALE(4   )) //must be divisible by 2
#define FILE_TRANSFER_BOX_HEIGHT        (UTOX_SCALE(14  ))



/* Main panel defines */
#define MAIN_LEFT                       (UTOX_SCALE(115 )) + 1
#define MAIN_TOP                        ( UTOX_SCALE(30 ))

/* Legacy defines, instead of using these, you should replace them with something more descriptive */
#define LIST_Y2                         (UTOX_SCALE(43 ))
#define LIST_BUTTON_Y                   (-UTOX_SCALE(13 ))
#define MESSAGES_SPACING                (UTOX_SCALE(2))
#define MESSAGES_X                      (UTOX_SCALE(55 ))
#define TIME_WIDTH                      (UTOX_SCALE(20 ))
#define ACTUAL_TIME_WIDTH               (UTOX_SCALE(16 ))
#define NAME_OFFSET                     (UTOX_SCALE(7 ))



/* main */
//#define MAIN_X
//#define MAIN_Y LIST_Y

/* colors */
#define C_RED                   RGB(200, 78, 78)
#define C_SCROLL                GRAY(209)
