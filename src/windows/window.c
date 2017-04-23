#include "window.h"

#include "main.h"
#include "notify.h"
#include "events.h"

#include "../branding.h"
#include "../debug.h"
#include "../macros.h"

#include <windows.h>

static HWND l_main;

HINSTANCE curr_instance;

void native_window_init(HINSTANCE instance) {
    static const wchar_t main_classname[] = L"uTox";

    curr_instance = instance;

    black_icon = LoadIcon(curr_instance, MAKEINTRESOURCE(101));
    unread_messages_icon = LoadIcon(curr_instance, MAKEINTRESOURCE(102));

    WNDCLASSW main_window_class = {
        .style         = CS_OWNDC | CS_DBLCLKS,
        .lpfnWndProc   = WindowProc,
        .hInstance     = instance,
        .hIcon         = black_icon,
        .lpszClassName = main_classname,
    };

    RegisterClassW(&main_window_class);
}

void native_window_raze(UTOX_WINDOW *UNUSED(window)) {
    return;
}

static bool update_DC_BM(UTOX_WINDOW *win, int w, int h) {
    win->window_DC = GetDC(win->window);

    win->draw_DC   = CreateCompatibleDC(win->window_DC);
    win->mem_DC    = CreateCompatibleDC(win->draw_DC);

    win->draw_BM   = CreateCompatibleBitmap(win->window_DC, w, h);


    return true;
}

#if 0
static HWND window_create() {
    return NULL;
}
#endif


UTOX_WINDOW *native_window_create_main(int x, int y, int w, int h) {
    static const wchar_t class[] = L"uTox";

    char pretitle[128];
    snprintf(pretitle, 128, "%s %s (version : %s)", TITLE, SUB_TITLE, VERSION);
    size_t  title_size = strlen(pretitle) + 1;
    wchar_t title[title_size];
    mbstowcs(title, pretitle, title_size);


    main_window.window = CreateWindowExW(0, class, title, WS_OVERLAPPEDWINDOW,
                                         x, y, w, h, NULL, NULL, NULL, NULL);

    // We may need to do this after MW_CREATE is called
    update_DC_BM(&main_window, w, h);

    return &main_window;
}

HWND native_window_create_video(int x, int y, int w, int h) {
    LOG_DEBUG("Windows WM", "Creating video window");
    HWND win = CreateWindowExW(0, L"uTox", L"TEMP TITLE CHANGE ME", WS_OVERLAPPEDWINDOW,
                               x, y, w, h, NULL, NULL, curr_instance, NULL);

    if (!win) {
        LOG_ERR("Windows WM", "ERROR trying to create video window");
        LOG_ERR("debug", "%u", GetLastError());
    }

    return win;
}

UTOX_WINDOW *popup = NULL;

UTOX_WINDOW *native_window_create_notify(int x, int y, int w, int h, void *panel) {
    static uint16_t notification_number = 0;

    static wchar_t class_name[] = L"uTox Notification";
    HICON notify_black_icon  = LoadIcon(curr_instance, MAKEINTRESOURCE(101));

    WNDCLASSW notify_window_class = {
        .style         = CS_DBLCLKS,
        .lpfnWndProc   = notify_msg_sys,
        .hInstance     = curr_instance,
        .hIcon         = notify_black_icon,
        .lpszClassName = class_name,
        .hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH),
    };
    RegisterClassW(&notify_window_class);

    char pre[128];
    snprintf(pre, 128, "uTox popup window %u", notification_number++);
    size_t  title_size = strlen(pre) + 1;
    wchar_t title[title_size];
    mbstowcs(title, pre, title_size);

    HWND window = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, class_name, title, WS_POPUP,
                           x, y, w, h, l_main, NULL, NULL, NULL);

    if (!popup) {
        popup = calloc(1, sizeof(UTOX_WINDOW)); // FIXME leaks
        if (!popup) {
            LOG_ERR("Windows Wind", "NativeWindow:\tUnable to alloc to create window container");
            return NULL;
        }
    }
    popup->window = window;

    update_DC_BM(popup, w, h);

    // In case we even need to raise this window to the top most z position.
    // SetWindowPos(window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    ShowWindow(window, SW_SHOWNOACTIVATE);

    popup->_.panel = panel;

    return popup;
}

UTOX_WINDOW *native_window_find_notify(void *window) {
    UTOX_WINDOW *win = popup;
    while (win) {
        if (win->window == *(HWND *)window) {
            return win;
        }
        win = win->_.next;
    }

    return NULL;
}


void native_window_create_screen_select() {
    return;
}

void native_window_tween(UTOX_WINDOW *UNUSED(win)) {
    return;
}
