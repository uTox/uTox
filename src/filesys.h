#ifndef FILESYS_H
#define FILESYS_H

#include <inttypes.h>

typedef struct file_transfer FILE_TRANSFER;

void file_save_inline(FILE_TRANSFER *file);

void native_autoselect_dir_ft(uint32_t fid, FILE_TRANSFER *file);

#endif
