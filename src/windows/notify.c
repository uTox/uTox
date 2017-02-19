#include "notify.h"

#include "main.h"
#include "window.h"

#include "../debug.h"
#include "../ui.h"

#include <windowsx.h>

static void redraw_notify(UTOX_WINDOW *win) {
    debug("redraw start\n");
    native_window_set_target(win);
    panel_draw(win->_.panel, 0, 0, win->_.w, win->_.h);
    SelectObject(win->draw_DC, win->draw_BM);
    BitBlt(win->window_DC, win->_.x, win->_.y, win->_.w, win->_.h, win->draw_DC, win->_.x, win->_.y, SRCCOPY);
    debug("redraw end\n");
}

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

        case WM_CREATE: {
            LOG_ERR("Win Notify", "NOTIFY::\tCreate");
            if (win) {
                win->window_DC = GetDC(window);
                win->draw_DC   = CreateCompatibleDC(win->window_DC);
                win->mem_DC    = CreateCompatibleDC(win->draw_DC);
                return false;
            }
            break;
        }

        case WM_SIZE: {
            LOG_ERR("Win Notify", "NOTIFY::\tSize");
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
            LOG_ERR("Win Notify", "NOTIFY::\tBGND");
            redraw_notify(win);
            return true;
        }

        case WM_PAINT: {
            LOG_ERR("Win Notify", "NOTIFY::\tPAINT");
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
