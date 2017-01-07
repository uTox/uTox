#ifndef NOTIFY_H
#define NOTIFY_H

#include "window.h"

typedef enum {
    TWEEN_NONE,
    TWEEN_UP,
} NOTIFY_TWEEN;

UTOX_WINDOW *notify_new(void);

void notify_tween(void);

#endif
