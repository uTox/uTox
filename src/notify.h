#ifndef NOTIFY_H
#define NOTIFY_H

typedef struct native_window UTOX_WINDOW;

typedef enum {
    NOTIFY_TYPE_NONE,
    NOTIFY_TYPE_MSG,
    NOTIFY_TYPE_CALL,
    NOTIFY_TYPE_CALL_VIDEO,
} NOTIFY_TYPE;

typedef enum {
    TWEEN_NONE,
    TWEEN_UP,
} NOTIFY_TWEEN;

UTOX_WINDOW *notify_new(NOTIFY_TYPE type);

void notify_tween(void);

#endif
