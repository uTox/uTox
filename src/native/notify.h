#ifndef NATIVE_NOTIFY_H
#define NATIVE_NOTIFY_H

void update_tray(void);

void notify(char *title, uint16_t title_length, const char *msg, uint16_t msg_length, void *object, bool is_group);

#endif
