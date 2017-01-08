#ifndef WINDOW_H
#define WINDOW_H

#include <stdbool.h>

typedef struct native_window UTOX_WINDOW;
/* The following is a hollow struct with window vars common across all platforms
 * Each platform's window struct starts with this, then follows with their own
 * specific includes. */
struct utox_window {
    UTOX_WINDOW *next;

    int x, y, w, h;

    /* TODO do we want to include ui.c here to use PANEL here? */
    void *panel;
};


bool window_init(void);

void window_raze(UTOX_WINDOW *window);

void window_create_video();

UTOX_WINDOW *window_find_notify(void *window);

UTOX_WINDOW *window_create_notify(int x, int y, int w, int h, void *panel);

void window_tween(void);

void window_create_screen_select(void);


#endif
