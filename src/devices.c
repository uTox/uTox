#include "devices.h"

#include "debug.h"

#ifdef ENABLE_MULTIDEVICE

static bool realloc_devices_list(uint16_t new_size) {
    if (new_size == 0) {
        if (devices) {
            free(devices);
            devices = NULL;
        }
        return 0;
    }

    if (new_size == self.device_list_size) {
        return 1;
    }

    UTOX_DEVICE *tmp = realloc(devices, sizeof(UTOX_DEVICE) * new_size);

    if (!tmp) {
        LOG_ERR("couldn't realloc for new_size %u" , new_size);
        return 0;
    }

    devices               = tmp;
    self.device_list_size = new_size;
    return 1;
}

void utox_devices_init(void) {
    if (devices) {
        LOG_FATAL_ERR(EXIT_FAILURE, "Devices", "Unable to init base devices, *devices exists");
    }

    devices               = calloc(self.device_list_count, sizeof(UTOX_DEVICE));
    self.device_list_size = self.device_list_count;

    if (devices == NULL) {
        LOG_FATAL_ERR(EXIT_MALLOC, "Devices", "Unable to init base devices, *devices is null");
    }
};

void utox_devices_decon(void) {
    if (devices) {
        free(devices);
        devices = NULL;
    }
}

void utox_device_init(Tox *tox, uint16_t dev_num) {
    if (dev_num >= self.device_list_size) {
        if (!realloc_devices_list(dev_num + 1)) {
            LOG_ERR("ERROR, unable to realloc for a new device");
            return;
        }
    }

    if (!devices) {
        LOG_ERR("devices is null");
        return;
    }

    TOX_ERR_DEVICE_GET error = 0;

    tox_self_get_device(tox, dev_num, devices[dev_num].name, &devices[dev_num].status, devices[dev_num].pubkey, &error);

    if (error) {
        LOG_ERR("Error getting device info dev_num %u error %u" , dev_num, error);
    }

    cid_to_string(devices[dev_num].pubkey_hex, devices[dev_num].pubkey);
}

static void devices_self_add_submit(uint8_t *name, size_t length, uint8_t id[TOX_ADDRESS_SIZE]) {
    if (length >= UINT16_MAX) { /* Max size of postmsg */
        LOG_ERR("Name length > UINT16_MAX");
        /* TODO send error to GUI */
        return;
    }

    uint8_t *data = malloc(length * sizeof(uint8_t) + sizeof(id[0]) * TOX_ADDRESS_SIZE);

    memcpy(data, id, TOX_ADDRESS_SIZE);
    memcpy(data + TOX_ADDRESS_SIZE, name, length * sizeof(uint8_t));

    postmessage_toxcore(TOX_SELF_NEW_DEVICE, length, 0, data);
}

static void delete_this_device(void) { LOG_ERR("Delete button pressed"); }

void devices_update_list(void) {}

void devices_update_ui(void) {
    if (!devices) {
        panel_settings_devices.child    = calloc(3, sizeof(void *));
        panel_settings_devices.child[0] = (void *)&button_add_new_device_to_self;
        panel_settings_devices.child[1] = (void *)&edit_add_new_device_to_self;
        panel_settings_devices.child[2] = NULL;
        return;
    }

    if (!panel_settings_devices.child) {
        panel_settings_devices.child    = calloc(3 + self.device_list_count * 2, sizeof(void *));
        panel_settings_devices.child[0] = (void *)&button_add_new_device_to_self;
        panel_settings_devices.child[1] = (void *)&edit_add_new_device_to_self;
    } else {
        panel_settings_devices.child =
            realloc(panel_settings_devices.child, (3 + self.device_list_count * 2) * sizeof(void *));
    }

    uint16_t i;
    for (i = 0; i < self.device_list_count; ++i) {
        EDIT *  edit = calloc(1, sizeof(EDIT));
        BUTTON *dele = calloc(1, sizeof(BUTTON));

        if (!edit) {
            LOG_FATAL_ERR(EXIT_MALLOC, "Devices", "Can't malloc for an extra device");
        }

        PANEL p_edit =
                  {
                    .type   = PANEL_EDIT,
                    .x      = SCALE(10),
                    .y      = SCALE(95) + (i * SCALE(27)),
                    .width  = SCALE(-25) - BM_SBUTTON_WIDTH,
                    .height = SCALE(24),
                  },

              b_delete = {
                  .type   = PANEL_BUTTON,
                  .x      = SCALE(-10) - BM_SBUTTON_WIDTH,
                  .y      = SCALE(95) + (i * SCALE(29)),
                  .width  = BM_SBUTTON_WIDTH,
                  .height = BM_SBUTTON_HEIGHT,
              };

        edit->panel  = p_edit;
        edit->length = TOX_PUBLIC_KEY_SIZE * 2, edit->maxlength = TOX_PUBLIC_KEY_SIZE * 2,
        edit->data = devices[i].pubkey_hex, edit->readonly = 1, edit->noborder = 0, edit->select_completely = 1,

        dele->panel  = b_delete;
        dele->bm_fill     = BM_SBUTTON;
        dele->update = button_setcolors_success, dele->on_mup = delete_this_device,
        dele->button_text.i18nal = STR_DELETE;

        panel_settings_devices.child[(i * 2) + 2] = (void *)edit;
        panel_settings_devices.child[(i * 2) + 3] = (void *)dele;
    }
    panel_settings_devices.child[(i * 2) + 2] = NULL;
}

void devices_self_add(uint8_t *device, size_t length) {
    uint8_t  name_cleaned[length];
    uint16_t length_cleaned = 0;

    unsigned int i;
    for (i = 0; i < length; ++i) {
        if (device[i] != ' ') {
            name_cleaned[length_cleaned] = device[i];
            ++length_cleaned;
        }
    }

    if (!length_cleaned) {
        addfriend_status = ADDF_NONAME;
        return;
    }

    uint8_t id[TOX_ADDRESS_SIZE];

    if (length_cleaned == TOX_ADDRESS_SIZE * 2 && string_to_id(id, name_cleaned)) {
        /* TODO, names! */
        devices_self_add_submit((uint8_t *)"Default device name", 19, id);
    } else {
        LOG_ERR("error trying to add this device");
    }
}

#endif
