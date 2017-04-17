/* Devices.h
 *
 * Functions to add and remove devices from our Tox device group
 */
#ifndef DEVICES_H
#define DEVICES_H

#include <inttypes.h>
#include <tox/tox.h>

#if TOX_VERSION_MAJOR > 0
#define ENABLE_MULTIDEVICE 1
#endif

#if TOX_VERSION_PATCH > 80
#define ENABLE_MULTIDEVICE 1
#endif


typedef struct UTOX_DEVICE {
    uint8_t pubkey[TOX_PUBLIC_KEY_SIZE];
    char    pubkey_hex[TOX_PUBLIC_KEY_SIZE * 2];

    uint8_t name[TOX_MAX_NAME_LENGTH];
    size_t  name_length;

    TOX_DEVICE_STATUS status;

} UTOX_DEVICE;

UTOX_DEVICE *devices;

void utox_devices_init(void);

void utox_devices_decon(void);

void utox_device_init(Tox *tox, uint16_t dev_num);

void devices_update_list(void);

void devices_update_ui(void);

void devices_self_add(char *device, size_t length);

#endif
