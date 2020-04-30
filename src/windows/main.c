#include "main.h"
#include "notify.h"
#include "screen_grab.h"
#include "utf8.h"
#include "window.h"

#include "../avatar.h"
#include "../chatlog.h"
#include "../commands.h"
#include "../debug.h"
#include "../file_transfers.h"
#include "../filesys.h"
#include "../flist.h"
#include "../friend.h"
#include "../macros.h"
#include "../main.h" // Lots of things. :(
#include "../self.h"
#include "../settings.h"
#include "../text.h"
#include "../theme.h"
#include "../tox.h"
#include "../ui.h"
#include "../utox.h"

#include "../av/utox_av.h"

#include "../native/filesys.h"
#include "../native/notify.h"
#include "../native/os.h"

#include "../ui/draw.h"
#include "../ui/edit.h"
#include "../ui/svg.h"

#include "../layout/background.h" // TODO do we want to remove this?
#include "../layout/friend.h"
#include "../layout/group.h"

#include "stb.h"

#include <io.h>
#include <libgen.h>

HFONT font[32];
HCURSOR cursors[8];
HICON black_icon, unread_messages_icon;
HBRUSH hdc_brush;
HWND video_hwnd[128]; // todo fixme
HWND preview_hwnd;    // todo fixme
char portable_mode_save_path[MAX_PATH];

/**
 * A null-terminated string that specifies the text for a standard tooltip.
 * For Windows 2000 and later, szTip can have a maximum of 128 characters,
 * including the terminating null character.
 * https://msdn.microsoft.com/en-us/library/windows/desktop/bb773352(v=vs.85).aspx
 */
static const uint8_t MAX_TIP_LENGTH = 128 - 1;

bool flashing = false;
bool hidden = false;

void native_export_chatlog_init(uint32_t friend_number) {
    FRIEND *f = get_friend(friend_number);
    if (!f) {
        LOG_ERR("Windows", "Could not get friend with number: %u", friend_number);
        return;
    }

    char *path = calloc(1, UTOX_FILE_NAME_LENGTH);
    if (!path) {
        LOG_ERR("Windows", "Could not allocate memory.");
        return;
    }

    snprintf(path, UTOX_FILE_NAME_LENGTH, "%.*s.txt", (int)f->name_length, f->name);

    wchar_t filepath[UTOX_FILE_NAME_LENGTH] = { 0 };
    utf8_to_nativestr(path, filepath, UTOX_FILE_NAME_LENGTH * 2);

    OPENFILENAMEW ofn = {
        .lStructSize = sizeof(OPENFILENAMEW),
        .lpstrFilter = L".txt",
        .lpstrFile   = filepath,
        .nMaxFile    = UTOX_FILE_NAME_LENGTH,
        .Flags       = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT,
        .lpstrDefExt = L"txt",
    };

    if (GetSaveFileNameW(&ofn)) {
        path = calloc(1, UTOX_FILE_NAME_LENGTH);
        if (!path) {
            LOG_ERR("Windows", "Could not allocate memory.");
            return;
        }

        native_to_utf8str(filepath, path, UTOX_FILE_NAME_LENGTH);

        FILE *file = utox_get_file_simple(path, UTOX_FILE_OPTS_WRITE);
        if (file) {
            utox_export_chatlog(f->id_str, file);
        } else {
            LOG_ERR("Windows", "Opening file %s failed.", path);
        }
    } else {
        LOG_ERR("Windows", "Unable to open file and export chatlog.");
    }
    free(path);
}

void native_select_dir_ft(uint32_t fid, uint32_t num, FILE_TRANSFER *file) {
    if (!sanitize_filename(file->name)) {
        LOG_ERR("Windows", "Filename is invalid and could not be sanitized.");
        return;
    }

    wchar_t filepath[UTOX_FILE_NAME_LENGTH] = { 0 };
    utf8_to_nativestr((char *)file->name, filepath, file->name_length * 2);

    OPENFILENAMEW ofn = {
        .lStructSize = sizeof(OPENFILENAMEW),
        .lpstrFile   = filepath,
        .nMaxFile    = UTOX_FILE_NAME_LENGTH,
        .Flags       = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT,
    };

    if (GetSaveFileNameW(&ofn)) {
        char *path = calloc(1, UTOX_FILE_NAME_LENGTH);
        if (!path) {
            LOG_ERR("Windows", "Could not allocate memory for path.");
            return;
        }

        native_to_utf8str(filepath, path, UTOX_FILE_NAME_LENGTH * 2);
        postmessage_toxcore(TOX_FILE_ACCEPT, fid, num, path);
    } else {
        LOG_ERR("Windows", "Unable to Get save file for incoming FT.");
    }
}

void native_autoselect_dir_ft(uint32_t fid, FILE_TRANSFER *file) {
    wchar_t *autoaccept_folder = NULL;

    if (settings.portable_mode) {
        autoaccept_folder = calloc(1, UTOX_FILE_NAME_LENGTH * sizeof(wchar_t));
        utf8_to_nativestr(portable_mode_save_path, autoaccept_folder, strlen(portable_mode_save_path) * 2);
    } else if (SHGetKnownFolderPath((REFKNOWNFOLDERID)&FOLDERID_Downloads,
                                    KF_FLAG_CREATE, NULL, &autoaccept_folder) != S_OK) {
        LOG_ERR("Windows", "Unable to get auto accept file folder.");
        return;
    }

    wchar_t subpath[UTOX_FILE_NAME_LENGTH] = { 0 };
    swprintf(subpath, UTOX_FILE_NAME_LENGTH, L"%ls%ls", autoaccept_folder, L"\\Tox_Auto_Accept");

    if (settings.portable_mode) {
        free(autoaccept_folder);
    } else {
        CoTaskMemFree(autoaccept_folder);
    }

    CreateDirectoryW(subpath, NULL);

    if (!sanitize_filename(file->name)) {
        LOG_ERR("Windows", "Filename is invalid and could not be sanitized.");
        return;
    }

    wchar_t filename[UTOX_FILE_NAME_LENGTH] = { 0 };
    utf8_to_nativestr((char *)file->name, filename, file->name_length * 2);

    wchar_t fullpath[UTOX_FILE_NAME_LENGTH] = { 0 };
    swprintf(fullpath, UTOX_FILE_NAME_LENGTH, L"%ls\\%ls", subpath, filename);

    char *path = calloc(1, UTOX_FILE_NAME_LENGTH);
    if (!path) {
        LOG_ERR("Windows", "Could not allocate memory for path.");
        return;
    }

    native_to_utf8str(fullpath, path, UTOX_FILE_NAME_LENGTH);
    postmessage_toxcore(TOX_FILE_ACCEPT_AUTO, fid, file->file_number, path);
}

void launch_at_startup(bool should) {
    const wchar_t *run_key_path = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

    if (should) {
        HKEY hKey;
        if (RegOpenKeyW(HKEY_CURRENT_USER, run_key_path, &hKey) == ERROR_SUCCESS) {
            wchar_t path[UTOX_FILE_NAME_LENGTH * 2];
            uint16_t path_length = GetModuleFileNameW(NULL, path + 1, UTOX_FILE_NAME_LENGTH * 2);
            path[0] = '\"';
            path[path_length + 1] = '\"';
            path[path_length + 2] = '\0';
            path_length += 2;

            // 2 bytes per wchar_t
            uint16_t ret = RegSetKeyValueW(hKey, NULL, L"uTox", REG_SZ, path, path_length * 2);
            if (ret == ERROR_SUCCESS) {
                LOG_INFO("Windows", "Set uTox to run at startup.");
            } else {
                LOG_ERR("Windows", "Unable to set Registry key for startup.");
            }

            RegCloseKey(hKey);
        }
    } else {
        HKEY hKey;
        if (ERROR_SUCCESS == RegOpenKeyW(HKEY_CURRENT_USER, run_key_path, &hKey)) {
            uint16_t ret = RegDeleteKeyValueW(hKey, NULL, L"uTox");
            if (ret == ERROR_SUCCESS) {
                LOG_INFO("Windows", "Set uTox to not run at startup.");
            } else {
                LOG_ERR("Windows", "Unable to delete Registry key for startup.");
            }
            RegCloseKey(hKey);
        }
    }
}

/** Open system file browser dialog */
void openfilesend(void) {
    wchar_t dir[UTOX_FILE_NAME_LENGTH];
    GetCurrentDirectoryW(COUNTOF(dir), dir);

    wchar_t filepath[UTOX_FILE_NAME_LENGTH] = { 0 };

    OPENFILENAMEW ofn = {
        .lStructSize = sizeof(OPENFILENAMEW),
        .hwndOwner   = main_window.window,
        .lpstrFile   = filepath,
        .nMaxFile    = UTOX_FILE_NAME_LENGTH,
        .Flags       = OFN_EXPLORER | OFN_FILEMUSTEXIST,
    };

    if (GetOpenFileNameW(&ofn)) {
        FRIEND *f = flist_get_sel_friend();
        if (!f) {
            LOG_ERR("Windows", "Unable to get friend for file send msg.");
            return;
        }

        UTOX_MSG_FT *msg = calloc(1, sizeof(UTOX_MSG_FT));
        if (!msg) {
            LOG_ERR("Windows", "Unable to calloc for file send msg.");
            return;
        }

        char *path = calloc(1, UTOX_FILE_NAME_LENGTH);
        if (!path) {
            LOG_ERR("Windows", "Could not allocate memory for path.");
            return;
        }

        native_to_utf8str(filepath, path, UTOX_FILE_NAME_LENGTH);
        msg->file = utox_get_file_simple(path, UTOX_FILE_OPTS_READ);
        msg->name = (uint8_t *)path;

        postmessage_toxcore(TOX_FILE_SEND_NEW, f->number, 0, msg);
    } else {
        LOG_ERR("Windows", "GetOpenFileName() failed.");
    }
    SetCurrentDirectoryW(dir);
}

void show_messagebox(const char *caption, uint16_t caption_length, const char *message, uint16_t message_length) {
    wchar_t message_native[message_length];
    memset(message_native, 0, message_length);
    utf8_to_nativestr(message, message_native, message_length * 2);

    wchar_t caption_native[caption_length];
    memset(caption_native, 0, caption_length);
    utf8_to_nativestr(caption, caption_native, caption_length * 2);

    MessageBoxW(NULL, message ? message_native : NULL, caption ? caption_native : NULL, MB_ICONWARNING);
}

void openfileavatar(void) {
    char *filepath = calloc(1, UTOX_FILE_NAME_LENGTH);
    if (!filepath) {
        LOG_ERR("Windows", "Could not allocate memory for path.");
        return;
    }

    wchar_t dir[UTOX_FILE_NAME_LENGTH];
    GetCurrentDirectoryW(COUNTOF(dir), dir);

    OPENFILENAME ofn = {
        .lStructSize = sizeof(OPENFILENAME),
        .lpstrFilter = "Supported Images\0*.GIF;*.PNG;*.JPG;*.JPEG" // TODO: add all the supported types.
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
        if (!GetOpenFileName(&ofn)) {
            LOG_TRACE("Windows", "GetOpenFileName() failed when trying to grab an avatar.");
            break;
        }

        int width, height, bpp, size;
        uint8_t *file_data = stbi_load(filepath, &width, &height, &bpp, 0);
        uint8_t *img = stbi_write_png_to_mem(file_data, 0, width, height, bpp, &size);
        free(file_data);

        if (!img) {
            MessageBox(NULL, (const char *)S(CANT_FIND_FILE_OR_EMPTY), NULL, MB_ICONWARNING);
            continue;
        }

        if (size > UTOX_AVATAR_MAX_DATA_LENGTH) {
            free(img);
            char message[1024];
            if (sizeof(message) < (unsigned)SLEN(AVATAR_TOO_LARGE_MAX_SIZE_IS) + 16) {
                LOG_ERR("Windows", "AVATAR_TOO_LARGE message is larger than allocated buffer(%llu bytes)\n",
                        sizeof(message));
                break;
            }

            // create message containing text that selected avatar is too large and what the max size is
            int len = sprintf(message, "%.*s", SLEN(AVATAR_TOO_LARGE_MAX_SIZE_IS),
                              S(AVATAR_TOO_LARGE_MAX_SIZE_IS));
            len += sprint_humanread_bytes(message + len, sizeof(message) - len, UTOX_AVATAR_MAX_DATA_LENGTH);
            message[len++] = '\0';

            MessageBox(NULL, message, NULL, MB_ICONWARNING);
            continue;
        }

        postmessage_utox(SELF_AVATAR_SET, size, 0, img);
        break;
    }

    free(filepath);
    SetCurrentDirectoryW(dir);
}

void file_save_inline_image_png(MSG_HEADER *msg) {
    wchar_t filepath[UTOX_FILE_NAME_LENGTH] = { 0 };
    utf8_to_nativestr((char *)msg->via.ft.name, filepath, msg->via.ft.name_length * 2);

    OPENFILENAMEW ofn = {
        .lStructSize    = sizeof(OPENFILENAMEW),
        .hwndOwner      = main_window.window,
        .lpstrFile      = filepath,
        .nMaxFile       = UTOX_FILE_NAME_LENGTH,
        .lpstrDefExt    = L"png",
        .lpstrFilter    = L"PNG Files\0*.png\0",
        .Flags          = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST,
    };

    if (GetSaveFileNameW(&ofn)) {
        char *path = calloc(1, UTOX_FILE_NAME_LENGTH);
        if (!path) {
            LOG_ERR("Windows", "Could not allocate memory for path.");
            return;
        }

        native_to_utf8str(filepath, path, UTOX_FILE_NAME_LENGTH);

        FILE *file = utox_get_file_simple(path, UTOX_FILE_OPTS_WRITE);
        if (file) {
            fwrite(msg->via.ft.data, msg->via.ft.data_size, 1, file);
            fclose(file);

            msg->via.ft.path = calloc(1, UTOX_FILE_NAME_LENGTH);
            if (!msg->via.ft.path) {
                LOG_ERR("Windows", "Could not allocate memory for path.");
                free(path);
                return;
            }

            msg->via.ft.path = (uint8_t *)strdup(path);
            msg->via.ft.name = basename(strdup(path));
            msg->via.ft.name_length = strlen((char *)msg->via.ft.name);

            msg->via.ft.inline_png = false;
        } else {
            LOG_ERR("Windows", "file_save_inline_image_png:\tCouldn't open path: `%s` to save inline file.", path);
        }

        free(path);
    } else {
        LOG_ERR("Windows", "GetSaveFileName() failed");
    }
}

bool native_save_image_png(const char *name, const uint8_t *image, const int image_size) {
    wchar_t filepath[UTOX_FILE_NAME_LENGTH] = { 0 };
    size_t length = strlen(name);
    utf8_to_nativestr(name, filepath, length * 2);

    OPENFILENAMEW ofn = {
        .lStructSize    = sizeof(OPENFILENAMEW),
        .hwndOwner      = main_window.window,
        .lpstrFile      = filepath,
        .nMaxFile       = UTOX_FILE_NAME_LENGTH,
        .lpstrDefExt    = L"png",
        .lpstrFilter    = L"PNG Files\0*.png\0",
        .Flags          = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST,
    };

    if (GetSaveFileNameW(&ofn)) {
        char *path = calloc(1, UTOX_FILE_NAME_LENGTH);
        if (!path) {
            LOG_ERR("Windows", "Could not allocate memory for path.");
            return false;
        }

        native_to_utf8str(filepath, path, UTOX_FILE_NAME_LENGTH);

        FILE *file = utox_get_file_simple(path, UTOX_FILE_OPTS_WRITE);
        if (!file) {
            LOG_ERR("Windows", "Could not open file %s for write.", path);
            free(path);
            return false;
        }

        fwrite(image, image_size, 1, file);
        fclose(file);
        free(path);
        return true;
    }

    return false;
}

void postmessage_utox(UTOX_MSG msg, uint16_t param1, uint16_t param2, void *data) {
    PostMessage(main_window.window, WM_TOX + (msg), ((param1) << 16) | (param2), (LPARAM)data);
}

void init_ptt(void) {
    settings.push_to_talk = true;
}

bool check_ptt_key(void) {
    if (!settings.push_to_talk) {
        // PTT is disabled. Always send audio.
        return true;
    }

    if (GetAsyncKeyState(VK_LCONTROL)) {
        return true;
    }

    return false;
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

void setselection(char *UNUSED(data), uint16_t UNUSED(length)) {
    // TODO: Implement.
}

void copy(int value) {
    const uint32_t max_size = UINT16_MAX + 1;
    char data[max_size]; //! TODO: De-hardcode this value.
    memset(data, 0, sizeof(data));
    int len = 0;

    if (edit_active()) {
        len = edit_copy(data, max_size - 1);
        data[len] = 0;
    } else if (flist_get_sel_friend()) {
        len = messages_selection(&messages_friend, data, max_size, value);
    } else if (flist_get_sel_group()) {
        len = messages_selection(&messages_group, data, max_size, value);
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
    NATIVE_IMAGE *image = calloc(1, sizeof(NATIVE_IMAGE));
    if (!image) {
        LOG_ERR("Windows", "Could not allocate memory for image.");
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
    if (width == 0 || height == 0) {
        return;
    }

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

    void *bits = calloc(1, (width + 3) * height * 3);

    GetDIBits(mem, hbm, 0, height, bits, &info, DIB_RGB_COLORS);

    uint8_t pbytes = width & 3;
    uint8_t *p = bits;
    uint8_t *pp = bits;
    uint8_t *end = p + width * height * 3;

    while (p != end) {
        for (int i = 0; i != width; i++) {
            uint8_t b    = pp[i * 3];
            p[i * 3]     = pp[i * 3 + 2];
            p[i * 3 + 1] = pp[i * 3 + 1];
            p[i * 3 + 2] = b;
        }
        p += width * 3;
        pp += width * 3 + pbytes;
    }

    int size = 0;

    UTOX_IMAGE out = stbi_write_png_to_mem(bits, 0, width, height, 3, &size);
    free(bits);

    NATIVE_IMAGE *image = create_utox_image(hbm, 0, width, height);
    friend_sendimage(flist_get_sel_friend(), image, width, height, out, size);
}

void paste(void) {
    OpenClipboard(NULL);
    HANDLE h = GetClipboardData(CF_UNICODETEXT);
    if (!h) {
        h = GetClipboardData(CF_BITMAP);
        if (h && flist_get_sel_friend()) {
            FRIEND *f = flist_get_sel_friend();
            if (!f->online) {
                return;
            }

            BITMAP  bm;
            GetObject(h, sizeof(bm), &bm);

            HDC tempdc = CreateCompatibleDC(NULL);
            SelectObject(tempdc, h);

            HBITMAP copy = CreateCompatibleBitmap(main_window.mem_DC, bm.bmWidth, bm.bmHeight);
            SelectObject(main_window.mem_DC, copy);
            BitBlt(main_window.mem_DC, 0, 0, bm.bmWidth, bm.bmHeight, tempdc, 0, 0, SRCCOPY);

            sendbitmap(main_window.mem_DC, copy, bm.bmWidth, bm.bmHeight);

            DeleteDC(tempdc);
        }
    } else {
        wchar_t *d = GlobalLock(h);
        char data[65536]; // TODO: De-hardcode this value.
        int len = WideCharToMultiByte(CP_UTF8, 0, d, -1, data, sizeof(data), NULL, NULL);
        if (edit_active()) {
            edit_paste(data, len, false);
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

    BITMAPINFO bmi = {
        .bmiHeader = {
            .biSize        = sizeof(BITMAPINFOHEADER),
            .biWidth       = width,
            .biHeight      = -height,
            .biPlanes      = 1,
            .biBitCount    = 32,
            .biCompression = BI_RGB,
        }
    };

    // create device independent bitmap, we can write the bytes to out
    // to put them in the bitmap
    uint8_t *out;
    HBITMAP bmp = CreateDIBSection(main_window.mem_DC, &bmi, DIB_RGB_COLORS, (void **)&out, NULL, 0);

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

void showkeyboard(bool UNUSED(show)) {} /* Added for android support. */

void edit_will_deactivate(void) {}

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
    uint16_t length = self.name_length + sizeof(" : ") + self.statusmsg_length;

    char tip[length];
    memset(tip, 0, length);

    length = snprintf(tip, length, "%.*s : %.*s",
                      self.name_length, self.name,
                      self.statusmsg_length, self.statusmsg);

    NOTIFYICONDATAW nid = {
        .uFlags = NIF_TIP,
        .hWnd = main_window.window,
        .cbSize = sizeof(nid),
    };

    uint16_t msg_len = safe_shrink(tip, length, MAX_TIP_LENGTH);
    utf8_to_nativestr(tip, nid.szTip, msg_len);

    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

void force_redraw(void) {
    redraw();
}

void openurl(char *str) {
    if (try_open_tox_uri(str)) {
        redraw();
        return;
    }

    wchar_t url[UTOX_FILE_NAME_LENGTH] = { 0 };
    utf8tonative(str, url, UTOX_FILE_NAME_LENGTH);
    ShellExecuteW(NULL, L"open", url, NULL, NULL, SW_SHOW);
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
    /*lf.lfWeight = FW_NORMAL; //FW_LIGHT <- light fonts don't antialias
    font[FONT_MSG_NAME] = CreateFontIndirect(&lf);
    lf.lfHeight = F(11);
    font[FONT_MSG] = CreateFontIndirect(&lf);
    lf.lfUnderline = 1;
    font[FONT_MSG_LINK] = CreateFontIndirect(&lf);*/

    SelectObject(main_window.draw_DC, font[FONT_TEXT]);
    TEXTMETRIC tm;
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

/*
 * CommandLineToArgvA implementation since CommandLineToArgvA doesn't exist in win32 api
 * Limitation: nested quotation marks are not handled
 * Credit: http://alter.org.ua/docs/win/args
 */
static PCHAR *CommandLineToArgvA(PCHAR CmdLine, int *_argc) {
    ULONG len = strlen(CmdLine);
    ULONG i = ((len + 2) / 2) * sizeof(PVOID) + sizeof(PVOID);
    PCHAR *argv = (PCHAR *)GlobalAlloc(GMEM_FIXED, i + (len + 2) * sizeof(CHAR));
    PCHAR _argv = (PCHAR)(((PUCHAR)argv) + i);

    ULONG argc = 0;
    argv[argc] = _argv;
    i = 0;

    BOOLEAN in_QM    = FALSE;
    BOOLEAN in_TEXT  = FALSE;
    BOOLEAN in_SPACE = TRUE;

    CHAR a;
    ULONG j = 0;
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

void tray_icon_init(HWND parent, HICON icon) {
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

static bool win_init_mutex(HANDLE *mutex, HINSTANCE hInstance, PSTR cmd, const char *instance_id) {
    *mutex = CreateMutex(NULL, false, instance_id);

    if (!mutex) {
        LOG_FATAL_ERR(-4, "Win Mutex", "Unable to create windows mutex.");
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND window = FindWindow(TITLE, NULL);
        if (window) {
            COPYDATASTRUCT data = {
                .cbData = strlen(cmd),
                .lpData = cmd
            };
            SendMessage(window, WM_COPYDATA, (WPARAM)hInstance, (LPARAM)&data);
            LOG_FATAL_ERR(-3, "Win Mutex", "Message sent.");
        }
        LOG_FATAL_ERR(-3, "Win Mutex", "Error getting mutex or window.");
    }

    return true;
}

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

    int argc;
    PCHAR *argv = CommandLineToArgvA(GetCommandLineA(), &argc);
    if (!argv) {
        printf("Init error -- CommandLineToArgvA failed.");
        return -5;
    }

    int8_t should_launch_at_startup, set_show_window;
    parse_args(argc, argv, &should_launch_at_startup, &set_show_window, NULL);
    GlobalFree(argv);

    char instance_id[MAX_PATH];
    if (settings.portable_mode == true) {
        /* force the working directory if opened with portable command */
        const HMODULE hModule = GetModuleHandle(NULL);
        GetModuleFileName(hModule, instance_id, MAX_PATH);

        char *utox_path = strdup(instance_id);
        char *utox_folder = dirname(utox_path);

        SetCurrentDirectory(utox_folder);
        strcpy(portable_mode_save_path, utox_folder);

        free(utox_path);
        sanitize_filename((uint8_t *)instance_id);
    } else {
        strcpy(instance_id, TITLE);
    }

    // We call utox_init after parse_args()
    utox_init();

    LOG_WARN("WinMain", "Normal windows build");

    #ifdef GIT_VERSION
        LOG_NOTE("WinMain", "uTox version %s \n", GIT_VERSION);
    #endif

    /* if opened with argument, check if uTox is already open and pass the argument to the existing process */
    HANDLE utox_mutex;
    win_init_mutex(&utox_mutex, hInstance, cmd, instance_id);

    if (should_launch_at_startup == 1) {
        launch_at_startup(1);
    } else if (should_launch_at_startup == -1) {
        launch_at_startup(0);
    }

    cursors_init();

    native_window_init(hInstance); // Needed to generate the Windows window class we use.

    screen_grab_init(hInstance);

    OleInitialize(NULL);

    theme_load(settings.theme);

    settings.window_width  = MAX((uint32_t)SCALE(MAIN_WIDTH), settings.window_width);
    settings.window_height = MAX((uint32_t)SCALE(MAIN_HEIGHT), settings.window_height);

    char pretitle[128];
    snprintf(pretitle, 128, "%s %s (version : %s)", TITLE, SUB_TITLE, VERSION);
    size_t  title_size = strlen(pretitle) + 1;
    wchar_t title[title_size];
    mbstowcs(title, pretitle, title_size);

    native_window_create_main(settings.window_x, settings.window_y, settings.window_width, settings.window_height);

    native_notify_init(hInstance);

    hdc_brush = GetStockObject(DC_BRUSH);

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
        const int len = strlen(cmd);
        do_tox_url((uint8_t *)cmd, len);
    }

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

    RECT wndrect = { 0 };
    GetWindowRect(main_window.window, &wndrect);

    settings.window_x      = wndrect.left < 0 ? 0 : wndrect.left;
    settings.window_y      = wndrect.top < 0 ? 0 : wndrect.top;
    settings.window_width  = (wndrect.right - wndrect.left);
    settings.window_height = (wndrect.bottom - wndrect.top);

    config_save();

    /* kill threads */
    postmessage_utoxav(UTOXAV_KILL, 0, 0, NULL);
    postmessage_toxcore(TOX_KILL, 0, 0, NULL);

    /* cleanup */

    tray_icon_decon(main_window.window);

    // wait for tox_thread to exit
    while (tox_thread_init) {
        yieldcpu(10);
    }

    // TODO: This should be a non-zero value determined by a message's wParam.
    return 0;
}
