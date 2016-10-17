/* dbus.h */
#ifndef DBUS_H
#define DBUS_H

#include <dbus/dbus.h>
#include <inttypes.h>

void dbus_notify(char *title, char *content, uint8_t *cid);

#endif
