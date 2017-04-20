#include "command_funcs.h"

#include "friend.h"
#include "groups.h"
#include "debug.h"
#include "tox.h"
#include "macros.h"

#include <stdlib.h>
#include <string.h>

bool slash_send_file(void *object, char *filepath, int UNUSED(arg_length)) {
    if (filepath) {
        FRIEND *f = object;
        LOG_TRACE("File path is: %s" , filepath);
        postmessage_toxcore(TOX_FILE_SEND_NEW_SLASH, f->number, 0xFFFF, (void *)filepath);
        return true;
    }

    LOG_ERR(" filepath was NULL.");
    return false;
}

bool slash_device(void *object, char *arg, int UNUSED(arg_length)) {
    FRIEND *f =  object;
    uint8_t id[TOX_ADDRESS_SIZE * 2];
    string_to_id(id, arg);
    void *data = malloc(TOX_ADDRESS_SIZE * sizeof(char));

    if (data) {
        memcpy(data, id, TOX_ADDRESS_SIZE);
        postmessage_toxcore(TOX_FRIEND_NEW_DEVICE, f->number, 0, data);
        return true;
    }
    LOG_ERR(" Could not allocate memory.");
    return false;
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
    if (f != NULL && f->online) {
        postmessage_toxcore(TOX_GROUP_SEND_INVITE, g->number, f->number, NULL);
        return true;
    }
    return false;
}

bool slash_topic(void *object, char *arg, int arg_length) {
    GROUPCHAT *g = object;
    void *d = malloc(arg_length);
    if (d) {
        memcpy(d, arg, arg_length);
        postmessage_toxcore(TOX_GROUP_SET_TOPIC, g->number, arg_length, d);
        return true;
    }
    LOG_ERR(" Could not allocate memory.");
    return false;
}
