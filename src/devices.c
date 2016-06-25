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

void devices_update_list(void) {
    if (!panel_settings_devices.child) {
        panel_settings_devices.child    = calloc(3 + self.device_list_count, sizeof(void*));
        panel_settings_devices.child[0] = (void*)&button_add_new_device_to_self;
        panel_settings_devices.child[1] = (void*)&edit_add_new_device_to_self;
        panel_settings_devices.child[2] = NULL;
    }

    uint16_t i;
    for (i = 0; i < self.device_list_count; ++i) {
        EDIT   *edit = calloc(1, sizeof(EDIT));
        char_t *data = calloc(TOX_PUBLIC_KEY_SIZE *2, sizeof(char_t));

        if (!edit || !data) {
            debug_error("Can't malloc for an extra device\n");
            exit(7);
        }

        PANEL p_edit = {
            .type   = PANEL_EDIT,
            .x      = SCALE( 10),
            .y      = SCALE( 95) + (i * SCALE(27)),
            .height = SCALE( 24),
            .width  = SCALE(-10),
        };

        edit->panel             = p_edit;
        edit->length            = TOX_PUBLIC_KEY_SIZE * 2,
        edit->maxlength         = TOX_PUBLIC_KEY_SIZE * 2,
        edit->data              = data,
        edit->readonly          = 1,
        edit->noborder          = 0,
        edit->select_completely = 1,

        panel_settings_devices.child[i + 2] = (void*)edit;
    }
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
