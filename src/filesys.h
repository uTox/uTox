#ifndef FILESYS_H
#define FILESYS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define UTOX_FILE_NAME_LENGTH 1024

/**
 * Options to define behavior when opening files.
 */
typedef enum UTOX_FILE_OPTS {
    UTOX_FILE_OPTS_READ   = 1 << 0,
    UTOX_FILE_OPTS_WRITE  = 1 << 1,
    UTOX_FILE_OPTS_APPEND = 1 << 2,
    UTOX_FILE_OPTS_MKDIR  = 1 << 3,
    UTOX_FILE_OPTS_DELETE = 1 << 7,
} UTOX_FILE_OPTS;

/**
 * OS independent way of opening a file from the utox storage folder.
 *
 * This function takes care of the environment, checking for portable mode.
 *
 * @param name file name, relative to utox storage folder.
 * @param size size of the file. This variable is written to by the function. Can be NULL if you don't care about the size.
 * @param opts options to apply when opening the file, see UTOX_FILE_OPTS for available options.
 * @return open file pointer, or NULL on failure.
 */
FILE *utox_get_file(const char *name, size_t *size, UTOX_FILE_OPTS opts);

/**
 * @brief OS independent way of opening a file from filesystem.
 *
 * @param name full path of file to open.
 * @param opts Options to apply when opening the file, see UTOX_FILE_OPTS for available options.
 *
 * @return open file pointer, or NULL on failure.
 */
FILE *utox_get_file_simple(const char *name, UTOX_FILE_OPTS opts);

/**
 * TODO DOCUMENTATION
 */
bool utox_remove_file(const uint8_t *full_name, size_t length);

bool utox_move_file(const uint8_t *current_name, const uint8_t *new_name);

/**
 * Takes a null-terminated utf8 filepath and creates it with permissions 0700
 * (in posix environments) if it doesn't already exist. In Windows environments
 * there are no security settings applied to the created folder.
 *
 * Returns a bool indicating if the path exists or not.
 */
bool native_create_dir(const uint8_t *filepath);

/**
 * @brief Get full path of the file in the Tox profile folder.
 *
 * @param name name of the file.
 * @return null-terminated string, or NULL on failure.
 */
char *utox_get_filepath(const char *name);

#endif
