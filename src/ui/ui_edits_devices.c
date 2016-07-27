#include "../main.h"
static uint8_t edit_add_new_device_to_self_data[TOX_FRIEND_ADDRESS_SIZE * 4];

static void edit_add_new_device_to_self_onenter(EDIT *edit) {
#ifdef ENABLE_MULTIDEVICE
    devices_self_add(edit_add_new_device_to_self.data, edit_add_new_device_to_self.length);
#endif
}


EDIT edit_add_new_device_to_self = {
    .maxlength  = sizeof(edit_add_new_device_to_self_data),
    .data       = edit_add_new_device_to_self_data,
    .onenter    = edit_add_new_device_to_self_onenter,
};
