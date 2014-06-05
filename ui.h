enum
{
    PANEL_NONE,
    PANEL_MAIN,
    PANEL_SYSMENU,
    PANEL_MESSAGES,
    PANEL_LIST,
    PANEL_BUTTON,
    PANEL_EDIT,
    PANEL_SCROLLABLE,
};

typedef struct scrollable SCROLLABLE;
typedef struct edit EDIT;
typedef struct panel PANEL;
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

extern PANEL panel_main, panel_item[5];
extern MESSAGES messages_friend, messages_group;
extern EDIT edit_name, edit_status, edit_addid, edit_addmsg, edit_msg;
extern SCROLLABLE scroll_list;

void panel_draw(PANEL *p, int x, int y, int width, int height);

_Bool panel_mmove(PANEL *p, int x, int y, int width, int height, int mx, int my, int dy);
_Bool panel_mdown(PANEL *p);
_Bool panel_mright(PANEL *p);
_Bool panel_mwheel(PANEL *p, int x, int y, int width, int height, double d);
_Bool panel_mup(PANEL *p);
_Bool panel_mleave(PANEL *p);

extern uint8_t bm_contact_bits[], bm_group_bits[], bm_file_bits[];
extern uint8_t bm_minimize_bits[], bm_maximize_bits[], bm_restore_bits[], bm_exit_bits[], bm_plus_bits[];
extern uint32_t bm_online_bits[], bm_away_bits[], bm_busy_bits[], bm_offline_bits[];

#define redraw() panel_draw(&panel_main, 0, 0, width, height)

#define LIST_X 12
#define ITEM_WIDTH 200
#define SIDE_X (LIST_X + ITEM_WIDTH + 23)
#define SIDE_Y 27

#define BLACK 0x000000
#define WHITE 0xFFFFFF
#define GRAY 0xEEEEEE
#define GRAY2 0xCCCCCC
#define GRAY3 0x818181
#define GRAY4 0x333333
#define GRAY5 0xCACACA
#define GRAY6 0xA9A9A9
#define BLUE 0xEAA64E
#define RED 0x4343E0
#define RED2 0x3D3D99
#define GREEN 0x4CB122

#define YELLOW 0x33FFFF

#define GRAY_BORDER 0x999999

#define COLOR_BORDER            0x999999
#define COLOR_BG                WHITE
#define COLOR_SYSMENU           0xCCCCCC
#define COLOR_TEXT              0x333333

#define INNER_BORDER            RGB(167, 215, 249)
#define TEXT_HIGHLIGHT          WHITE
#define TEXT_HIGHLIGHT_BG       RGB(51, 153, 255)
#define BUTTON_AREA             WHITE
#define BUTTON_AREA_HIGHLIGHT   RGB(51, 153, 255)
