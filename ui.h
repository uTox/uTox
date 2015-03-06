
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

extern PANEL panel_main, panel_item[];
extern MESSAGES messages_friend, messages_group;
extern EDIT edit_name, edit_status, edit_addid, edit_addmsg, edit_msg, edit_search, edit_proxy_ip, edit_proxy_port;
extern SCROLLABLE scroll_list;
extern BUTTON button_add, button_settings, button_transfer;

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

#define SEARCH_Y (31 * SCALE) - 1

/* main */
//#define MAIN_X
//#define MAIN_Y LIST_Y

/* colors */
#define C_RED                   RGB(200, 78, 78)
#define C_GRAY                  GRAY(209)
#define C_SCROLL                GRAY(209)
