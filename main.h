#ifndef MAIN_H
#define MAIN_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <string.h>

#include <tox/tox.h>
#include <tox/toxav.h>
#include <vpx/vpx_codec.h>

#define debug(...) printf(__VA_ARGS__)
#define countof(x) (sizeof(x)/sizeof(*(x)))
#define volatile(x) (*((volatile typeof(x)*)&x))

#define IPV6_ENABLED 1
#define DEFAULT_NAME "Tox User"
#define DEFAULT_STATUS "Toxing on uTox"
#define DEFAULT_ADD_MESSAGE "Please accept this friend request."
#define DEFAULT_SCALE 2

typedef struct
{
    uint16_t length;
    uint8_t id[TOX_CLIENT_ID_SIZE], msg[1];
}FRIENDREQ;

typedef struct
{
    uint32_t n, width, height, id;
    uint16_t istart, start, iend, end;
    void **data;
    double scroll;
}MSG_DATA;

typedef struct groupchat GROUPCHAT;
typedef struct friend FRIEND;
typedef struct edit_change EDIT_CHANGE;

typedef struct msg_file MSG_FILE;

typedef uint8_t char_t;

#include "tox.h"
#include "dns.h"
#include "friend.h"

#ifdef __WIN32__
#include "win32/main.h"
#else
#include "xlib/main.h"
#endif

#include "ui.h"
#include "svg.h"

#include "list.h"
#include "messages.h"
#include "edit.h"
#include "scrollable.h"
#include "button.h"
#include "dropdown.h"

#include "text.h"
#include "util.h"

struct groupchat
{
    uint32_t peers;
    uint16_t name_length, topic_length, typed_length;
    uint8_t name[128], topic[128]; //static sizes for now
    uint8_t *typed;
    uint8_t *peername[256];

    EDIT_CHANGE *current, *next, *last;

    MSG_DATA msg;
};

volatile _Bool tox_thread_init, av_thread_init;
volatile _Bool video_preview, audio_preview;
_Bool tox_connected;

//friends and groups
//note: assumes array size will always be large enough
FRIEND friend[256];
GROUPCHAT group[1024];
uint32_t friends, groups;

//window
int width, height;
_Bool maximized;

_Bool hand, overtext;

_Bool mdown;

//fonts
//HFONT font_big, font_big2, font_med, font_med2, font_small, font_msg;
int font_small_lineheight, font_msg_lineheight;

uint16_t video_width, video_height;

enum
{
    FONT_TEXT,
    FONT_TITLE,

    FONT_MSG,
    FONT_MSG_NAME,
    FONT_MSG_LINK,

    FONT_SELF_NAME,
    FONT_STATUS,
    FONT_LIST_NAME,

    FONT_MISC,
};

//sysmenu icons
enum
{
    BM_ONLINE,
    BM_AWAY,
    BM_BUSY,
    BM_OFFLINE,
    BM_STATUS_NOTIFY,

    BM_ADD,
    BM_GROUPS,
    BM_TRANSFER,
    BM_SETTINGS,

    BM_LBUTTON,
    BM_SBUTTON,

    BM_CONTACT,
    BM_GROUP,

    BM_FILE,
    BM_CALL,
    BM_VIDEO,

    BM_FT,
    BM_FTM,
    BM_FTB1,
    BM_FTB2,

    BM_NO,
    BM_PAUSE,
    BM_YES,

    BM_SCROLLHALFTOP,
    BM_SCROLLHALFBOT,
    BM_STATUSAREA,
};

void drawalpha(int bm, int x, int y, int width, int height, uint32_t color);
void loadalpha(int bm, void *data, int width, int height);
void setscale(void);

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

#define BORDER 1
#define CAPTION 26

#define MAIN_WIDTH 800
#define MAIN_HEIGHT 600

#define inrect(x, y, rx, ry, width, height) ((x) >= (rx) && (y) >= (ry) && (x) < ((rx) + (width)) && (y) < ((ry) + (height)))

#define strcmp2(x, y) (memcmp(x, y, sizeof(y) - 1))
#define strcpy2(x, y) (memcpy(x, y, sizeof(y) - 1))

void postmessage(uint32_t msg, uint16_t param1, uint16_t param2, void *data);

/* draw functions*/
void drawtext(int x, int y, uint8_t *str, uint16_t length);
int drawtext_getwidth(int x, int y, uint8_t *str, uint16_t length);
void drawtextwidth(int x, int width, int y, uint8_t *str, uint16_t length);
void drawtextwidth_right(int x, int width, int y, uint8_t *str, uint16_t length);
void drawtextrange(int x, int x2, int y, uint8_t *str, uint16_t length);
void drawtextrangecut(int x, int x2, int y, uint8_t *str, uint16_t length);

int textwidth(uint8_t *str, uint16_t length);
int textfit(uint8_t *str, uint16_t length, int width);

void framerect(int x, int y, int right, int bottom, uint32_t color);
void drawrect(int x, int y, int right, int bottom, uint32_t color);
void drawrectw(int x, int y, int width, int height, uint32_t color);

void drawhline(int x, int y, int x2, uint32_t color);
void drawvline(int x, int y, int y2, uint32_t color);
#define drawpixel(x, y, color) drawvline(x, y, (y) + 1, color)

void setfont(int id);
uint32_t setcolor(uint32_t color);
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

void video_frame(uint32_t id, vpx_image_t *frame);
void video_begin(uint32_t id, uint8_t *name, uint16_t name_length, uint16_t width, uint16_t height);
void video_end(uint32_t id);

void* video_detect(void);
_Bool video_init(void *handle);
void video_close(void *handle);
_Bool video_getframe(vpx_image_t *image);
_Bool video_startread(void);
_Bool video_endread(void);

#define drawstr(x, y, str) drawtext(x, y, (uint8_t*)str, sizeof(str) - 1)
#define drawstr_getwidth(x, y, str) drawtext_getwidth(x, y, (uint8_t*)str, sizeof(str) - 1)
#define strwidth(x) textwidth((uint8_t*)x, sizeof(x) - 1)

#endif
