#ifndef NATIVE_FILESYS_H
#define NATIVE_FILESYS_H

// For UTOX_FILE_OPTS
#include "../filesys.h"

#include <stdio.h>

typedef struct file_transfer FILE_TRANSFER;

FILE *native_get_file(const uint8_t *name, size_t *size, UTOX_FILE_OPTS opts, bool portable_mode);

FILE *native_get_file_simple(const char *name, UTOX_FILE_OPTS opts);

/** given a filename, native_remove_file will delete that file from the local config dir */
bool native_remove_file(const uint8_t *name, size_t length, bool portable_mode);

bool native_move_file(const uint8_t *current_name, const uint8_t *new_name);

// shows a file chooser to the user and calls utox_export_chatlog in turn
// TODO not let this depend on chatlogs
// TODO refactor this to be a simple filechooser which returns the file instead
void native_export_chatlog_init(uint32_t chat_number, bool is_groupchat);

// TODO same as for chatlogs, this is mainly native because of the file selector thing

typedef struct msg_header MSG_HEADER;
void file_save_inline_image_png(MSG_HEADER *msg);
// TODO same as for chatlogs, this is mainly native because of the file selector thing

bool native_save_image_png(const char *name, const uint8_t *image, const int image_size);

typedef struct file_transfer FILE_TRANSFER;
void native_autoselect_dir_ft(uint32_t fid, FILE_TRANSFER *file);
void native_select_dir_ft(uint32_t fid, uint32_t num, FILE_TRANSFER *file);

/**
 * @brief Get full path of the file in the Tox profile folder.
 *
 * @param name name of the file.
 * @return null-terminated string, or NULL on failure.
 */
char *native_get_filepath(const char *name);

// OS interface replacements
void flush_file(FILE *file);
int ch_mod(uint8_t *file);
int file_lock(FILE *file, uint64_t start, size_t length);
int file_unlock(FILE *file, uint64_t start, size_t length);

#endif
