#ifndef UTOX_MAIN_H
#define UTOX_MAIN_H

// Versions
#define TITLE "uTox"
#define SUB_TITLE "(Alpha)"
#define RELEASE_TITLE "WORKING"
#define VERSION "0.10.9"
#define VER_MAJOR 0
#define VER_MINOR 10
#define VER_PATCH 9
#define UTOX_VERSION_NUMBER 10009u /* major, minor, patch */
// Defaults
#define DEFAULT_NAME "Tox User"
#define DEFAULT_STATUS "Toxing on uTox"
#define DEFAULT_SCALE 11

// Limits and sizes
#define UTOX_MAX_CALLS 16
#define UTOX_MAX_BACKLOG_MESSAGES 256
#define UTOX_MAX_NUM_GROUPS 512
#define UTOX_FILE_NAME_LENGTH 1024

#define UTOX_MAX_NAME_LENGTH TOX_MAX_NAME_LENGTH

#define TOX_FRIEND_ADDRESS_SIZE TOX_ADDRESS_SIZE

#define BORDER 1
#define CAPTION 26
#define MAIN_WIDTH 800
#define MAIN_HEIGHT 600

#define inrect(x, y, rx, ry, width, height) \
    ((x) >= (rx) && (y) >= (ry) && (x) < ((rx) + (width)) && (y) < ((ry) + (height)))

#define strcmp2(x, y) (memcmp(x, y, sizeof(y) - 1))
#define strcpy2(x, y) (memcpy(x, y, sizeof(y) - 1))

#define isdesktop(x) ((size_t)(x) == 1)

#define countof(x) (sizeof(x) / sizeof(*(x)))

//  fixes compile with apple headers
/*** This breaks both android and Windows video... but it's needed to fix complation in clang (Cocoa & asan)
 ***  TODO fix them?
#ifndef __OBJC__
#define volatile(x) (*((volatile typeof(x)*)&x))
#endif */

#ifndef __OBJC__
#define volatile(x)(x)
#endif

#define SCALE(x) (((int)((ui_scale / 10.0) * ((double)x))) ?: 1)
#define UI_FSCALE(x) (((ui_scale / 10.0) * ((double)x)) ?: 1)

#define drawstr(x, y, i) drawtext(x, y, S(i), SLEN(i))
#define drawstr_getwidth(x, y, str) drawtext_getwidth(x, y, (char *)str, sizeof(str) - 1)
#define strwidth(x) textwidth((char *)x, sizeof(x) - 1)

/* Support for large files. */
#define _LARGEFILE_SOURCE
#define _FILE_OFFSET_BITS 64

#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <tox/tox.h>

#if TOX_VERSION_MAJOR > 0
#define ENABLE_MULTIDEVICE 1
#endif

typedef enum UTOX_FILE_OPTS {
    UTOX_FILE_OPTS_READ   = 1 << 0,
    UTOX_FILE_OPTS_WRITE  = 1 << 1,
    UTOX_FILE_OPTS_APPEND = 1 << 2,
    UTOX_FILE_OPTS_MKDIR  = 1 << 3,
    UTOX_FILE_OPTS_DELETE = 1 << 7,
} UTOX_FILE_OPTS;

/* House keeping for uTox save file. */
#define UTOX_SAVE_VERSION 3
typedef struct {
    uint8_t save_version;
    uint8_t scale;
    uint8_t enableipv6;
    uint8_t disableudp;

    uint16_t window_x, window_y, window_width, window_height;
    uint16_t proxy_port;

    uint8_t proxyenable;

    uint8_t logging_enabled : 1;
    uint8_t audible_notifications_enabled : 1;
    uint8_t filter : 1;
    uint8_t audio_filtering_enabled : 1;
    uint8_t close_to_tray : 1;
    uint8_t start_in_tray : 1;
    uint8_t auto_startup : 1;
    uint8_t no_typing_notifications : 1;

    uint16_t audio_device_in;
    uint16_t audio_device_out;

    uint8_t theme;

    uint8_t push_to_talk : 1;
    uint8_t use_mini_flist : 1;
    uint8_t group_notifications : 4;
    uint8_t status_notifications : 1;
    uint8_t zero : 1;

    uint32_t utox_last_version; // I don't like this here either,
                                // but I'm not ready to rewrite and update this struct yet.

    uint16_t unused[29];
    uint8_t  proxy_ip[0];
} UTOX_SAVE;

#define LOGFILE_SAVE_VERSION 3
typedef struct {
    uint8_t log_version;

    time_t time;
    size_t author_length;
    size_t msg_length;

    uint8_t author : 1;
    uint8_t receipt : 1;
    uint8_t flags : 5;
    uint8_t deleted : 1;

    uint8_t msg_type;

    uint8_t zeroes[2];
} LOG_FILE_MSG_HEADER;

volatile uint16_t loaded_audio_in_device, loaded_audio_out_device;

bool tox_connected;

/* Super global vars */
volatile bool tox_thread_init, utox_av_ctrl_init, utox_audio_thread_init, utox_video_thread_init;

double ui_scale;

typedef struct utox_settings {
    // uTox versions settings
    uint32_t curr_version;
    uint32_t last_version;
    bool     show_splash;

    // Low level settings (network, profile, portable-mode)
    bool use_proxy;
    bool force_proxy;
    bool enable_udp;
    bool enable_ipv6;
    bool use_encryption;
    bool portable_mode;

    uint16_t proxy_port;

    // User interface settings
    bool close_to_tray;
    bool logging_enabled;
    bool ringtone_enabled;
    bool audiofilter_enabled;
    bool start_in_tray;
    bool start_with_system;
    bool push_to_talk;
    bool audio_preview;
    bool video_preview;
    bool send_typing_status;
    bool use_mini_flist;
    bool inline_video;
    bool use_long_time_msg;
    bool accept_inline_images;
    bool status_notifications;

    uint8_t group_notifications;

    uint8_t verbose;

    uint32_t theme;

    // OS interface settings
    uint32_t window_height;
    uint32_t window_width;
    uint32_t window_baseline;

    bool    window_maximized;
} SETTINGS;

/* This might need to be volatile type... */
SETTINGS settings;

// add friend page
uint8_t addfriend_status;

int      font_small_lineheight, font_msg_lineheight;
uint16_t video_width, video_height, max_video_width, max_video_height;
char     proxy_address[256]; /* Magic Number inside toxcore */

// Enums
/* uTox debug levels */
enum {
    VERBOSITY_OFF,
    VERBOSITY_ERROR,
    VERBOSITY_WARNING,
    VERBOSITY_NOTICE,
    VERBOSITY_INFO,
    VERBOSITY_DEBUG,
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
    FONT_SELF_NAME,
    FONT_STATUS,
    FONT_LIST_NAME,
    FONT_MISC,

    FONT_END,
};

typedef enum {
    FILEDATA_OVERWRITE = 0,
    FILEDATA_APPEND    = 1,
} FILEDATA_SAVETYPE;

#ifdef UNUSED
#undef UNUSED
#endif

#ifdef __GNUC__
#define UNUSED(x) UNUSED_##x __attribute__((__unused__))
#elif defined(__LCLINT__)
#define UNUSED(x) /*@unused@*/ x
#else
#define UNUSED(x) x
#endif


#include "stb_image.h"
#include "stb_image_write.h"
extern unsigned char *stbi_write_png_to_mem(unsigned char *pixels, int stride_bytes, int x, int y, int n, int *out_len);
typedef uint8_t *UTOX_IMAGE;

pthread_mutex_t messages_lock;

enum {
    USER_STATUS_AVAILABLE,
    USER_STATUS_AWAY_IDLE,
    USER_STATUS_DO_NOT_DISTURB,
};

typedef struct avatar AVATAR;
// me
struct utox_self {
    uint8_t status;
    char    name[TOX_MAX_NAME_LENGTH];
    char    statusmsg[TOX_MAX_STATUS_MESSAGE_LENGTH];
    size_t  name_length, statusmsg_length;

    size_t friend_list_count;
    size_t friend_list_size;

    size_t groups_list_count;
    size_t groups_list_size;

    size_t device_list_count;
    size_t device_list_size;

    char   id_str[TOX_FRIEND_ADDRESS_SIZE * 2];
    size_t id_str_length;

    uint8_t id_binary[TOX_FRIEND_ADDRESS_SIZE];

    AVATAR *avatar;
    void  *png_data;
    size_t png_size;
} self;

struct utox_mouse {
    int x, y;
} mouse;

uint8_t cursor;
bool    mdown;

/**
 * Takes a filepath and creates it with permissions 0700 (in posix environments)
 * if it doesn't already exist.
 *
 * Returns a bool indicating if the path exists or not.
 */
bool native_create_dir(const char *filepath);


FILE *native_get_file(char *name, size_t *size, UTOX_FILE_OPTS flag);

/** given a filename, native_remove_file will delete that file from the local config dir */
bool native_remove_file(const uint8_t *name, size_t length);

/**
 * TODO DOCUMENTATION
 */
bool utox_data_save_tox(uint8_t *data, size_t length);

/**
 * TODO DOCUMENTATION
 */
uint8_t *utox_data_load_tox(size_t *size);

/**
 * TODO DOCUMENTATION
 */
bool utox_data_save_utox(UTOX_SAVE *data, size_t length);

/**
 * TODO DOCUMENTATION
 */
UTOX_SAVE *utox_data_load_utox(void);

/**
 * TODO DOCUMENTATION
 */
uint8_t *utox_data_load_custom_theme(size_t *out);

/**
 * TODO DOCUMENTATION
 */
size_t utox_save_chatlog(char hex[TOX_PUBLIC_KEY_SIZE * 2], uint8_t *data, size_t length);

/** This one actually does the work of reading the logfile information.
 *
 * inside main.c is probably the wrong place for it, but I'll leave chosing
 * the correct location to someone else. */
uint8_t **utox_load_chatlog(char hex[TOX_PUBLIC_KEY_SIZE * 2], size_t *size, uint32_t count, uint32_t skip);

/** utox_update_chatlog Updates the data for this friend's history.
 *
 * When given a friend_number and offset, utox_update_chatlog will overwrite the file, with
 * the supplied data * length. It makes no attempt to verify the data or length, it'll just
 * write blindly. */
bool utox_update_chatlog(char hex[TOX_PUBLIC_KEY_SIZE * 2], size_t offset, uint8_t *data, size_t length);

/**
 * TODO DOCUMENTATION
 */
bool utox_data_save_avatar(char hex[TOX_PUBLIC_KEY_SIZE * 2], const uint8_t *data, size_t length);
/**
 * TODO DOCUMENTATION
 */
uint8_t *utox_data_load_avatar(const char hex[TOX_PUBLIC_KEY_SIZE * 2], size_t *size);
/**
 * TODO DOCUMENTATION
 */
bool utox_data_del_avatar(uint32_t friend_number);

/**
 * TODO DOCUMENTATION
 */
bool utox_remove_file(const uint8_t *full_name, size_t length);
/**
 * TODO DOCUMENTATION
 */
bool utox_remove_friend_chatlog(char hex[TOX_PUBLIC_KEY_SIZE * 2]);
bool utox_remove_file(const uint8_t *full_name, size_t length);
bool utox_remove_friend_history(char hex[TOX_PUBLIC_KEY_SIZE * 2]);

/**
 * TODO DOCUMENTATION
 */
void utox_export_chatlog_init(uint32_t friend_number);

/**
 * TODO DOCUMENTATION
 */
void utox_export_chatlog(char hex[TOX_PUBLIC_KEY_SIZE * 2], FILE *dest_file);
/* TODO: sort everything below this line! */


/**
 * TODO DOCUMENTATION
 */
void parse_args(int argc, char *argv[], bool *theme_was_set_on_argv, int8_t *should_launch_at_startup,
               int8_t *set_show_window, bool *no_updater);


/**
 * TODO DOCUMENTATION
 */
void utox_init(void);
// inserts/deletes a value into the registry to launch uTox after boot
void launch_at_startup(int is_launch_at_startup);

void drawalpha(int bm, int x, int y, int width, int height, uint32_t color);
void loadalpha(int bm, void *data, int width, int height);
void desktopgrab(bool video);
void notify(char *title, uint16_t title_length, const char *msg, uint16_t msg_length, void *object, bool is_group);
void setscale(void);
void setscale_fonts(void);

enum {
    FILTER_NEAREST, // ugly and quick filtering
    FILTER_BILINEAR // prettier and a bit slower filtering
};

typedef struct native_image NATIVE_IMAGE;

/* set filtering method used when resizing given image to one of above enum */
void image_set_filter(NATIVE_IMAGE *image, uint8_t filter);

/* set scale of image so that when it's drawn it will be `scale' times as large(2.0 for double size, 0.5 for half, etc.)
 *  notes: theoretically lowest possible scale is (1.0/65536.0), highest is 65536.0, values outside of this range will
 * create weird issues
 *         scaling will be rounded to pixels, so it might not be exact
 */
void image_set_scale(NATIVE_IMAGE *image, double scale);

/* draws an utox image with or without alpha channel into the rect of (x,y,width,height) on the screen,
 * starting at position (imgx,imgy) of the image
 * WARNING: Windows can fail to show the image at all if the rect (imgx,imgy,width,height) contains even 1 pixel outside
 * of
 * the image's size AFTER SCALING, so be careful.
 * TODO: improve this so this function is safer to use */
void draw_image(const NATIVE_IMAGE *image, int x, int y, uint32_t width, uint32_t height, uint32_t imgx, uint32_t imgy);

/* Native wrapper to ready and call draw_image */
void draw_inline_image(uint8_t *img_data, size_t size, uint16_t w, uint16_t h, int x, int y);

/* converts a png to a NATIVE_IMAGE, returns a pointer to it, keeping alpha channel only if keep_alpha is 1 */
NATIVE_IMAGE *utox_image_to_native(const UTOX_IMAGE, size_t size, uint16_t *w, uint16_t *h, bool keep_alpha);

/* free an image created by utox_image_to_native */
void image_free(NATIVE_IMAGE *image);

void showkeyboard(bool show);
void redraw(void);
void update_tray(void);
void force_redraw(void); // TODO: as parameter for redraw()?

void flush_file(FILE *file);
int ch_mod(uint8_t *file);
void config_osdefaults(UTOX_SAVE *r);

void postmessage(uint32_t msg, uint16_t param1, uint16_t param2, void *data);

/** returns 0 if push to talk is enabled, and the button is up, else returns 1. */
void init_ptt(void);
bool get_ptt_key(void);
bool set_ptt_key(void);
bool check_ptt_key(void);
void exit_ptt(void);

/* draw functions*/
void drawtext(int x, int y, const char *str, uint16_t length);
int drawtext_getwidth(int x, int y, const char *str, uint16_t length);
void drawtextwidth(int x, int width, int y, const char *str, uint16_t length);
void drawtextwidth_right(int x, int width, int y, const char *str, uint16_t length);
void drawtextrange(int x, int x2, int y, const char *str, uint16_t length);
void drawtextrangecut(int x, int x2, int y, const char *str, uint16_t length);

int textwidth(const char *str, uint16_t length);
int textfit(const char *str, uint16_t length, int width);
int textfit_near(const char *str, uint16_t length, int width);
// TODO: Seems to be unused. Remove?
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
void flush_file(FILE *file);
int ch_mod(uint8_t *file);
int file_lock(FILE *file, uint64_t start, size_t length);
int file_unlock(FILE *file, uint64_t start, size_t length);

/* OS-specific cleanup function for when edits are defocused. Commit IME state, etc. */
void edit_will_deactivate(void);

/* other */
void thread(void func(void *), void *args);
void yieldcpu(uint32_t ms);
uint64_t get_time(void);

void copy(int value);
void paste(void);

void openurl(char *str);
void openfilesend(void);

/* use the file chooser to pick an avatar and set it as the user's */
void openfileavatar(void);

void setselection(char *data, uint16_t length);

void video_frame(uint32_t id, uint8_t *img_data, uint16_t width, uint16_t height, bool resize);
void video_begin(uint32_t id, char *name, uint16_t name_length, uint16_t width, uint16_t height);
void video_end(uint32_t id);

void audio_detect(void);
bool audio_init(void *handle);
bool audio_close(void *handle);
bool audio_frame(int16_t *buffer);

void audio_play(int32_t call_index, const int16_t *data, int length, uint8_t channels);
void audio_begin(int32_t call_index);
void audio_end(int32_t call_index);

uint16_t native_video_detect(void);
bool native_video_init(void *handle);
void native_video_close(void *handle);
int native_video_getframe(uint8_t *y, uint8_t *u, uint8_t *v, uint16_t width, uint16_t height);
bool  native_video_startread(void);
bool  native_video_endread(void);


#endif
