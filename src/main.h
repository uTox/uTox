// Versions
#define TITLE         "uTox"
#define SUB_TITLE     "(Alpha)"
#define RELEASE_TITLE "Mild Shock"
#define PATCH_TITLE   "Acting"
#define VERSION       "0.8.2"
#define VER_MAJOR     0
#define VER_MINOR     8
#define VER_PATCH     1

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
#include <pthread.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <tox/tox.h>
#include <tox/toxav.h>
#include <tox/toxencryptsave.h>
#include <vpx/vpx_codec.h>
#include <vpx/vpx_image.h>

#ifdef EMOJI_IDS
#include <base_emoji.h>
#endif


// Defaults
#define DEFAULT_NAME   "Tox User"
#define DEFAULT_STATUS "Toxing on uTox"
#define DEFAULT_SCALE  11

// Limits and sizes
#define UTOX_MAX_CALLS            16
#define UTOX_MAX_NUM_FRIENDS      256 /* Deprecated; Avoid Use */
#define UTOX_MAX_BACKLOG_MESSAGES 256
#define UTOX_MAX_NUM_GROUPS       512
#define UTOX_FILE_NAME_LENGTH     1024

#define MAX_CALLS               UTOX_MAX_CALLS       /* Deprecated; Avoid Use */
#define MAX_NUM_FRIENDS         UTOX_MAX_NUM_FRIENDS /* Deprecated; Avoid Use */
#define MAX_NUM_GROUPS          UTOX_MAX_NUM_GROUPS  /* Deprecated; Avoid Use */
#define TOX_FRIEND_ADDRESS_SIZE TOX_ADDRESS_SIZE

#define BORDER      1
#define CAPTION     26
#define MAIN_WIDTH  800
#define MAIN_HEIGHT 600

#define inrect(x, y, rx, ry, width, height) ((x) >= (rx) && (y) >= (ry) && (x) < ((rx) + (width)) && (y) < ((ry) + (height)))

#define strcmp2(x, y) (memcmp(x, y, sizeof(y) - 1))
#define strcpy2(x, y) (memcpy(x, y, sizeof(y) - 1))

#define isdesktop(x) ((size_t)(x) == 1)

#define countof(x) (sizeof(x)/sizeof(*(x)))

//  fixes compile with apple headers
/*** This breaks both android and Windows video... but it's needed to fix complation in clang (Cocoa & asan)
 ***  TODO fix them?
#ifndef __OBJC__
#define volatile(x) (*((volatile typeof(x)*)&x))
#endif */

#ifndef __OBJC__
#define volatile(x) (x)
#endif
/* UTOX_SCALE is used as the default so that we have a lot of options for scale size.
 * When ever you see UTOX_SCALE(x) double the size, and use SCALE instead!           */
#define  UTOX_SCALE(x) (((int)((ui_scale * 2.0 / 10.0) * ((double)x) )) ? : 1)
#define       SCALE(x) (((int)((ui_scale / 10.0)       * ((double)x) )) ? : 1)
#define   UI_FSCALE(x) ((      (ui_scale / 10.0)       * ((double)x) )  ? : 1)

#define drawstr(x, y, i) drawtext(x, y, S(i), SLEN(i))
#define drawstr_getwidth(x, y, str) drawtext_getwidth(x, y, (char_t*)str, sizeof(str) - 1)
#define strwidth(x) textwidth((char_t*)x, sizeof(x) - 1)

/* House keeping for uTox save file. */
#define SAVE_VERSION 3
typedef struct {
    uint8_t  version, scale, enableipv6, disableudp;
    uint16_t window_x, window_y, window_width, window_height;
    uint16_t proxy_port;
    uint8_t  proxyenable;

    uint8_t  logging_enabled                : 1;
    uint8_t  audible_notifications_enabled  : 1;
    uint8_t  filter                         : 1;
    uint8_t  audio_filtering_enabled        : 1;
    uint8_t  close_to_tray                  : 1;
    uint8_t  start_in_tray                  : 1;
    uint8_t  auto_startup                   : 1;
    uint8_t  no_typing_notifications        : 1;

    uint16_t audio_device_in;
    uint16_t audio_device_out;

    uint8_t  theme;

    uint8_t  push_to_talk                   : 1;
    uint8_t  use_mini_roster                : 1;
    uint8_t  zero                           : 6;

    uint16_t unused[31];
    uint8_t  proxy_ip[0];
} UTOX_SAVE;

typedef struct {
    uint8_t  log_version;
    time_t   time;
    size_t   author_length;
    size_t   msg_length;
    uint8_t  author  : 1;
    uint8_t  receipt : 1;
    uint8_t  flags   : 5;
    uint8_t  deleted : 1;
    uint8_t  msg_type;
    uint8_t  zeroes[2];
} LOG_FILE_MSG_HEADER;

volatile uint16_t loaded_audio_in_device, loaded_audio_out_device;
_Bool tox_connected;

/* Super global vars */
volatile _Bool tox_thread_init,
               utox_av_ctrl_init,
               utox_audio_thread_init,
               utox_video_thread_init;

typedef struct utox_settings {
    _Bool close_to_tray;
    _Bool logging_enabled;
    _Bool ringtone_enabled;
    _Bool audiofilter_enabled;
    _Bool start_in_tray;
    _Bool start_with_system;
    _Bool push_to_talk;
    _Bool use_encryption;
    _Bool audio_preview;
    _Bool video_preview;
    _Bool send_typing_status;
    _Bool use_mini_roster;
    _Bool portable_mode;
    _Bool inline_video;
    _Bool use_long_time_msg;

    uint8_t verbose;

    int     window_height;
    int     window_width;
    int     window_baseline;

    _Bool   window_maximized;
} SETTINGS;

/* This might need to be volatile type... */
SETTINGS settings;

//add friend page
uint8_t addfriend_status;

//HFONT font_big, font_big2, font_med, font_med2, font_small, font_msg;
int font_small_lineheight, font_msg_lineheight;
uint16_t video_width, video_height, max_video_width, max_video_height;
char proxy_address[256];
extern struct Tox_Options options;

// Structs
typedef struct edit_change EDIT_CHANGE;

// Enums
/* uTox debug levels */
enum {
    VERB_ANCIENT_MONK,      // Off
    VERB_JANICE_ACCOUNTING, // Error (default)
    VERB_CONCERNED_PARENT,  // Notice
    VERB_NEW_ADHD_MEDS,     // Info
    VERB_TEENAGE_GIRL,      // Debug
};


enum {
    CURSOR_NONE,
    CURSOR_TEXT,
    CURSOR_HAND,
    CURSOR_SELECT,
    CURSOR_ZOOM_IN,
    CURSOR_ZOOM_OUT,
};

enum {
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

/* SVG Bitmap names. */
enum {
    BM_ONLINE = 1,
    BM_AWAY,
    BM_BUSY,
    BM_OFFLINE,
    BM_STATUS_NOTIFY,

    BM_ADD,
    BM_GROUPS,
    BM_TRANSFER,
    BM_SETTINGS,
    BM_SETTINGS_THREE_BAR,

    BM_LBUTTON,
    BM_SBUTTON,

    BM_CONTACT,
    BM_CONTACT_MINI,
    BM_GROUP,
    BM_GROUP_MINI,

    BM_FILE,
    BM_CALL,
    BM_VIDEO,

    BM_FT,
    BM_FTM,
    BM_FTB1,
    BM_FTB2,
    BM_FT_CAP,

    BM_NO,
    BM_PAUSE,
    BM_RESUME,
    BM_YES,

    BM_SCROLLHALFTOP,
    BM_SCROLLHALFBOT,
    BM_SCROLLHALFTOP_SMALL,
    BM_SCROLLHALFBOT_SMALL,
    BM_STATUSAREA,

    BM_CHAT_BUTTON_LEFT,
    BM_CHAT_BUTTON_RIGHT,
    BM_CHAT_BUTTON_OVERLAY_SCREENSHOT,
    BM_CHAT_SEND,
    BM_CHAT_SEND_OVERLAY,
    BM_ENDMARKER,
};

// µTox includes
#include "unused.h"

#include "stb_image.h"
#include "stb_image_write.h"
extern unsigned char *stbi_write_png_to_mem(unsigned char *pixels, int stride_bytes, int x, int y, int n, int *out_len);
typedef uint8_t *UTOX_IMAGE;

#include "tox.h"
#include "audio.h"
#include "video.h"
#include "utox_av.h"
#include "tox_callbacks.h"

#if defined __WIN32__
    #include "windows/main.h"
#elif defined __ANDROID__
    #include "android/main.h"
#elif defined __OBJC__
    #include "cocoa/main.h"
#else
    #include "xlib/main.h"
#endif

#include "sized_string.h"
#include "ui_i18n_decls.h"

#include "ui.h"
#include "svg.h"
#include "avatar.h"
#include "theme.h"
#include "text.h"

#include "messages.h"
#include "friend.h"
#include "groups.h"
#include "roster.h"
#include "inline_video.h"
#include "button.h"
#include "dropdown.h"
#include "edit.h"
#include "scrollable.h"

#include "contextmenu.h"
#include "tooltip.h"
#include "commands.h"

#include "util.h"
#include "dns.h"
#include "file_transfers.h"

#include "ui_edits.h"
#include "ui_buttons.h"
#include "ui_dropdown.h"

pthread_mutex_t messages_lock;

//friends and groups
//note: assumes array size will always be large enough
FRIEND friend[MAX_NUM_FRIENDS];
GROUPCHAT group[MAX_NUM_GROUPS];
uint32_t friends, groups;

enum {
    USER_STATUS_AVAILABLE,
    USER_STATUS_AWAY_IDLE,
    USER_STATUS_DO_NOT_DISTURB,
};

//me
struct {
    uint8_t     status;
    uint8_t     name[TOX_MAX_NAME_LENGTH];
    uint8_t     *statusmsg;
    size_t      name_length, statusmsg_length;

    uint8_t     id_buffer[TOX_FRIEND_ADDRESS_SIZE * 4];
    size_t      id_buffer_length;

    uint8_t     id_binary[TOX_FRIEND_ADDRESS_SIZE];

    AVATAR      avatar;
    uint32_t    avatar_format;
    uint8_t     *avatar_data;
    size_t      avatar_size;
} self;
uint8_t cursor;

_Bool mdown;

struct {
    int x, y;
} mouse;

//fonts
//HFONT font_big, font_big2, font_med, font_med2, font_small, font_msg;
int font_small_lineheight, font_msg_lineheight;

uint16_t video_width, video_height, max_video_width, max_video_height;

char proxy_address[256];
extern struct Tox_Options options;

UTOX_FRAME_PKG *current_frame;

/** Takes data from µTox and saves it, just how the OS likes it saved!
 *
 * Returns the start of the offset on success, and 0 on failure.
 * Used to set save_needed in tox thread
 * And msg->disk_offset in history/messages */
size_t native_save_data(const uint8_t *name, size_t name_length, const uint8_t *data, size_t length, _Bool append);

/** Takes data from µTox and loads it up! */
uint8_t *native_load_data(const uint8_t *name, size_t name_length, size_t *out_size);

/** Selects the correct file on the platform and passes it to the global log reading function */
FILE *native_load_data_logfile(uint32_t friend_number);

/** given a filename, native_remove_file will delete that file from the local config dir */
_Bool native_remove_file(const uint8_t *name, size_t length);



/* Global wrappers for the native_ data functions */
_Bool      utox_save_data_tox(uint8_t *data, size_t length);
uint8_t   *utox_load_data_tox(size_t *size);

_Bool      utox_save_data_utox(UTOX_SAVE *data, size_t length);
UTOX_SAVE *utox_load_data_utox(void);

size_t     utox_save_data_log(uint32_t friend_number, uint8_t *data, size_t length);
/** This one actually does the work of reading the logfile information.
 *
 * inside main.c is probably the wrong place for it, but I'll leave chosing
 * the correct location to someone else. */
uint8_t **utox_load_data_log(uint32_t friend_number, size_t *size, uint32_t count, uint32_t skip);

/** utox_update_data_log Updates the data for this friend's history.
 *
 * When given a friend_number and offset, utox_update_data_log will overwrite the file, with
 * the supplied data * length. It makes no attempt to verify the data or length, it'll just
 * write blindly. */
_Bool utox_update_data_log(uint32_t friend_number, size_t offset, uint8_t *data, size_t length);


_Bool    utox_save_data_avatar(uint32_t friend_number, const uint8_t *data, size_t length);
uint8_t *utox_load_data_avatar(uint32_t friend_number, size_t *size);

_Bool    utox_remove_file(const uint8_t *full_name, size_t length);
_Bool utox_remove_friend_history(uint32_t friend_number);




/* TODO: sort everything below this line! */

void parse_args(int argc, char *argv[], _Bool *theme_was_set_on_argv, int8_t *should_launch_at_startup, int8_t *set_show_window, _Bool *no_updater);

// inserts/deletes a value into the registry to launch uTox after boot
void launch_at_startup(int is_launch_at_startup);

void drawalpha(int bm, int x, int y, int width, int height, uint32_t color);
void loadalpha(int bm, void *data, int width, int height);
void desktopgrab(_Bool video);
void notify(char_t *title, uint16_t title_length, char_t *msg, uint16_t msg_length, FRIEND *f);
void setscale(void);
void setscale_fonts(void);

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

/* Native wrapper to ready and call draw_image */
void draw_inline_image(uint8_t *img_data, size_t size, uint16_t w, uint16_t h, int x, int y);


/* converts a png to a UTOX_NATIVE_IMAGE, returns a pointer to it, keeping alpha channel only if keep_alpha is 1 */
UTOX_NATIVE_IMAGE *decode_image_rgb(const UTOX_IMAGE, size_t size, uint16_t *w, uint16_t *h, _Bool keep_alpha);

/* free an image created by decode_image_rgb */
void image_free(UTOX_NATIVE_IMAGE *image);

void showkeyboard(_Bool show);
void redraw(void);
void update_tray(void);
void force_redraw(void); // TODO: as parameter for redraw()?

/* gets a subdirectory of tox's datapath and puts the full pathname in dest,
 * returns number of characters written */
void flush_file(FILE *file);
int ch_mod(uint8_t *file);
void config_osdefaults(UTOX_SAVE *r);

void postmessage(uint32_t msg, uint16_t param1, uint16_t param2, void *data);

/** returns 0 if push to talk is enabled, and the button is up, else returns 1. */
void  init_ptt(void);
_Bool get_ptt_key(void);
_Bool set_ptt_key(void);
_Bool check_ptt_key(void);
void  exit_ptt(void);

/* draw functions*/
void drawtext(int x, int y, char_t *str, uint16_t length);
int drawtext_getwidth(int x, int y, char_t *str, uint16_t length);
void drawtextwidth(int x, int width, int y, char_t *str, uint16_t length);
void drawtextwidth_right(int x, int width, int y, char_t *str, uint16_t length);
void drawtextrange(int x, int x2, int y, char_t *str, uint16_t length);
void drawtextrangecut(int x, int x2, int y, char_t *str, uint16_t length);

int textwidth(char_t *str, uint16_t length);
int textfit(char_t *str, uint16_t length, int width);
int textfit_near(char_t *str, uint16_t length, int width);
//TODO: Seems to be unused. Remove?
int text_drawline(int x, int right, int y, uint8_t *str, int i, int length, int highlight, int hlen, uint16_t lineheight);


void drawrect(int x, int y, int right, int bottom, uint32_t color);
void draw_rect_frame(int x, int y, int width, int height, uint32_t color);
void draw_rect_fill(int x, int y, int width, int height, uint32_t color);

void drawhline(int x, int y, int x2, uint32_t color);
void drawvline(int x, int y, int y2, uint32_t color);
#define drawpixel(x, y, color) drawvline(x, y, (y) + 1, color)

void setfont(int id);
uint32_t setcolor(uint32_t color);
void pushclip(int x, int y, int width, int height);
void popclip(void);
void enddraw(int x, int y, int width, int height);

/* OS interface replacements */
int datapath(uint8_t *dest);
void flush_file(FILE *file);
int ch_mod(uint8_t *file);
int file_lock(FILE *file, uint64_t start, size_t length);
int file_unlock(FILE *file, uint64_t start, size_t length);

/* OS-specific cleanup function for when edits are defocused. Commit IME state, etc. */
void edit_will_deactivate(void);

/** Creates a tray baloon popup with the message, and flashes the main window
 *
 * accepts: char_t *title, title length, char_t *msg, msg length;
 * returns void;
 */
void notify(char_t *title, uint16_t title_length, char_t *msg, uint16_t msg_length, FRIEND *f);


/* other */
void thread(void func(void*), void *args);
void yieldcpu(uint32_t ms);
uint64_t get_time(void);


void copy(int value);
void paste(void);

void openurl(char_t *str);
void openfilesend(void);

/* use the file chooser to pick an avatar and set it as the user's */
void openfileavatar(void);
void native_select_dir_ft(uint32_t fid, MSG_FILE *file);
void native_autoselect_dir_ft(uint32_t fid, FILE_TRANSFER *file);
void savefiledata(MSG_FILE *file);


void setselection(char_t *data, uint16_t length);

void video_frame(uint32_t id, uint8_t *img_data, uint16_t width, uint16_t height, _Bool resize);
void video_begin(uint32_t id, char_t *name, uint16_t name_length, uint16_t width, uint16_t height);
void video_end(uint32_t id);

uint16_t native_video_detect(void);
_Bool    native_video_init(void *handle);
void     native_video_close(void *handle);
int      native_video_getframe(uint8_t *y, uint8_t *u, uint8_t *v, uint16_t width, uint16_t height);
_Bool    native_video_startread(void);
_Bool    native_video_endread(void);

void audio_detect(void);
_Bool audio_init(void *handle);
_Bool audio_close(void *handle);
_Bool audio_frame(int16_t *buffer);

ToxAV* global_av;

void audio_play(int32_t call_index, const int16_t *data, int length, uint8_t channels);
void audio_begin(int32_t call_index);
void audio_end(int32_t call_index);
