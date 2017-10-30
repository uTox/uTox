#ifndef UTOX_GTK_H
#define UTOX_GTK_H

#include <stdint.h>
#include <stdbool.h>

typedef struct file_transfer FILE_TRANSFER;
typedef struct msg_header MSG_HEADER;

typedef struct {
	uint32_t chat_number;
	bool 	 is_groupchat;
} CHAT;

void ugtk_openfilesend(void);

void ugtk_openfileavatar(void);

void ugtk_native_select_dir_ft(uint32_t fid, FILE_TRANSFER *file);

void ugtk_file_save_inline(MSG_HEADER *msg);

void ugtk_save_chatlog(CHAT *chat);

void *ugtk_load(void);

#endif // UTOX_GTK_H
