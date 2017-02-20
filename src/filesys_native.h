#ifndef FILESYS_NATIVE_H
#define FILESYS_NATIVE_H

#include <stdio.h>

FILE *native_get_file(const uint8_t *name, size_t *size, UTOX_FILE_OPTS opts, bool portable_mode);

/** given a filename, native_remove_file will delete that file from the local config dir */
bool native_remove_file(const uint8_t *name, size_t length, bool portable_mode);

bool native_move_file(const uint8_t *current_name, const uint8_t *new_name);

// shows a file chooser to the user and calls utox_export_chatlog in turn
// TODO not let this depend on chatlogs
// TODO refactor this to be a simple filechooser which returns the file instead
void native_export_chatlog_init(uint32_t friend_number);
// TODO same as for chatlogs, this is mainly native because of the file selector thing
typedef struct msg_header MSG_HEADER;
void file_save_inline_image_png(MSG_HEADER *msg);
// TODO same as for chatlogs, this is mainly native because of the file selector thing
typedef struct file_transfer FILE_TRANSFER;
void native_autoselect_dir_ft(uint32_t fid, FILE_TRANSFER *file);
void native_select_dir_ft(uint32_t fid, uint32_t num, FILE_TRANSFER *file);

#endif // FILESYS_NATIVE_H
