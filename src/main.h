#ifndef UTOX_MAIN_H
#define UTOX_MAIN_H

/**********************************************************
 * Includes
 *********************************************************/
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

#include "filesys.h"


/**********************************************************
 * uTox Versions and header information
 *********************************************************/
#include "branding.h"


/**********************************************************
 * Forward-declares
 *********************************************************/

typedef struct avatar AVATAR;


/**********************************************************
 * UI and Toxcore Limits
 *********************************************************/

#if TOX_VERSION_IS_API_COMPATIBLE(0, 1, 0)
// YAY!!
#else
  #error "Unable to compile uTox with this Toxcore version. uTox expects v0.1.*!"
#endif

// Limits and sizes
#define UTOX_MAX_CALLS 16
#define UTOX_MAX_BACKLOG_MESSAGES 256
#define UTOX_MAX_NUM_GROUPS 512

#define UTOX_MAX_NAME_LENGTH TOX_MAX_NAME_LENGTH

#define TOX_FRIEND_ADDRESS_SIZE TOX_ADDRESS_SIZE

#define BORDER 1
#define CAPTION 26
#define MAIN_WIDTH 1000
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

/* Support for large files. */
#define _LARGEFILE_SOURCE
#define _FILE_OFFSET_BITS 64

#if TOX_VERSION_MAJOR > 0
#define ENABLE_MULTIDEVICE 1
#endif

/* House keeping for uTox save file. */
#define UTOX_SAVE_VERSION 3
typedef struct utox_save {
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

    uint8_t push_to_talk         : 1;
    uint8_t use_mini_flist       : 1;
    uint8_t group_notifications  : 4;
    uint8_t status_notifications : 1;
    uint8_t zero                 : 1;

    uint32_t utox_last_version; // I don't like this here either,
                                // but I'm not ready to rewrite and update this struct yet.

    uint8_t auto_update         : 1;
    uint8_t update_to_develop   : 1;
    uint8_t send_version        : 1;
    uint8_t zero_2              : 5;
    uint8_t zero_3              : 8;

    uint16_t unused[28];
    uint8_t  proxy_ip[];
} UTOX_SAVE;


volatile uint16_t loaded_audio_in_device, loaded_audio_out_device;

bool tox_connected;

/* Super global vars */
volatile bool utox_av_ctrl_init, utox_audio_thread_init, utox_video_thread_init;
typedef enum {
    // tox_thread is not initialized yet
    UTOX_TOX_THREAD_INIT_NONE = 0,
    // tox_thread is initialized successfully
    // this means a tox instance has been created
    UTOX_TOX_THREAD_INIT_SUCCESS = 1,
    // tox_thread is initialized but not successfully
    // this means a tox instance may have not been created
    UTOX_TOX_THREAD_INIT_ERROR = 2,
} UTOX_TOX_THREAD_INIT;

UTOX_TOX_THREAD_INIT tox_thread_init;

// add friend page
uint8_t addfriend_status;

uint16_t video_width, video_height, max_video_width, max_video_height;
char     proxy_address[256]; /* Magic Number inside toxcore */

// Enums
enum {
    CURSOR_NONE,
    CURSOR_TEXT,
    CURSOR_HAND,
    CURSOR_SELECT,
    CURSOR_ZOOM_IN,
    CURSOR_ZOOM_OUT,
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

    uint32_t nospam;
    uint32_t old_nospam;
    char nospam_str[(sizeof(uint32_t) * 2) + 1];

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
 * Takes data and the size of data and writes it to the disk
 *
 * Returns a bool indicating whether a save is needed
 */
bool utox_data_save_tox(uint8_t *data, size_t length);

/**
 * Saves the settings for uTox
 *
 * Returns a bool indicating if it succeeded or not
 */
bool utox_data_save_utox(UTOX_SAVE *data, size_t length);

/**
 * Reads the tox data from the disk and sets size
 *
 * Returns a pointer to the tox data, the caller needs to free it
 * Returns NULL on failure
 */
uint8_t *utox_data_load_tox(size_t *size);

/**
 * Loads uTox settings
 *
 * Returns a memory pointer of *size, the caller needs to free this
 * Returns NULL on failure
 */
UTOX_SAVE *utox_data_load_utox(void);

/**
 * Loads a custom theme and sets out to the size of the data
 *
 * Returns a pointer to the theme data on success, the caller needs to free this
 * Returns NULL on failure
 */
uint8_t *utox_data_load_custom_theme(size_t *out);



/* TODO: sort everything below this line! */


/**
 * Parses the arguments passed to uTox
 */
void parse_args(int argc, char *argv[],
                bool *skip_updater,
                bool *from_updater,
                bool *theme_was_set_on_argv,
                int8_t *should_launch_at_startup,
                int8_t *set_show_window);


/**
 * Initialize uTox
 */
void utox_init(void);

/* TODO: Consider sorting the functions below this line */

// inserts/deletes a value into the registry to launch uTox after boot
void launch_at_startup(int is_launch_at_startup);

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

/** returns 0 if push to talk is enabled, and the button is up, else returns 1. */
void init_ptt(void);
bool get_ptt_key(void);
bool set_ptt_key(void);
bool check_ptt_key(void);
void exit_ptt(void);

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
