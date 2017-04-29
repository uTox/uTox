#ifndef UTOX_MAIN_H
#define UTOX_MAIN_H

/**********************************************************
 * Includes
 *********************************************************/
#include <tox/tox.h>


/**********************************************************
 * uTox Versions and header information
 *********************************************************/
#include "branding.h"


/**********************************************************
 * UI and Toxcore Limits
 *********************************************************/

#if TOX_VERSION_IS_API_COMPATIBLE(0, 1, 0)
// YAY!!
#else
  #error "Unable to compile uTox with this Toxcore version. uTox expects v0.1.*!"
#endif

#define MAIN_WIDTH 750
#define MAIN_HEIGHT 500

//  fixes compile with apple headers
/*** This breaks both android and Windows video... but it's needed to fix complation in clang (Cocoa & asan)
 ***  TODO fix them?
#if !defined (__OBJC__) && !defined (__NetBSD__)
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

/* Super global vars */
volatile bool utox_av_ctrl_init, utox_audio_thread_init, utox_video_thread_init;

bool move_window_down; // When the mouse is currently down over the move_window_button().
                       // non-ideal but I wasn't ready to write a better state system for
                       // moving windows from inside uTox.
                       // seems to be unused?

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

#endif
