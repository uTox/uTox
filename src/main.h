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

bool move_window_down; // When the mouse is currently down over the move_window_button().
                       // non-ideal but I wasn't ready to write a better state system for
                       // moving windows from inside uTox.


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
 * Reads the tox data from the disk and sets size
 *
 * Returns a pointer to the tox data, the caller needs to free it
 * Returns NULL on failure
 */
uint8_t *utox_data_load_tox(size_t *size);


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

/**
 * Free used resources
 */
void utox_raze(void);

// Android audio
void audio_play(int32_t call_index, const int16_t *data, int length, uint8_t channels);
void audio_begin(int32_t call_index);
void audio_end(int32_t call_index);

#endif
