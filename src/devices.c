#include "main.h"

static void devices_self_add_submit(uint8_t *name, size_t length, uint8_t id[TOX_FRIEND_ADDRESS_SIZE]) {

    if (length >= UINT16_MAX) { /* Max size of postmsg */
        debug_error("Devices:\tName length > UINT16_MAX\n");
        /* TODO send error to GUI */
        return;
    }

    uint8_t *data = malloc(length * sizeof(uint8_t) + sizeof(id[0]) * TOX_FRIEND_ADDRESS_SIZE);

    memcpy(data, id, TOX_FRIEND_ADDRESS_SIZE);
    memcpy(data + TOX_FRIEND_ADDRESS_SIZE, name, length * sizeof(uint8_t));


    postmessage_toxcore(TOX_SELF_NEW_DEVICE, length, 0, data);
}

void devices_self_add(uint8_t *device, size_t length) {
    uint8_t name_cleaned[length];
    uint16_t length_cleaned = 0;

    unsigned int i;
    for (i = 0; i < length; ++i) {
        if (device[i] != ' ') {
            name_cleaned[length_cleaned] = device[i];
            ++length_cleaned;
        }
    }

    if(!length_cleaned) {
        addfriend_status = ADDF_NONAME;
        return;
    }

    uint8_t id[TOX_FRIEND_ADDRESS_SIZE];

    if(length_cleaned == TOX_FRIEND_ADDRESS_SIZE * 2 && string_to_id(id, name_cleaned)) {
        /* TODO, names! */
        devices_self_add_submit((uint8_t*)"Default device name", 19, id);
    } else {
        debug_error("error trying to add this device\n");
    }
}
