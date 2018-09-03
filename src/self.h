#ifndef SELF_H
#define SELF_H

#include <tox/tox.h>
#include "native/image.h"

typedef struct avatar AVATAR;

struct utox_self {
    uint8_t status;
    char    name[TOX_MAX_NAME_LENGTH];
    char    statusmsg[TOX_MAX_STATUS_MESSAGE_LENGTH];
    size_t  name_length, statusmsg_length;

    size_t friend_list_count;
    size_t friend_list_size;

    size_t groups_list_count;
    size_t groups_list_size;

    size_t device_list_count;
    size_t device_list_size;

    char   id_str[TOX_ADDRESS_SIZE * 2];
    size_t id_str_length;

    NATIVE_IMAGE *qr_image;
    int qr_image_size;

    uint8_t *qr_data;
    int qr_data_size;

    uint8_t id_binary[TOX_ADDRESS_SIZE];

    uint32_t nospam;
    uint32_t old_nospam;
    char nospam_str[(sizeof(uint32_t) * 2) + 1];

    AVATAR *avatar;
    uint8_t *png_data;
    size_t png_size;
} self;

void init_self(Tox *tox);

void self_create_group_info(Group_Chat_Self_Peer_Info *self_info);

#endif
