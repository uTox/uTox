#ifndef WIN_NOTIFY_H
#define WIN_NOTIFY_H

#include <windows.h>

LRESULT CALLBACK notify_msg_sys(HWND window, UINT msg, WPARAM wParam, LPARAM lParam);

void native_notify_init(HINSTANCE app_instance);

HWND native_notify_new(HWND parent, HINSTANCE app_instance);

#endif
