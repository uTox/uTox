#include "../util.h"

typedef struct file_transfer FILE_TRANSFER;

void ugtk_openfilesend(void);

void ugtk_openfileavatar(void);

void ugtk_native_select_dir_ft(uint32_t fid, FILE_TRANSFER *file);

void ugtk_file_save_inline(FILE_TRANSFER *file);

void ugtk_save_chatlog(uint32_t friend_number);

void *ugtk_load(void);
