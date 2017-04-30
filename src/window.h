#ifndef WINDOW_H
#define WINDOW_H

typedef struct native_window UTOX_WINDOW;
typedef struct panel PANEL;

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
UTOX_WINDOW *window_create_notify(int x, int y, int w, int h, PANEL *panel);

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

#endif
