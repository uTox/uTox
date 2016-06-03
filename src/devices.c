#include "main.h"

static void devices_self_add_submit(uint8_t id[]) {
    void *data = malloc(TOX_FRIEND_ADDRESS_SIZE * sizeof(uint8_t));
    memcpy(data, id, TOX_FRIEND_ADDRESS_SIZE);

    postmessage_toxcore(TOX_SELF_NEW_DEVICE, 0, 0, data);
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
        devices_self_add_submit(id);
    } else {
        debug_error("error trying to add this device\n");
    }
}
