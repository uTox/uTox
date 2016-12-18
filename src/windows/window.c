#include "main.h"

static HWND window_create() {

    return NULL;
}

HWND window_create_main(wchar_t *class, wchar_t *title, int x, int y, int w, int h) {
    return CreateWindowExW(0, class, title, WS_OVERLAPPEDWINDOW, x, y, w, h, NULL, NULL, NULL, NULL);
}

void window_create_video() {
    return;
}

HWND window_create_notify(HWND parent, wchar_t *class, wchar_t *title, int x, int y, int w, int h) {
    HWND window = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, class, title, WS_POPUP,
                           x, y, w, h, parent, NULL, NULL, NULL);
    // In case we even need to raise this window to the top most z position.
    // SetWindowPos(window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    ShowWindow(window, SW_SHOWNOACTIVATE);
    return window;
}

void winodw_create_screen_select() {
    return;
}
