#ifndef FILESYS_H
#define FILESYS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct file_transfer FILE_TRANSFER;

typedef enum UTOX_FILE_OPTS {
    UTOX_FILE_OPTS_READ   = 1 << 0,
    UTOX_FILE_OPTS_WRITE  = 1 << 1,
    UTOX_FILE_OPTS_APPEND = 1 << 2,
    UTOX_FILE_OPTS_MKDIR  = 1 << 3,
    UTOX_FILE_OPTS_DELETE = 1 << 7,
} UTOX_FILE_OPTS;

void file_save_inline(FILE_TRANSFER *file);

void native_autoselect_dir_ft(uint32_t fid, FILE_TRANSFER *file);


/**
 * Takes a null-terminated utf8 filepath and creates it with permissions 0700
 * (in posix environments) if it doesn't already exist. In Windows environments
 * there are no security settings applied to the created folder.
 *
 * Returns a bool indicating if the path exists or not.
 */
bool native_create_dir(const uint8_t *filepath);

FILE *native_get_file(const uint8_t *name, size_t *size, UTOX_FILE_OPTS opts);

/** given a filename, native_remove_file will delete that file from the local config dir */
bool native_remove_file(const uint8_t *name, size_t length);

/**
 * TODO DOCUMENTATION
 */
bool utox_remove_file(const uint8_t *full_name, size_t length);

#endif
