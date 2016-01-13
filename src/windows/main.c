#include "../main.h"
#include <windows.h>
#include <windowsx.h>

static _Bool flashing, desktopgrab_video;
static _Bool hidden;


static TRACKMOUSEEVENT tme = {sizeof(TRACKMOUSEEVENT), TME_LEAVE, 0, 0};
static _Bool mouse_tracked = 0;
_Bool draw = 0;
float scale = 1.0;
_Bool connected = 0;
_Bool havefocus;


BLENDFUNCTION blend_function = {
    .BlendOp = AC_SRC_OVER,
    .BlendFlags = 0,
    .SourceConstantAlpha = 0xFF,
    .AlphaFormat = AC_SRC_ALPHA
};

/** Translate a char* from UTF-8 encoding to OS native;
 *
 * Accepts char_t pointer, native array pointer, length of input;
 * Returns: number of chars writen, or 0 on failure.
 *
 */
static int utf8tonative(char_t *str, wchar_t *out, int length){
    return MultiByteToWideChar(CP_UTF8, 0, (char*)str, length, out, length);
}

static int utf8str_to_native(char_t *str, wchar_t *out, int length){
    /* must be null terminated string                   ↓ */
    return MultiByteToWideChar(CP_UTF8, 0, (char*)str, -1, out, length);
}

void postmessage(uint32_t msg, uint16_t param1, uint16_t param2, void *data)
{
    PostMessage(hwnd, WM_TOX + (msg), ((param1) << 16) | (param2), (LPARAM)data);
}

void init_ptt(void){ push_to_talk = 1; }

_Bool check_ptt_key(void){
    if (!push_to_talk) {
        // debug("PTT is disabled\n");
        return 1; /* If push to talk is disabled, return true. */
    }

    if ( GetAsyncKeyState(VK_LCONTROL) ) {
        // debug("PTT key is down\n");
        return 1;
    } else {
        // debug("PTT key is up\n");
        return 0;
    }
}

void exit_ptt(void){ push_to_talk = 0; }

void drawalpha(int bm, int x, int y, int width, int height, uint32_t color)
{
    if(!bitmap[bm]) {
        return;
    }

    BITMAPINFO bmi = {
        .bmiHeader = {
            .biSize = sizeof(BITMAPINFOHEADER),
            .biWidth = width,
            .biHeight = -height,
            .biPlanes = 1,
            .biBitCount = 32,
            .biCompression = BI_RGB,
        }
    };

    // create pointer to beginning and end of the alpha-channel-only bitmap
    uint8_t *alpha_pixel = bitmap[bm], *end = alpha_pixel + width * height;


    // create temporary bitmap we'll combine the alpha and colors on
    uint32_t *out_pixel;
    HBITMAP temp = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, (void**)&out_pixel, NULL, 0);
    SelectObject(hdcMem, temp);

    // create pixels for the drawable bitmap based on the alpha value of
    // each pixel in the alpha bitmap and the color given by 'color',
    // the Win32 API requires we pre-apply our alpha channel as well by
    // doing (color * alpha / 255) for each color channel
    // NOTE: Input color is in the format 0BGR, output pixel is in the format BGRA
    while(alpha_pixel != end) {
        uint8_t alpha = *alpha_pixel++;
        *out_pixel++ = (((color & 0xFF) * alpha / 255) << 16) // red
                     | ((((color >> 8) & 0xFF) * alpha / 255) << 8)  // green
                     | ((((color >> 16) & 0xFF) * alpha / 255) << 0) // blue
                     | (alpha << 24); // alpha
    }

    // draw temporary bitmap on screen
    AlphaBlend(hdc, x, y, width, height, hdcMem, 0, 0, width, height, blend_function);

    // clean up
    DeleteObject(temp);
}

void image_set_filter(UTOX_NATIVE_IMAGE *image, uint8_t filter)
{
    switch (filter) {
    case FILTER_NEAREST:
        image->stretch_mode = COLORONCOLOR;
        break;
    case FILTER_BILINEAR:
        image->stretch_mode = HALFTONE;
        break;
    default:
        debug("Warning: Tried to set image to unrecognized filter(%u).\n", filter);
        return;
    }
}

void image_set_scale(UTOX_NATIVE_IMAGE *image, double img_scale)
{
    image->scaled_width = (uint32_t)(((double)image->width * img_scale) + 0.5);
    image->scaled_height = (uint32_t)(((double)image->height * img_scale) + 0.5);
}

static _Bool image_is_stretched(const UTOX_NATIVE_IMAGE *image)
{
    return image->width != image->scaled_width ||
           image->height != image->scaled_height;
}

// NOTE: This function is way more complicated than the XRender variant, because
// the Win32 API is a lot more limited, so all scaling, clipping, and handling
// transparency has to be done explicitly
void draw_image(const UTOX_NATIVE_IMAGE *image, int x, int y, uint32_t width, uint32_t height, uint32_t imgx, uint32_t imgy)
{
    HDC drawdc; // device context we'll do the eventual drawing with
    HBITMAP tmp = NULL; // used when scaling

    if (!image_is_stretched(image)) {

        SelectObject(hdcMem, image->bitmap);
        drawdc = hdcMem;

    } else {
        // temporary device context for the scaling operation
        drawdc = CreateCompatibleDC(NULL);

        // set stretch mode from image
        SetStretchBltMode(drawdc, image->stretch_mode);

        // scaled bitmap will be drawn onto this bitmap
        tmp = CreateCompatibleBitmap(hdcMem, image->scaled_width, image->scaled_height);
        SelectObject(drawdc, tmp);

        SelectObject(hdcMem, image->bitmap);

        // stretch image onto temporary bitmap
        if (image->has_alpha) {
            AlphaBlend(drawdc, 0, 0, image->scaled_width, image->scaled_height, hdcMem, 0, 0, image->width, image->height, blend_function);
        } else {
            StretchBlt(drawdc, 0, 0, image->scaled_width, image->scaled_height, hdcMem, 0, 0, image->width, image->height, SRCCOPY);
        }
    }

    // clip and draw
    if (image->has_alpha) {
        AlphaBlend(hdc, x, y, width, height, drawdc, imgx, imgy, width, height, blend_function);
    } else {
        BitBlt(hdc, x, y, width, height, drawdc, imgx, imgy, SRCCOPY);
    }

    // clean up
    if (image_is_stretched(image)) {
        DeleteObject(tmp);
        DeleteDC(drawdc);
    }
}

void drawtext(int x, int y, char_t *str, STRING_IDX length)
{
    wchar_t out[length];
    length = utf8tonative(str, out, length);

    TextOutW(hdc, x, y, out, length);
}

int drawtext_getwidth(int x, int y, char_t *str, STRING_IDX length)
{
    wchar_t out[length];
    length = utf8tonative(str, out, length);

    SIZE size;
    TextOutW(hdc, x, y, out, length);
    GetTextExtentPoint32W(hdc, out, length, &size);
    return size.cx;
}

void drawtextwidth(int x, int width, int y, char_t *str, STRING_IDX length)
{
    wchar_t out[length];
    length = utf8tonative(str, out, length);

    RECT r = {x, y, x + width, y + 256};
    DrawTextW(hdc, out, length, &r, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
}

void drawtextwidth_right(int x, int width, int y, char_t *str, STRING_IDX length)
{
    wchar_t out[length];
    length = utf8tonative(str, out, length);

    RECT r = {x, y, x + width, y + 256};
    DrawTextW(hdc, out, length, &r, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX | DT_RIGHT);
}

void drawtextrange(int x, int x2, int y, char_t *str, STRING_IDX length)
{
    wchar_t out[length];
    length = utf8tonative(str, out, length);

    RECT r = {x, y, x2, y + 256};
    DrawTextW(hdc, out, length, &r, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
}

void drawtextrangecut(int x, int x2, int y, char_t *str, STRING_IDX length)
{
    wchar_t out[length];
    length = utf8tonative(str, out, length);

    RECT r = {x, y, x2, y + 256};
    DrawTextW(hdc, out, length, &r, DT_SINGLELINE | DT_NOPREFIX);
}

int textwidth(char_t *str, STRING_IDX length)
{
    wchar_t out[length];
    length = utf8tonative(str, out, length);

    SIZE size;
    GetTextExtentPoint32W(hdc, out, length, &size);
    return size.cx;
}

int textfit(char_t *str, STRING_IDX len, int width)
{
    wchar_t out[len];
    int length = utf8tonative(str, out, len);

    int fit;
    SIZE size;
    GetTextExtentExPointW(hdc, out, length, width, &fit, NULL, &size);

    return WideCharToMultiByte(CP_UTF8, 0, out, fit, (char*)str, len, NULL, 0);
}

int textfit_near(char_t *str, STRING_IDX len, int width)
{
    /*todo: near*/
    wchar_t out[len];
    int length = utf8tonative(str, out, len);

    int fit;
    SIZE size;
    GetTextExtentExPointW(hdc, out, length, width, &fit, NULL, &size);

    return WideCharToMultiByte(CP_UTF8, 0, out, fit, (char*)str, len, NULL, 0);
}

void draw_rect_frame(int x, int y, int width, int height, uint32_t color) {
    RECT r = {x, y, x + width, y + height};
    SetDCBrushColor(hdc, color);
    FrameRect(hdc, &r, hdc_brush);
}

void draw_rect_fill(int x, int y, int width, int height, uint32_t color) {
    RECT r = {x, y, x + width, y + height};
    SetDCBrushColor(hdc, color);
    FillRect(hdc, &r, hdc_brush);
}

void drawrect(int x, int y, int right, int bottom, uint32_t color)
{
    RECT r = {x, y, right, bottom};
    SetDCBrushColor(hdc, color);
    FillRect(hdc, &r, hdc_brush);
}


void drawhline(int x, int y, int x2, uint32_t color)
{
    RECT r = {x, y, x2, y + 1};
    SetDCBrushColor(hdc, color);
    FillRect(hdc, &r, hdc_brush);
}

void drawvline(int x, int y, int y2, uint32_t color)
{
    RECT r = {x, y, x + 1, y2};
    SetDCBrushColor(hdc, color);
    FillRect(hdc, &r, hdc_brush);
}

void setfont(int id)
{
    SelectObject(hdc, font[id]);
}

uint32_t setcolor(uint32_t color)
{
    return SetTextColor(hdc, color);
}

RECT clip[16];

static int clipk;

void pushclip(int left, int top, int width, int height)
{
    int right = left + width, bottom = top + height;

    RECT *r = &clip[clipk++];
    r->left = left;
    r->top = top;
    r->right = right;
    r->bottom = bottom;

    HRGN rgn = CreateRectRgn(left, top, right, bottom);
    SelectClipRgn (hdc, rgn);
    DeleteObject(rgn);
}

void popclip(void)
{
    clipk--;
    if(!clipk) {
        SelectClipRgn(hdc, NULL);
        return;
    }

    RECT *r = &clip[clipk - 1];

    HRGN rgn = CreateRectRgn(r->left, r->top, r->right, r->bottom);
    SelectClipRgn (hdc, rgn);
    DeleteObject(rgn);
}

void enddraw(int x, int y, int width, int height)
{
    SelectObject(hdc, hdc_bm);
    BitBlt(main_hdc, x, y, width, height, hdc, x, y, SRCCOPY);
}

void thread(void func(void*), void *args)
{
    _beginthread(func, 0, args);
}

void yieldcpu(uint32_t ms)
{
    Sleep(ms);
}

uint64_t get_time(void)
{
    return ((uint64_t)clock() * 1000 * 1000);
}

void openurl(char_t *str)
{
    //!convert
    ShellExecute(NULL, "open", (char*)str, NULL, NULL, SW_SHOW);
}

/** Open system file browser dialog */
void openfilesend(void)
{
    char *filepath = malloc(1024);
    filepath[0] = 0;

    wchar_t dir[1024];
    GetCurrentDirectoryW(countof(dir), dir);

    OPENFILENAME ofn = {
        .lStructSize = sizeof(OPENFILENAME),
        .hwndOwner = hwnd,
        .lpstrFile = filepath,
        .nMaxFile = 1024,
        .Flags = OFN_EXPLORER | OFN_ALLOWMULTISELECT | OFN_FILEMUSTEXIST,
    };

    if(GetOpenFileName(&ofn)) {
        postmessage_toxcore(TOX_FILE_SEND_NEW, (FRIEND*)selected_item->data - friend, ofn.nFileOffset, filepath);
    } else {
        debug("GetOpenFileName() failed\n");
    }

    SetCurrentDirectoryW(dir);
}

void openfileavatar(void)
{
    char *filepath = malloc(1024);
    filepath[0] = 0;

    wchar_t dir[1024];
    GetCurrentDirectoryW(countof(dir), dir);

    OPENFILENAME ofn = {
        .lStructSize = sizeof(OPENFILENAME),
        .lpstrFilter = "PNG Files\0*.PNG\0\0",
        .hwndOwner = hwnd,
        .lpstrFile = filepath,
        .nMaxFile = 1024,
        .Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST,
    };

    while (1) { // loop until we have a good file or the user closed the dialog
        if(GetOpenFileName(&ofn)) {
            uint32_t size;

            void *file_data = file_raw(filepath, &size);
            if (!file_data) {
                MessageBox(NULL, (const char *)S(CANT_FIND_FILE_OR_EMPTY), NULL, MB_ICONWARNING);
            } else if (size > UTOX_AVATAR_MAX_DATA_LENGTH) {
                free(file_data);
                char_t message[1024];
                if (sizeof(message) < SLEN(AVATAR_TOO_LARGE_MAX_SIZE_IS) + 16) {
                    debug("error: AVATAR_TOO_LARGE message is larger than allocated buffer(%u bytes)\n", (unsigned int)sizeof(message));
                    break;
                }
                // create message containing text that selected avatar is too large and what the max size is
                int len = sprintf((char *)message, "%.*s", SLEN(AVATAR_TOO_LARGE_MAX_SIZE_IS), S(AVATAR_TOO_LARGE_MAX_SIZE_IS));
                len += sprint_humanread_bytes(message+len, sizeof(message)-len, UTOX_AVATAR_MAX_DATA_LENGTH);
                message[len++] = '\0';
                MessageBox(NULL, (char *)message, NULL, MB_ICONWARNING);
            } else {
                postmessage(SELF_AVATAR_SET, size, 0, file_data);
                break;
            }
        } else {
            debug("GetOpenFileName() failed\n");
            break;
        }
    }

    free(filepath);
    SetCurrentDirectoryW(dir);
}

void savefilerecv(uint32_t fid, MSG_FILE *file)
{
    char *path = malloc(256);
    memcpy(path, file->name, file->name_length);
    path[file->name_length] = 0;

    OPENFILENAME ofn = {
        .lStructSize = sizeof(OPENFILENAME),
        .hwndOwner = hwnd,
        .lpstrFile = path,
        .nMaxFile = UTOX_FILE_NAME_LENGTH,
        .Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT,
    };

    if(GetSaveFileName(&ofn)) {
        postmessage_toxcore(TOX_FILE_ACCEPT, fid, file->filenumber, path);
    } else {
        debug("GetSaveFileName() failed\n");
    }
}

void savefiledata(MSG_FILE *file)
{
    char *path = malloc(UTOX_FILE_NAME_LENGTH);
    memcpy(path, file->name, file->name_length);
    path[file->name_length] = 0;

    OPENFILENAME ofn = {
        .lStructSize = sizeof(OPENFILENAME),
        .hwndOwner = hwnd,
        .lpstrFile = path,
        .nMaxFile = UTOX_FILE_NAME_LENGTH,
        .Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT,
    };

    if(GetSaveFileName(&ofn)) {
        FILE *fp = fopen(path, "wb");
        if(fp) {
            fwrite(file->path, file->size, 1, fp);
            fclose(fp);

            free(file->path);
            file->path = (uint8_t*)strdup(path);
            file->inline_png = 0;
        }
    } else {
        debug("GetSaveFileName() failed\n");
    }
}

void setselection(char_t *data, STRING_IDX length)
{
}

/** Toggles the main window to/from hidden to tray/shown. */
void togglehide(int show){
    if ( hidden || show ) {
        ShowWindow(hwnd, SW_RESTORE);
        SetForegroundWindow(hwnd);
        redraw();
        hidden = 0;
    } else {
        ShowWindow(hwnd, SW_HIDE);
        hidden = 1;
    }
}

/** Right click context menu for the tray icon */
void ShowContextMenu(void)
{
    POINT pt;
    GetCursorPos(&pt);
    HMENU hMenu = CreatePopupMenu();
    if(hMenu) {
        InsertMenu(hMenu, -1, MF_BYPOSITION, TRAY_SHOWHIDE, hidden ? "Restore" : "Hide");

        InsertMenu(hMenu, -1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);

        InsertMenu(hMenu, -1, MF_BYPOSITION | ((self.status == TOX_USER_STATUS_NONE) ? MF_CHECKED : 0), TRAY_STATUS_AVAILABLE, "Available");
        InsertMenu(hMenu, -1, MF_BYPOSITION | ((self.status == TOX_USER_STATUS_AWAY) ? MF_CHECKED : 0), TRAY_STATUS_AWAY, "Away");
        InsertMenu(hMenu, -1, MF_BYPOSITION | ((self.status == TOX_USER_STATUS_BUSY) ? MF_CHECKED : 0), TRAY_STATUS_BUSY, "Busy");

        InsertMenu(hMenu, -1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);

        InsertMenu(hMenu, -1, MF_BYPOSITION, TRAY_EXIT, "Exit");

        // note:    must set window to the foreground or the
        //          menu won't disappear when it should
        SetForegroundWindow(hwnd);

        TrackPopupMenu(hMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, NULL);
        DestroyMenu(hMenu);
    }
}

static void parsecmd(uint8_t *cmd, int len)
{
    debug("Command: %.*s\n", len, cmd);

    //! lacks max length checks, writes to inputs even on failure, no notice of failure
    //doesnt reset unset inputs

    if(len > 6 && memcmp(cmd, "tox://", 6) == 0) {
        cmd += 6;
        len -= 6;
    } else if (len > 4 && memcmp(cmd, "tox:", 4) == 0) {
        cmd += 4;
        len -= 4;
    } else {
        return;
    }


    uint8_t *b = edit_add_id.data, *a = cmd, *end = cmd + len;
    uint16_t *l = &edit_add_id.length;
    *l = 0;
    while(a != end)
    {
        switch(*a)
        {
            case 'a' ... 'z':
            case 'A' ... 'Z':
            case '0' ... '9':
            case '@':
            case '.':
            case ' ':
            {
                *b++ = *a;
                *l = *l + 1;
                break;
            }

            case '+':
            {
                *b++ = ' ';
                *l = *l + 1;
                break;
            }

            case '?':
            case '&':
            {
                a++;        /* Anyone know what pin is used for? */
                if(end - a >= 4 && memcmp(a, "pin=", 4) == 0)
                {

                    l = &edit_add_id.length;
                    b = edit_add_id.data + *l;
                    *b++ = ':';
                    *l = *l + 1;
                    a += 3;
                    break;
                }
                else if(end - a >= 8 && memcmp(a, "message=", 8) == 0)
                {
                    b = edit_add_msg.data;
                    l = &edit_add_msg.length;
                    *l = 0;
                    a += 7;
                    break;
                }
                return;
            }

            case '/':
            {
                break;
            }

            default:
            {
                return;
            }
        }
        a++;
    }

    list_selectaddfriend();
}

void loadalpha(int bm, void *data, int width, int height)
{
    bitmap[bm] = data;
}

// creates an UTOX_NATIVE image based on given arguments
// image should be freed with image_free
static UTOX_NATIVE_IMAGE *create_utox_image(HBITMAP bmp, _Bool has_alpha, uint32_t width, uint32_t height)
{
    UTOX_NATIVE_IMAGE *image = malloc(sizeof(UTOX_NATIVE_IMAGE));
    image->bitmap = bmp;
    image->has_alpha = has_alpha;
    image->width = width;
    image->height = height;
    image->scaled_width = width;
    image->scaled_height = height;
    image->stretch_mode = COLORONCOLOR;

    return image;
}


static void sendbitmap(HDC mem, HBITMAP hbm, int width, int height)
{
    if (width == 0 || height == 0)
        return;

    BITMAPINFO info = {
        .bmiHeader = {
            .biSize = sizeof(BITMAPINFOHEADER),
            .biWidth = width,
            .biHeight = -(int)height,
            .biPlanes = 1,
            .biBitCount = 24,
            .biCompression = BI_RGB,
        }
    };

    void *bits = malloc((width + 3) * height * 3);

    GetDIBits(mem, hbm, 0, height, bits, &info, DIB_RGB_COLORS);

    uint8_t pbytes = width & 3, *p = bits, *pp = bits, *end = p + width * height * 3;
    //uint32_t offset = 0;
    while(p != end) {
        int i;
        for(i = 0; i != width; i++) {
            uint8_t b = pp[i * 3];
            p[i * 3] = pp[i * 3 + 2];
            p[i * 3 + 1] = pp[i * 3 + 1];
            p[i * 3 + 2] = b;
        }
        p += width * 3;
        pp += width * 3 + pbytes;
    }

    uint8_t *out;
    size_t size;
    lodepng_encode_memory(&out, &size, bits, width, height, LCT_RGB, 8);
    free(bits);

    UTOX_NATIVE_IMAGE *image = create_utox_image(hbm, 0, width, height);
    friend_sendimage(selected_item->data, image, width, height, (UTOX_PNG_IMAGE)out, size);
}

void copy(int value)
{
    char_t data[32768];//!TODO: De-hardcode this value.
    int len;

    if(edit_active()) {
        len = edit_copy(data, 32767);
        data[len] = 0;
    } else if(selected_item->item == ITEM_FRIEND) {
        len = messages_selection(&messages_friend, data, 32768, value);
    } else if(selected_item->item == ITEM_GROUP) {
        len = messages_selection(&messages_group, data, 32768, value);
    } else {
        return;
    }

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (len + 1) * 2);
    wchar_t *d = GlobalLock(hMem);
    utf8tonative(data, d, len + 1); //because data is nullterminated
    GlobalUnlock(hMem);
    OpenClipboard(hwnd);
    EmptyClipboard();
    SetClipboardData(CF_UNICODETEXT, hMem);
    CloseClipboard();
}

void paste(void)
{
    OpenClipboard(NULL);
    HANDLE h = GetClipboardData(CF_UNICODETEXT);
    if(!h) {
        h = GetClipboardData(CF_BITMAP);
        if(h && selected_item->item == ITEM_FRIEND) {
            HBITMAP copy;
            BITMAP bm;
            HDC tempdc;
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
        char_t data[65536]; //TODO: De-hardcode this value.
        int len = WideCharToMultiByte(CP_UTF8, 0, d, -1, (char*)data, sizeof(data), NULL, 0);
        if(edit_active()) {
            edit_paste(data, len, 0);
        }
    }

    GlobalUnlock(h);
    CloseClipboard();
}

UTOX_NATIVE_IMAGE *png_to_image(const UTOX_PNG_IMAGE data, size_t size, uint16_t *w, uint16_t *h, _Bool keep_alpha)
{
    uint8_t *rgba_data;
    unsigned width, height;
    unsigned r = lodepng_decode32(&rgba_data, &width, &height, data->png_data, size);

    if(r != 0 || !width || !height) {
        return NULL; // invalid image
    }

    BITMAPINFO bmi = {
        .bmiHeader = {
            .biSize = sizeof(BITMAPINFOHEADER),
            .biWidth = width,
            .biHeight = -height,
            .biPlanes = 1,
            .biBitCount = 32,
            .biCompression = BI_RGB,
        }
    };

    // create device independent bitmap, we can write the bytes to out
    // to put them in the bitmap
    uint8_t *out;
    HBITMAP bmp = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, (void**)&out, NULL, 0);

    // convert RGBA data to internal format
    // pre-applying the alpha if we're keeping the alpha channel,
    // put the result in out
    // NOTE: input pixels are in format RGBA, output is BGRA
    uint8_t *p, *end = rgba_data + width * height * 4;
    p = rgba_data;
    if (keep_alpha) {
        uint8_t alpha;
        do {
            alpha = p[3];
            out[0] = p[2] * (alpha / 255.0); // pre-apply alpha
            out[1] = p[1] * (alpha / 255.0);
            out[2] = p[0] * (alpha / 255.0);
            out[3] = alpha;
            out += 4;
            p += 4;
        } while(p != end);
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


    UTOX_NATIVE_IMAGE *image = create_utox_image(bmp, keep_alpha, width, height);

    *w = width;
    *h = height;
    return image;
}

void image_free(UTOX_NATIVE_IMAGE *image)
{
    DeleteObject(image->bitmap);
    free(image);
}

int datapath_old(uint8_t *dest)
{
    if (utox_portable) {
        return 0;
    } else {
        if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, (char*)dest))) {
            uint8_t *p = dest + strlen((char*)dest);
            strcpy((char *)p, "\\Tox"); p += 4;
            *p++ = '\\';
            return p - dest;
        }
    }

    return 0;
}

int datapath(uint8_t *dest)
{
    if (utox_portable) {
        uint8_t *p = dest;
        strcpy((char *)p, utox_portable_save_path); p += strlen(utox_portable_save_path);
        strcpy((char *)p, "\\Tox"); p += 4;
        CreateDirectory((char*)dest, NULL);
        *p++ = '\\';
        return p - dest;
    } else {
        if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, (char*)dest))) {
            uint8_t *p = dest + strlen((char*)dest);
            strcpy((char *)p, "\\Tox"); p += 4;
            CreateDirectory((char*)dest, NULL);
            *p++ = '\\';
            return p - dest;
        }
    }

    return 0;
}

int datapath_subdir(uint8_t *dest, const char *subdir)
{
    int l = datapath(dest);
    l += sprintf((char*)(dest+l), "%s", subdir);
    CreateDirectory((char*)dest, NULL);
    dest[l++] = '\\';

    return l;
}

void flush_file(FILE *file)
{
    fflush(file);
    int fd = _fileno(file);
    _commit(fd);
}

int ch_mod(uint8_t *file){
    /* You're probably looking for ./xlib as windows is lamesauce and wants nothing to do with sane permissions */
    return 1;
}

int file_lock(FILE *file, uint64_t start, size_t length){
    OVERLAPPED lock_overlap;
    lock_overlap.Offset     = start;
    lock_overlap.OffsetHigh = start+length;
    lock_overlap.hEvent     = 0;
    return !LockFileEx(file, LOCKFILE_FAIL_IMMEDIATELY, 0, start, start + length, &lock_overlap);
}

int file_unlock(FILE *file, uint64_t start, size_t length){
    OVERLAPPED lock_overlap;
    lock_overlap.Offset     = start;
    lock_overlap.OffsetHigh = start+length;
    lock_overlap.hEvent     = 0;
    return UnlockFileEx(file, 0, start, start + length, &lock_overlap);
}

/** Creates a tray baloon popup with the message, and flashes the main window
 *
 * accepts: char_t *title, title length, char_t *msg, msg length;
 * returns void;
 */
void notify(char_t *title, STRING_IDX title_length, char_t *msg, STRING_IDX msg_length, FRIEND *f){
    if( havefocus || self.status == 2 ) {
        return;
    }

    FlashWindow(hwnd, 1);
    flashing = 1;

    NOTIFYICONDATAW nid = {
        .uFlags = NIF_ICON | NIF_INFO,
        .hWnd = hwnd,
        .hIcon = unread_messages_icon,
        .uTimeout = 5000,
        .dwInfoFlags = 0,
        .cbSize = sizeof(nid),
    };

    utf8tonative(title, nid.szInfoTitle, title_length > sizeof(nid.szInfoTitle) / sizeof(*nid.szInfoTitle) - 1 ? sizeof(nid.szInfoTitle) / sizeof(*nid.szInfoTitle) - 1 : title_length);
    utf8tonative(msg, nid.szInfo, msg_length > sizeof(nid.szInfo) / sizeof(*nid.szInfo) - 1 ? sizeof(nid.szInfo) / sizeof(*nid.szInfo) - 1 : msg_length);

    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

void showkeyboard(_Bool show){} /* Added for android support. */

void edit_will_deactivate(void){}

/* Redraws the main UI window */
void redraw(void) {
    panel_draw(&panel_root, 0, 0, utox_window_width, utox_window_height);
}

/**
 * update_tray(void)
 * creates a win32 NOTIFYICONDATAW struct, sets the tiptab flag, gives *hwnd,
 * sets struct .cbSize, and resets the tibtab to native self.name;
 */
void update_tray(void){
    uint32_t tip_length;
    char *tip;
    tip = malloc(128 * sizeof(char)); //128 is the max length of nid.szTip

    snprintf(tip, 127*sizeof(char), "%s : %s", self.name, self.statusmsg);
    tip_length = self.name_length + 3 + self.statusmsg_length;

    NOTIFYICONDATAW nid = {
        .uFlags = NIF_TIP,
        .hWnd = hwnd,
        .cbSize = sizeof(nid),
    };

    utf8str_to_native((char_t *)tip, nid.szTip, tip_length);

    Shell_NotifyIconW(NIM_MODIFY, &nid);

    free(tip);
}

void force_redraw(void) {
    redraw();
}

static int grabx, graby, grabpx, grabpy;
static _Bool grabbing;

void desktopgrab(_Bool video)
{
    int x, y, w, h;

    x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    h = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    debug("result: %i %i %i %i\n", x, y, w, h);

    capturewnd = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_LAYERED, L"uToxgrab", L"Tox", WS_POPUP, x, y, w, h, NULL, NULL, hinstance, NULL);
    if(!capturewnd) {
        debug("CreateWindowExW() failed\n");
        return;
    }

    SetLayeredWindowAttributes(capturewnd, 0xFFFFFF, 128, LWA_ALPHA | LWA_COLORKEY);


    //UpdateLayeredWindow(hwnd, NULL, NULL, NULL, NULL, NULL, 0xFFFFFF, ULW_ALPHA | ULW_COLORKEY);

    ShowWindow(capturewnd, SW_SHOW);
    SetForegroundWindow(capturewnd);

    desktopgrab_video = video;

    //SetCapture(hwnd);
    //grabbing = 1;

    //postmessage_video(VIDEO_SET, 0, 0, (void*)1);
}

LRESULT CALLBACK GrabProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
{
    POINT p = {
        .x = GET_X_LPARAM(lParam),
        .y = GET_Y_LPARAM(lParam)
    };

    ClientToScreen(window, &p);

    if(msg == WM_MOUSEMOVE) {

        if(grabbing) {
            HDC dc = GetDC(window);
            BitBlt(dc, grabx, graby, grabpx - grabx, grabpy - graby, dc, grabx, graby, BLACKNESS);
            grabpx = p.x;
            grabpy = p.y;
            BitBlt(dc, grabx, graby, grabpx - grabx, grabpy - graby, dc, grabx, graby, WHITENESS);
            ReleaseDC(window, dc);
        }

        return 0;
    }

    if(msg == WM_LBUTTONDOWN) {
        grabx = grabpx = p.x;
        graby = grabpy = p.y;
        grabbing = 1;
        SetCapture(window);
        return 0;
    }

    if(msg == WM_LBUTTONUP) {
        ReleaseCapture();
        grabbing = 0;

        if(grabx < grabpx) {
            grabpx -= grabx;
        } else {
            int w = grabx - grabpx;
            grabx = grabpx;
            grabpx = w;
        }

        if(graby < grabpy) {
            grabpy -= graby;
        } else {
            int w = graby - grabpy;
            graby = grabpy;
            grabpy = w;
        }

        if(desktopgrab_video) {
            postmessage_video(VIDEO_SET, 0, 0, (void*)1);
            DestroyWindow(window);
        } else {
            FRIEND *f = selected_item->data;
            if(selected_item->item == ITEM_FRIEND && f->online) {
                DestroyWindow(window);
                HWND dwnd = GetDesktopWindow();
                HDC ddc = GetDC(dwnd);
                HDC mem = CreateCompatibleDC(ddc);

                HBITMAP capture = CreateCompatibleBitmap(ddc, grabpx, grabpy);
                SelectObject(mem, capture);

                BitBlt(mem, 0, 0, grabpx, grabpy, ddc, grabx, graby, SRCCOPY | CAPTUREBLT);
                sendbitmap(mem, capture, grabpx, grabpy);

                ReleaseDC(dwnd, ddc);
                DeleteDC(mem);
            }
        }


        return 0;
    }

    if(msg == WM_DESTROY) {
        grabbing = 0;
    }

    return DefWindowProcW(window, msg, wParam, lParam);
}

void freefonts(){
    for(int i = 0; i != countof(font); i++) {
        if(font[i]) {
            DeleteObject(font[i]);
        }
    }
}

void loadfonts(){
    LOGFONT lf = {
        .lfWeight = FW_NORMAL,
        //.lfCharSet = ANSI_CHARSET,
        .lfOutPrecision = OUT_TT_PRECIS,
        .lfQuality = CLEARTYPE_QUALITY,
        .lfFaceName = "DejaVu Sans",
    };

    #define F(x) ((UTOX_SCALE(-x) - 1) / 2)
    lf.lfHeight = F(12);
    font[FONT_TEXT] = CreateFontIndirect(&lf);

    lf.lfHeight = F(11);
    font[FONT_STATUS] = CreateFontIndirect(&lf);
    lf.lfHeight = F(12);
    font[FONT_LIST_NAME] = CreateFontIndirect(&lf);
    lf.lfWeight = FW_BOLD;
    font[FONT_TITLE] = CreateFontIndirect(&lf);
    lf.lfHeight = F(14);
    font[FONT_SELF_NAME] = CreateFontIndirect(&lf);
    lf.lfHeight = F(10);
    font[FONT_MISC] = CreateFontIndirect(&lf);
    /*lf.lfWeight = FW_NORMAL; //FW_LIGHT <- light fonts dont antialias
    font[FONT_MSG_NAME] = CreateFontIndirect(&lf);
    lf.lfHeight = F(11);
    font[FONT_MSG] = CreateFontIndirect(&lf);
    lf.lfUnderline = 1;
    font[FONT_MSG_LINK] = CreateFontIndirect(&lf);*/
    #undef F

    TEXTMETRIC tm;
    SelectObject(hdc, font[FONT_TEXT]);
    GetTextMetrics(hdc, &tm);
    font_small_lineheight = tm.tmHeight + tm.tmExternalLeading;
    //SelectObject(hdc, font[FONT_MSG]);
    //GetTextMetrics(hdc, &tm);
    //font_msg_lineheight = tm.tmHeight + tm.tmExternalLeading;

}

void setscale_fonts(void){
    freefonts();
    loadfonts();
}

void setscale(void){
    svg_draw(1);
}

void config_osdefaults(UTOX_SAVE *r)
{
    r->window_x = (GetSystemMetrics(SM_CXSCREEN) - MAIN_WIDTH) / 2;
    r->window_y = (GetSystemMetrics(SM_CYSCREEN) - MAIN_HEIGHT) / 2;
    r->window_width = MAIN_WIDTH;
    r->window_height = MAIN_HEIGHT;
}

/** client main()
 *
 * Main thread
 * generates settings, loads settings from save file, generates main UI, starts
 * tox, generates tray icon, handles client messages. Cleans up, and exits.
 *
 * also handles call from other apps.
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR cmd, int nCmdShow){

    /* if opened with argument, check if uTox is already open and pass the argument to the existing process */
    HANDLE utox_mutex = CreateMutex(NULL, 0, TITLE);

    if (!utox_mutex) {
        return 0;
    }
    if(GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND window = FindWindow(TITLE, NULL);
        if (window) {
            COPYDATASTRUCT data = {
                .cbData = strlen(cmd),
                .lpData = cmd
            };
            SendMessage(window, WM_COPYDATA, (WPARAM)hInstance, (LPARAM)&data);
        }
        return 0;
    }

    /* Process argc/v the backwards (read: windows) way. */
    LPWSTR *arglist;
    int argc, i;
    /* Variables for --set */
    int32_t set_show_window = 0;


    _Bool no_updater = 0;
    /* Convert PSTR command line args from windows to argc */
    arglist = CommandLineToArgvW(GetCommandLineW(), &argc);
    if( NULL == arglist ){
      debug("CommandLineToArgvW failed\n");
    } else {
        for( i=0; i < argc; i++) {
            if(wcscmp(arglist[i], L"--version") == 0) {
                debug("uTox version: %s\n", VERSION);
                return 0;
            } else if (wcscmp(arglist[i], L"--portable") == 0) {
                /* force the working directory if opened with portable command */
                HMODULE hModule = GetModuleHandle(NULL);
                char path[MAX_PATH];
                int len = GetModuleFileName(hModule, path, MAX_PATH);
                unsigned int i;
                for (i = (len - 1); path[i] != '\\'; --i);
                path[i] = 0;//!
                SetCurrentDirectory(path);
                utox_portable = 1;
                strcpy(utox_portable_save_path, path);
                debug("Starting uTox in portable mode: Data will be saved to tox/ in the current directory: %s\n", utox_portable_save_path);
            } else if(wcscmp(arglist[i], L"--theme") == 0){
                debug("Searching for theme from argv\n");
                if(arglist[(i+1)]){
                    if(wcscmp(arglist[(i+1)], L"default") == 0){
                        theme = THEME_DEFAULT;
                    } else if(wcscmp(arglist[(i+1)], L"dark") == 0){
                        theme = THEME_DARK;
                    } else if(wcscmp(arglist[(i+1)], L"light") == 0){
                        theme = THEME_LIGHT;
                    } else if(wcscmp(arglist[(i+1)], L"highcontrast") == 0){
                        theme = THEME_HIGHCONTRAST;
                    } else if(wcscmp(arglist[(i+1)], L"zenburn") == 0){
                        theme = THEME_ZENBURN;
                    } else {
                        debug("Please specify correct theme (please check user manual for list of correct values).");
                        theme = THEME_DEFAULT;
                    }
                }
            /* Set flags */
            } else if(wcsncmp(arglist[i], L"--set", 5) == 0){
                // debug("Set flag on\n");
                if(wcsncmp(arglist[i], L"--set=", 6) == 0){
                    if(wcscmp(arglist[i]+6, L"start-on-boot") == 0){
                        launch_at_startup(1);
                    } else if(wcscmp(arglist[i]+6, L"show-window") == 0){
                        set_show_window = 1;
                    } else if(wcscmp(arglist[i]+6, L"hide-window") == 0){
                        set_show_window = -1;
                    }
                } else {
                    if(arglist[i+1]){
                        if(wcscmp(arglist[i+1], L"start-on-boot") == 0){
                            launch_at_startup(1);
                        } else if(wcscmp(arglist[i+1], L"show-window") == 0){
                            set_show_window = 1;
                        } else if(wcscmp(arglist[i+1], L"hide-window") == 0){
                            set_show_window = -1;
                        }
                    }
                }
            /* Unset flags */
            } else if(wcsncmp(arglist[i], L"--unset", 7) == 0){
                // debug("Unset flag on\n");
                if(wcsncmp(arglist[i], L"--unset=", 8) == 0){
                    if(wcscmp(arglist[i]+8, L"start-on-boot") == 0)
                        // debug("unset start\n");
                        launch_at_startup(0);
                } else {
                    if(arglist[i+1]){
                        if(wcscmp(arglist[i+1], L"start-on-boot") == 0)
                            // debug("unset start\n");
                            launch_at_startup(0);
                    }
                }
            } else if(wcscmp(arglist[i], L"--no-updater") == 0){
                no_updater = 1;
            }
        }
    }
#ifdef UPDATER_BUILD
#define UTOX_EXE "\\uTox.exe"
#define UTOX_UPDATER_EXE "\\utox_runner.exe"
#define UTOX_VERSION_FILE "\\version"

    if (!no_updater) {
        char path[MAX_PATH + 20];
        int len = GetModuleFileName(NULL, path, MAX_PATH);

        /* Is the uTox exe named like the updater one. */
        if (len > sizeof(UTOX_EXE) && memcmp(path + (len - (sizeof(UTOX_EXE) - 1)), UTOX_EXE, sizeof(UTOX_EXE)) == 0) {
            memcpy(path + (len - (sizeof(UTOX_EXE) - 1)), UTOX_VERSION_FILE, sizeof(UTOX_VERSION_FILE));
            FILE *fp = fopen(path, "rb");
            if (fp) {
                fclose(fp);
                /* Updater is here. */
                memcpy(path + (len - (sizeof(UTOX_EXE) - 1)), UTOX_UPDATER_EXE, sizeof(UTOX_UPDATER_EXE));
                FILE *fp = fopen(path, "rb");
                if (fp) {
                    fclose(fp);
                    CloseHandle(utox_mutex);
                    /* This is an updater build not being run by the updater. Run the updater and exit. */
                    ShellExecute(NULL, "open", path, cmd, NULL, SW_SHOW);
                    return 0;
                }
            }
        }
    }
#endif

#ifdef __WIN_LEGACY
    debug("Legacy windows build\n");
#else
    debug("Normal windows build\n");
#endif

    theme_load(theme);

    // Free memory allocated for CommandLineToArgvW arguments.
    LocalFree(arglist);

    /* */
    MSG msg;
    //int x, y;
    wchar_t classname[] = L"uTox", popupclassname[] = L"uToxgrab";

    my_icon = LoadIcon(hInstance, MAKEINTRESOURCE(101));
    unread_messages_icon = LoadIcon(hInstance, MAKEINTRESOURCE(102));

    cursors[CURSOR_NONE] = LoadCursor(NULL, IDC_ARROW);
    cursors[CURSOR_HAND] = LoadCursor(NULL, IDC_HAND);
    cursors[CURSOR_TEXT] = LoadCursor(NULL, IDC_IBEAM);
    cursors[CURSOR_SELECT] = LoadCursor(NULL, IDC_CROSS);
    cursors[CURSOR_ZOOM_IN] = LoadCursor(NULL, IDC_SIZEALL);
    cursors[CURSOR_ZOOM_OUT] = LoadCursor(NULL, IDC_SIZEALL);

    hinstance = hInstance;

    WNDCLASSW wc = {
        .style = CS_OWNDC | CS_DBLCLKS,
        .lpfnWndProc = WindowProc,
        .hInstance = hInstance,
        .hIcon = my_icon,
        .lpszClassName = classname,
    },

    wc2 = {
        .lpfnWndProc = GrabProc,
        .hInstance = hInstance,
        .hIcon = my_icon,
        .lpszClassName = popupclassname,
        .hbrBackground = (HBRUSH)GetStockObject (BLACK_BRUSH),
    };

    NOTIFYICONDATA nid = {
        .uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP,
        .uCallbackMessage = WM_NOTIFYICON,
        .hIcon = my_icon,
        .szTip = "uTox default tooltip",
        .cbSize = sizeof(nid),
    };

    OleInitialize(NULL);
    RegisterClassW(&wc);
    RegisterClassW(&wc2);

    uint16_t langid = GetUserDefaultUILanguage() & 0xFFFF;
    LANG = ui_guess_lang_by_windows_lang_id(langid, DEFAULT_LANG);

    dropdown_language.selected = dropdown_language.over = LANG;

    UTOX_SAVE *save = config_load();

    char pretitle[128];
    snprintf(pretitle, 128, "%s %s (version : %s)", TITLE, SUB_TITLE, VERSION);
    size_t title_size = strlen(pretitle)+1;
    wchar_t title[title_size];
    mbstowcs(title,pretitle,title_size);
    /* trim first letter that appears for god knows why */
    /* needed if/when the uTox becomes a muTox */
    //wmemmove(title, title+1, wcslen(title));

    hwnd = CreateWindowExW(0, classname, title, WS_OVERLAPPEDWINDOW,
                            save->window_x, save->window_y,
                            save->window_width, save->window_height,
                            NULL, NULL, hInstance, NULL);

    free(save);

    hdc_brush = GetStockObject(DC_BRUSH);


    tme.hwndTrack = hwnd;

    nid.hWnd = hwnd;
    Shell_NotifyIcon(NIM_ADD, &nid);

    SetBkMode(hdc, TRANSPARENT);

    dnd_init(hwnd);

    //start tox thread (hwnd needs to be set first)
    thread(toxcore_thread, NULL);

    if (*cmd) {
        int len = strlen(cmd);
        parsecmd((uint8_t*)cmd, len);
    }

    draw = 1;
    redraw();
    update_tray();

    /* From --set flag */
    if(set_show_window){
        if(set_show_window == 1){
            start_in_tray = 0;
        } else if(set_show_window == -1){
            start_in_tray = 1;
        }
    }

    if(start_in_tray){
        ShowWindow(hwnd, SW_HIDE);
        hidden = 1;
    } else {
        ShowWindow(hwnd, nCmdShow);
    }

    while(GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    /* kill threads */
    postmessage_audio(AUDIO_KILL, 0, 0, NULL);
    postmessage_video(VIDEO_KILL, 0, 0, NULL);
    postmessage_utoxav(UTOXAV_KILL, 0, 0, NULL);
    postmessage_toxcore(TOX_KILL, 0, 0, NULL);

    /* cleanup */

    /* delete tray icon */
    Shell_NotifyIcon(NIM_DELETE, &nid);

    /* wait for threads to exit */
    while(tox_thread_init) {
        yieldcpu(10);
    }

    RECT wndrect = {0};
    GetWindowRect(hwnd, &wndrect);
    UTOX_SAVE d = {
        .window_x = wndrect.left < 0 ? 0 : wndrect.left,
        .window_y = wndrect.top < 0 ? 0 : wndrect.top,
        .window_width = (wndrect.right - wndrect.left),
        .window_height = (wndrect.bottom - wndrect.top),
    };
    config_save(&d);

    printf("uTox Clean Exit    ::\n");

    return 0;
}

/** Handles all callback requests from winmain();
 *
 * handles the window functions internally, and ships off the tox calls to tox
 */
LRESULT CALLBACK WindowProc(HWND hwn, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static int mx, my;

    if(hwnd && hwn != hwnd) {
        if(msg == WM_DESTROY) {
            if(hwn == video_hwnd[0]) {
                if(video_preview) {
                    video_preview = 0;
                    postmessage_video(VIDEO_PREVIEW_STOP, 0, 0, NULL);
                }

                return 0;
            }

            int i;
            for(i = 0; i != countof(friend); i++) {
                if(video_hwnd[i + 1] == hwn) {
                    FRIEND *f = &friend[i];
                    postmessage_toxcore(TOX_CALL_DISCONNECT, f->number, 0, NULL);
                    break;
                }
            }
            if(i == countof(friend)) {
                debug("this should not happen\n");
            }
        }

        return DefWindowProcW(hwn, msg, wParam, lParam);
    }

    switch(msg) {
    case WM_QUIT:
    case WM_CLOSE:
    case WM_DESTROY: {
        debug("quit msg\n");
        if(close_to_tray){
            debug("Closing to tray.\n");
            togglehide(0);
            return 1;
        } else {
            PostQuitMessage(0);
            return 0;
        }
    }

    case WM_GETMINMAXINFO: {
        POINT min = {UTOX_SCALE(320), UTOX_SCALE(160)};
        ((MINMAXINFO*)lParam)->ptMinTrackSize = min;

        break;
    }

    case WM_CREATE: {
        main_hdc = GetDC(hwn);
        hdc = CreateCompatibleDC(main_hdc);
        hdcMem = CreateCompatibleDC(hdc);

        return 0;
    }

    case WM_SIZE: {
        switch(wParam) {
        case SIZE_MAXIMIZED: {
            utox_window_maximized = 1;
            break;
        }

        case SIZE_RESTORED: {
            utox_window_maximized = 0;
            break;
        }
        }

        int w, h;

        w = GET_X_LPARAM(lParam);
        h = GET_Y_LPARAM(lParam);

        if(w != 0) {
            RECT r;
            GetClientRect(hwn, &r);
            w = r.right;
            h = r.bottom;

            utox_window_width = w;
            utox_window_height = h;

            ui_set_scale(dropdown_dpi.selected + 6);
            ui_size(w, h);

            if(hdc_bm) {
                DeleteObject(hdc_bm);
            }

            hdc_bm = CreateCompatibleBitmap(main_hdc, utox_window_width, utox_window_height);
            SelectObject(hdc, hdc_bm);
            redraw();
        }
        break;
    }

    case WM_SETFOCUS: {
        if(flashing) {
            FlashWindow(hwnd, 0);
            flashing = 0;

            NOTIFYICONDATAW nid = {
                    .uFlags = NIF_ICON,
                    .hWnd = hwnd,
                    .hIcon = my_icon,
                    .cbSize = sizeof(nid),
            };

            Shell_NotifyIconW(NIM_MODIFY, &nid);
        }

        havefocus = 1;
        break;
    }

    case WM_KILLFOCUS: {
        havefocus = 0;
        break;
    }

    case WM_ERASEBKGND: {
        return 1;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;

        BeginPaint(hwn, &ps);

        RECT r = ps.rcPaint;
        BitBlt(main_hdc, r.left, r.top, r.right - r.left, r.bottom - r.top, hdc, r.left, r.top, SRCCOPY);

        EndPaint(hwn, &ps);
        return 0;
    }

    case WM_SYSKEYDOWN: // called instead of WM_KEYDOWN when ALT is down or F10 is pressed
    case WM_KEYDOWN: {
        _Bool control = ((GetKeyState(VK_CONTROL) & 0x80) != 0);
        _Bool shift = ((GetKeyState(VK_SHIFT) & 0x80) != 0);
        _Bool alt = ((GetKeyState(VK_MENU) & 0x80) != 0);

        if (wParam >= VK_NUMPAD0 && wParam <= VK_NUMPAD9) {
            // normalize keypad and non-keypad numbers
            wParam = wParam - VK_NUMPAD0 + '0';
        }

        if(control && wParam == 'C') {
            copy(1);
            return 0;
        }

        if(control) {
            if ((wParam == VK_TAB && shift) || wParam == VK_PRIOR) {
                previous_tab();
                redraw();
                return 0;
            } else if (wParam == VK_TAB || wParam == VK_NEXT) {
                next_tab();
                redraw();
                return 0;
            }
        }

        if (control || alt) {
            if (wParam >= '1' && wParam <= '9') {
                list_selectchat(wParam - '1');
                redraw();
                return 0;
            } else if (wParam == '0') {
                list_selectchat(9);
                redraw();
                return 0;
            }
        }

        if(edit_active()) {
            if (control) {
                switch(wParam) {
                case 'V':
                    paste();
                    return 0;
                case 'X':
                    copy(0);
                    edit_char(KEY_DEL, 1, 0);
                    return 0;
                }
            }

            if(control || ((wParam < 'A' || wParam > 'Z') && wParam != VK_RETURN && wParam != VK_BACK)) {
                edit_char(wParam, 1, (control << 2) | shift);
            }
        } else {
            messages_char(wParam);
            break;
        }

        break;
    }

    case WM_CHAR: {
        if(edit_active()) {
            if(wParam == KEY_RETURN && (GetKeyState(VK_SHIFT) & 0x80)) {
                wParam = '\n';
            }
            if (wParam != KEY_TAB) {
                edit_char(wParam, 0, 0);
            }
            return 0;
        }

        return 0;
    }

    case WM_MOUSEWHEEL: {
        /* Important  Do not use the LOWORD or HIWORD macros to extract the x- and y- coordinates of the cursor position
         * because these macros return incorrect results on systems with multiple monitors. Systems with multiple
         * monitors can have negative x- and y- coordinates, and LOWORD and HIWORD treat the coordinates as unsigned
         * quantities.
         *
         * FIXME: if there is a way to determine whether deltas are precise on windows, do it
         */
        panel_mwheel(&panel_root, 0, 0, utox_window_width, utox_window_height, (double)((int16_t)HIWORD(wParam)) / (double)(WHEEL_DELTA), 1);
        return 0;
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
        panel_mmove(&panel_root, 0, 0, utox_window_width, utox_window_height, x, y, dx, dy);

        SetCursor(cursors[cursor]);

        if(!mouse_tracked) {
            TrackMouseEvent(&tme);
            mouse_tracked = 1;
        }

        return 0;
    }

    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK: {
        int x, y;

        x = GET_X_LPARAM(lParam);
        y = GET_Y_LPARAM(lParam);

        if(x != mx || y != my) {
            panel_mmove(&panel_root, 0, 0, utox_window_width, utox_window_height, x, y, x - mx, y - my);
            mx = x;
            my = y;
        }

        //double redraw>
        panel_mdown(&panel_root);
        if(msg == WM_LBUTTONDBLCLK) {
            panel_dclick(&panel_root, 0);
        }

        SetCapture(hwn);
        mdown = 1;
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
            mdown = 0;
        }

        break;
    }

    case WM_MOUSELEAVE: {
        ui_mouseleave();
        mouse_tracked = 0;
        break;
    }


    case WM_COMMAND: {
        int menu = LOWORD(wParam);//, msg = HIWORD(wParam);

        switch(menu) {
        case TRAY_SHOWHIDE: {
            togglehide(0);
            break;
        }

        case TRAY_EXIT: {
            PostQuitMessage(0);
            break;
        }

#define setstatus(x) if(self.status != x) { \
            postmessage_toxcore(TOX_SELF_SET_STATE, x, 0, NULL); self.status = x; redraw(); }

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

        switch(message) {
        case WM_MOUSEMOVE: {
            break;
        }

        case WM_LBUTTONDOWN:{
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
        return 0;
    }

    case WM_COPYDATA: {
        togglehide(1);
        SetForegroundWindow(hwn);
        COPYDATASTRUCT *data = (void*)lParam;
        if (data->lpData){
            parsecmd(data->lpData, data->cbData);
        }
        return 0;
    }

    case WM_TOX ... WM_TOX + 128: {
        tox_message(msg - WM_TOX, wParam >> 16, wParam, (void*)lParam);
        return 0;
    }
    }

    return DefWindowProcW(hwn, msg, wParam, lParam);
}

void video_frame(uint32_t id, uint8_t *img_data, uint16_t width, uint16_t height, _Bool resize)
{
    if(!video_hwnd[id]) {
        debug("frame for null window\n");
        return;
    }

    if(resize) {
        RECT r = {
            .left = 0,
            .top = 0,
            .right = width,
            .bottom = height
        };
        AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, 0);

        int w, h;
        w = r.right - r.left;
        h = r.bottom - r.top;
        if(w > GetSystemMetrics(SM_CXSCREEN)) {
            w = GetSystemMetrics(SM_CXSCREEN);
        }

        if(h > GetSystemMetrics(SM_CYSCREEN)) {
            h = GetSystemMetrics(SM_CYSCREEN);
        }

        SetWindowPos(video_hwnd[id], 0, 0, 0, w, h, SWP_NOZORDER | SWP_NOMOVE);
    }

    BITMAPINFO bmi = {
        .bmiHeader = {
            .biSize = sizeof(BITMAPINFOHEADER),
            .biWidth = width,
            .biHeight = -height,
            .biPlanes = 1,
            .biBitCount = 32,
            .biCompression = BI_RGB,
        }
    };


    RECT r = {0};
    GetClientRect(video_hwnd[id], &r);

    HDC dc = GetDC(video_hwnd[id]);

    if(width == r.right && height == r.bottom) {
        SetDIBitsToDevice(dc, 0, 0, width, height, 0, 0, 0, height, img_data, &bmi, DIB_RGB_COLORS);
    } else {
        StretchDIBits(dc, 0, 0, r.right, r.bottom, 0, 0, width, height, img_data, &bmi, DIB_RGB_COLORS, SRCCOPY);
    }
}

void video_begin(uint32_t id, char_t *name, STRING_IDX name_length, uint16_t width, uint16_t height) {
    if(video_hwnd[id]) {
        debug("Win Video:\tvideo_hwnd exists\n");
        return;
    }

    HWND *h = &video_hwnd[id];
    wchar_t out[name_length + 1];
    int len = utf8tonative(name, out, name_length);
    out[len] = 0;

    RECT r = {
        .left = 0,
        .top = 0,
        .right = width,
        .bottom = height
    };
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, 0);

    width = r.right - r.left;
    height = r.bottom - r.top;

    if(width > GetSystemMetrics(SM_CXSCREEN)) {
        width = GetSystemMetrics(SM_CXSCREEN);
    }

    if(height > GetSystemMetrics(SM_CYSCREEN)) {
        height = GetSystemMetrics(SM_CYSCREEN);
    }

    *h = CreateWindowExW(0, L"uTox", out, WS_OVERLAPPEDWINDOW, 0, 0, width, height, NULL, NULL, hinstance, NULL);

    ShowWindow(*h, SW_SHOW);
}

void video_end(uint32_t id)
{
    if(!video_hwnd[id]) {
        return;
    }

    DestroyWindow(video_hwnd[id]);
    video_hwnd[id] = NULL;
}

volatile _Bool newframe = 0;
uint8_t *frame_data;

HRESULT STDMETHODCALLTYPE test_SampleCB(ISampleGrabberCB *lpMyObj, double SampleTime, IMediaSample *pSample)
{
    // you can call functions like:
    //REFERENCE_TIME   tStart, tStop;
    void *sampleBuffer;
    int length;

    pSample->lpVtbl->GetPointer(pSample, (BYTE**)&sampleBuffer);
    length = pSample->lpVtbl->GetActualDataLength(pSample);

    /*pSample->GetTime(&tStart, &tStop);
    */
    if(length == (uint32_t)video_width * video_height * 3)
    {
        uint8_t *p = frame_data + video_width * video_height * 3;
        int y;
        for(y = 0; y != video_height; y++) {
            p -= video_width * 3;
            memcpy(p, sampleBuffer, video_width * 3);
            sampleBuffer += video_width * 3;
        }

        newframe = 1;
    }

    //debug("frame %u\n", length);
    return S_OK;
}

IGraphBuilder *pGraph;
IBaseFilter *pGrabberF;
IMediaControl *pControl;
ISampleGrabber *pGrabber;

STDMETHODIMP test_QueryInterface(ISampleGrabberCB *lpMyObj, REFIID riid, LPVOID FAR * lppvObj)
{
    return 0;
}

STDMETHODIMP_(ULONG) test_AddRef(ISampleGrabberCB *lpMyObj)
{
    return 1;
}

STDMETHODIMP_(ULONG) test_Release(ISampleGrabberCB *lpMyObj)
{
    free(lpMyObj->lpVtbl);
    free(lpMyObj);
    return 0;
}

#define SafeRelease(x) if(*(x)) {(*(x))->lpVtbl->Release(*(x));}

HRESULT IsPinConnected(IPin *pPin, BOOL *pResult)
{
    IPin *pTmp = NULL;
    HRESULT hr = pPin->lpVtbl->ConnectedTo(pPin, &pTmp);
    if (SUCCEEDED(hr))
    {
        *pResult = TRUE;
    }
    else if (hr == VFW_E_NOT_CONNECTED)
    {
        // The pin is not connected. This is not an error for our purposes.
        *pResult = FALSE;
        hr = S_OK;
    }

    SafeRelease(&pTmp);
    return hr;
}

HRESULT IsPinDirection(IPin *pPin, PIN_DIRECTION dir, BOOL *pResult)
{
    PIN_DIRECTION pinDir;
    HRESULT hr = pPin->lpVtbl->QueryDirection(pPin, &pinDir);
    if (SUCCEEDED(hr))
    {
        *pResult = (pinDir == dir);
    }
    return hr;
}

HRESULT MatchPin(IPin *pPin, PIN_DIRECTION direction, BOOL bShouldBeConnected, BOOL *pResult)
{
    //assert(pResult != NULL);

    BOOL bMatch = FALSE;
    BOOL bIsConnected = FALSE;

    HRESULT hr = IsPinConnected(pPin, &bIsConnected);
    if (SUCCEEDED(hr))
    {
        if (bIsConnected == bShouldBeConnected)
        {
            hr = IsPinDirection(pPin, direction, &bMatch);
        }
    }

    if (SUCCEEDED(hr))
    {
        *pResult = bMatch;
    }
    return hr;
}

HRESULT FindUnconnectedPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin)
{
    IEnumPins *pEnum = NULL;
    IPin *pPin = NULL;
    BOOL bFound = FALSE;

    HRESULT hr = pFilter->lpVtbl->EnumPins(pFilter, &pEnum);
    if (FAILED(hr))
    {
        goto done;
    }

    while (S_OK == pEnum->lpVtbl->Next(pEnum, 1, &pPin, NULL))
    {
        hr = MatchPin(pPin, PinDir, FALSE, &bFound);
        if (FAILED(hr))
        {
            goto done;
        }
        if (bFound)
        {
            *ppPin = pPin;
            (*ppPin)->lpVtbl->AddRef(*ppPin);
            break;
        }
        SafeRelease(&pPin);
    }

    if (!bFound)
    {
        hr = VFW_E_NOT_FOUND;
    }

done:
    SafeRelease(&pPin);
    SafeRelease(&pEnum);
    return hr;
}

IPin* ConnectFilters2(IGraphBuilder *_pGraph, IPin *pOut, IBaseFilter *pDest)
{
    IPin *pIn = NULL;

    // Find an input pin on the downstream filter.
    HRESULT hr = FindUnconnectedPin(pDest, PINDIR_INPUT, &pIn);
    if (SUCCEEDED(hr))
    {
        // Try to connect them.
        hr = pGraph->lpVtbl->Connect(_pGraph, pOut, pIn);
        pIn->lpVtbl->Release(pIn);
    }
    return SUCCEEDED(hr) ? pIn : NULL;
}

HRESULT ConnectFilters(IGraphBuilder *_pGraph, IBaseFilter *pSrc, IBaseFilter *pDest)
{
    IPin *pOut = NULL;

    // Find an output pin on the first filter.
    HRESULT hr = FindUnconnectedPin(pSrc, PINDIR_OUTPUT, &pOut);
    if (SUCCEEDED(hr))
    {
        if(!ConnectFilters2(_pGraph, pOut, pDest)) {
            hr = 1;
        }
        pOut->lpVtbl->Release(pOut);
    }
    return hr;
}

//!TODO: free resources correctly (on failure, etc)
IBaseFilter *pNullF = NULL;

void* video_detect(void)
{
    // Indicate that we support desktop capturing.
    postmessage(VIDEO_IN_DEVICE, STR_VIDEO_IN_DESKTOP, 0, (void*)1);

    max_video_width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    max_video_height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    HRESULT hr;
    CoInitialize(NULL);

    IMediaEventEx *pEvent;

    hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, &IID_IGraphBuilder, (void**)&pGraph);
    if(FAILED(hr)) {
        return 0;
    }

    hr = pGraph->lpVtbl->QueryInterface(pGraph, &IID_IMediaControl, (void**)&pControl);
    if(FAILED(hr)) {
        return 0;
    }

    hr = pGraph->lpVtbl->QueryInterface(pGraph, &IID_IMediaEventEx, (void**)&pEvent);
    if(FAILED(hr)) {
        return 0;
    }

    hr = CoCreateInstance(&CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (void**)&pGrabberF);
    if(FAILED(hr)) {
        return 0;
    }

    hr = pGraph->lpVtbl->AddFilter(pGraph, pGrabberF, L"Sample Grabber");
    if(FAILED(hr)) {
        return 0;
    }

    hr = pGrabberF->lpVtbl->QueryInterface(pGrabberF, &IID_ISampleGrabber, (void**)&pGrabber);
    if(FAILED(hr)) {
        return 0;
    }

    AM_MEDIA_TYPE mt = {
        .majortype = MEDIATYPE_Video,
        .subtype = MEDIASUBTYPE_RGB24,
    };

    hr = pGrabber->lpVtbl->SetMediaType(pGrabber, &mt);
    if(FAILED(hr)) {
        return 0;
    }

    ICreateDevEnum *pSysDevEnum = NULL;
    hr = CoCreateInstance(&CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, &IID_ICreateDevEnum, (void**)&pSysDevEnum);
    if(FAILED(hr)) {
        debug("CoCreateInstance failed()\n");
        return 0;
    }
    // Obtain a class enumerator for the video compressor category.
    IEnumMoniker *pEnumCat = NULL;
    hr = pSysDevEnum->lpVtbl->CreateClassEnumerator(pSysDevEnum, &CLSID_VideoInputDeviceCategory, &pEnumCat, 0);
    if(hr != S_OK) {

        pSysDevEnum->lpVtbl->Release(pSysDevEnum);
        debug("CreateClassEnumerator failed()\n");
        return 0;
    }

    IBaseFilter *pFilter = NULL;
    IMoniker *pMoniker = NULL;

    ULONG cFetched;
    while(pEnumCat->lpVtbl->Next(pEnumCat, 1, &pMoniker, &cFetched) == S_OK) {
        IPropertyBag *pPropBag;
        hr = pMoniker->lpVtbl->BindToStorage(pMoniker, 0, 0, &IID_IPropertyBag, (void **)&pPropBag);
        if(SUCCEEDED(hr)) {
            // To retrieve the filter's friendly name, do the following:
            VARIANT varName;
            VariantInit(&varName);
            hr = pPropBag->lpVtbl->Read(pPropBag, L"FriendlyName", &varName, 0);
            if (SUCCEEDED(hr)) {
                if(varName.vt == VT_BSTR) {
                    debug("friendly name: %ls\n", varName.bstrVal);
                } else {
                    debug("unfriendly name\n");
                }

                // Display the name in your UI somehow.
            }
            VariantClear(&varName);

            // To create an instance of the filter, do the following:
            IBaseFilter *temp;
            hr = pMoniker->lpVtbl->BindToObject(pMoniker, NULL, NULL, &IID_IBaseFilter, (void**)&temp);
            if(SUCCEEDED(hr)) {
                if(!pFilter) {
                    pFilter = temp;
                }
                int len = wcslen(varName.bstrVal);
                void *data = malloc(sizeof(void*) + len * 3 / 2);
                WideCharToMultiByte(CP_UTF8, 0, varName.bstrVal, -1, data + sizeof(void*), len * 3 / 2, NULL, 0);
                memcpy(data, &temp, sizeof(pFilter));
                postmessage(VIDEO_IN_DEVICE, UI_STRING_ID_INVALID, 1, data);
            }

            // Now add the filter to the graph.
            //Remember to release pFilter later.
            pPropBag->lpVtbl->Release(pPropBag);
        }
        pMoniker->lpVtbl->Release(pMoniker);
    }
    pEnumCat->lpVtbl->Release(pEnumCat);
    pSysDevEnum->lpVtbl->Release(pSysDevEnum);

    hr = CoCreateInstance(&CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (void**)&pNullF);
    if(FAILED(hr)) {
        debug("CoCreateInstance failed\n");
        return 0;
    }

    hr = pGraph->lpVtbl->AddFilter(pGraph, pNullF, L"Null Filter");
    if(FAILED(hr)) {
        debug("AddFilter failed\n");
        return 0;
    }

    hr = ConnectFilters(pGraph, pGrabberF, pNullF);
    if(FAILED(hr)) {
        debug("ConnectFilters (2) failed\n");
        return 0;
    }

    ISampleGrabberCB *test;
    test = malloc(sizeof(ISampleGrabberCB));
    test->lpVtbl = malloc(sizeof(*(test->lpVtbl)));
    // no idea what im doing here
    test->lpVtbl->QueryInterface = test_QueryInterface;
    test->lpVtbl->AddRef = test_AddRef;
    test->lpVtbl->Release = test_Release;
    test->lpVtbl->SampleCB = test_SampleCB;
    test->lpVtbl->BufferCB = 0;

    hr = pGrabber->lpVtbl->SetCallback(pGrabber, test, 0);
    if(FAILED(hr)) {
        return 0;
    }

    return pFilter;
}

IPin *pPin = NULL, *pIPin;
HWND desktopwnd;
HDC desktopdc, capturedc;
HBITMAP capturebitmap;
_Bool capturedesktop;
void *dibits;
static uint16_t video_x, video_y;

_Bool video_init(void *handle)
{
    if((size_t)handle == 1) {
        video_x = volatile(grabx);
        video_y = volatile(graby);
        video_width = volatile(grabpx);
        video_height = volatile(grabpy);

        if(video_width & 1) {
            if(video_x & 1) {
                video_x--;
            }
            video_width++;
        }

        if(video_width & 2) {
            video_width -= 2;
        }

        if(video_height & 1) {
            if(video_y & 1) {
                video_y--;
            }
            video_height++;
        }

        debug("size: %u %u\n", video_width, video_height);

        desktopwnd = GetDesktopWindow();
        if(!desktopwnd) {
            debug("GetDesktopWindow() failed\n");
            return 0;
        }

        if(!(desktopdc = GetDC(desktopwnd))) {
            debug("GetDC(desktopwnd) failed\n");
           return 0;
        }

        if(!(capturedc = CreateCompatibleDC(desktopdc))) {
            debug("CreateCompatibleDC(desktopdc) failed\n");
            return 0;
        }

        if(!(capturebitmap = CreateCompatibleBitmap(desktopdc, video_width, video_height))) {
            debug("CreateCompatibleBitmap(desktopdc) failed\n");
            return 0;
        }

        SelectObject(capturedc, capturebitmap);
        dibits = malloc(video_width * video_height * 3);
        capturedesktop = 1;
        return 1;
    }

    HRESULT hr;
    IBaseFilter *pFilter = handle;

    hr = pGraph->lpVtbl->AddFilter(pGraph, pFilter, L"Video Capture");
    if(FAILED(hr)) {
        debug("AddFilter failed\n");
        return 0;
    }

    IEnumPins *pEnum = NULL;


    /* build filter graph */
    hr = pFilter->lpVtbl->EnumPins(pFilter, &pEnum);
    if(FAILED(hr)) {
        debug("EnumPins failed\n");
        return 0;
    }

    while(S_OK == pEnum->lpVtbl->Next(pEnum, 1, &pPin, NULL)) {
        pIPin = ConnectFilters2(pGraph, pPin, pGrabberF);
        SafeRelease(&pPin);
        if(pIPin) {
            break;
        }
    }

    if(FAILED(hr)) {
        debug("failed to connect a filter\n");
        return 0;
    }

    IAMStreamConfig *pConfig = NULL;
    AM_MEDIA_TYPE *pmt = NULL;

    hr = pPin->lpVtbl->QueryInterface(pPin, &IID_IAMStreamConfig, (void**)&pConfig);
    if(FAILED(hr)) {
        debug("QueryInterface failed\n");
        return 0;
    }

    hr = pConfig->lpVtbl->GetFormat(pConfig, &pmt);
    if(FAILED(hr)) {
        debug("GetFormat failed\n");
        return 0;
    }

    BITMAPINFOHEADER *bmiHeader;
    if(IsEqualGUID(&pmt->formattype, &FORMAT_VideoInfo)) {
        VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER*)pmt->pbFormat;
        bmiHeader = &(pvi->bmiHeader);
        video_width = bmiHeader->biWidth;
        video_height = bmiHeader->biHeight;
    } else {
        debug("got bad format\n");
        video_width = 0;
        video_height = 0;
    }

    frame_data = malloc((size_t)video_width * video_height * 3);

    debug("width height %u %u\n", video_width, video_height);

    return 1;
}

void video_close(void *handle)
{
    if((size_t)handle == 1) {
        ReleaseDC(desktopwnd, desktopdc);
        DeleteDC(capturedc);
        DeleteObject(capturebitmap);
        free(dibits);
        capturedesktop = 0;
        return;
    }

    HRESULT hr;
    IBaseFilter *pFilter = handle;

    hr = pGraph->lpVtbl->RemoveFilter(pGraph, pFilter);
    if(FAILED(hr)) {
        return;
    }

    hr = pGraph->lpVtbl->Disconnect(pGraph, pPin);
    if(FAILED(hr)) {
        return;
    }

    hr = pGraph->lpVtbl->Disconnect(pGraph, pIPin);
    if(FAILED(hr)) {
        return;
    }

    debug("closed webcam\n");
}

int video_getframe(uint8_t *y, uint8_t *u, uint8_t *v, uint16_t width, uint16_t height)
{
    if (width != video_width || height != video_height) {
        debug("width/height mismatch %u %u != %u %u\n", width, height, video_width, video_height);
        return 0;
    }

    if(capturedesktop) {
        static uint64_t lasttime;
        uint64_t t = get_time();
        if(t - lasttime >= (uint64_t)1000 * 1000 * 1000 / 24) {
            BITMAPINFO info = {
                .bmiHeader = {
                    .biSize = sizeof(BITMAPINFOHEADER),
                    .biWidth = video_width,
                    .biHeight = -(int)video_height,
                    .biPlanes = 1,
                    .biBitCount = 24,
                    .biCompression = BI_RGB,
                }
            };

            BitBlt(capturedc, 0, 0, video_width, video_height, desktopdc, video_x, video_y, SRCCOPY | CAPTUREBLT);
            GetDIBits(capturedc, capturebitmap, 0, video_height, dibits, &info, DIB_RGB_COLORS);
            bgrtoyuv420(y, u, v, dibits, video_width, video_height);
            lasttime = t;
            return 1;
        }
        return 0;
    }

    if(newframe) {
        newframe = 0;
        bgrtoyuv420(y, u, v, frame_data, video_width, video_height);
        return 1;
    }
    return 0;
}

_Bool video_startread(void)
{
    if(capturedesktop) {
        return 1;
    }
    debug("start webcam\n");
    HRESULT hr;
    hr = pControl->lpVtbl->Run(pControl);
    if(FAILED(hr)) {
        debug("Run failed\n");
        return 0;
    }
    return 1;
}

_Bool video_endread(void)
{
    if(capturedesktop) {
        return 1;
    }
    debug("stop webcam\n");
    HRESULT hr;
    hr = pControl->lpVtbl->StopWhenReady(pControl);
    if(FAILED(hr)) {
        debug("Stop failed\n");
        return 0;
    }
    return 1;
}
