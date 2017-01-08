#include "notify.h"

#include "main.h"
#include "window.h"
#include "drawing.h"

#include "../main.h"
#include "../ui.h"
#include "../ui/layout_notify.h"

#include <windowsx.h>

static void redraw_notify(UTOX_WINDOW *win) {
    debug("redraw start\n");
    draw_set_curr_win(win);
    panel_draw(&panel_notify, 0, 0, 400, 150); // TODO don't assume 400x150, use *win
    SelectObject(win->draw_DC, win->draw_BM);
    BitBlt(win->window_DC, win->_.x, win->_.y, win->_.w, win->_.h, win->draw_DC, win->_.x, win->_.y, SRCCOPY);
    debug("redraw end\n");
}

static uint16_t notification_number = 0;

LRESULT CALLBACK notify_msg_sys(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) {
    UTOX_WINDOW *win = native_window_find_notify(window);

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
            // window_set_make(window, notification_number);
            break;
        }

        case WM_CREATE: {
            debug_error("NOTIFY::\tCreate\n");
            if (win) {
                win->window_DC = GetDC(window);
                win->draw_DC   = CreateCompatibleDC(win->window_DC);
                win->mem_DC    = CreateCompatibleDC(win->draw_DC);
                return false;
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

                if (win) {
                    if (win->draw_BM) {
                        DeleteObject(win->draw_BM);
                    }
                    win->draw_BM = CreateCompatibleBitmap(win->window_DC, w, h);
                    redraw_notify(win);
                }
            }
            break;
        }

        case WM_ERASEBKGND: {
            debug_error("NOTIFY::\tBGND\n");
            redraw_notify(win);
            return true;
        }

        case WM_PAINT: {
            debug_error("NOTIFY::\tPAINT\n");
            PAINTSTRUCT ps;

            BeginPaint(window, &ps);

            RECT r = ps.rcPaint;
            BitBlt(win->window_DC, r.left, r.top, r.right - r.left, r.bottom - r.top, win->draw_DC, r.left, r.top, SRCCOPY);

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
            redraw_notify(win);
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

static HINSTANCE current_instance = NULL;
void native_notify_init(HINSTANCE instance) {
    current_instance = instance;
}

HWND native_notify_new(HWND parent, HINSTANCE app_instance) {
    debug("Notify:\tCreating Notification #%u\n", notification_number);

    int notify_x = 20;
    int notify_y = 20;

    int notify_w = 400;
    int notify_h = 150;

    RECT rect;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);

    // TODO wrap at max screen size

    // RECT rect;
    // SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);

    // int x = rect.right - notify_x - notify_w;
    // int y = rect.top   + notify_y + (notify_y + notify_h) * notification_number;


    // char pre[128];
    // snprintf(pre, 128, "uTox popup window %u", notification_number);
    // size_t  title_size = strlen(pre) + 1;
    // wchar_t title[title_size];
    // mbstowcs(title, pre, title_size);


    // TODO wrap at max screen size
    int x = rect.right - notify_x - notify_w;
    int y = rect.top   + notify_y + (notify_y + notify_h) * notification_number;

    ++notification_number;

    return native_window_create_notify(x, y, notify_w, notify_h);
}
