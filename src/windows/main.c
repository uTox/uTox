#include "main.h"

#include "../commands.h"
#include "../file_transfers.h"
#include "../filesys.h"
#include "../flist.h"
#include "../friend.h"
#include "../debug.h"
#include "../macros.h"
#include "../main.h"
#include "../settings.h"
#include "../theme.h"
#include "../text.h"
#include "../tox.h"
#include "../utox.h"

#include "../av/utox_av.h"
#include "../ui/draw.h"
#include "../ui/dropdowns.h"
#include "../ui/edit.h"
#include "../ui/svg.h"

#include <windowsx.h>

static bool flashing, desktopgrab_video;
static bool hidden;

static TRACKMOUSEEVENT tme           = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, 0, 0 };
static bool            mouse_tracked = false;

bool  draw      = false;
float scale     = 1.0;
bool  connected = false;
bool  havefocus;

/** Translate a char* from UTF-8 encoding to OS native;
 *
 * Accepts char pointer, native array pointer, length of input;
 * Returns: number of chars writen, or 0 on failure.
 *
 */
static int utf8tonative(char *str, wchar_t *out, int length) {
    return MultiByteToWideChar(CP_UTF8, 0, (char *)str, length, out, length);
}

static int utf8_to_nativestr(char *str, wchar_t *out, int length) {
    /* must be null terminated string                   ↓ */
    return MultiByteToWideChar(CP_UTF8, 0, (char *)str, -1, out, length);
}

/** Open system file browser dialog */
void openfilesend(void) {
    char *filepath = calloc(10, UTOX_FILE_NAME_LENGTH); /* lets pick 10 as the number of files we want to work with. */
    if (filepath == NULL) {
        debug_error("Windows:\t Could not allocate memory for path.\n");
        return;
    }

    wchar_t dir[UTOX_FILE_NAME_LENGTH];
    GetCurrentDirectoryW(countof(dir), dir);

    OPENFILENAME ofn = {
        .lStructSize = sizeof(OPENFILENAME),
        .hwndOwner   = hwnd,
        .lpstrFile   = filepath,
        .nMaxFile    = UTOX_FILE_NAME_LENGTH * 10,
        .Flags       = OFN_EXPLORER | OFN_ALLOWMULTISELECT | OFN_FILEMUSTEXIST,
    };

    if (GetOpenFileName(&ofn)) {
        FRIEND *f = flist_get_selected()->data;
        postmessage_toxcore(TOX_FILE_SEND_NEW, f->number, ofn.nFileOffset, filepath);
    } else {
        debug_error("GetOpenFileName() failed\n");
    }

    SetCurrentDirectoryW(dir);
}

void openfileavatar(void) {
    char *filepath = malloc(UTOX_FILE_NAME_LENGTH);
    filepath[0]    = 0;

    wchar_t dir[UTOX_FILE_NAME_LENGTH];
    GetCurrentDirectoryW(countof(dir), dir);

    OPENFILENAME ofn = {
        .lStructSize = sizeof(OPENFILENAME),
        .lpstrFilter = "Supported Images\0*.GIF;*.PNG;*.JPG;*.JPEG" /* TODO: add all the supported types */
                       "All Files\0*.*\0"
                       "GIF Files\0*.GIF\0"
                       "PNG Files\0*.PNG\0"
                       "JPG Files\0*.JPG;*.JPEG\0"
                       "\0",
        .hwndOwner = hwnd,
        .lpstrFile = filepath,
        .nMaxFile  = UTOX_FILE_NAME_LENGTH,
        .Flags     = OFN_EXPLORER | OFN_FILEMUSTEXIST,
    };

    while (1) { // loop until we have a good file or the user closed the dialog
        if (GetOpenFileName(&ofn)) {
            uint32_t size;

            void *file_data = file_raw(filepath, &size);
            if (!file_data) {
                MessageBox(NULL, (const char *)S(CANT_FIND_FILE_OR_EMPTY), NULL, MB_ICONWARNING);
            } else if (size > UTOX_AVATAR_MAX_DATA_LENGTH) {
                free(file_data);
                char message[1024];
                if (sizeof(message) < SLEN(AVATAR_TOO_LARGE_MAX_SIZE_IS) + 16) {
                    debug("error: AVATAR_TOO_LARGE message is larger than allocated buffer(%zu bytes)\n",
                          sizeof(message));
                    break;
                }
                // create message containing text that selected avatar is too large and what the max size is
                int len = sprintf((char *)message, "%.*s", SLEN(AVATAR_TOO_LARGE_MAX_SIZE_IS),
                                  S(AVATAR_TOO_LARGE_MAX_SIZE_IS));
                len += sprint_humanread_bytes(message + len, sizeof(message) - len, UTOX_AVATAR_MAX_DATA_LENGTH);
                message[len++] = '\0';
                MessageBox(NULL, (char *)message, NULL, MB_ICONWARNING);
            } else {
                postmessage_utox(SELF_AVATAR_SET, size, 0, file_data);
                break;
            }
        } else {
            debug("GetOpenFileName() failed when trying to grab an avatar.\n");
            break;
        }
    }

    free(filepath);
    SetCurrentDirectoryW(dir);
}

void file_save_inline(FILE_TRANSFER *file) {
    char *path = calloc(1, UTOX_FILE_NAME_LENGTH);
    if (path == NULL) {
        debug_error("file_save_inline:\tCould not allocate memory for path.\n");
        return;
    }
    strcpy(path, file->path);

    OPENFILENAME ofn = {
        .lStructSize = sizeof(OPENFILENAME),
        .hwndOwner   = hwnd,
        .lpstrFile   = path,
        .nMaxFile    = UTOX_FILE_NAME_LENGTH,
        .Flags       = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT,
    };

    if (GetSaveFileName(&ofn)) {
        FILE *fp = fopen(path, "wb");
        if (fp) {
            fwrite(file->via.memory, file->target_size, 1, fp);
            fclose(fp);

            snprintf((char *)file->path, UTOX_FILE_NAME_LENGTH, "inline.png");
        } else {
            debug_error("file_save_inline:\tCouldn't open path: `%s` to save inline file.", path);
        }
    } else {
        debug_error("GetSaveFileName() failed\n");
    }
    free(path);
}
















int native_to_utf8str(wchar_t *str_in, char *str_out, uint32_t max_size) {
    /* must be null terminated string          ↓                     */
    return WideCharToMultiByte(CP_UTF8, 0, str_in, -1, str_out, max_size, NULL, NULL);
}

void postmessage_utox(UTOX_MSG msg, uint16_t param1, uint16_t param2, void *data) {
    PostMessage(hwnd, WM_TOX + (msg), ((param1) << 16) | (param2), (LPARAM)data);
}

void init_ptt(void) {
    settings.push_to_talk = true;
}

bool check_ptt_key(void) {
    if (!settings.push_to_talk) {
        // debug("PTT is disabled\n");
        return true; /* If push to talk is disabled, return true. */
    }

    if (GetAsyncKeyState(VK_LCONTROL)) {
        // debug("PTT key is down\n");
        return true;
    } else {
        // debug("PTT key is up\n");
        return false;
    }
}

void exit_ptt(void) {
    settings.push_to_talk = false;
}

void thread(void func(void *), void *args) {
    _beginthread(func, 0, args);
}

void yieldcpu(uint32_t ms) {
    Sleep(ms);
}

uint64_t get_time(void) {
    return ((uint64_t)clock() * 1000 * 1000);
}

void openurl(char *str) {
    //! convert
    ShellExecute(NULL, "open", (char *)str, NULL, NULL, SW_SHOW);
}

void setselection(char *data, uint16_t length) {}

/** Toggles the main window to/from hidden to tray/shown. */
void togglehide(int show) {
    if (hidden || show) {
        ShowWindow(hwnd, SW_RESTORE);
        SetForegroundWindow(hwnd);
        redraw();
        hidden = false;
    } else {
        ShowWindow(hwnd, SW_HIDE);
        hidden = true;
    }
}

/** Right click context menu for the tray icon */
void ShowContextMenu(void) {
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
        SetForegroundWindow(hwnd);

        TrackPopupMenu(hMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, NULL);
        DestroyMenu(hMenu);
    }
}

// creates an UTOX_NATIVE image based on given arguments
// image should be freed with image_free
static NATIVE_IMAGE *create_utox_image(HBITMAP bmp, bool has_alpha, uint32_t width, uint32_t height) {
    NATIVE_IMAGE *image = malloc(sizeof(NATIVE_IMAGE));
    if (image == NULL) {
        debug("create_utox_image:\t Could not allocate memory for image.\n");
        return NULL;
    }
    image->bitmap        = bmp;
    image->has_alpha     = has_alpha;
    image->width         = width;
    image->height        = height;
    image->scaled_width  = width;
    image->scaled_height = height;
    image->stretch_mode  = COLORONCOLOR;

    return image;
}

static void sendbitmap(HDC mem, HBITMAP hbm, int width, int height) {
    if (width == 0 || height == 0)
        return;

    BITMAPINFO info = {.bmiHeader = {
                           .biSize        = sizeof(BITMAPINFOHEADER),
                           .biWidth       = width,
                           .biHeight      = -(int)height,
                           .biPlanes      = 1,
                           .biBitCount    = 24,
                           .biCompression = BI_RGB,
                       } };

    void *bits = malloc((width + 3) * height * 3);

    GetDIBits(mem, hbm, 0, height, bits, &info, DIB_RGB_COLORS);

    uint8_t pbytes = width & 3, *p = bits, *pp = bits, *end = p + width * height * 3;
    // uint32_t offset = 0;
    while (p != end) {
        int i;
        for (i = 0; i != width; i++) {
            uint8_t b    = pp[i * 3];
            p[i * 3]     = pp[i * 3 + 2];
            p[i * 3 + 1] = pp[i * 3 + 1];
            p[i * 3 + 2] = b;
        }
        p += width * 3;
        pp += width * 3 + pbytes;
    }

    int      size = 0;
    uint8_t *out  = stbi_write_png_to_mem(bits, 0, width, height, 3, &size);
    free(bits);

    NATIVE_IMAGE *image = create_utox_image(hbm, 0, width, height);
    friend_sendimage(flist_get_selected()->data, image, width, height, (UTOX_IMAGE)out, size);
}

void copy(int value) {
    char data[32768]; //! TODO: De-hardcode this value.
    int  len;

    if (edit_active()) {
        len       = edit_copy(data, 32767);
        data[len] = 0;
    } else if (flist_get_selected()->item == ITEM_FRIEND) {
        len = messages_selection(&messages_friend, data, 32768, value);
    } else if (flist_get_selected()->item == ITEM_GROUP) {
        len = messages_selection(&messages_group, data, 32768, value);
    } else {
        return;
    }

    HGLOBAL  hMem = GlobalAlloc(GMEM_MOVEABLE, (len + 1) * 2);
    wchar_t *d    = GlobalLock(hMem);
    utf8tonative(data, d, len + 1); // because data is nullterminated
    GlobalUnlock(hMem);
    OpenClipboard(hwnd);
    EmptyClipboard();
    SetClipboardData(CF_UNICODETEXT, hMem);
    CloseClipboard();
}

void paste(void) {
    OpenClipboard(NULL);
    HANDLE h = GetClipboardData(CF_UNICODETEXT);
    if (!h) {
        h = GetClipboardData(CF_BITMAP);
        if (h && flist_get_selected()->item == ITEM_FRIEND) {
            FRIEND *f = flist_get_selected()->data;
            if (!f->online) {
                return;
            }
            HBITMAP copy;
            BITMAP  bm;
            HDC     tempdc;
            GetObject(h, sizeof(bm), &bm);

            tempdc = CreateCompatibleDC(NULL);
            SelectObject(tempdc, h);

            copy = CreateCompatibleBitmap(hdcMem, bm.bmWidth, bm.bmHeight);
            SelectObject(hdcMem, copy);
            BitBlt(hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, tempdc, 0, 0, SRCCOPY);

            sendbitmap(hdcMem, copy, bm.bmWidth, bm.bmHeight);

            DeleteDC(tempdc);
        }
    } else {
        wchar_t *d = GlobalLock(h);
        char     data[65536]; // TODO: De-hardcode this value.
        int      len = WideCharToMultiByte(CP_UTF8, 0, d, -1, (char *)data, sizeof(data), NULL, 0);
        if (edit_active()) {
            edit_paste(data, len, 0);
        }
    }

    GlobalUnlock(h);
    CloseClipboard();
}

NATIVE_IMAGE *utox_image_to_native(const UTOX_IMAGE data, size_t size, uint16_t *w, uint16_t *h, bool keep_alpha) {
    int      width, height, bpp;
    uint8_t *rgba_data = stbi_load_from_memory(data, size, &width, &height, &bpp, 4);

    if (rgba_data == NULL || width == 0 || height == 0) {
        return NULL; // invalid image
    }

    BITMAPINFO bmi = {.bmiHeader = {
                          .biSize        = sizeof(BITMAPINFOHEADER),
                          .biWidth       = width,
                          .biHeight      = -height,
                          .biPlanes      = 1,
                          .biBitCount    = 32,
                          .biCompression = BI_RGB,
                      } };

    // create device independent bitmap, we can write the bytes to out
    // to put them in the bitmap
    uint8_t *out;
    HBITMAP  bmp = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, (void **)&out, NULL, 0);

    // convert RGBA data to internal format
    // pre-applying the alpha if we're keeping the alpha channel,
    // put the result in out
    // NOTE: input pixels are in format RGBA, output is BGRA
    uint8_t *p, *end = rgba_data + width * height * 4;
    p = rgba_data;
    if (keep_alpha) {
        uint8_t alpha;
        do {
            alpha  = p[3];
            out[0] = p[2] * (alpha / 255.0); // pre-apply alpha
            out[1] = p[1] * (alpha / 255.0);
            out[2] = p[0] * (alpha / 255.0);
            out[3] = alpha;
            out += 4;
            p += 4;
        } while (p != end);
    } else {
        do {
            out[0] = p[2];
            out[1] = p[1];
            out[2] = p[0];
            out[3] = 0;
            out += 4;
            p += 4;
        } while (p != end);
    }

    free(rgba_data);


    NATIVE_IMAGE *image = create_utox_image(bmp, keep_alpha, width, height);

    *w = width;
    *h = height;
    return image;
}

void image_free(NATIVE_IMAGE *image) {
    if (!image) {
        return;
    }
    DeleteObject(image->bitmap);
    free(image);
}

void flush_file(FILE *file) {
    fflush(file);
    int fd = _fileno(file);
    _commit(fd);
}

int ch_mod(uint8_t *file) {
    /* You're probably looking for ./xlib as windows is lamesauce and wants nothing to do with sane permissions */
    return true;
}

int file_lock(FILE *file, uint64_t start, size_t length) {
    OVERLAPPED lock_overlap;
    lock_overlap.Offset     = start;
    lock_overlap.OffsetHigh = start + length;
    lock_overlap.hEvent     = 0;
    return !LockFileEx(file, LOCKFILE_FAIL_IMMEDIATELY, 0, start, start + length, &lock_overlap);
}

int file_unlock(FILE *file, uint64_t start, size_t length) {
    OVERLAPPED lock_overlap;
    lock_overlap.Offset     = start;
    lock_overlap.OffsetHigh = start + length;
    lock_overlap.hEvent     = 0;
    return UnlockFileEx(file, 0, start, start + length, &lock_overlap);
}

/** Creates a tray baloon popup with the message, and flashes the main window
 *
 * accepts: char *title, title length, char *msg, msg length;
 * returns void;
 */
void notify(char *title, uint16_t title_length, const char *msg, uint16_t msg_length, void *object, bool is_group) {
    if (havefocus || self.status == 2) {
        return;
    }

    FlashWindow(hwnd, 1);
    flashing = true;

    NOTIFYICONDATAW nid = {
        .uFlags      = NIF_ICON | NIF_INFO,
        .hWnd        = hwnd,
        .hIcon       = unread_messages_icon,
        .uTimeout    = 5000,
        .dwInfoFlags = 0,
        .cbSize      = sizeof(nid),
    };

    utf8tonative(title, nid.szInfoTitle, title_length > sizeof(nid.szInfoTitle) / sizeof(*nid.szInfoTitle) - 1 ?
                                             sizeof(nid.szInfoTitle) / sizeof(*nid.szInfoTitle) - 1 :
                                             title_length);
    utf8tonative(msg, nid.szInfo, msg_length > sizeof(nid.szInfo) / sizeof(*nid.szInfo) - 1 ?
                                      sizeof(nid.szInfo) / sizeof(*nid.szInfo) - 1 :
                                      msg_length);

    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

void showkeyboard(bool show) {} /* Added for android support. */

void edit_will_deactivate(void) {}

/* Redraws the main UI window */
void redraw(void) {
    panel_draw(&panel_root, 0, 0, settings.window_width, settings.window_height);
}

/**
 * update_tray(void)
 * creates a win32 NOTIFYICONDATAW struct, sets the tiptab flag, gives *hwnd,
 * sets struct .cbSize, and resets the tibtab to native self.name;
 */
void update_tray(void) {
    uint32_t tip_length;
    char *   tip;

    /* TODO; this is likely to over/under-run FIXME! */

    tip = malloc(128 * sizeof(char)); // 128 is the max length of nid.szTip
    if (tip == NULL) {
        debug("update_trip:\t Could not allocate memory.\n");
        return;
    }

    snprintf(tip, 127 * sizeof(char), "%s : %s", self.name, self.statusmsg);
    tip_length = self.name_length + 3 + self.statusmsg_length;

    NOTIFYICONDATAW nid = {
        .uFlags = NIF_TIP, .hWnd = hwnd, .cbSize = sizeof(nid),
    };

    utf8_to_nativestr((char *)tip, nid.szTip, tip_length);

    Shell_NotifyIconW(NIM_MODIFY, &nid);

    free(tip);
}

void force_redraw(void) {
    redraw();
}

void desktopgrab(bool video) {
    int x, y, w, h;

    x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    h = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    debug("result: %i %i %i %i\n", x, y, w, h);

    capturewnd = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_LAYERED, L"uToxgrab", L"Tox", WS_POPUP, x, y, w, h, NULL,
                                 NULL, hinstance, NULL);
    if (!capturewnd) {
        debug("CreateWindowExW() failed\n");
        return;
    }

    SetLayeredWindowAttributes(capturewnd, 0xFFFFFF, 128, LWA_ALPHA | LWA_COLORKEY);


    // UpdateLayeredWindow(hwnd, NULL, NULL, NULL, NULL, NULL, 0xFFFFFF, ULW_ALPHA | ULW_COLORKEY);

    ShowWindow(capturewnd, SW_SHOW);
    SetForegroundWindow(capturewnd);

    desktopgrab_video = video;

    // SetCapture(hwnd);
    // grabbing = true;

    // postmessage_video(VIDEO_SET, 0, 0, (void*)1);
}

LRESULT CALLBACK GrabProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) {
    POINT p = {.x = GET_X_LPARAM(lParam), .y = GET_Y_LPARAM(lParam) };

    ClientToScreen(window, &p);

    if (msg == WM_MOUSEMOVE) {

        if (grabbing) {
            HDC dc = GetDC(window);
            BitBlt(dc, video_grab_x, video_grab_y, video_grab_w - video_grab_x, video_grab_h - video_grab_y, dc,
                   video_grab_x, video_grab_y, BLACKNESS);
            video_grab_w = p.x;
            video_grab_h = p.y;
            BitBlt(dc, video_grab_x, video_grab_y, video_grab_w - video_grab_x, video_grab_h - video_grab_y, dc,
                   video_grab_x, video_grab_y, WHITENESS);
            ReleaseDC(window, dc);
        }

        return false;
    }

    if (msg == WM_LBUTTONDOWN) {
        video_grab_x = video_grab_w = p.x;
        video_grab_y = video_grab_h = p.y;
        grabbing                    = true;
        SetCapture(window);
        return false;
    }

    if (msg == WM_LBUTTONUP) {
        ReleaseCapture();
        grabbing = false;

        if (video_grab_x < video_grab_w) {
            video_grab_w -= video_grab_x;
        } else {
            const int w  = video_grab_x - video_grab_w;
            video_grab_x = video_grab_w;
            video_grab_w = w;
        }

        if (video_grab_y < video_grab_h) {
            video_grab_h -= video_grab_y;
        } else {
            const int w  = video_grab_y - video_grab_h;
            video_grab_y = video_grab_h;
            video_grab_h = w;
        }

        if (desktopgrab_video) {
            DestroyWindow(window);
            postmessage_utoxav(UTOXAV_SET_VIDEO_IN, 1, 0, NULL);
        } else {
            FRIEND *f = flist_get_selected()->data;
            if (flist_get_selected()->item == ITEM_FRIEND && f->online) {
                DestroyWindow(window);
                HWND dwnd = GetDesktopWindow();
                HDC  ddc  = GetDC(dwnd);
                HDC  mem  = CreateCompatibleDC(ddc);

                HBITMAP capture = CreateCompatibleBitmap(ddc, video_grab_w, video_grab_h);
                SelectObject(mem, capture);

                BitBlt(mem, 0, 0, video_grab_w, video_grab_h, ddc, video_grab_x, video_grab_y, SRCCOPY | CAPTUREBLT);
                sendbitmap(mem, capture, video_grab_w, video_grab_h);

                ReleaseDC(dwnd, ddc);
                DeleteDC(mem);
            }
        }


        return false;
    }

    if (msg == WM_DESTROY) {
        grabbing = false;
    }

    return DefWindowProcW(window, msg, wParam, lParam);
}

void freefonts() {
    for (size_t i = 0; i != countof(font); i++) {
        if (font[i]) {
            DeleteObject(font[i]);
        }
    }
}

void loadfonts() {
    LOGFONT lf = {
        .lfWeight = FW_NORMAL,
        //.lfCharSet = ANSI_CHARSET,
        .lfOutPrecision = OUT_TT_PRECIS,
        .lfQuality      = DEFAULT_QUALITY,
        .lfFaceName     = "DejaVu Sans",
    };

    lf.lfHeight          = (SCALE(-24) - 1) / 2;
    font[FONT_TEXT]      = CreateFontIndirect(&lf);
    lf.lfHeight          = (SCALE(-22) - 1) / 2;
    font[FONT_STATUS]    = CreateFontIndirect(&lf);
    lf.lfHeight          = (SCALE(-24) - 1) / 2;
    font[FONT_LIST_NAME] = CreateFontIndirect(&lf);
    lf.lfWeight          = FW_BOLD;
    font[FONT_TITLE]     = CreateFontIndirect(&lf);
    lf.lfHeight          = (SCALE(-28) - 1) / 2;
    font[FONT_SELF_NAME] = CreateFontIndirect(&lf);
    lf.lfHeight          = (SCALE(-20) - 1) / 2;
    font[FONT_MISC]      = CreateFontIndirect(&lf);
    /*lf.lfWeight = FW_NORMAL; //FW_LIGHT <- light fonts dont antialias
    font[FONT_MSG_NAME] = CreateFontIndirect(&lf);
    lf.lfHeight = F(11);
    font[FONT_MSG] = CreateFontIndirect(&lf);
    lf.lfUnderline = 1;
    font[FONT_MSG_LINK] = CreateFontIndirect(&lf);*/

    TEXTMETRIC tm;
    SelectObject(hdc, font[FONT_TEXT]);
    GetTextMetrics(hdc, &tm);
    font_small_lineheight = tm.tmHeight + tm.tmExternalLeading;
    // SelectObject(hdc, font[FONT_MSG]);
    // GetTextMetrics(hdc, &tm);
    // font_msg_lineheight = tm.tmHeight + tm.tmExternalLeading;
}

void setscale_fonts(void) {
    freefonts();
    loadfonts();
}

void setscale(void) {
    svg_draw(1);
}

void config_osdefaults(UTOX_SAVE *r) {
    r->window_x      = (GetSystemMetrics(SM_CXSCREEN) - MAIN_WIDTH) / 2;
    r->window_y      = (GetSystemMetrics(SM_CYSCREEN) - MAIN_HEIGHT) / 2;
    r->window_width  = MAIN_WIDTH;
    r->window_height = MAIN_HEIGHT;
}

/*
 * CommandLineToArgvA implementation since CommandLineToArgvA doesn't exist in win32 api
 * Limitation: nested quotation marks are not handled
 * Credit: http://alter.org.ua/docs/win/args
 */
PCHAR *CommandLineToArgvA(PCHAR CmdLine, int *_argc) {
    PCHAR *argv;
    PCHAR  _argv;
    ULONG  len;
    ULONG  argc;
    CHAR   a;
    ULONG  i, j;

    BOOLEAN in_QM;
    BOOLEAN in_TEXT;
    BOOLEAN in_SPACE;

    len = strlen(CmdLine);
    i   = ((len + 2) / 2) * sizeof(PVOID) + sizeof(PVOID);

    argv = (PCHAR *)GlobalAlloc(GMEM_FIXED, i + (len + 2) * sizeof(CHAR));

    _argv = (PCHAR)(((PUCHAR)argv) + i);

    argc       = 0;
    argv[argc] = _argv;
    in_QM      = FALSE;
    in_TEXT    = FALSE;
    in_SPACE   = TRUE;
    i          = 0;
    j          = 0;

    while ((a = CmdLine[i])) {
        if (in_QM) {
            if (a == '\"') {
                in_QM = FALSE;
            } else {
                _argv[j] = a;
                j++;
            }
        } else {
            switch (a) {
                case '\"':
                    in_QM   = TRUE;
                    in_TEXT = TRUE;
                    if (in_SPACE) {
                        argv[argc] = _argv + j;
                        argc++;
                    }
                    in_SPACE = FALSE;
                    break;
                case ' ':
                case '\t':
                case '\n':
                case '\r':
                    if (in_TEXT) {
                        _argv[j] = '\0';
                        j++;
                    }
                    in_TEXT  = FALSE;
                    in_SPACE = TRUE;
                    break;
                default:
                    in_TEXT = TRUE;
                    if (in_SPACE) {
                        argv[argc] = _argv + j;
                        argc++;
                    }
                    _argv[j] = a;
                    j++;
                    in_SPACE = FALSE;
                    break;
            }
        }
        i++;
    }
    _argv[j]   = '\0';
    argv[argc] = NULL;

    (*_argc) = argc;
    return argv;
}

static bool auto_update(PSTR cmd) {
    char path[MAX_PATH + 20];
    int  len = GetModuleFileName(NULL, path, MAX_PATH);

    /* Is the uTox exe named like the updater one. */
    char *file = path + len - (sizeof("uTox.exe") - 1);
    if (len > sizeof("uTox.exe")) {
        memcpy(file, "uTox_updater.exe", sizeof("uTox_updater.exe"));
        FILE *fp = fopen(path, "rb");
        if (fp) {
            char real_cmd[strlen(cmd) * 2];
            snprintf(real_cmd, strlen(cmd) * 2, "%s %S %2x%2x%2x", cmd, "--version ", VER_MAJOR, VER_MINOR, VER_PATCH);
            fclose(fp);
            /* This is an updater build not being run by the updater. Run the updater and exit. */
            ShellExecute(NULL, "open", path, real_cmd, NULL, SW_SHOW);
            return true;
        }
    }
    return false;
}

/** client main()
 *
 * Main thread
 * generates settings, loads settings from save file, generates main UI, starts
 * tox, generates tray icon, handles client messages. Cleans up, and exits.
 *
 * also handles call from other apps.
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR cmd, int nCmdShow) {

    pthread_mutex_init(&messages_lock, NULL);

    /* if opened with argument, check if uTox is already open and pass the argument to the existing process */
    HANDLE utox_mutex = CreateMutex(NULL, 0, TITLE);

    if (!utox_mutex) {
        return false;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND window = FindWindow(TITLE, NULL);
        if (window) {
            COPYDATASTRUCT data = {.cbData = strlen(cmd), .lpData = cmd };
            SendMessage(window, WM_COPYDATA, (WPARAM)hInstance, (LPARAM)&data);
        }
        return false;
    }

    /* Process argc/v the backwards (read: windows) way. */
    PCHAR *argv;
    int    argc;
    argv = CommandLineToArgvA(GetCommandLineA(), &argc);

    if (NULL == argv) {
        debug("CommandLineToArgvA failed\n");
        return true;
    }

    bool   theme_was_set_on_argv;
    int8_t should_launch_at_startup;
    int8_t set_show_window;
    bool   skip_updater, from_updater;

    parse_args(argc, argv, &skip_updater, &from_updater, &theme_was_set_on_argv,
               &should_launch_at_startup, &set_show_window );

    if (settings.portable_mode == true) {
        /* force the working directory if opened with portable command */
        HMODULE      hModule = GetModuleHandle(NULL);
        char         path[MAX_PATH];
        int          len = GetModuleFileName(hModule, path, MAX_PATH);
        unsigned int i;
        for (i = (len - 1); path[i] != '\\'; --i);
        path[i] = 0; //!
        SetCurrentDirectory(path);
        strcpy(portable_mode_save_path, (char *)path);
    }

    if (should_launch_at_startup == 1) {
        launch_at_startup(1);
    } else if (should_launch_at_startup == -1) {
        launch_at_startup(0);
    }

    debug_error("skip updater\n");
    if (!skip_updater) {
        debug_error("don't skip updater\n");
        if (auto_update(cmd)) {
            CloseHandle(utox_mutex);
            return 0;
        }
    }

    #ifdef __WIN_LEGACY
        debug("Legacy windows build\n");
    #else
        debug("Normal windows build\n");
    #endif

    // Free memory allocated by CommandLineToArgvA
    GlobalFree(argv);

    #ifdef GIT_VERSION
        debug_notice("uTox version %s \n", GIT_VERSION);
    #endif

    /* */
    MSG msg;
    // int x, y;
    wchar_t classname[] = L"uTox", popupclassname[] = L"uToxgrab";

    my_icon              = LoadIcon(hInstance, MAKEINTRESOURCE(101));
    unread_messages_icon = LoadIcon(hInstance, MAKEINTRESOURCE(102));

    cursors[CURSOR_NONE]     = LoadCursor(NULL, IDC_ARROW);
    cursors[CURSOR_HAND]     = LoadCursor(NULL, IDC_HAND);
    cursors[CURSOR_TEXT]     = LoadCursor(NULL, IDC_IBEAM);
    cursors[CURSOR_SELECT]   = LoadCursor(NULL, IDC_CROSS);
    cursors[CURSOR_ZOOM_IN]  = LoadCursor(NULL, IDC_SIZEALL);
    cursors[CURSOR_ZOOM_OUT] = LoadCursor(NULL, IDC_SIZEALL);

    hinstance = hInstance;

    WNDCLASSW wc =
                  {
                    .style         = CS_OWNDC | CS_DBLCLKS,
                    .lpfnWndProc   = WindowProc,
                    .hInstance     = hInstance,
                    .hIcon         = my_icon,
                    .lpszClassName = classname,
                  },

              wc2 = {
                  .lpfnWndProc   = GrabProc,
                  .hInstance     = hInstance,
                  .hIcon         = my_icon,
                  .lpszClassName = popupclassname,
                  .hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH),
              };

    NOTIFYICONDATA nid = {
        .uFlags           = NIF_MESSAGE | NIF_ICON | NIF_TIP,
        .uCallbackMessage = WM_NOTIFYICON,
        .hIcon            = my_icon,
        .szTip            = "uTox default tooltip",
        .cbSize           = sizeof(nid),
    };

    OleInitialize(NULL);
    RegisterClassW(&wc);
    RegisterClassW(&wc2);


    uint16_t langid = GetUserDefaultUILanguage() & 0xFFFF;
    LANG            = ui_guess_lang_by_windows_lang_id(langid, DEFAULT_LANG);

    dropdown_language.selected = dropdown_language.over = LANG;

    UTOX_SAVE *save = config_load();

    if (!theme_was_set_on_argv) {
        dropdown_theme.selected = save->theme;
        settings.theme          = save->theme;
    }
    theme_load(settings.theme);

    utox_init();

    save->window_width  = save->window_width < SCALE(MAIN_WIDTH) ? SCALE(MAIN_WIDTH) : save->window_width;
    save->window_height = save->window_height < SCALE(MAIN_HEIGHT) ? SCALE(MAIN_HEIGHT) : save->window_height;

    char pretitle[128];
    snprintf(pretitle, 128, "%s %s (version : %s)", TITLE, SUB_TITLE, VERSION);
    size_t  title_size = strlen(pretitle) + 1;
    wchar_t title[title_size];
    mbstowcs(title, pretitle, title_size);
    /* trim first letter that appears for god knows why */
    /* needed if/when the uTox becomes a muTox */
    // wmemmove(title, title+1, wcslen(title));

    hwnd = CreateWindowExW(0, classname, title, WS_OVERLAPPEDWINDOW, save->window_x, save->window_y, save->window_width,
                           save->window_height, NULL, NULL, hInstance, NULL);


    hdc_brush = GetStockObject(DC_BRUSH);


    tme.hwndTrack = hwnd;

    nid.hWnd = hwnd;
    Shell_NotifyIcon(NIM_ADD, &nid);

    SetBkMode(hdc, TRANSPARENT);

    dnd_init(hwnd);

    // start tox thread (hwnd needs to be set first)
    thread(toxcore_thread, NULL);

    // wait for tox_thread init
    while (!tox_thread_init && !settings.save_encryption) {
        yieldcpu(1);
    }

    if (*cmd) {
        int len = strlen(cmd);
        do_tox_url((uint8_t *)cmd, len);
    }

    draw = true;
    redraw();
    update_tray();

    /* From --set flag */
    if (set_show_window) {
        if (set_show_window == 1) {
            settings.start_in_tray = false;
        } else if (set_show_window == -1) {
            settings.start_in_tray = true;
        }
    }

    if (settings.start_in_tray) {
        ShowWindow(hwnd, SW_HIDE);
        hidden = true;
    } else {
        ShowWindow(hwnd, nCmdShow);
    }

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    /* kill threads */
    postmessage_utoxav(UTOXAV_KILL, 0, 0, NULL);
    postmessage_toxcore(TOX_KILL, 0, 0, NULL);

    /* cleanup */

    /* delete tray icon */
    Shell_NotifyIcon(NIM_DELETE, &nid);

    // wait for tox_thread to exit
    while (tox_thread_init) {
        yieldcpu(10);
    }

    RECT wndrect = { 0 };
    GetWindowRect(hwnd, &wndrect);
    UTOX_SAVE d = {
        .window_x      = wndrect.left < 0 ? 0 : wndrect.left,
        .window_y      = wndrect.top < 0 ? 0 : wndrect.top,
        .window_width  = (wndrect.right - wndrect.left),
        .window_height = (wndrect.bottom - wndrect.top),
    };
    config_save(&d);

    debug_info("uTox:\tClean exit.\n");

    return false;
}

/** Handles all callback requests from winmain();
 *
 * handles the window functions internally, and ships off the tox calls to tox
 */
LRESULT CALLBACK WindowProc(HWND hwn, UINT msg, WPARAM wParam, LPARAM lParam) {
    static int mx, my;

    if (hwnd && hwn != hwnd) {
        if (msg == WM_DESTROY) {
            if (hwn == video_hwnd[0]) {
                if (settings.video_preview) {
                    settings.video_preview = false;
                    postmessage_utoxav(UTOXAV_STOP_VIDEO, 0, 0, NULL);
                }

                return false;
            }

            int i;
            for (i = 0; i != countof(friend); i++) {
                if (video_hwnd[i + 1] == hwn) {
                    FRIEND *f = &friend[i];
                    postmessage_utoxav(UTOXAV_STOP_VIDEO, f->number, 0, NULL);
                    break;
                }
            }
            if (i == countof(friend)) {
                debug("this should not happen\n");
            }
        }

        return DefWindowProcW(hwn, msg, wParam, lParam);
    }

    switch (msg) {
        case WM_QUIT:
        case WM_CLOSE:
        case WM_DESTROY: {
            if (settings.close_to_tray) {
                debug("Closing to tray.\n");
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
            main_hdc = GetDC(hwn);
            hdc      = CreateCompatibleDC(main_hdc);
            hdcMem   = CreateCompatibleDC(hdc);

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

            int w, h;

            w = GET_X_LPARAM(lParam);
            h = GET_Y_LPARAM(lParam);

            if (w != 0) {
                RECT r;
                GetClientRect(hwn, &r);
                w = r.right;
                h = r.bottom;

                settings.window_width  = w;
                settings.window_height = h;

                ui_set_scale(dropdown_dpi.selected + 6);
                ui_size(w, h);

                if (hdc_bm) {
                    DeleteObject(hdc_bm);
                }

                hdc_bm = CreateCompatibleBitmap(main_hdc, settings.window_width, settings.window_height);
                SelectObject(hdc, hdc_bm);
                redraw();
            }
            break;
        }

        case WM_SETFOCUS: {
            if (flashing) {
                FlashWindow(hwnd, 0);
                flashing = false;

                NOTIFYICONDATAW nid = {
                    .uFlags = NIF_ICON, .hWnd = hwnd, .hIcon = my_icon, .cbSize = sizeof(nid),
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

            BeginPaint(hwn, &ps);

            RECT r = ps.rcPaint;
            BitBlt(main_hdc, r.left, r.top, r.right - r.left, r.bottom - r.top, hdc, r.left, r.top, SRCCOPY);

            EndPaint(hwn, &ps);
            return false;
        }

        case WM_SYSKEYDOWN: // called instead of WM_KEYDOWN when ALT is down or F10 is pressed
        case WM_KEYDOWN: {
            bool control = ((GetKeyState(VK_CONTROL) & 0x80) != 0);
            bool shift   = ((GetKeyState(VK_SHIFT) & 0x80) != 0);
            bool alt     = ((GetKeyState(VK_MENU) & 0x80) != 0); /* Be careful not to clobber alt+num symbols */

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
                        case 'V': paste(); return false;
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
                return false;
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

            cursor = 0;
            panel_mmove(&panel_root, 0, 0, settings.window_width, settings.window_height, x, y, dx, dy);

            SetCursor(cursors[cursor]);

            if (!mouse_tracked) {
                TrackMouseEvent(&tme);
                mouse_tracked = true;
            }

            return false;
        }

        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK: {
            int x, y;

            x = GET_X_LPARAM(lParam);
            y = GET_Y_LPARAM(lParam);

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

            SetCapture(hwn);
            mdown = true;
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

#define setstatus(x)                                         \
    if (self.status != x) {                                  \
        postmessage_toxcore(TOX_SELF_SET_STATE, x, 0, NULL); \
        self.status = x;                                     \
        redraw();                                            \
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
            SetForegroundWindow(hwn);
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

    return DefWindowProcW(hwn, msg, wParam, lParam);
}
