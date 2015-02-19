#ifndef MAIN_H
#define MAIN_H

/* Support for large files. */
#define _LARGEFILE_SOURCE
#define _FILE_OFFSET_BITS 64

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <ctype.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <tox/tox.h>
#include <tox/toxav.h>
#include <vpx/vpx_codec.h>

#define countof(x) (sizeof(x)/sizeof(*(x)))
#define volatile(x) (*((volatile typeof(x)*)&x))

#define DEFAULT_NAME "Tox User"
#define DEFAULT_STATUS "Toxing on uTox"
#define DEFAULT_SCALE 2

#define TITLE "uTox"
#define SUB_TITLE "(Alpha)"
#define VERSION "0.2.n"

#define MAX_CALLS 16
#define MAX_BACKLOG_MESSAGES 128

typedef struct
{
    uint8_t version, scale, enableipv6, disableudp;
    uint16_t window_x, window_y, window_width, window_height;
    uint16_t proxy_port;
    uint8_t proxyenable;
    uint8_t logging_enabled : 1;
    uint8_t audible_notifications_enabled : 1;
    uint8_t filter : 1;
    uint8_t audio_filtering_enabled : 1;
    uint8_t close_to_tray : 1;
    uint8_t start_in_tray : 1;
    uint8_t zero : 2;
    uint16_t audio_device_in;
    uint16_t audio_device_out;
    uint16_t unused[32];
    uint8_t proxy_ip[0];
}UTOX_SAVE;

#define SAVE_VERSION 3

typedef struct
{
    uint16_t length;
    uint8_t id[TOX_FRIEND_ADDRESS_SIZE], msg[0];
}FRIENDREQ;

typedef struct {
    // Castless wrapper for lodepng data arguments.
    unsigned char png_data[0];
} *UTOX_PNG_IMAGE;

typedef struct edit_change EDIT_CHANGE;

#include "unused.h"

#include "png/png.h"

#include "tox.h"

#ifdef __WIN32__
#include "win32/main.h"
#else
#ifdef __ANDROID__
#include "android/main.h"
#else
#include "xlib/main.h"
#endif
#endif

#include "sized_string.h"
#include "ui_i18n_decls.h"

#include "ui.h"
#include "svg.h"

#include "avatar.h"
#include "messages.h"
#include "dns.h"
#include "transfer.h"
#include "friend.h"
#include "list.h"
#include "edit.h"
#include "scrollable.h"
#include "button.h"
#include "dropdown.h"
#include "contextmenu.h"
#include "tooltip.h"

#include "text.h"
#include "util.h"

#include "ui_dropdown.h"

volatile _Bool tox_thread_init, audio_thread_init, video_thread_init, toxav_thread_init;
_Bool tox_connected;

_Bool audio_preview, video_preview;

volatile _Bool logging_enabled, audible_notifications_enabled, audio_filtering_enabled, close_to_tray, start_in_tray;

volatile uint16_t loaded_audio_in_device, loaded_audio_out_device;

#define MAX_NUM_FRIENDS 256
#define MAX_NUM_GROUPS 512

//friends and groups
//note: assumes array size will always be large enough
FRIEND friend[MAX_NUM_FRIENDS];
GROUPCHAT group[MAX_NUM_GROUPS];
uint32_t friends, groups;

//window
int utox_window_width, utox_window_height, utox_window_baseline;
//todo use this to size the inturrutp as a portion of the display size
//int utox_window_width, utox_window_height, utox_window_baseline;
_Bool utox_window_maximized;

enum
{
    CURSOR_NONE,
    CURSOR_TEXT,
    CURSOR_HAND,
    CURSOR_SELECT,
    CURSOR_ZOOM_IN,
    CURSOR_ZOOM_OUT,
};
uint8_t cursor;

_Bool mdown;

struct {
    int x, y;
} mouse;

//fonts
//HFONT font_big, font_big2, font_med, font_med2, font_small, font_msg;
int font_small_lineheight, font_msg_lineheight;

uint16_t video_width, video_height, max_video_width, max_video_height;

Tox_Options options;

enum
{
    FONT_TEXT,
    FONT_TITLE,

    /*FONT_MSG,
    FONT_MSG_NAME,
    FONT_MSG_LINK,*/

    FONT_SELF_NAME,
    FONT_STATUS,
    FONT_LIST_NAME,

    FONT_MISC,
};

//sysmenu icons
enum
{
    BM_ONLINE = 1,
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
    BM_RESUME,
    BM_YES,

    BM_SCROLLHALFTOP,
    BM_SCROLLHALFBOT,
    BM_STATUSAREA,

    BM_CB1,
    BM_CB2,
    BM_CHAT_SEND,
    BM_CHAT_SEND_OVERLAY,
    BM_CI1
};

#define isdesktop(x) ((size_t)(x) == 1)

void drawalpha(int bm, int x, int y, int width, int height, uint32_t color);
void loadalpha(int bm, void *data, int width, int height);
void desktopgrab(_Bool video);
void notify(char_t *title, STRING_IDX title_length, char_t *msg, STRING_IDX msg_length, uint8_t *cid);
void setscale(void);

enum {
    FILTER_NEAREST, // ugly and quick filtering
    FILTER_BILINEAR // prettier and a bit slower filtering
};
/* set filtering method used when resizing given image to one of above enum */
void image_set_filter(UTOX_NATIVE_IMAGE *image, uint8_t filter);

/* set scale of image so that when it's drawn it will be `scale' times as large(2.0 for double size, 0.5 for half, etc.)
 *  notes: theoretically lowest possible scale is (1.0/65536.0), highest is 65536.0, values outside of this range will create weird issues
 *         scaling will be rounded to pixels, so it might not be exact
 */
void image_set_scale(UTOX_NATIVE_IMAGE *image, double scale);

/* draws an utox image with or without alpha channel into the rect of (x,y,width,height) on the screen,
 * starting at position (imgx,imgy) of the image
 * WARNING: Windows can fail to show the image at all if the rect (imgx,imgy,width,height) contains even 1 pixel outside of
 * the image's size AFTER SCALING, so be careful.
 * TODO: improve this so this function is safer to use */
void draw_image(const UTOX_NATIVE_IMAGE *image, int x, int y, uint32_t width, uint32_t height, uint32_t imgx, uint32_t imgy);

/* converts a png to a UTOX_NATIVE_IMAGE, returns a pointer to it, keeping alpha channel only if keep_alpha is 1 */
UTOX_NATIVE_IMAGE *png_to_image(const UTOX_PNG_IMAGE, size_t size, uint16_t *w, uint16_t *h, _Bool keep_alpha);

/* free an image created by png_to_image */
void image_free(UTOX_NATIVE_IMAGE *image);

void showkeyboard(_Bool show);

void redraw_utox();
void redraw_interrupt();
/* deprecated */
void redraw(void);
void update_tray(void);
void force_redraw(void); // TODO: as parameter for redraw()?

int datapath_old(uint8_t *dest);
int datapath(uint8_t *dest);

/* gets a subdirectory of tox's datapath and puts the full pathname in dest,
 * returns number of characters written */
int datapath_subdir(uint8_t *dest, const char *subdir);
void flush_file(FILE *file);
int ch_mod(uint8_t *file);
void config_osdefaults(UTOX_SAVE *r);

void incoming_call_inturrupt();

//me
struct
{
    uint8_t status;
    STRING_IDX name_length, statusmsg_length;
    char_t *statusmsg, name[TOX_MAX_NAME_LENGTH];
    char_t id[TOX_FRIEND_ADDRESS_SIZE * 2];
    AVATAR avatar;
}self;

//add friend page
uint8_t addfriend_status;

#define BORDER 1
#define CAPTION 26

#define MAIN_WIDTH 800
#define MAIN_HEIGHT 600
#define INTERRUPT_WIDTH 300
#define INTERRUPT_HEIGHT 200

#define inrect(x, y, rx, ry, width, height) ((x) >= (rx) && (y) >= (ry) && (x) < ((rx) + (width)) && (y) < ((ry) + (height)))

#define strcmp2(x, y) (memcmp(x, y, sizeof(y) - 1))
#define strcpy2(x, y) (memcpy(x, y, sizeof(y) - 1))

void postmessage(uint32_t msg, uint16_t param1, uint16_t param2, void *data);

/* draw functions*/
void drawtext(int x, int y, char_t *str, STRING_IDX length);
int drawtext_getwidth(int x, int y, char_t *str, STRING_IDX length);
void drawtextwidth(int x, int width, int y, char_t *str, STRING_IDX length);
void drawtextwidth_right(int x, int width, int y, char_t *str, STRING_IDX length);
void drawtextrange(int x, int x2, int y, char_t *str, STRING_IDX length);
void drawtextrangecut(int x, int x2, int y, char_t *str, STRING_IDX length);

int textwidth(char_t *str, STRING_IDX length);
int textfit(char_t *str, STRING_IDX length, int width);
int textfit_near(char_t *str, STRING_IDX length, int width);
//TODO: Seems to be unused. Remove?
int text_drawline(int x, int right, int y, uint8_t *str, int i, int length, int highlight, int hlen, uint16_t lineheight);

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

void copy(int value);
void paste(void);

void address_to_clipboard(void);
void openurl(char_t *str);
void openfilesend(void);

/* use the file chooser to pick an avatar and set it as the user's */
void openfileavatar(void);
void savefilerecv(uint32_t fid, MSG_FILE *file);
void savefiledata(MSG_FILE *file);

void sysmexit(void);
void sysmsize(void);
void sysmmini(void);

void setselection(char_t *data, STRING_IDX length);

void video_frame(uint32_t id, uint8_t *img_data, uint16_t width, uint16_t height, _Bool resize);
void video_begin(uint32_t id, char_t *name, STRING_IDX name_length, uint16_t width, uint16_t height);
void video_end(uint32_t id);

void* video_detect(void);
_Bool video_init(void *handle);
void video_close(void *handle);
int video_getframe(vpx_image_t *image);
_Bool video_startread(void);
_Bool video_endread(void);

void audio_detect(void);
_Bool audio_init(void *handle);
_Bool audio_close(void *handle);
_Bool audio_frame(int16_t *buffer);

ToxAv* global_av;

void audio_play(int32_t call_index, const int16_t *data, int length, uint8_t channels);
void audio_begin(int32_t call_index);
void audio_end(int32_t call_index);

#define drawstr(x, y, i) drawtext(x, y, S(i), SLEN(i))
#define drawstr_getwidth(x, y, str) drawtext_getwidth(x, y, (char_t*)str, sizeof(str) - 1)
#define strwidth(x) textwidth((char_t*)x, sizeof(x) - 1)

#endif
