#ifndef FILE_TRANSFERS_H
#define FILE_TRANSFERS_H

#include "main_native.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <tox/tox.h>

#define MAX_FILE_TRANSFERS 32

typedef enum {
    FILE_TRANSFER_STATUS_NONE,
    FILE_TRANSFER_STATUS_ACTIVE,
    FILE_TRANSFER_STATUS_PAUSED_US,
    FILE_TRANSFER_STATUS_PAUSED_BOTH,
    FILE_TRANSFER_STATUS_PAUSED_THEM,
    FILE_TRANSFER_STATUS_BROKEN,
    FILE_TRANSFER_STATUS_COMPLETED,
    FILE_TRANSFER_STATUS_KILLED,
} UTOX_FILE_TRANSFER_STATUS;

typedef struct {
    FILE *file;
    uint8_t *name;
} UTOX_MSG_FT;

typedef struct FILE_TRANSFER {
    bool in_use;
    bool incoming;
    bool in_memory;
    bool avatar;
    bool inline_img;
    bool resumeable;

    uint32_t friend_number;
    uint32_t file_number;

    uint8_t  data_hash[TOX_HASH_LENGTH];

    UTOX_FILE_TRANSFER_STATUS status;

    uint8_t path[UTOX_FILE_NAME_LENGTH];

    uint8_t *name;
    size_t  name_length;

    size_t target_size;
    size_t current_size;

    union {
        uint8_t *memory;
        uint8_t *avatar;
        FILE    *file;
    } via;

    /* speed + progress calculations. */
    uint32_t speed, num_packets;
    uint64_t last_check_time, last_check_transferred;

    FILE    *resume_data;
    uint8_t  resume_update;

    // Don't really want this to be void ... MSG_FILE is better, but dependency hell
    void *ui_data;
} FILE_TRANSFER;

void file_transfer_local_control(Tox *tox, uint32_t friend_number, uint32_t file_number, TOX_FILE_CONTROL control);

uint32_t ft_send_avatar(Tox *tox, uint32_t friend_number);

uint32_t ft_send_file(Tox *tox, uint32_t friend_number, FILE *file, uint8_t *name, size_t name_length);

uint32_t ft_send_data(Tox *tox, uint32_t friend_number, uint8_t *data, size_t size, uint8_t *name, size_t name_length);

int utox_file_start_write(uint32_t friend_number, uint32_t file_number, const char *filepath);

void utox_set_callbacks_file_transfer(Tox *tox);

void ft_friend_online(Tox *tox, uint32_t friend_number);
void ft_friend_offline(Tox *tox, uint32_t friend_number);

#endif
