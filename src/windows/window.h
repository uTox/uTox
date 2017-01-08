#ifndef WIN_WINDOW_H
#define WIN_WINDOW_H

#include "../window.h"

#include "main.h"

#include <windows.h>

struct native_window {
    struct utox_window _;

    HWND    window;

    HDC     window_DC;

    HDC     draw_DC;
    HDC     mem_DC;

    HBITMAP draw_BM;
};

UTOX_WINDOW main_window;

HINSTANCE curr_instance;

void native_window_init(HINSTANCE instance);

void native_window_raze(UTOX_WINDOW *window);

UTOX_WINDOW *native_window_create_main(int x, int y, int w, int h);

HWND native_window_create_video(int x, int y, int w, int h);

UTOX_WINDOW *native_window_create_notify(int x, int y, int w, int h, void *panel);

UTOX_WINDOW *native_window_find_notify(HWND window);

void native_window_create_screen_select();

void native_window_tween(UTOX_WINDOW *win);
#endif
