#ifndef WINDOW_H
#define WINDOW_H

#include <stdbool.h>

typedef struct native_window UTOX_WINDOW;
/* The following is a hollow struct with window vars common across all platforms
 * Each platform's window struct starts with this, then follows with their own
 * specific includes. */
struct utox_window {
    UTOX_WINDOW *next;

    int x, y;
    unsigned w, h;

    /* TODO do we want to include ui.c here to use PANEL here? */
    void *panel;
};

/** window_raze()
 *
 * Cleans and frees all the data related to a created window.
 */
void window_raze(UTOX_WINDOW *window);

/** window_create_video()
 *
 * Currently a no-op
 *
 * Creates a window struct for a popout video window.
 */
void window_create_video(int x, int y, int w, int h);

/** window_find_notify()
 *
 * Finds the struct for a popout interactive notification, when given
 * the "native" type of window. Eg, HWND in Win32 or Window for XLIB
 */
UTOX_WINDOW *window_find_notify(void *window);

/** window_create_notify()
 *
 * Create an interactive notification window and struct.
 */
UTOX_WINDOW *window_create_notify(int x, int y, int w, int h, void *panel);

/** window_tween()
 *
 * Example code to move a window off screen.
 *
 * Pointless UI fun.
 */
void window_tween(UTOX_WINDOW *win);

/** window_create_screen_select()
 *
 * Creates the special window for selecting a portion of screen for
 * screen sharing or desktop images.
 */
void window_create_screen_select(void);

/**
 * Sets the target of the next series of drawing commands.
 *
 * Returns true if the window was changed.
 *         false if the window is the same.
 */
bool native_window_set_target(UTOX_WINDOW *new_win);

#endif
