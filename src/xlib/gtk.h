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

typedef struct {
    char *name;
    uint8_t *data;
    int data_size;
} FILE_IMAGE;

void ugtk_openfilesend(void);

void ugtk_openfileavatar(void);

void ugtk_native_select_dir_ft(uint32_t fid, FILE_TRANSFER *file);

void ugtk_file_save_inline(MSG_HEADER *msg);

void ugtk_save_chatlog(CHAT *chat);

/**
 * @brief Save passed image to selected file.
 *
 * Takes ownership of file_image.
 */
void ugtk_file_save_image_png(FILE_IMAGE *file_image);

void *ugtk_load(void);

#endif // UTOX_GTK_H
