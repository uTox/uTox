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

#if TOX_VERSION_IS_API_COMPATIBLE(0, 2, 0)
// YAY!!
#else
  #error "Unable to compile uTox with this Toxcore version. uTox expects v0.2.*!"
#endif

#define MAIN_WIDTH 750
#define MAIN_HEIGHT 500

#ifndef __OBJC__
#define volatile(x)(x)
#endif

/* Support for large files. */
#define _LARGEFILE_SOURCE
#define _FILE_OFFSET_BITS 64

#if TOX_VERSION_MAJOR > 0
#define ENABLE_MULTIDEVICE 1
#endif

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
                int8_t *should_launch_at_startup,
                int8_t *set_show_window,
                bool *allow_root);


/**
 * Initialize uTox
 */
void utox_init(void);

/**
 * Free used resources
 */
void utox_raze(void);

#endif
