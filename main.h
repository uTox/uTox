#ifndef MAIN_H
#define MAIN_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <string.h>

#include <tox/tox.h>

#define debug(...) printf(__VA_ARGS__)
#define countof(x) (sizeof(x)/sizeof(*(x)))
#define volatile(x) (*((volatile typeof(x)*)&x))

#define IPV6_ENABLED 0
#define DEFAULT_NAME "Tox User"
#define DEFAULT_STATUS "Toxing on winTox"

typedef struct
{
    uint16_t length;
    uint8_t id[TOX_CLIENT_ID_SIZE], msg[1];
}FRIENDREQ;

typedef struct
{
    uint32_t n, height, id;
    uint16_t istart, start, iend, end;
    void **data;
    double scroll;
}MSG_DATA;

typedef struct
{
    uint32_t peers;
    uint16_t name_length, topic_length, typed_length;
    uint8_t name[128], topic[128]; //static sizes for now
    uint8_t *typed;
    uint8_t *peername[256];

    MSG_DATA msg;
}GROUPCHAT;

typedef struct msg_file MSG_FILE;

typedef uint8_t char_t;

#ifdef __WIN32__
#include "win32/main.h"
#else
#include "xlib/main.h"
#endif

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

volatile _Bool tox_thread_init;
_Bool tox_connected;

//friends and groups
//note: assumes array size will always be large enough
FRIEND friend[256];
GROUPCHAT group[64];
uint32_t friends, groups;

//window
int width, height;
_Bool maximized;

_Bool hand;

_Bool mdown;

//fonts
//HFONT font_big, font_big2, font_med, font_med2, font_small, font_msg;
int font_small_lineheight, font_msg_lineheight;

enum
{
    FONT_TEXT,
    FONT_TEXT_LARGE,
    FONT_TITLE,
    FONT_SUBTITLE,
    FONT_MED,
    FONT_MSG,
    FONT_MSG_NAME,
    FONT_MSG_LINK
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
    BM_GROUP,
    BM_FILE
};


//me
struct
{
    uint8_t status;
    uint16_t name_length, statusmsg_length;
    char_t *statusmsg, name[TOX_MAX_NAME_LENGTH];
    char_t id[TOX_FRIEND_ADDRESS_SIZE * 2];
}self;

//add friend page
uint8_t addfriend_status;

struct
{
    uint16_t start, length;
    uint16_t p1, p2;
}edit_sel;
_Bool edit_select;

#define BORDER 1
#define CAPTION 26

#define MAIN_WIDTH 800
#define MAIN_HEIGHT 600

#define inrect(x, y, rx, ry, width, height) ((x) >= (rx) && (y) >= (ry) && (x) < (rx) + (width) && (y) < (ry + height))

#define strcmp2(x, y) (memcmp(x, y, sizeof(y) - 1))
#define strcpy2(x, y) (memcpy(x, y, sizeof(y) - 1))

void postmessage(uint32_t msg, uint16_t param1, uint16_t param2, void *data);

/* draw functions*/
void drawbitmap(int bm, int x, int y, int width, int height);
void drawbitmaptrans(int bm, int x, int y, int width, int height);
void drawbitmapalpha(int bm, int x, int y, int width, int height);

void drawtext(int x, int y, uint8_t *str, uint16_t length);
void drawtextW(int x, int y, char_t *str, uint16_t length);
int drawtext_getwidth(int x, int y, uint8_t *str, uint16_t length);
int drawtext_getwidthW(int x, int y, char_t *str, uint16_t length);
void drawtextwidth(int x, int width, int y, uint8_t *str, uint16_t length);
void drawtextwidth_right(int x, int width, int y, uint8_t *str, uint16_t length);
void drawtextwidth_rightW(int x, int width, int y, char_t *str, uint16_t length);
void drawtextrange(int x, int x2, int y, uint8_t *str, uint16_t length);
void drawtextrangecut(int x, int x2, int y, uint8_t *str, uint16_t length);
void drawtextrangecutW(int x, int x2, int y, char_t *str, uint16_t length);

int textwidth(uint8_t *str, uint16_t length);
int textwidthW(char_t *str, uint16_t length);
int textfit(uint8_t *str, uint16_t length, int width);
int textfitW(char_t *str, uint16_t length, int width);


void drawrect(int x, int y, int width, int height, uint32_t color);
void drawhline(int x, int y, int x2, uint32_t color);
void drawvline(int x, int y, int y2, uint32_t color);


void fillrect(RECT *r, uint32_t color);
void framerect(RECT *r, uint32_t color);
void setfont(int id);
uint32_t setcolor(uint32_t color);
void setbkcolor(uint32_t color);
void setbgcolor(uint32_t color);
void pushclip(int x, int y, int width, int height);
void popclip(void);
void enddraw(int x, int y, int width, int height);

/* other */
void thread(void func(void*), void *args);
void yieldcpu(uint32_t ms);
uint64_t get_time(void);

void address_to_clipboard(void);
void editpopup(void);
void listpopup(uint8_t item);
void openurl(char_t *str);
void openfilesend(void);
void savefilerecv(uint32_t fid, MSG_FILE *file);

void sysmexit(void);
void sysmsize(void);
void sysmmini(void);

void setselection(void);

#define drawstr(x, y, str) drawtext(x, y, (uint8_t*)str, sizeof(str) - 1)
#define drawstr_getwidth(x, y, str) drawtext_getwidth(x, y, (uint8_t*)str, sizeof(str) - 1)
#define strwidth(x) textwidth((uint8_t*)x, sizeof(x) - 1)

#endif
