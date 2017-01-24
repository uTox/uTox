#ifndef UTOX_MAIN_H
#define UTOX_MAIN_H

/**********************************************************
 * Includes
 *********************************************************/
#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
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
typedef uint8_t *UTOX_IMAGE;


/**********************************************************
 * UI and Toxcore Limits
 *********************************************************/

#if TOX_VERSION_IS_API_COMPATIBLE(0, 1, 0)
// YAY!!
#else
  #error "Unable to compile uTox with this Toxcore version. uTox expects v0.1.*!"
#endif

// Limits and sizes
// UTOX_MAX_NUM_GROUPS is never used. Remove?
#define UTOX_MAX_NUM_GROUPS 512

#define BORDER 1
#define CAPTION 26
#define MAIN_WIDTH 800
#define MAIN_HEIGHT 500

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

#include "stb_image.h"
#include "stb_image_write.h"
extern unsigned char *stbi_write_png_to_mem(unsigned char *pixels, int stride_bytes, int x, int y, int n, int *out_len);

enum {
    USER_STATUS_AVAILABLE,
    USER_STATUS_AWAY_IDLE,
    USER_STATUS_DO_NOT_DISTURB,
};

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

// Android audio
void audio_play(int32_t call_index, const int16_t *data, int length, uint8_t channels);
void audio_begin(int32_t call_index);
void audio_end(int32_t call_index);

#endif
