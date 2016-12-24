#ifndef WINDOW_H
#define WINDOW_H

#include <stdbool.h>

/* The following is a hollow struct with window vars common across all platforms
 * Each platform's window struct starts with this, then follows with their own
 * specific includes. */
struct utox_window {
    struct native_window *next;

    int x, y, w, h;

    /* TODO do we want to include ui.c here to use PANEL here? */
    void *panel;
};

typedef struct native_window UTOX_WINDOW;

bool window_init(void);

void window_raze(UTOX_WINDOW *window);

void window_create_video();

UTOX_WINDOW *window_create_notify(int x, int y, int w, int h);

void window_tween(void);

void window_create_screen_select(void);


#endif
