#include "main.h"

#include "notify.h"
#include "screen_grab.h"
#include "window.h"

#include "../commands.h"
#include "../file_transfers.h"
#include "../filesys.h"
#include "../flist.h"
#include "../friend.h"
#include "../logging_native.h"
#include "../macros.h"
#include "../main.h"
#include "../main_native.h"
#include "../self.h"
#include "../settings.h"
#include "../text.h"
#include "../theme.h"
#include "../tox.h"
#include "../ui.h"
#include "../utox.h"

#include "../av/utox_av.h"
#include "../ui/draw.h"
#include "../ui/dropdown.h"
#include "../ui/edit.h"
#include "../ui/svg.h"

#include <windowsx.h>

static bool flashing;
static bool hidden;

static TRACKMOUSEEVENT tme           = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, 0, 0 };

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
    char *filepath = calloc(1, UTOX_FILE_NAME_LENGTH);
    if (filepath == NULL) {
        debug_error("Windows:\t Could not allocate memory for path.\n");
        return;
    }

    wchar_t dir[UTOX_FILE_NAME_LENGTH];
    GetCurrentDirectoryW(COUNTOF(dir), dir);

    OPENFILENAME ofn = {
        .lStructSize = sizeof(OPENFILENAME),
        .hwndOwner   = main_window.window,
        .lpstrFile   = filepath,
        .nMaxFile    = UTOX_FILE_NAME_LENGTH,
        .Flags       = OFN_EXPLORER | OFN_FILEMUSTEXIST,
    };

    if (GetOpenFileName(&ofn)) {
        FRIEND *f = flist_get_selected()->data;
        UTOX_MSG_FT *msg = calloc(1, sizeof(UTOX_MSG_FT));
        if (!msg) {
            debug_error("Windows:\tUnable to calloc for file send msg\n");
            return;
        }
        msg->file = fopen(filepath, "rb");
        msg->name = (uint8_t *)filepath;

        postmessage_toxcore(TOX_FILE_SEND_NEW, f->number, 0, msg);
    } else {
        debug_error("GetOpenFileName() failed\n");
    }
    SetCurrentDirectoryW(dir);
}

void openfileavatar(void) {
    char *filepath = malloc(UTOX_FILE_NAME_LENGTH);
    filepath[0]    = 0;

    wchar_t dir[UTOX_FILE_NAME_LENGTH];
    GetCurrentDirectoryW(COUNTOF(dir), dir);

    OPENFILENAME ofn = {
        .lStructSize = sizeof(OPENFILENAME),
        .lpstrFilter = "Supported Images\0*.GIF;*.PNG;*.JPG;*.JPEG" /* TODO: add all the supported types */
                       "All Files\0*.*\0"
                       "GIF Files\0*.GIF\0"
                       "PNG Files\0*.PNG\0"
                       "JPG Files\0*.JPG;*.JPEG\0"
                       "\0",
        .hwndOwner = main_window.window,
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
                    debug("error: AVATAR_TOO_LARGE message is larger than allocated buffer(%"PRIu64" bytes)\n",
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

void file_save_inline(MSG_HEADER *msg) {
    char *path = calloc(1, UTOX_FILE_NAME_LENGTH);
    if (path == NULL) {
        debug_error("file_save_inline:\tCould not allocate memory for path.\n");
        return;
    }
    snprintf(path, UTOX_FILE_NAME_LENGTH, "%.*s", (int)msg->via.ft.name_length, (char *)msg->via.ft.name);

    OPENFILENAME ofn = {
        .lStructSize    = sizeof(OPENFILENAME),
        .hwndOwner      = main_window.window,
        .lpstrFile      = path,
        .nMaxFile       = UTOX_FILE_NAME_LENGTH,
        .lpstrDefExt    = "png",
        .nFileExtension = strlen(path) - 3,
        .Flags          = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT,
    };

    if (GetSaveFileName(&ofn)) {
        FILE *fp = fopen(path, "wb");
        if (fp) {
            fwrite(msg->via.ft.data, msg->via.ft.data_size, 1, fp);
            fclose(fp);

            snprintf((char *)msg->via.ft.path, UTOX_FILE_NAME_LENGTH, "%s", path);
            msg->via.ft.inline_png = false;
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
    PostMessage(main_window.window, WM_TOX + (msg), ((param1) << 16) | (param2), (LPARAM)data);
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

void setselection(char *UNUSED(data), uint16_t UNUSED(length)) {}


#include "../layout/friend.h"
#include "../layout/group.h"
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
    OpenClipboard(main_window.window);
    EmptyClipboard();
    SetClipboardData(CF_UNICODETEXT, hMem);
    CloseClipboard();
}

/* TODO DRY, this exists in screen_grab.c */
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

/* TODO DRY, this exists in screen_grab.c */
static void sendbitmap(HDC mem, HBITMAP hbm, int width, int height) {
    if (width == 0 || height == 0)
        return;

    BITMAPINFO info = {
        .bmiHeader = {
            .biSize        = sizeof(BITMAPINFOHEADER),
            .biWidth       = width,
            .biHeight      = -(int)height,
            .biPlanes      = 1,
            .biBitCount    = 24,
            .biCompression = BI_RGB,
        }
    };

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

    int size = 0;
    uint8_t *out = stbi_write_png_to_mem(bits, 0, width, height, 3, &size);

    free(bits);

    NATIVE_IMAGE *image = create_utox_image(hbm, 0, width, height);
    friend_sendimage(flist_get_selected()->data, image, width, height, (UTOX_IMAGE)out, size);
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

            copy = CreateCompatibleBitmap(main_window.mem_DC, bm.bmWidth, bm.bmHeight);
            SelectObject(main_window.mem_DC, copy);
            BitBlt(main_window.mem_DC, 0, 0, bm.bmWidth, bm.bmHeight, tempdc, 0, 0, SRCCOPY);

            sendbitmap(main_window.mem_DC, copy, bm.bmWidth, bm.bmHeight);

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
    HBITMAP  bmp = CreateDIBSection(main_window.mem_DC, &bmi, DIB_RGB_COLORS, (void **)&out, NULL, 0);

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

int ch_mod(uint8_t *UNUSED(file)) {
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
void notify(char *title, uint16_t title_length, const char *msg, uint16_t msg_length, void *UNUSED(object), bool UNUSED(is_group)) {
    if (havefocus || self.status == 2) {
        return;
    }

    FlashWindow(main_window.window, 1);
    flashing = true;

    NOTIFYICONDATAW nid = {
        .uFlags      = NIF_ICON | NIF_INFO,
        .hWnd        = main_window.window,
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

void showkeyboard(bool UNUSED(show)) {} /* Added for android support. */

void edit_will_deactivate(void) {}

#include "../layout/background.h" // TODO do we want to remove this?
/* Redraws the main UI window */
void redraw(void) {
    native_window_set_target(&main_window);

    SelectObject(main_window.draw_DC, main_window.draw_BM);

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
        .uFlags = NIF_TIP,
        .hWnd = main_window.window,
        .cbSize = sizeof(nid),
    };

    utf8_to_nativestr((char *)tip, nid.szTip, tip_length);

    Shell_NotifyIconW(NIM_MODIFY, &nid);

    free(tip);
}

void force_redraw(void) {
    redraw();
}

void freefonts() {
    for (size_t i = 0; i != COUNTOF(font); i++) {
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
    SelectObject(main_window.draw_DC, font[FONT_TEXT]);
    GetTextMetrics(main_window.draw_DC, &tm);
    font_small_lineheight = tm.tmHeight + tm.tmExternalLeading;
    // SelectObject(main_window.draw_DC, font[FONT_MSG]);
    // GetTextMetrics(main_window.draw_DC, &tm);
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

static void tray_icon_init(HWND parent, HICON icon) {
    NOTIFYICONDATA nid = {
        .uFlags           = NIF_MESSAGE | NIF_ICON | NIF_TIP,
        .uCallbackMessage = WM_NOTIFYICON,
        .hIcon            = icon,
        .szTip            = "uTox default tooltip",
        .hWnd             = parent,
        .cbSize           = sizeof(nid),
    };

    Shell_NotifyIcon(NIM_ADD, &nid);
}

static void tray_icon_decon(HWND parent) {
    NOTIFYICONDATA nid = {
        .hWnd   = parent,
        .cbSize = sizeof(nid),
    };

    Shell_NotifyIcon(NIM_DELETE, &nid);
}

static void cursors_init(void) {
    cursors[CURSOR_NONE]     = LoadCursor(NULL, IDC_ARROW);
    cursors[CURSOR_HAND]     = LoadCursor(NULL, IDC_HAND);
    cursors[CURSOR_TEXT]     = LoadCursor(NULL, IDC_IBEAM);
    cursors[CURSOR_SELECT]   = LoadCursor(NULL, IDC_CROSS);
    cursors[CURSOR_ZOOM_IN]  = LoadCursor(NULL, IDC_SIZEALL);
    cursors[CURSOR_ZOOM_OUT] = LoadCursor(NULL, IDC_SIZEALL);
}

#define UTOX_EXE "\\uTox.exe"
#define UTOX_UPDATER_EXE "\\utox_runner.exe"
#define UTOX_VERSION_FILE "\\version"

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

#include "../layout/settings.h" // TODO remove, in for dropdown.lang

/** client main()
 *
 * Main thread
 * generates settings, loads settings from save file, generates main UI, starts
 * tox, generates tray icon, handles client messages. Cleans up, and exits.
 *
 * also handles call from other apps.
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE UNUSED(hPrevInstance), PSTR cmd, int nCmdShow) {

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

    cursors_init();

    native_window_init(hInstance); // Needed to generate the Windows window class we use.

    screen_grab_init(hInstance);

    OleInitialize(NULL);


    uint16_t langid = GetUserDefaultUILanguage() & 0xFFFF;

    LANG = ui_guess_lang_by_windows_lang_id(langid, DEFAULT_LANG);

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

    native_window_create_main(save->window_x, save->window_y, save->window_width, save->window_height);

    native_notify_init(hInstance);

    hdc_brush = GetStockObject(DC_BRUSH);
    tme.hwndTrack = main_window.window;

    tray_icon_init(main_window.window, LoadIcon(hInstance, MAKEINTRESOURCE(101)));

    SetBkMode(main_window.draw_DC, TRANSPARENT);

    dnd_init(main_window.window);

    // start tox thread (main_window.window needs to be set first)
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
        ShowWindow(main_window.window, SW_HIDE);
        hidden = true;
    } else {
        ShowWindow(main_window.window, nCmdShow);
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    /* kill threads */
    postmessage_utoxav(UTOXAV_KILL, 0, 0, NULL);
    postmessage_toxcore(TOX_KILL, 0, 0, NULL);

    /* cleanup */

    tray_icon_decon(main_window.window);

    // wait for tox_thread to exit
    while (tox_thread_init) {
        yieldcpu(10);
    }

    RECT wndrect = { 0 };
    GetWindowRect(main_window.window, &wndrect);
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
