#include "main.h"
#include "window.h"
#include "drawing.h"

#include "../main.h"
#include "../ui.h"

#include <windowsx.h>

struct n_block {
    HWND window;

    uint16_t number;

    HDC main_DC;
    HDC draw_DC;
    HDC  mem_DC;

    HBITMAP draw_BM;
};

static struct n_block list[20] = { 0 };

static void window_set_make(HWND window, uint16_t number) {
    for (uint8_t i = 0; i < 20; ++i) {
        if (list[i].number == number) {
            list[i].window = window;
            return;
        }
    }

    for (uint8_t i = 0; i < 20; ++i) {
        if (list[i].number == 0) {
            list[i].window = window;
            list[i].number = number;
            return;
        }
    }
}

static struct n_block *window_set_find(HWND window) {
    for (uint8_t i = 0; i < 20; ++i) {
        if (list[i].window == window) {
            return &list[i];
        }
    }
    return NULL;
}

static void redraw_notify(void) {
    for (uint8_t i = 0; i < 20; ++i) {
        if (list[i].window) {
            target_DC = list[i].main_DC;
            active_DC = list[i].draw_DC;
            active_BM = list[i].draw_BM;

            SelectObject(active_DC, active_BM);

            panel_draw(&panel_notify, 0, 0, 400, 150);
        }
    }
}

void enddraw_notify(int x, int y, int width, int height) {
    for (uint8_t i = 0; i < 20; ++i) {
        struct n_block *win = &list[i];
        if (win->window && win->main_DC && win->draw_BM && win->draw_DC) {
            SelectObject(win->draw_DC, win->draw_BM);
            BitBlt(win->main_DC, x, y, width, height, win->draw_DC, x, y, SRCCOPY);
        }
    }
}

static uint16_t notification_number = 0;

static LRESULT CALLBACK notify_msg_sys(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) {
    struct n_block *win = window_set_find(window);

    static int mdown_x, mdown_y;
    switch (msg) {
        case WM_QUIT: {
            debug("NOTIFY::\tQUIT\n");
            break;

        }
        case WM_CLOSE: {
            debug("NOTIFY::\tCLOSE\n");
            break;

        }
        case WM_DESTROY: {
            debug("NOTIFY::\tDESTROY\n");
            break;

        }
        case WM_GETMINMAXINFO: {
            debug("NOTIFY::\tMINMAX_INFO\n");
            POINT min = { SCALE(200), SCALE(200) };
            ((MINMAXINFO *)lParam)->ptMinTrackSize = min;
            break;
        }

        case WM_NCCREATE: {
            window_set_make(window, notification_number);
            break;
        }

        case WM_CREATE: {
            debug_error("NOTIFY::\tCreate\n");
            if (win) {
                win->main_DC = GetDC(window);
                win->draw_DC = CreateCompatibleDC(win->main_DC);
                win->mem_DC  = CreateCompatibleDC(win->draw_DC);
                return 0;
            }
            break;
        }

        case WM_SIZE: {
            debug_error("NOTIFY::\tSize\n");
            int w, h;

            w = GET_X_LPARAM(lParam);
            h = GET_Y_LPARAM(lParam);

            if (w != 0) {
                RECT r;
                GetClientRect(window, &r);
                w = r.right;
                h = r.bottom;

                settings.window_width  = w;
                settings.window_height = h;

                if (win->draw_BM) {
                    DeleteObject(win->draw_BM);
                }

                win->draw_BM = CreateCompatibleBitmap(win->main_DC, w, h);
                redraw_notify();
            }
            break;
        }

        case WM_ERASEBKGND: {
            debug_error("NOTIFY::\tBGND\n");
            return true;
        }

        case WM_PAINT: {
            debug_error("NOTIFY::\tPAINT\n");
            PAINTSTRUCT ps;

            BeginPaint(window, &ps);

            RECT r = ps.rcPaint;
            BitBlt(win->main_DC, r.left, r.top, r.right - r.left, r.bottom - r.top, win->draw_DC, r.left, r.top, SRCCOPY);

            EndPaint(window, &ps);
            return false;
        }

        case WM_MOUSEMOVE: {
            // debug("NOTIFY::\tMMOVE\n");
            return false;
        }

        case WM_LBUTTONDOWN: {
            mdown_x = GET_X_LPARAM(lParam);
            mdown_y = GET_Y_LPARAM(lParam);
            debug("NOTIFY::\tLeft down %i %i\n", mdown_x, mdown_y);
            break;
        }
        case WM_LBUTTONUP: {
            debug("NOTIFY::\tLeft up\n");
            ReleaseCapture();
            redraw_notify();
            break;
        }
        case WM_LBUTTONDBLCLK: {
            debug("NOTIFY::\tDbl click, going to close\n");
            DestroyWindow(window);
            break;
        }

        case WM_RBUTTONDOWN: {
            debug("NOTIFY::\tR BTN DOWN\n");
            break;
        }

        case WM_RBUTTONUP: {
            debug("NOTIFY::\tR BTN UP\n");
            break;
        }

    }

    return DefWindowProcW(window, msg, wParam, lParam);
}

HWND native_notify_new(HWND parent, HINSTANCE app_instance) {
    debug("Notify:\tCreating Notification #%u\n", notification_number);

    wchar_t class_name[] = L"uTox Notification";
    HICON black_icon  = LoadIcon(app_instance, MAKEINTRESOURCE(101));

    WNDCLASSW notify_window_class = {
        .style         = CS_DBLCLKS,
        .lpfnWndProc   = notify_msg_sys,
        .hInstance     = app_instance,
        .hIcon         = black_icon,
        .lpszClassName = class_name,
        .hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH),
    };
    RegisterClassW(&notify_window_class);

    int notify_x = 20;
    int notify_y = 20;

    int notify_w = 400;
    int notify_h = 150;

    RECT rect;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);

    int x = rect.right - notify_x - notify_w;
    int y = rect.top   + notify_y + (notify_y + notify_h) * notification_number;

    ++notification_number;

    char pre[128];
    snprintf(pre, 128, "uTox popup window %u", notification_number);
    size_t  title_size = strlen(pre) + 1;
    wchar_t title[title_size];
    mbstowcs(title, pre, title_size);

    return window_create_notify(parent, class_name, title, x, y, notify_w, notify_h);
}
