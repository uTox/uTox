#include "command_funcs.h"

#include "friend.h"
#include "groups.h"
#include "debug.h"
#include "tox.h"
#include "macros.h"
#include "ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool slash_send_file(void *object, char *filepath, int UNUSED(arg_length)) {
    if (!filepath) {
        LOG_ERR("slash_send_file", " filepath was NULL.");
        return false;
    }

    FRIEND *f = object;
    LOG_TRACE("slash_send_file", "File path is: %s" , filepath);
    postmessage_toxcore(TOX_FILE_SEND_NEW_SLASH, f->number, 0xFFFF, (void *)filepath);
    return true;
}

bool slash_device(void *object, char *arg, int UNUSED(arg_length)) {
    FRIEND *f =  object;
    uint8_t id[TOX_ADDRESS_SIZE * 2];
    void *data;

    data = malloc(TOX_ADDRESS_SIZE * sizeof(char));
    if (!data) {
        LOG_ERR("slash_device", " Could not allocate memory.");
        return false;
    }

    string_to_id(id, arg);
    memcpy(data, id, TOX_ADDRESS_SIZE);
    postmessage_toxcore(TOX_FRIEND_NEW_DEVICE, f->number, 0, data);
    return true;
}


bool slash_alias(void *object, char *arg, int arg_length) {
    FRIEND *f =  object;

    if (arg) {
        friend_set_alias(f, (uint8_t *)arg, arg_length);
    } else {
        friend_set_alias(f, NULL, 0);
    }

    utox_write_metadata(f);
    return true;
}

bool slash_invite(void *object, char *arg, int UNUSED(arg_length)) {
    GROUPCHAT *g =  object;
    FRIEND *f = find_friend_by_name((uint8_t *)arg);

    if (!f || !f->online) {
        return false;
    }

    size_t msg_length;
    size_t msg_size = UTOX_FRIEND_NAME_LENGTH(f) + SLEN(GROUP_MESSAGE_INVITE) + 1;
    uint8_t msg[msg_size];

    snprintf((char *)msg, msg_size, S(GROUP_MESSAGE_INVITE),
             UTOX_FRIEND_NAME_LENGTH(f), UTOX_FRIEND_NAME(f));
    msg_length = strnlen((char *)msg, msg_size - 1);

    group_add_message(g, 0, msg, msg_length, MSG_TYPE_NOTICE);
    postmessage_toxcore(TOX_GROUP_SEND_INVITE, g->number, f->number, NULL);
    return true;
}

bool slash_topic(void *object, char *arg, int arg_length) {
    GROUPCHAT *g = object;
    void *d;

    d = malloc(arg_length);
    if (!d) {
        LOG_ERR("slash_topic", " Could not allocate memory.");
        return false;
    }

    memcpy(d, arg, arg_length);
    postmessage_toxcore(TOX_GROUP_SET_TOPIC, g->number, arg_length, d);
    return true;
}
