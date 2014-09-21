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

typedef struct {
    uint8_t *str;
    uint16_t length;
} STRING;

extern PANEL panel_main, panel_item[];
extern MESSAGES messages_friend, messages_group;
extern EDIT edit_name, edit_status, edit_addid, edit_addmsg, edit_msg, edit_search, edit_proxy_ip, edit_proxy_port;
extern SCROLLABLE scroll_list;
extern BUTTON button_add, button_settings, button_transfer;

enum {
    LANG_BG,
    LANG_DE,
    LANG_EN,
    LANG_ES,
    LANG_FR,
    LANG_HI, //5
    LANG_JA,
    LANG_IT,
    LANG_LV,
    LANG_NL,
    LANG_NO, //10
    LANG_PL,
    LANG_RO,
    LANG_RU,
    LANG_TR,
    LANG_UA, //15
    LANG_CN,
    LANG_TW,
    //Do NOT add new values beyond this point, or alter LANGS_MAX accordingly.
    LANGS_MAX = LANG_TW
};

enum {
    STR_REQ_SENT,
    STR_REQ_RESOLVE,
    STR_REQ_INVALID_ID,
    STR_REQ_EMPTY_ID,
    STR_REQ_LONG_MSG,
    STR_REQ_NO_MSG,
    STR_REQ_SELF_ID,
    STR_REQ_ALREADY_FRIENDS,
    STR_REQ_UNKNOWN,
    STR_REQ_BAD_CHECKSUM,
    STR_REQ_BAD_NOSPAM,
    STR_REQ_NO_MEMORY,

    STR_TRANSFER_NEW,
    STR_TRANSFER_STARTED,
    STR_TRANSFER___,
    STR_TRANSFER_PAUSED,
    STR_TRANSFER_BROKEN,
    STR_TRANSFER_CANCELLED,
    STR_TRANSFER_COMPLETE,

    STR_CALL_CANCELLED,
    STR_CALL_INVITED,
    STR_CALL_RINGING,
    STR_CALL_STARTED,

    STR_ADDFRIENDS,
    STR_TOXID,
    STR_MESSAGE,
    STR_SEARCHFRIENDS,
    STR_ADD,
    STR_SWITCHPROFILE,
    STR_FRIENDREQUEST,
    STR_USERSETTINGS,
    STR_NAME,
    STR_STATUSMESSAGE,
    STR_PREVIEW,
    STR_DEVICESELECTION,
    STR_AUDIOINPUTDEVICE,
    STR_AUDIOOUTPUTDEVICE,
    STR_VIDEOINPUTDEVICE,
    STR_OTHERSETTINGS,
    STR_DPI,
    STR_SAVELOCATION,
    STR_LANGUAGE,
    STR_NETWORK,
    STR_IPV6,
    STR_UDP,
    STR_PROXY,
    STR_WARNING,
    STR_LOGGING,
    STR_IS_TYPING,

    STR_COPY,
    STR_COPYWITHOUTNAMES,
    STR_CUT,
    STR_PASTE,
    STR_DELETE,
    STR_SELECTALL,
    STR_REMOVE,
    STR_LEAVE,
    STR_ACCEPT,
    STR_IGNORE,

    STR_CLICKTOSAVE,
    STR_CLICKTOOPEN,
    STR_CANCELLED,

    STR_DPI_TINY,
    STR_DPI_NORMAL,
    STR_DPI_BIG,
    STR_DPI_LARGE,
    STR_DPI_HUGE,

    STR_CONTACTS_FILTER_ALL,
    STR_CONTACTS_FILTER_ONLINE,

    STR_PROXY_DISABLED,
    STR_PROXY_FALLBACK,
    STR_PROXY_ALWAYS_USE,

    STR_NO,
    STR_YES,

    STR_VIDEO_IN_NONE,
    STR_VIDEO_IN_DESKTOP,

    STR_LANG_NATIVE_NAME,
    STR_LANG_ENGLISH_NAME,
    //Do NOT add new values beyond this point, or alter STRS_MAX accordingly.
    STRS_MAX = STR_LANG_ENGLISH_NAME
};

#define DEFAULT_LANG LANG_EN
#define S(x) (ui_gettext(LANG, (STR_##x))->str)
#define SLEN(x) (ui_gettext(LANG, (STR_##x))->length)
#define SPTR(x) (ui_gettext(LANG, (STR_##x)))
#define SPTRFORLANG(l,x) (ui_gettext((l), (x)))
typedef uint8_t UI_LANG_ID;
typedef uint16_t UI_STRING_ID;
STRING* ui_gettext(UI_LANG_ID lang, UI_STRING_ID string_id);

uint8_t LANG;

void ui_scale(uint8_t scale);
void ui_size(int width, int height);

void panel_draw(PANEL *p, int x, int y, int width, int height);

_Bool panel_mmove(PANEL *p, int x, int y, int width, int height, int mx, int my, int dx, int dy);
void panel_mdown(PANEL *p);
_Bool panel_dclick(PANEL *p, _Bool triclick);
_Bool panel_mright(PANEL *p);
_Bool panel_mwheel(PANEL *p, int x, int y, int width, int height, double d);
_Bool panel_mup(PANEL *p);
_Bool panel_mleave(PANEL *p);

extern uint32_t status_color[];

#define GRAY(x) (((x) << 16) | ((x) << 8) | (x))

#define BLACK 0
#define WHITE 0xFFFFFF

#define TEXT_SELF               0x595959
#define TEXT_HIGHLIGHT          WHITE
#define TEXT_HIGHLIGHT_BG       RGB(51, 153, 255)

#define COLOR_TEXT              0x333333
#define COLOR_LINK              RGB(0, 0, 255)

#define BLUE RGB(0x4E, 0xA6,0xEA)

/* search
 */

uint8_t SEARCH;
uint8_t FILTER;
int search_offset[1024];
int search_unset[1024];
char search_data[128];

/* metrics
 */
uint8_t SCALE;

/* side */
#define LIST_X (8 * SCALE)
#define LIST_RIGHT (111 * SCALE)
#define LIST_Y (31 * SCALE)
#define LIST_Y2 (43 * SCALE)
#define LIST_BOTTOM (-18 * SCALE)

#define LIST_NAME_X (37 * SCALE)
#define LIST_NAME_Y (6 * SCALE)

#define LIST_STATUS_X (37 * SCALE)
#define LIST_STATUS_Y (13 * SCALE)

#define LIST_AVATAR_X (LIST_X + 5 * SCALE / 2)
#define LIST_AVATAR_Y (5 * SCALE / 2)

#define LIST_BUTTON_Y (-13 * SCALE)

#define ITEM_HEIGHT (25 * SCALE)

#define SCROLL_WIDTH (4 * SCALE) //must be divisible by 2

#define SELF_NAME_X (32 * SCALE)
#define SELF_NAME_Y (8 * SCALE)

#define SELF_MSG_X (32 * SCALE)
#define SELF_MSG_Y (15 * SCALE)

#define SELF_AVATAR_X (5 * SCALE)
#define SELF_AVATAR_Y (5 * SCALE)

#define SELF_STATUS_X (96 * SCALE)
#define SELF_STATUS_Y (5 * SCALE)

#define MESSAGES_SPACING (SCALE * 2)
#define MESSAGES_X (55 * SCALE)
#define TIME_WIDTH (16 * SCALE)
#define NAME_OFFSET (7 * SCALE)

#define MESSAGES_BOTTOM (-47 * SCALE)

#define SEARCH_Y (31 * SCALE)

/* main */
//#define MAIN_X
//#define MAIN_Y LIST_Y


/* colors
 */

#define C_STATUS                GRAY(209)
#define C_GREEN                 RGB(107, 194, 96)
#define C_GREEN_LIGHT           RGB(118, 213, 106)
#define C_YELLOW                RGB(206, 191, 69)
#define C_YELLOW_LIGHT          RGB(227, 210, 76)
#define C_RED                   RGB(200, 78, 78)
#define C_RED_LIGHT             RGB(220, 86, 86)

#define LIST_MAIN               GRAY(65)
#define LIST_HIGHLIGHT          GRAY(80)
#define LIST_SELECTED           WHITE
#define LIST_DARK               GRAY(28)
#define LIST_DARK_LIGHT         GRAY(40)

#define LIST_EDGE               GRAY(56)
#define LIST_EDGE2              GRAY(196)
#define LIST_EDGE3              GRAY(198)
#define LIST_EDGE4              GRAY(207)
#define LIST_EDGE5              GRAY(219)
#define LIST_EDGE6              GRAY(101)
#define LIST_EDGE7              GRAY(113)

#define C_GRAY                  GRAY(209)
#define C_GRAY2                 GRAY(150)
#define C_SCROLL                GRAY(209)

#define C_TITLE                 GRAY(68)
#define CHAT_SELF               GRAY(89)
