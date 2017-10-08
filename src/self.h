#ifndef SELF_H
#define SELF_H

#include <tox/tox.h>

typedef struct avatar AVATAR;

typedef enum {
    USER_STATUS_AVAILABLE,
    USER_STATUS_AWAY_IDLE,
    USER_STATUS_DO_NOT_DISTURB,
    USER_STATUS_OFFLINE
} USER_STATUS;

struct utox_self {
    USER_STATUS status;
    char        name[TOX_MAX_NAME_LENGTH];
    char        statusmsg[TOX_MAX_STATUS_MESSAGE_LENGTH];
    size_t      name_length, statusmsg_length;

    size_t friend_list_count;
    size_t friend_list_size;

    size_t groups_list_count;
    size_t groups_list_size;

    size_t device_list_count;
    size_t device_list_size;

    char   id_str[TOX_ADDRESS_SIZE * 2];
    size_t id_str_length;

    uint8_t id_binary[TOX_ADDRESS_SIZE];

    uint32_t nospam;
    uint32_t old_nospam;
    char nospam_str[(sizeof(uint32_t) * 2) + 1];

    AVATAR *avatar;
    uint8_t *png_data;
    size_t png_size;
} self;

void init_self(Tox *tox);

#endif
