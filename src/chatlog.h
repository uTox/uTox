#ifndef CHATLOG_H
#define CHATLOG_H

#include <tox/tox.h>

#include <stddef.h>
#include <stdio.h>
#include <time.h>


#define LOGFILE_SAVE_VERSION 3
typedef struct {
    uint8_t log_version;

    time_t time;
    size_t author_length;
    size_t msg_length;

    uint8_t author : 1;
    uint8_t receipt : 1;
    uint8_t flags : 5;
    uint8_t deleted : 1;

    uint8_t msg_type;

    uint8_t zeroes[2];
} LOG_FILE_MSG_HEADER;


typedef struct msg_header MSG_HEADER;

/**
 * Saves chat log for friend with id hex
 *
 * Returns the offset on success
 * Returns 0 on failure
 */
size_t utox_save_chatlog(char hex[TOX_PUBLIC_KEY_SIZE * 2], uint8_t *data, size_t length);

// This one actually does the work of reading the logfile information.
MSG_HEADER **utox_load_chatlog(char hex[TOX_PUBLIC_KEY_SIZE * 2], size_t *size, uint32_t count, uint32_t skip);

/** utox_update_chatlog Updates the data for this friend's history.
 *
 * When given a friend_number and offset, utox_update_chatlog will overwrite the file, with
 * the supplied data * length. It makes no attempt to verify the data or length, it'll just
 * write blindly. */
bool utox_update_chatlog(char hex[TOX_PUBLIC_KEY_SIZE * 2], size_t offset, uint8_t *data, size_t length);

/**
 * Deletes the chat log file for the friend with id hex
 *
 * Returns bool indicating if it succeeded
 */
bool utox_remove_friend_chatlog(char hex[TOX_PUBLIC_KEY_SIZE * 2]);

/**
 * Setup for exporting the chat log to plain text
 */
void utox_export_chatlog_init(uint32_t friend_number);

/**
 * Export the chat log to plain text
 */
void utox_export_chatlog(char hex[TOX_PUBLIC_KEY_SIZE * 2], FILE *dest_file);

#endif
