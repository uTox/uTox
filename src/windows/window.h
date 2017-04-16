#ifndef WIN_WINDOW_H
#define WIN_WINDOW_H

#include "../native/window.h"

#include <windef.h>

struct native_window {
    struct utox_window _;

    HWND    window;

    HDC     window_DC;

    HDC     draw_DC;
    HDC     mem_DC;

    HBITMAP draw_BM;
};

extern UTOX_WINDOW main_window;
extern HINSTANCE curr_instance;

#endif
