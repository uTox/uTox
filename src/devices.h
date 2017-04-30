/* Devices.h
 *
 * Functions to add and remove devices from our Tox device group
 */
#ifndef DEVICES_H
#define DEVICES_H

#include <stdint.h>
#include <tox/tox.h>

typedef struct UTOX_DEVICE {
    uint8_t pubkey[TOX_PUBLIC_KEY_SIZE];
    uint8_t pubkey_hex[TOX_PUBLIC_KEY_SIZE * 2];

    uint8_t name[TOX_MAX_NAME_LENGTH];
    size_t  name_length;

    //TOX_DEVICE_STATUS status;
    uint8_t status;
} UTOX_DEVICE;

UTOX_DEVICE *devices;

void utox_devices_init(void);

void utox_devices_decon(void);

void utox_device_init(Tox *tox, uint16_t dev_num);

void devices_update_list(void);

void devices_update_ui(void);

void devices_self_add(uint8_t *device, size_t length);

#endif
