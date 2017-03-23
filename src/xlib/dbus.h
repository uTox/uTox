/* dbus.h */
#ifndef uDBUS_H
#define uDBUS_H
#ifdef HAVE_DBUS

#include <stdint.h>

void dbus_notify(char *title, char *content, uint8_t *cid);

#endif // HAVE_DBUS
#endif // uDBUS_H
