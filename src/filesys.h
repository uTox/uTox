#ifndef FILESYS_H
#define FILESYS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct file_transfer FILE_TRANSFER;
typedef struct msg_header MSG_HEADER;

#define UTOX_FILE_NAME_LENGTH 1024

typedef enum UTOX_FILE_OPTS {
    UTOX_FILE_OPTS_READ   = 1 << 0,
    UTOX_FILE_OPTS_WRITE  = 1 << 1,
    UTOX_FILE_OPTS_APPEND = 1 << 2,
    UTOX_FILE_OPTS_MKDIR  = 1 << 3,
    UTOX_FILE_OPTS_DELETE = 1 << 7,
} UTOX_FILE_OPTS;

void file_save_inline(MSG_HEADER *msg);

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

bool utox_move_file(const uint8_t *current_name, const uint8_t *new_name);

/* read a whole file from a path,
 *  on success: returns pointer to data (must be free()'d later), writes size of data to *size if size is not NULL
 *  on failure: returns NULL
 */
void *file_raw(char *path, uint32_t *size);

#endif
