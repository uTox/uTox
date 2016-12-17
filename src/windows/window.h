#ifndef WIN_WINDOW_H
#define WIN_WINDOW_H

#include "main.h"

HWND window_create_main(wchar_t *class, wchar_t *title, int x, int y, int w, int h);

void window_create_video(void);

HWND window_create_notify(HWND parent, wchar_t *class, wchar_t *title, int x, int y, int w, int h);

void winodw_create_screen_select();

#endif
