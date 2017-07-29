#ifndef UTOX_GTK_H
#define UTOX_GTK_H

#include <stdint.h>

#include "../typedefs.h"

typedef struct file_transfer FILE_TRANSFER;

typedef struct file_transfer FILE_TRANSFER;

void ugtk_openfilesend(void);

void ugtk_openfileavatar(void);

void ugtk_native_select_dir_ft(uint32_t fid, FILE_TRANSFER *file);

void ugtk_file_save_inline(MSG_HEADER *msg);

void ugtk_save_chatlog(uint32_t friend_number);

void *ugtk_load(void);

#endif // UTOX_GTK_H
