#include "window.h"

typedef enum {
    TWEEN_NONE,
    TWEEN_UP,
} NOTIFY_TWEEN;

void enddraw_notify(int x, int y, int width, int height) ;

UTOX_WINDOW *notify_new(void);

void notify_set_focus(void *win); // Needs to take a UTOX_WINDOW pointer

void notify_tween(void);
