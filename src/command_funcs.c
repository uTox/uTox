#include "command_funcs.h"
#include "tox.h"
#include "friend.h"
#include "groups.h"
#include "util.h"

bool slash_send_file(void *object, char *filepath, int UNUSED(arg_length)) {
    if (filepath) {
        FRIEND *f = object;
        debug("slash_send_file:\tFile path is: %s\n", filepath);
        postmessage_toxcore(TOX_FILE_SEND_NEW_SLASH, f - friend, 0xFFFF, (void *)filepath);
        return true;
    }

    debug_error("slash_send_file:\t filepath was NULL.\n");
    return false;
}

bool slash_device(void *object, char *arg, int UNUSED(arg_length)) {
    FRIEND *f =  object;
    uint8_t id[TOX_FRIEND_ADDRESS_SIZE * 2];
    string_to_id(id, arg);
    void *data = malloc(TOX_FRIEND_ADDRESS_SIZE * sizeof(char));

    if (data) {
        memcpy(data, id, TOX_FRIEND_ADDRESS_SIZE);
        postmessage_toxcore(TOX_FRIEND_NEW_DEVICE, f->number, 0, data);
        return true;
    }
    debug_error("slash_device:\t Could not allocate memory.\n");
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
        postmessage_toxcore(TOX_GROUP_SET_TOPIC, (g - group), arg_length, d);
        return true;
    }
    debug_error("slash_topic:\t Could not allocate memory.\n");
    return false;
}
