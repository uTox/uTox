#include "events.h"

#include "main.h"
#include "window.h"

#include "../commands.h"
#include "../debug.h"
#include "../flist.h"
#include "../friend.h"
#include "../macros.h"
#include "../self.h"
#include "../settings.h"
#include "../theme.h"
#include "../tox.h"
#include "../utox.h"

#include "../av/utox_av.h"

#include "../native/clipboard.h"
#include "../native/keyboard.h"
#include "../native/ui.h"

#include "../ui/dropdown.h"
#include "../ui/edit.h"
#include "../ui/svg.h"

#include "../layout/background.h"
#include "../layout/notify.h"
#include "../layout/settings.h"

#include <windowsx.h>

#include "../main.h" // main_width

static TRACKMOUSEEVENT tme = {
    sizeof(TRACKMOUSEEVENT),
    TME_LEAVE,
    0,
    0,
};

static bool mouse_tracked = false;

/** Toggles the main window to/from hidden to tray/shown. */
static void togglehide(int show) {
    if (hidden || show) {
        ShowWindow(main_window.window, SW_RESTORE);
        SetForegroundWindow(main_window.window);
        redraw();
        hidden = false;
    } else {
        ShowWindow(main_window.window, SW_HIDE);
        hidden = true;
    }
}

/** Right click context menu for the tray icon */
static void ShowContextMenu(void) {
    POINT pt;
    GetCursorPos(&pt);
    HMENU hMenu = CreatePopupMenu();
    if (hMenu) {
        InsertMenu(hMenu, -1, MF_BYPOSITION, TRAY_SHOWHIDE, hidden ? "Restore" : "Hide");

        InsertMenu(hMenu, -1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);

        InsertMenu(hMenu, -1, MF_BYPOSITION | ((self.status == TOX_USER_STATUS_NONE) ? MF_CHECKED : 0),
                   TRAY_STATUS_AVAILABLE, "Available");
        InsertMenu(hMenu, -1, MF_BYPOSITION | ((self.status == TOX_USER_STATUS_AWAY) ? MF_CHECKED : 0),
                   TRAY_STATUS_AWAY, "Away");
        InsertMenu(hMenu, -1, MF_BYPOSITION | ((self.status == TOX_USER_STATUS_BUSY) ? MF_CHECKED : 0),
                   TRAY_STATUS_BUSY, "Busy");

        InsertMenu(hMenu, -1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);

        InsertMenu(hMenu, -1, MF_BYPOSITION, TRAY_EXIT, "Exit");

        // note:    must set window to the foreground or the
        //          menu won't disappear when it should
        SetForegroundWindow(main_window.window);

        TrackPopupMenu(hMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, main_window.window, NULL);
        DestroyMenu(hMenu);
    }
}

/* TODO should this be moved to window.c? */
static void move_window(int x, int y){
    debug("delta x == %i\n", x);
    debug("delta y == %i\n", y);
    SetWindowPos(main_window.window, 0, main_window._.x + x, main_window._.y + y, 0, 0,
                          SWP_NOSIZE | SWP_NOZORDER | SWP_NOREDRAW);
    main_window._.x += x;
    main_window._.y += y;
}

#define setstatus(x)                                         \
    if (self.status != x) {                                  \
        postmessage_toxcore(TOX_SELF_SET_STATE, x, 0, NULL); \
        self.status = x;                                     \
        redraw();                                            \
    }

/** Handles all callback requests from winmain();
 *
 * handles the window functions internally, and ships off the tox calls to tox
 */
LRESULT CALLBACK WindowProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) {
    static int mx, my;
    static bool mdown = false;
    static int mdown_x, mdown_y;

    if (main_window.window && window != main_window.window) {
        if (msg == WM_DESTROY) {
            if (window == preview_hwnd) {
                if (settings.video_preview) {
                    settings.video_preview = false;
                    postmessage_utoxav(UTOXAV_STOP_VIDEO, UINT16_MAX, 0, NULL);
                }

                return false;
            }

            for (uint8_t i = 0; i < self.friend_list_count; i++) {
                if (video_hwnd[i] == window) {
                    FRIEND *f = get_friend(i);
                    postmessage_utoxav(UTOXAV_STOP_VIDEO, f->number, 0, NULL);
                    break;
                }
            }
        }

        LOG_TRACE("WinEvent", "Uncaught event %u & %u", wParam, lParam);
        return DefWindowProcW(window, msg, wParam, lParam);
    }

    switch (msg) {
        case WM_QUIT:
        case WM_CLOSE:
        case WM_DESTROY: {
            if (settings.close_to_tray) {
                LOG_INFO("Events", "Closing to tray." );
                togglehide(0);
                return true;
            } else {
                PostQuitMessage(0);
                return false;
            }
        }

        case WM_GETMINMAXINFO: {
            POINT min = { SCALE(MAIN_WIDTH), SCALE(MAIN_HEIGHT) };
            ((MINMAXINFO *)lParam)->ptMinTrackSize = min;

            break;
        }

        case WM_CREATE: {
            LOG_INFO("Windows", "WM_CREATE");
            return false;
        }

        case WM_SIZE: {
            switch (wParam) {
                case SIZE_MAXIMIZED: {
                    settings.window_maximized = true;
                    break;
                }

                case SIZE_RESTORED: {
                    settings.window_maximized = false;
                    break;
                }
            }

            int w = GET_X_LPARAM(lParam);
            int h = GET_Y_LPARAM(lParam);

            if (w != 0) {
                RECT r;
                GetClientRect(window, &r);
                w = r.right;
                h = r.bottom;

                settings.window_width  = w;
                settings.window_height = h;

                ui_rescale(dropdown_dpi.selected + 6);
                ui_size(w, h);

                if (main_window.draw_BM) {
                    DeleteObject(main_window.draw_BM);
                }

                main_window.draw_BM = CreateCompatibleBitmap(main_window.window_DC, settings.window_width,
                                                             settings.window_height);
                SelectObject(main_window.window_DC, main_window.draw_BM);
                redraw();
            }
            break;
        }

        case WM_SETFOCUS: {
            if (flashing) {
                FlashWindow(main_window.window, false);
                flashing = false;

                NOTIFYICONDATAW nid = {
                    .uFlags = NIF_ICON,
                    .hWnd   = main_window.window,
                    .hIcon  = black_icon,
                    .cbSize = sizeof(nid),
                };

                Shell_NotifyIconW(NIM_MODIFY, &nid);
            }

            havefocus = true;
            break;
        }

        case WM_KILLFOCUS: {
            havefocus = false;
            break;
        }

        case WM_ERASEBKGND: {
            return true;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;

            BeginPaint(window, &ps);

            RECT r = ps.rcPaint;
            BitBlt(main_window.window_DC, r.left, r.top, r.right - r.left, r.bottom - r.top,
                   main_window.draw_DC, r.left, r.top, SRCCOPY);

            EndPaint(window, &ps);
            return false;
        }

        case WM_SYSKEYDOWN: // called instead of WM_KEYDOWN when ALT is down or F10 is pressed
        case WM_KEYDOWN: {
            bool control = (GetKeyState(VK_CONTROL) & 0x80) != 0;
            bool shift   = (GetKeyState(VK_SHIFT) & 0x80) != 0;
            bool alt     = (GetKeyState(VK_MENU) & 0x80) != 0; /* Be careful not to clobber alt+num symbols */

            if (wParam >= VK_NUMPAD0 && wParam <= VK_NUMPAD9) {
                // normalize keypad and non-keypad numbers
                wParam = wParam - VK_NUMPAD0 + '0';
            }

            if (control && wParam == 'C') {
                copy(1);
                return false;
            }

            if (control) {
                if ((wParam == VK_TAB && shift) || wParam == VK_PRIOR) {
                    flist_previous_tab();
                    redraw();
                    return false;
                } else if (wParam == VK_TAB || wParam == VK_NEXT) {
                    flist_next_tab();
                    redraw();
                    return false;
                }
            }

            if (control && !alt) {
                if (wParam >= '1' && wParam <= '9') {
                    flist_selectchat(wParam - '1');
                    redraw();
                    return false;
                } else if (wParam == '0') {
                    flist_selectchat(9);
                    redraw();
                    return false;
                }
            }

            if (edit_active()) {
                if (control) {
                    switch (wParam) {
                        case 'V':
                            paste();
                            return false;
                        case 'X':
                            copy(0);
                            edit_char(KEY_DEL, 1, 0);
                            return false;
                    }
                }

                if (control || ((wParam < 'A' || wParam > 'Z') && wParam != VK_RETURN && wParam != VK_BACK)) {
                    edit_char(wParam, 1, (control << 2) | shift);
                }
            } else {
                messages_char(wParam);
                redraw(); // TODO maybe if this
                break;
            }

            break;
        }

        case WM_CHAR: {
            if (edit_active()) {
                if (wParam == KEY_RETURN && (GetKeyState(VK_SHIFT) & 0x80)) {
                    wParam = '\n';
                }

                if (wParam != KEY_TAB) {
                    edit_char(wParam, 0, 0);
                }
            }

            return false;
        }

        case WM_MOUSEWHEEL: {
            double delta = (double)GET_WHEEL_DELTA_WPARAM(wParam);
            mx           = GET_X_LPARAM(lParam);
            my           = GET_Y_LPARAM(lParam);

            panel_mwheel(&panel_root, mx, my, settings.window_width, settings.window_height,
                         delta / (double)(WHEEL_DELTA), 1);
            return false;
        }

        case WM_MOUSEMOVE: {
            int x, y, dx, dy;

            x = GET_X_LPARAM(lParam);
            y = GET_Y_LPARAM(lParam);

            dx = x - mx;
            dy = y - my;
            mx = x;
            my = y;


            if (btn_move_window_down) {
                move_window(x - mdown_x, y - mdown_y);
            }

            cursor = 0;
            panel_mmove(&panel_root, 0, 0, settings.window_width, settings.window_height, x, y, dx, dy);

            SetCursor(cursors[cursor]);

            if (!mouse_tracked) {
                TrackMouseEvent(&tme);
                mouse_tracked = true;
            }

            return false;
        }

        case WM_LBUTTONDOWN: {
            mdown_x = GET_X_LPARAM(lParam);
            mdown_y = GET_Y_LPARAM(lParam);
            // Intentional fall through to save the original mdown location.
        }
        case WM_LBUTTONDBLCLK: {
            mdown = true;

            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);

            if (x != mx || y != my) {
                panel_mmove(&panel_root, 0, 0, settings.window_width, settings.window_height, x, y, x - mx, y - my);
                mx = x;
                my = y;
            }

            // double redraw>
            panel_mdown(&panel_root);
            if (msg == WM_LBUTTONDBLCLK) {
                panel_dclick(&panel_root, 0);
            }

            SetCapture(window);
            break;
        }

        case WM_RBUTTONDOWN: {
            panel_mright(&panel_root);
            break;
        }

        case WM_RBUTTONUP: {
            break;
        }

        case WM_LBUTTONUP: {
            ReleaseCapture();
            break;
        }

        case WM_CAPTURECHANGED: {
            if (mdown) {
                panel_mup(&panel_root);
                mdown = false;
            }

            break;
        }

        case WM_MOUSELEAVE: {
            ui_mouseleave();
            mouse_tracked = false;
            btn_move_window_down = false;
            debug("mouse leave\n");
            break;
        }


        case WM_COMMAND: {
            int menu = LOWORD(wParam); //, msg = HIWORD(wParam);

            switch (menu) {
                case TRAY_SHOWHIDE: {
                    togglehide(0);
                    break;
                }

                case TRAY_EXIT: {
                    PostQuitMessage(0);
                    break;
                }

                case TRAY_STATUS_AVAILABLE: {
                    setstatus(TOX_USER_STATUS_NONE);
                    break;
                }

                case TRAY_STATUS_AWAY: {
                    setstatus(TOX_USER_STATUS_AWAY);
                    break;
                }

                case TRAY_STATUS_BUSY: {
                    setstatus(TOX_USER_STATUS_BUSY);
                    break;
                }
            }

            break;
        }

        case WM_NOTIFYICON: {
            int message = LOWORD(lParam);

            switch (message) {
                case WM_MOUSEMOVE: {
                    break;
                }

                case WM_LBUTTONDOWN: {
                    togglehide(0);
                    break;
                }

                case WM_LBUTTONDBLCLK: {
                    togglehide(1);
                    break;
                }

                case WM_LBUTTONUP: {
                    break;
                }

                case WM_RBUTTONDOWN: {
                    break;
                }

                case WM_RBUTTONUP:
                case WM_CONTEXTMENU: {
                    ShowContextMenu();
                    break;
                }
            }

            return false;
        }

        case WM_COPYDATA: {
            togglehide(1);
            SetForegroundWindow(window);
            COPYDATASTRUCT *data = (void *)lParam;
            if (data->lpData) {
                do_tox_url(data->lpData, data->cbData);
            }

            return false;
        }

        case WM_TOX ... WM_TOX + 128: {
            utox_message_dispatch(msg - WM_TOX, wParam >> 16, wParam, (void *)lParam);
            return false;
        }
    }

    return DefWindowProcW(window, msg, wParam, lParam);
}
