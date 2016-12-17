#include "main.h"
#include "window.h"

#include "../main.h"
#include "../ui.h"

#include <windowsx.h>

HDC notify_hdc;
HDC notify_dc;
HDC notify_mem;
HBITMAP notify_bm;

static void redraw_notify(void) {
    panel_draw(&panel_root, 0, 0, 400, 400);
}

void enddraw_notify(int x, int y, int width, int height) {
    SelectObject(notify_dc, notify_bm);
    BitBlt(notify_hdc, x, y, width, height, hdc, x, y, SRCCOPY);
}

static LRESULT CALLBACK notify_msg_sys(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) {
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
            POINT min = { SCALE(300), SCALE(300) };
            ((MINMAXINFO *)lParam)->ptMinTrackSize = min;

            break;
        }

        case WM_CREATE: {
            debug_error("NOTIFY::\tCreate\n");
            notify_hdc = GetDC(window);
            notify_dc  = CreateCompatibleDC(notify_hdc);
            notify_mem = CreateCompatibleDC(notify_dc);

            return false;
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


                if (notify_bm) {
                    DeleteObject(notify_bm);
                }

                hdc_bm = CreateCompatibleBitmap(notify_hdc, w, h);
                SelectObject(notify_dc, notify_bm);
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
            BitBlt(notify_hdc, r.left, r.top, r.right - r.left, r.bottom - r.top, notify_dc, r.left, r.top, SRCCOPY);

            EndPaint(window, &ps);
            return false;
        }

        case WM_MOUSEMOVE: {
            debug("NOTIFY::\tMMOVE\n");
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

static uint16_t notification_number = 0;

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
    int notify_h = 200;

    RECT rect;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);

    int x = rect.right - notify_x - notify_w;
    int y = rect.top   + notify_y + notify_h * notification_number;

    ++notification_number;

    char pre[128];
    snprintf(pre, 128, "uTox popup window %u", notification_number);
    size_t  title_size = strlen(pre) + 1;
    wchar_t title[title_size];
    mbstowcs(title, pre, title_size);


    return window_create_notify(parent, class_name, title, x, y, notify_w, notify_h);
}
