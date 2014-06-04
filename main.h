#ifndef MAIN_H
#define MAIN_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x500
#endif

#ifndef WINVER
#define WINVER 0x410
#endif

#define CLEARTYPE_QUALITY 5

#include <tox/tox.h>

#include <process.h>
#include <windows.h>
#include <windowsx.h>

#define IPV6_ENABLED 1
#define DEFAULT_NAME "NSA"
#define DEFAULT_STATUS "Adding backdoors to Tox"

typedef struct
{
    uint32_t mstart, mend;
    uint16_t start, end;
}MSGSEL;

typedef struct
{
    uint16_t length;
    uint8_t id[TOX_CLIENT_ID_SIZE], msg[1];
}FRIENDREQ;

typedef struct
{
    uint32_t peers, msg;
    double scroll;
    //MESSAGES m;
    MSGSEL sel;
    uint16_t name_length, topic_length, typed_length;
    uint8_t name[128], topic[128]; //static sizes for now
    uint8_t *typed;
    uint8_t *peername[256];
    void **message;
}GROUPCHAT;

#include "tox.h"
#include "dns.h"
#include "ui.h"
#include "friend.h"
#include "list.h"
#include "sysmenu.h"
#include "messages.h"
#include "edit.h"
#include "scrollable.h"
#include "button.h"
#include "util.h"

#define debug(...) printf(__VA_ARGS__)
#define thread(func, args) _beginthread(func, 0, args)
#define countof(x) (sizeof(x)/sizeof(*(x)))
#define volatile(x) (*((volatile typeof(x)*)&x))

#define WM_NOTIFYICON   (WM_APP + 0)
#define WM_TOX          (WM_APP + 1)
#define postmessage(msg, param1, param2, data) PostMessage(hwnd, WM_TOX + (msg), ((param1) << 16) | (param2), (LPARAM)data)
#define yieldcpu() Sleep(1)

#define get_time() ((uint64_t)clock() * 1000 * 1000)

//WM_COMMAND
enum
{
    TRAY_SHOWHIDE,
    TRAY_EXIT,
    TRAY_STATUS_AVAILABLE,
    TRAY_STATUS_AWAY,
    TRAY_STATUS_BUSY,
    EDIT_CUT,
    EDIT_COPY,
    EDIT_PASTE,
    EDIT_DELETE,
    EDIT_SELECTALL,
    LIST_DELETE,
    LIST_ACCEPT
};

#define BORDER 1
#define CAPTION 26

#define MAIN_WIDTH 800
#define MAIN_HEIGHT 600

#define LIST_Y 12
#define ITEM_HEIGHT 49
#define SCROLL_X (LIST_X + ITEM_WIDTH + 1)
#define SCROLL_Y LIST_Y
#define SCROLL_WIDTH 9
#define SCROLL_BOTTOM (height - 13)

#define MAIN_X (SCROLL_X + SCROLL_WIDTH + 13)
#define MAIN_Y 27

#define MESSAGES_X (MAIN_X + 1)
#define MESSAGES_Y (MAIN_Y + 47)
#define MESSAGES_RIGHT (width - 24)
#define MESSAGES_BOTTOM (height - 152)

#define PEERS_WIDTH 100
#define NAMES_WIDTH 100

#define GMESSAGES_RIGHT (MESSAGES_RIGHT - PEERS_WIDTH)
#define MESSAGES_WIDTH (MESSAGES_RIGHT - MESSAGES_X - NAMES_WIDTH)
#define GMESSAGES_WIDTH (GMESSAGES_RIGHT - MESSAGES_X - NAMES_WIDTH)

volatile _Bool tox_thread_init;
_Bool tox_connected;

//friends and groups
//note: assumes array size will always be large enough
FRIEND friend[256];
GROUPCHAT group[64];
uint32_t friends, groups;

//window
HWND hwnd;
HDC main_hdc, hdc, hdcMem;
HBRUSH hdc_brush;
HBITMAP hdc_bm;

int width, height;
_Bool maximized;

//fonts
HFONT font_big, font_big2, font_med, font_med2, font_small;
int font_small_lineheight;

enum
{
    FONT_TEXT,
    FONT_TEXT_LARGE,
    FONT_BUTTON,
    FONT_TITLE,
    FONT_SUBTITLE,
    FONT_MED,
};

//sysmenu icons
enum
{
    BM_MINIMIZE,
    BM_RESTORE,
    BM_MAXIMIZE,
    BM_EXIT,
    BM_CORNER,
    BM_PLUS,
    BM_ONLINE,
    BM_AWAY,
    BM_BUSY,
    BM_OFFLINE,
    BM_CONTACT,
    BM_GROUP
};

HBITMAP bitmap[16];

//me
struct
{
    uint8_t status;
    uint16_t name_length, statusmsg_length;
    uint8_t *statusmsg, name[TOX_MAX_NAME_LENGTH];
    uint8_t id[TOX_FRIEND_ADDRESS_SIZE * 2];
}self;

//add friend page
uint8_t addfriend_status;

struct
{
    uint16_t start, length;
    uint16_t p1, p2;
}edit_sel;
_Bool edit_select;

//messages
struct
{
    union
    {
        MSGSEL m;
        struct
        {
            uint32_t mstart, mend;
            uint16_t start, end;
        };
    };
    uint32_t mp;
    uint16_t p;
}msg_sel;

#define inrect(x, y, rx, ry, width, height) ((x) >= (rx) && (y) >= (ry) && (x) < (rx) + (width) && (y) < (ry + height))


/* draw functions*/
void drawbitmap(int bm, int x, int y, int width, int height);
void drawbitmaptrans(int bm, int x, int y, int width, int height);
void drawbitmapalpha(int bm, int x, int y, int width, int height);
#define drawtext(x, y, str, len) TextOut(hdc, x, y, (char*)(str), len)
#define drawstr(x, y, str) TextOut(hdc, x, y, str, sizeof(str) - 1)
void drawtextwidth(int x, int width, int y, uint8_t *str, uint16_t length);
void drawtextrange(int x, int x2, int y, uint8_t *str, uint16_t length);
void drawtextrangecut(int x, int x2, int y, uint8_t *str, uint16_t length);
int drawtextrect(int x, int y, int right, int bottom, uint8_t *str, uint16_t length);
void drawrect(int x, int y, int width, int height, uint32_t color);
void drawhline(int x, int y, int x2, uint32_t color);
void drawvline(int x, int y, int y2, uint32_t color);
void fillrect(RECT *r, uint32_t color);
void framerect(RECT *r, uint32_t color);
void setfont(int id);
void setcolor(uint32_t color);
void setbkcolor(uint32_t color);
void setbgcolor(uint32_t color);
void pushclip(int x, int y, int width, int height);
void popclip(void);
void enddraw(int x, int y, int width, int height);
void address_to_clipboard(void);
void editpopup(void);
void listpopup(uint8_t item);

#endif
