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
typedef struct dropdown DROPDOWN;
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
    ADDF_TOOLONG
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
extern DROPDOWN dropdown_audio_in, dropdown_audio_out, dropdown_video, dropdown_dpi, dropdown_language, dropdown_proxy, dropdown_ipv6, dropdown_udp;

enum {
    LANG_DE,
    LANG_EN,
    LANG_IT,
    LANG_ES,
    LANG_FR,
    LANG_JA,
    LANG_PL,
    LANG_RU,
};

enum {
    //NOT REFERRED TO BY NAME
    REQ_STRING_1,
    REQ_RESOLVE,
    REQ_INVALID_ID,
    REQ_EMPTY_ID,
    REQ_LONG_MSG,
    REQ_NO_MSG,
    REQ_SELF_ID,
    REQ_ALREADY_FRIENDS,
    REQ_UNKNOWN,
    REQ_BAD_CHECKSUM,
    REQ_BAD_NOSPAM,
    REQ_NO_MEMORY,

    FILE_STRING_1,
    TRANSFER_STARTED,
    TRANSFER___,
    TRANSFER_PAUSED,
    TRANSFER_BROKEN,
    TRANSFER_CANCELLED,
    TRANSFER_COMPLETE,

    CALL_STRING_1,
    CALL_STRING_2,
    CALL_STRING_3,
    CALL_STRING_4,

    //REFERRED TO BY NAME
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
};

#define S(x) strings[LANG][STR_##x].str
#define SLEN(x) strings[LANG][STR_##x].length
extern STRING strings[][64];

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
