#include "devices.h"

#include "debug.h"
#include "friend.h" // string_to_id needs to be moved -_-
#include "main.h"
#include "self.h"
#include "tox.h"

#include "ui/button.h"
#include "ui/edit.h"
#include "ui/panel.h"
#include "ui/svg.h"
#include "layout/settings.h"

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
        LOG_ERR("Devices", "couldn't realloc for new_size %u" , new_size);
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
            LOG_ERR("Devices", "ERROR, unable to realloc for a new device");
            return;
        }
    }

    if (!devices) {
        LOG_ERR("Devices", "devices is null");
        return;
    }

    TOX_ERR_DEVICE_GET error = 0;

    tox_self_get_device(tox, dev_num, devices[dev_num].name, &devices[dev_num].status, devices[dev_num].pubkey, &error);

    if (error) {
        LOG_ERR("Devices", "Error getting device info dev_num %u error %u" , dev_num, error);
    }

    id_to_string(devices[dev_num].pubkey_hex, devices[dev_num].pubkey);
}

static void devices_self_add_submit(char *name, size_t length, uint8_t id[TOX_ADDRESS_SIZE]) {
    if (length >= UINT16_MAX) { /* Max size of postmsg */
        LOG_ERR("Devices", "Name length > UINT16_MAX");
        /* TODO send error to GUI */
        return;
    }

    uint8_t *data = malloc(length * sizeof(uint8_t) + sizeof(id[0]) * TOX_ADDRESS_SIZE);

    memcpy(data, id, TOX_ADDRESS_SIZE);
    memcpy(data + TOX_ADDRESS_SIZE, name, length * sizeof(uint8_t));

    postmessage_toxcore(TOX_SELF_NEW_DEVICE, length, 0, data);
}

static void delete_this_device(void) { LOG_ERR("Devices", "Delete button pressed"); }

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
        EDIT   *edit = calloc(1, sizeof(EDIT));
        BUTTON *del  = calloc(1, sizeof(BUTTON));

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

        del->panel  = b_delete;
        del->bm     = BM_SBUTTON;
        del->update = button_setcolors_success, del->on_mup = delete_this_device,
        del->button_text.i18nal = STR_DELETE;

        panel_settings_devices.child[(i * 2) + 2] = (void *)edit;
        panel_settings_devices.child[(i * 2) + 3] = (void *)del;
    }
    panel_settings_devices.child[(i * 2) + 2] = NULL;
}

void devices_self_add(char *device, size_t length) {
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
        addfriend_status = 4; // ADDF_NONAME == 4 in dns.h which I'm not willing to include;
        return;
    }

    uint8_t id[TOX_ADDRESS_SIZE];

    if (length_cleaned == TOX_ADDRESS_SIZE * 2 && string_to_id(id, (char*)name_cleaned)) {
        /* TODO, names! */
        devices_self_add_submit("Default device name", 19, id);
    } else {
        LOG_ERR("Devices", "error trying to add this device");
    }
}

#endif
