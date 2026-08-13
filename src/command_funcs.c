#include "command_funcs.h"

#include "flist.h"
#include "friend.h"
#include "groups.h"
#include "debug.h"
#include "tox.h"
#include "macros.h"

#include <stdlib.h>
#include <string.h>

bool slash_send_file(void *UNUSED(object), char *filepath, int UNUSED(arg_length)) {
    FRIEND *f = flist_get_sel_friend();
    if (!f || !filepath) {
        LOG_ERR("slash_send_file", " filepath was NULL.");
        return false;
    }

    LOG_TRACE("slash_send_file", "File path is: %s" , filepath);
    postmessage_toxcore(TOX_FILE_SEND_NEW_SLASH, f->number, 0xFFFF, (void *)filepath);
    return true;
}

bool slash_device(void *UNUSED(object), char *arg, int UNUSED(arg_length)) {
    FRIEND *f = flist_get_sel_friend();
    if (!f || !arg) {
        return false;
    }

    uint8_t id[TOX_ADDRESS_SIZE];
    if (!string_to_id(id, arg)) {
        return false;
    }

    void *data = malloc(TOX_ADDRESS_SIZE * sizeof(char));
    if (data) {
        memcpy(data, id, TOX_ADDRESS_SIZE);
        postmessage_toxcore(TOX_FRIEND_NEW_DEVICE, f->number, 0, data);
        return true;
    }
    LOG_ERR("slash_device", " Could not allocate memory.");
    return false;
}


bool slash_alias(void *UNUSED(object), char *arg, int arg_length) {
    FRIEND *f = flist_get_sel_friend();
    if (!f) {
        return false;
    }

    if (arg) {
        friend_set_alias(f, (uint8_t *)arg, arg_length);
    } else {
        friend_set_alias(f, NULL, 0);
    }

    utox_write_metadata(f);
    return true;
}

bool slash_invite(void *UNUSED(object), char *arg, int UNUSED(arg_length)) {
    GROUPCHAT *g = flist_get_sel_group();
    if (!g || !arg) {
        return false;
    }

    FRIEND *f = find_friend_by_name((uint8_t *)arg);
    if (f != NULL && f->online) {
        postmessage_toxcore(TOX_GROUP_SEND_INVITE, g->number, f->number, NULL);
        return true;
    }
    return false;
}

bool slash_topic(void *UNUSED(object), char *arg, int arg_length) {
    GROUPCHAT *g = flist_get_sel_group();
    if (!g || !arg || arg_length < 0) {
        return false;
    }

    void *d = malloc((size_t)arg_length);
    if (d) {
        memcpy(d, arg, (size_t)arg_length);
        postmessage_toxcore(TOX_GROUP_SET_TOPIC, g->number, arg_length, d);
        return true;
    }
    LOG_ERR("slash_topic", " Could not allocate memory.");
    return false;
}
