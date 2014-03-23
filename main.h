
//todo:
//-make it so window resizes correctly when hitting screen borders, and doesnt maximize over taskbar
//-core_postmessage without volatiles (?)

#ifndef MAIN_H
#define MAIN_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#define _WIN32_WINNT 0x500
#define CLEARTYPE_QUALITY 5

#include <process.h>
#include <windows.h>
#include <windowsx.h>

#include <tox.h>

typedef struct
{
    uint32_t mstart, mend;
    uint16_t start, end;
}MSGSEL;

#include "core.h"
#include "draw.h"
#include "friends.h"
#include "groups.h"
#include "list.h"
#include "sysmenu.h"
#include "messages.h"
#include "edit.h"
#include "button.h"
#include "colors.h"
#include "icons.h"
#include "bootstrap.h"
#include "strings.h"

#define thread(func, args) _beginthread(func, 0, args)
#define countof(x) (sizeof(x)/sizeof(*(x)))

#define WM_NOTIFYICON   (WM_APP + 0)
#define WM_FREQUEST     (WM_APP + 1)
#define WM_FMESSAGE     (WM_APP + 2)
#define WM_FACTION      (WM_APP + 3)
#define WM_FNAME        (WM_APP + 4)
#define WM_FSTATUSMSG   (WM_APP + 5)
#define WM_FSTATUS      (WM_APP + 6)
#define WM_FTYPING      (WM_APP + 7)
#define WM_FRECEIPT     (WM_APP + 8)
#define WM_FONLINE      (WM_APP + 9)
#define WM_FADD         (WM_APP + 10)
#define WM_FACCEPT      (WM_APP + 11)
#define WM_GADD         (WM_APP + 12)
#define WM_GMESSAGE     (WM_APP + 13)
#define WM_GACTION      (WM_APP + 14)
#define WM_GPEERNAME    (WM_APP + 15)
#define WM_GPEERADD     (WM_APP + 16)
#define WM_GPEERDEL     (WM_APP + 17)

//WM_COMMAND
enum
{
    TRAY_SHOWHIDE,
    TRAY_EXIT,
    EDIT_CUT,
    EDIT_COPY,
    EDIT_PASTE,
    EDIT_DELETE,
    EDIT_SELECTALL,
};

#define updatefriend(fp) list_draw(); if(sitem && fp == sitem->data) {main_draw();}
#define updategroup(gp) list_draw(); if(sitem && gp == sitem->data) {main_draw();}

#define commitdraw(x, y, width, height) BitBlt(main_hdc, x, y, width, height, hdc, x, y, SRCCOPY);

#define SAVE_NAME "tox.data"

#define BORDER 1
#define CAPTION 26

#define MAIN_WIDTH 1280
#define MAIN_HEIGHT 720

#define LIST_X 24
#define LIST_Y 27
#define ITEM_WIDTH 306
#define ITEM_HEIGHT 52
#define SCROLL_X (LIST_X + ITEM_WIDTH + 1)
#define SCROLL_Y LIST_Y
#define SCROLL_WIDTH 9
#define SCROLL_BOTTOM (height - 24)

#define REQ_WIDTH 200
#define REQ_HEIGHT 52
#define REQSCROLL_X (REQ_WIDTH + 1)

#define MAIN_X (SCROLL_X + SCROLL_WIDTH + 1)
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

volatile _Bool tox_thread_b, tox_thread_msg;
uint8_t tox_address_string[TOX_FRIEND_ADDRESS_SIZE * 2];

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

//resources

HBRUSH white, border, black, gray, gray2, gray3, gray5, gray6, blue, red, red2, green;

//fonts
HFONT font_big, font_big2, font_med, font_med2, font_small;
int font_small_lineheight;

//sysmenu icons
HBITMAP bm_minimize, bm_restore, bm_maximize, bm_exit;

//other
HBITMAP bm_corner, bm_plus, bm_plus2;

//me
uint8_t status;
uint32_t name_length, status_length;
uint8_t name[128], statusmsg[1024];


//add friend page
uint8_t addfriend_status;

//edit boxes
EDIT *sedit;

struct
{
    uint16_t start, length;
    uint16_t p1, p2;
}edit_sel;
_Bool edit_select;

uint8_t edit_name_data[128], edit_status_data[128], edit_addid_data[TOX_FRIEND_ADDRESS_SIZE * 2], edit_addmsg_data[1024], edit_msg_data[1024];

EDIT edit_name = {
    .multiline = 0,
    .maxlength = 128,
    .x = MAIN_X + 32,
    .y = MAIN_Y + 74,
    .bottom = MAIN_Y + 98,
    .data = edit_name_data,
    .onenter = edit_name_onenter
    },

    edit_status = {
    .multiline = 0,
    .maxlength = 128,
    .x = MAIN_X + 32,
    .y = MAIN_Y + 124,
    .bottom = MAIN_Y + 148,
    .data = edit_status_data,
    .onenter = edit_status_onenter
    },

    edit_addid = {
    .multiline = 0,
    .maxlength = sizeof(edit_addid_data),
    .x = MAIN_X + 32,
    .y = MAIN_Y + 74,
    .bottom = MAIN_Y + 98,
    .data = edit_addid_data,
    },

    edit_addmsg = {
    .multiline = 0,//1,
    .maxlength = sizeof(edit_addmsg_data),
    .x = MAIN_X + 32,
    .y = MAIN_Y + 124,
    .bottom = MAIN_Y + 148 + 70,
    .data = edit_addmsg_data,
    },

    edit_msg = {
    .multiline = 0,//1,
    .maxlength = sizeof(edit_msg_data),
    .x = MAIN_X,
    .data = edit_msg_data,
    .onenter = edit_msg_onenter
    };

#define BUTTON_TEXT(x) .text = (uint8_t*)x, .text_length = sizeof(x) - 1

BUTTON button_copyid = {
    .x = MAIN_X + 32,
    .y = MAIN_Y + 200,
    .width = 150,
    .height = 18,
    .onpress = button_copyid_onpress,
    BUTTON_TEXT("copy to clipboard")
    },

    button_addfriend = {
    .width = 50,
    .height = 18,
    .onpress = button_addfriend_onpress,
    BUTTON_TEXT("add")
    },

    button_newgroup = {
    .x = MAIN_X + 32,
    .y = MAIN_Y,
    .width = 50,
    .height = 18,
    .onpress = button_newgroup_onpress,
    BUTTON_TEXT("add")
    };

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

///
_Bool strtoid(uint8_t *w, uint8_t *a)
{
    uint8_t *end = w + TOX_FRIEND_ADDRESS_SIZE;
    while(w != end)
    {
        uint8_t c, v;

        c = *a++;
        if(c >= '0' && c <= '9')
        {
            v = (c - '0') << 4;
        }
        else if(c >= 'A' && c <= 'F')
        {
            v = (c - 'A' + 10) << 4;
        }
        else
        {
            return 0;
        }

        c = *a++;
        if(c >= '0' && c <= '9')
        {
            v |= (c - '0');
        }
        else if(c >= 'A' && c <= 'F')
        {
            v |= (c - 'A' + 10);
        }
        else
        {
            return 0;
        }

        *w++ = v;
    }

    return 1;
}

void drawtextwidth(int x, int width, int y, uint8_t *str, uint16_t length)
{
    RECT r = {x, y, x + width, y + 30};
    DrawText(hdc, (char*)str, length, &r, DT_SINGLELINE | DT_END_ELLIPSIS);
}

void drawtextrange(int x, int x2, int y, uint8_t *str, uint16_t length)
{
    RECT r = {x, y, x2, y + 30};
    DrawText(hdc, (char*)str, length, &r, DT_SINGLELINE | DT_END_ELLIPSIS);
}

int drawtextrect(int x, int y, int right, int bottom, uint8_t *str, uint16_t length)
{
    RECT r = {x, y, right, bottom};
    return DrawText(hdc, (char*)str, length, &r, DT_WORDBREAK);
}

/*int drawtextrect2(int x, int y, int right, int bottom, uint8_t *str, uint16_t length)
{
    RECT r = {x, y, right, bottom};
    DrawText(hdc, (char*)str, length, &r, DT_WORDBREAK | DT_CALCRECT);
    return r.right - r.left;
}*/

void drawrect(int x, int y, int width, int height, uint32_t color)
{
    RECT r = {x, y, x + width, y + height};
    SetDCBrushColor(hdc, color);
    FillRect(hdc, &r, hdc_brush);
}

void drawhline(int x, int y, int width, uint32_t color)
{
    RECT r = {x, y, x + width, y + 1};
    SetDCBrushColor(hdc, color);
    FillRect(hdc, &r, hdc_brush);
}

void fillrect(RECT *r, uint32_t color)
{
    SetDCBrushColor(hdc, color);
    FillRect(hdc, r, hdc_brush);
}

void framerect(RECT *r, uint32_t color)
{
    SetDCBrushColor(hdc, color);
    FrameRect(hdc, r, hdc_brush);
}

void sprint_address(uint8_t *a, uint8_t *p)
{
    uint8_t b, c, *end = p + TOX_FRIEND_ADDRESS_SIZE;

    while(p != end)
    {
        b = *p++;

        c = (b & 0xF);
        b = (b >> 4);

        if(b < 10)
        {
            *a++ = b + '0';
        }
        else
        {
            *a++ = b - 10 + 'A';
        }

        if(c < 10)
        {
            *a++ = c + '0';
        }
        else
        {
            *a++ = c  - 10 + 'A';
        }
    }
}

void address_to_clipboard(void)
{
    #define size sizeof(tox_address_string)

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size + 1);
    uint8_t *p = GlobalLock(hMem);
    memcpy(p, tox_address_string, size + 1);
    p[size] = 0;
    GlobalUnlock(hMem);
    OpenClipboard(0);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();

    #undef size
}

void main_draw(void);
void drawall(void);

#include "core.c"
#include "friends.c"
#include "groups.c"
#include "list.c"
#include "sysmenu.c"
#include "messages.c"
#include "edit.c"
#include "button.c"

#endif
