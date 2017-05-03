#ifndef NATIVE_WINDOW_H
#define NATIVE_WINDOW_H

#include <stdbool.h>

typedef struct native_window UTOX_WINDOW;
typedef struct panel PANEL;

// The following is a hollow struct with window vars common across all platforms
// Each platform's window struct starts with this, then follows with their own
// specific includes.
struct utox_window {
    UTOX_WINDOW *next;

    int x, y;
    unsigned w, h;

    PANEL *panel;
};


void native_window_raze(UTOX_WINDOW *win);
UTOX_WINDOW *native_window_find_notify(void *win);
UTOX_WINDOW *native_window_create_notify(int x, int y, int w, int h, PANEL *panel);
void native_window_tween(UTOX_WINDOW *win);
void native_window_create_screen_select(void);

/**
 * Sets the target of the next series of drawing commands.
 *
 * Returns true if the window was changed.
 *         false if the window is the same.
 */
bool native_window_set_target(UTOX_WINDOW *new_win);


// These deal with platform-specific structures

#if defined __WIN32__ || defined _WIN32 || defined __CYGWIN__
#include <windef.h>
    void native_window_init(HINSTANCE instance);
    HWND native_window_create_video(int x, int y, int w, int h);
    UTOX_WINDOW *native_window_create_main(int x, int y, int w, int h);
#else
    // Everything else.
    bool native_window_init(void);
    UTOX_WINDOW *native_window_create_video(int x, int y, int w, int h);
    UTOX_WINDOW *native_window_create_main(int x, int y, int w, int h, char **argv, int argc);
#endif


#endif // NATIVE_WINDOW_H
