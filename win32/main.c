#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x500

#ifndef WINVER
#define WINVER 0x410
#endif

#define STRSAFE_NO_DEPRECATE

#include <windows.h>
#include <windowsx.h>

#ifdef __CRT__NO_INLINE
#undef __CRT__NO_INLINE
#define DID_UNDEFINE__CRT__NO_INLINE
#include <dshow.h>
#ifdef DID_UNDEFINE__CRT__NO_INLINE
#define __CRT__NO_INLINE
#endif
#endif

#include <strmif.h>
#include <amvideo.h>
#include <control.h>
#include <uuids.h>
#include <vfwmsgs.h>

#include <qedit.h>
extern const CLSID CLSID_SampleGrabber;
extern const CLSID CLSID_NullRenderer;

#include <audioclient.h>
#include <mmdeviceapi.h>

#include "audio.c"

#include <process.h>

#undef CLEARTYPE_QUALITY
#define CLEARTYPE_QUALITY 5

#define WM_NOTIFYICON   (WM_APP + 0)
#define WM_TOX          (WM_APP + 1)

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

_Bool draw = 0;

float scale = 1.0;
_Bool connected = 0;
_Bool havefocus;

enum {
    MENU_TEXTINPUT = 101,
    MENU_MESSAGES = 102,
};

//HBITMAP bitmap[32];
void *bitmap[32];
HFONT font[32];
HCURSOR cursors[8];

HWND hwnd, capturewnd;
HINSTANCE hinstance;
HDC main_hdc, hdc, hdcMem;
HBRUSH hdc_brush;
HBITMAP hdc_bm;
HWND video_hwnd[256];


static char save_path[280];

static _Bool flashing, desktopgrab_video;

static TRACKMOUSEEVENT tme = {sizeof(TRACKMOUSEEVENT), TME_LEAVE, 0, 0};
static _Bool mouse_tracked = 0;

static _Bool hidden;

//WM_COMMAND
enum
{
    TRAY_SHOWHIDE,
    TRAY_EXIT,
    TRAY_STATUS_AVAILABLE,
    TRAY_STATUS_AWAY,
    TRAY_STATUS_BUSY,
};

static int utf8tonative(char_t *str, wchar_t *out, int length)
{
    return MultiByteToWideChar(CP_UTF8, 0, (char*)str, length, out, length);
}

void postmessage(uint32_t msg, uint16_t param1, uint16_t param2, void *data)
{
    PostMessage(hwnd, WM_TOX + (msg), ((param1) << 16) | (param2), (LPARAM)data);
}

void drawalpha(int bm, int x, int y, int width, int height, uint32_t color)
{
    if(!bitmap[bm]) {
        return;
    }

    BLENDFUNCTION ftn = {
        .BlendOp = AC_SRC_OVER,
        .BlendFlags = 0,
        .SourceConstantAlpha = 0xFF,
        .AlphaFormat = AC_SRC_ALPHA
    };

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

    uint8_t *p = bitmap[bm], *end = p + width * height;


    uint32_t *np;
    HBITMAP temp = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, (void**)&np, NULL, 0);
    SelectObject(hdcMem, temp);

    while(p != end) {
        uint8_t v = *p++;
        *np++ = (((color & 0xFF) * v / 255) << 16) | ((((color >> 8) & 0xFF) * v / 255) << 8) | ((((color >> 16) & 0xFF) * v / 255) << 0) | (v << 24);
    }

    AlphaBlend(hdc, x, y, width, height, hdcMem, 0, 0, width, height, ftn);

    DeleteObject(temp);
}

void drawimage(void *data, int x, int y, int width, int height, int maxwidth, _Bool zoom, double position)
{
    HBITMAP bm = data;
    SelectObject(hdcMem, bm);
    if(!zoom && width > maxwidth) {
        StretchBlt(hdc, x, y, maxwidth, height * maxwidth / width, hdcMem, 0, 0, width, height, SRCCOPY);
    } else {
        if(width > maxwidth) {
            BitBlt(hdc, x, y, maxwidth, height, hdcMem, (int)((double)(width - maxwidth) * position), 0, SRCCOPY);
        } else {
            BitBlt(hdc, x, y, width, height, hdcMem, 0, 0, SRCCOPY);
        }
    }

}

void drawtext(int x, int y, char_t *str, uint16_t length)
{
    wchar_t out[length];
    length = utf8tonative(str, out, length);

    TextOutW(hdc, x, y, out, length);
}

int drawtext_getwidth(int x, int y, char_t *str, uint16_t length)
{
    wchar_t out[length];
    length = utf8tonative(str, out, length);

    SIZE size;
    TextOutW(hdc, x, y, out, length);
    GetTextExtentPoint32W(hdc, out, length, &size);
    return size.cx;
}

void drawtextwidth(int x, int width, int y, uint8_t *str, uint16_t length)
{
    wchar_t out[length];
    length = utf8tonative(str, out, length);

    RECT r = {x, y, x + width, y + 256};
    DrawTextW(hdc, out, length, &r, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
}

void drawtextwidth_right(int x, int width, int y, char_t *str, uint16_t length)
{
    wchar_t out[length];
    length = utf8tonative(str, out, length);

    RECT r = {x, y, x + width, y + 256};
    DrawTextW(hdc, out, length, &r, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX | DT_RIGHT);
}

void drawtextrange(int x, int x2, int y, uint8_t *str, uint16_t length)
{
    wchar_t out[length];
    length = utf8tonative(str, out, length);

    RECT r = {x, y, x2, y + 256};
    DrawTextW(hdc, out, length, &r, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
}

void drawtextrangecut(int x, int x2, int y, char_t *str, uint16_t length)
{
    wchar_t out[length];
    length = utf8tonative(str, out, length);

    RECT r = {x, y, x2, y + 256};
    DrawTextW(hdc, out, length, &r, DT_SINGLELINE | DT_NOPREFIX);
}

int textwidth(char_t *str, uint16_t length)
{
    wchar_t out[length];
    length = utf8tonative(str, out, length);

    SIZE size;
    GetTextExtentPoint32W(hdc, out, length, &size);
    return size.cx;
}

int textfit(char_t *str, uint16_t length, int width)
{
    wchar_t out[length];
    length = utf8tonative(str, out, length);

    int fit;
    SIZE size;
    GetTextExtentExPointW(hdc, out, length, width, &fit, NULL, &size);

    return WideCharToMultiByte(CP_UTF8, 0, out, fit, (char*)str, 65536, NULL, 0);
}

int textfit_near(char_t *str, uint16_t length, int width)
{
    /*todo: near*/
    wchar_t out[length];
    length = utf8tonative(str, out, length);

    int fit;
    SIZE size;
    GetTextExtentExPointW(hdc, out, length, width, &fit, NULL, &size);

    return WideCharToMultiByte(CP_UTF8, 0, out, fit, (char*)str, 65536, NULL, 0);
}

void framerect(int x, int y, int right, int bottom, uint32_t color)
{
    RECT r = {x, y, right, bottom};
    SetDCBrushColor(hdc, color);
    FrameRect(hdc, &r, hdc_brush);
}

void drawrect(int x, int y, int right, int bottom, uint32_t color)
{
    RECT r = {x, y, right, bottom};
    SetDCBrushColor(hdc, color);
    FillRect(hdc, &r, hdc_brush);
}

void drawrectw(int x, int y, int width, int height, uint32_t color)
{
    RECT r = {x, y, x + width, y + height};
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

void address_to_clipboard(void)
{
#define size sizeof(self.id)

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (size + 1) * 2);
    wchar_t *p = GlobalLock(hMem);
    utf8tonative(self.id, p, size);
    p[size] = 0;
    GlobalUnlock(hMem);
    OpenClipboard(hwnd);
    EmptyClipboard();
    SetClipboardData(CF_UNICODETEXT, hMem);
    CloseClipboard();

#undef size
}

void openurl(char_t *str)
{
    //!convert
    ShellExecute(NULL, "open", (char*)str, NULL, NULL, SW_SHOW);
}

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
        tox_postmessage(TOX_SENDFILES, (FRIEND*)sitem->data - friend, ofn.nFileOffset, filepath);
    } else {
        debug("GetOpenFileName() failed\n");
    }

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
        .nMaxFile = 256,
        .Flags = OFN_EXPLORER | OFN_NOCHANGEDIR,
    };

    if(GetSaveFileName(&ofn)) {
        tox_postmessage(TOX_ACCEPTFILE, fid, file->filenumber, path);
    } else {
        debug("GetSaveFileName() failed\n");
    }
}

void savefiledata(MSG_FILE *file)
{
    char *path = malloc(256);
    memcpy(path, file->name, file->name_length);
    path[file->name_length] = 0;

    OPENFILENAME ofn = {
        .lStructSize = sizeof(OPENFILENAME),
        .hwndOwner = hwnd,
        .lpstrFile = path,
        .nMaxFile = 256,
        .Flags = OFN_EXPLORER | OFN_NOCHANGEDIR,
    };

    if(GetSaveFileName(&ofn)) {
        FILE *fp = fopen(path, "wb");
        if(fp) {
            fwrite(file->path + ((file->flags & 1) ? 4 : 0), file->size, 1, fp);
            fclose(fp);

            free(file->path);
            file->path = (uint8_t*)strdup("inline.png");
            file->inline_png = 0;
        }
    } else {
        debug("GetSaveFileName() failed\n");
    }
}

void setselection(uint8_t *data, uint16_t length)
{
}

void togglehide(void)
{
    if(hidden) {
        ShowWindow(hwnd, SW_RESTORE);
        SetForegroundWindow(hwnd);
        redraw();
        hidden = 0;
    } else {
        ShowWindow(hwnd, SW_HIDE);
        hidden = 1;
    }
}

void ShowContextMenu(void)
{
    POINT pt;
    GetCursorPos(&pt);
    HMENU hMenu = CreatePopupMenu();
    if(hMenu) {
        InsertMenu(hMenu, -1, MF_BYPOSITION, TRAY_SHOWHIDE, hidden ? "Restore" : "Hide");

        InsertMenu(hMenu, -1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);

        InsertMenu(hMenu, -1, MF_BYPOSITION | ((self.status == TOX_USERSTATUS_NONE) ? MF_CHECKED : 0), TRAY_STATUS_AVAILABLE, "Available");
        InsertMenu(hMenu, -1, MF_BYPOSITION | ((self.status == TOX_USERSTATUS_AWAY) ? MF_CHECKED : 0), TRAY_STATUS_AWAY, "Away");
        InsertMenu(hMenu, -1, MF_BYPOSITION | ((self.status == TOX_USERSTATUS_BUSY) ? MF_CHECKED : 0), TRAY_STATUS_BUSY, "Busy");

        InsertMenu(hMenu, -1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);

        InsertMenu(hMenu, -1, MF_BYPOSITION, TRAY_EXIT, "Exit");

        // note:	must set window to the foreground or the
        //			menu won't disappear when it should
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
    if(len < 6)
    {
        return;
    }

    if(memcmp(cmd, "tox://", 6) != 0) {
        return;
    }

    cmd += 6;
    len -= 6;

    uint8_t *b = edit_addid.data, *a = cmd, *end = cmd + len;
    uint16_t *l = &edit_addid.length;
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
                a++;
                if(end - a >= 4 && memcmp(a, "pin=", 4) == 0)
                {

                    l = &edit_addid.length;
                    b = edit_addid.data + *l;
                    *b++ = ':';
                    *l = *l + 1;
                    a += 3;
                    break;
                }
                else if(end - a >= 8 && memcmp(a, "message=", 8) == 0)
                {
                    b = edit_addmsg.data;
                    l = &edit_addmsg.length;
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

static void sendbitmap(HDC mem, HBITMAP hbm, int width, int height)
{
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
    uint32_t offset = 0;
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

    uint32_t s = size;
    void *data = malloc(size + 4);
    memcpy(data, &s, 4);
    memcpy(data + 4, out, size);
    free(out);

    friend_sendimage(sitem->data, hbm, data, width, height);
}

void copy(int value)
{
    uint8_t data[32768];//!
    int len;

    if(edit_active()) {
        len = edit_copy(data, 32767);
        data[len] = 0;
    } else if(sitem->item == ITEM_FRIEND) {
        len = messages_selection(&messages_friend, data, 32768, value);
    } else if(sitem->item == ITEM_GROUP) {
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
        if(h && sitem->item == ITEM_FRIEND) {
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
        uint8_t data[65536];
        int len = WideCharToMultiByte(CP_UTF8, 0, d, -1, (char*)data, 65536, NULL, 0);
        if(edit_active()) {
            edit_paste(data, len, 0);
        }
    }

    GlobalUnlock(h);
    CloseClipboard();
}

void* png_to_image(void *data, uint16_t *w, uint16_t *h, uint32_t size)
{
    uint8_t *out;
    unsigned width, height;
    unsigned r = lodepng_decode32(&out, &width, &height, data, size);
    //free(data);

    if(r != 0 || !width || !height) {
        return NULL;
    }

    HBITMAP bm = CreateCompatibleBitmap(hdcMem, width, height);
    SelectObject(hdcMem, bm);

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

    uint8_t *p = out, *end = p + width * height * 4;
    do {
        uint8_t r = p[0];
        p[0] = p[2];
        p[2] = r;
        p += 4;
    } while(p != end);

    SetDIBitsToDevice(hdcMem, 0, 0, width, height, 0, 0, 0, height, out, &bmi, DIB_RGB_COLORS);

    *w = width;
    *h = height;
    free(out);

    return bm;
}

void* loadsavedata(uint32_t *len)
{
    int end = GetCurrentDirectory(sizeof(save_path) - 16, save_path);
    memcpy(save_path + end, "\\tox_save", 9);

    return file_raw(save_path, len);
}

void writesavedata(void *data, uint32_t len)
{
    FILE *file;
    file = fopen(save_path, "wb");
    if(file) {
        fwrite(data, len, 1, file);
        fclose(file);
        debug("Saved data\n");
    }
}

void notify(uint8_t *title, uint16_t title_length, uint8_t *msg, uint16_t msg_length)
{
    if(havefocus) {
        return;
    }

    FlashWindow(hwnd, 1);
    flashing = 1;

    NOTIFYICONDATAW nid = {
        .uFlags = NIF_INFO,
        .hWnd = hwnd,
        .uTimeout = 5000,
        .dwInfoFlags = 0,
        .cbSize = sizeof(nid),
    };

    utf8tonative(title, nid.szInfoTitle, title_length > sizeof(nid.szInfoTitle) / sizeof(*nid.szInfoTitle) - 1 ? sizeof(nid.szInfoTitle) / sizeof(*nid.szInfoTitle) - 1 : title_length);
    utf8tonative(msg, nid.szInfo, msg_length > sizeof(nid.szInfo) / sizeof(*nid.szInfo) - 1 ? sizeof(nid.szInfo) / sizeof(*nid.szInfo) - 1 : msg_length);

    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

void showkeyboard(_Bool show)
{

}

void redraw(void)
{
    panel_draw(&panel_main, 0, 0, width, height);
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

    capturewnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_LAYERED, L"uToxgrab", L"Tox", WS_POPUP, x, y, w, h, NULL, NULL, hinstance, NULL);
    if(!capturewnd) {
        debug("CreateWindowExW() failed\n");
        return;
    }

    HDC hdc = GetDC(capturewnd);
    BitBlt(hdc, 0, 0, w, h, hdc, 0, 0, BLACKNESS);

    SetLayeredWindowAttributes(capturewnd, 0xFFFFFF, 128, LWA_ALPHA | LWA_COLORKEY);


    //UpdateLayeredWindow(hwnd, NULL, NULL, NULL, NULL, NULL, 0xFFFFFF, ULW_ALPHA | ULW_COLORKEY);

    ShowWindow(capturewnd, SW_SHOW);
    SetForegroundWindow(capturewnd);

    desktopgrab_video = video;

    //SetCapture(hwnd);
    //grabbing = 1;

    //toxvideo_postmessage(VIDEO_SET, 0, 0, (void*)1);
}

LRESULT CALLBACK GrabProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    POINT p = {
        .x = GET_X_LPARAM(lParam),
        .y = GET_Y_LPARAM(lParam)
    };

    ClientToScreen(hwnd, &p);

    if(msg == WM_MOUSEMOVE) {

        if(grabbing) {
            HDC hdc = GetDC(hwnd);
            BitBlt(hdc, grabx, graby, grabpx - grabx, grabpy - graby, hdc, grabx, graby, BLACKNESS);
            grabpx = p.x;
            grabpy = p.y;
            BitBlt(hdc, grabx, graby, grabpx - grabx, grabpy - graby, hdc, grabx, graby, WHITENESS);
        }

        return 0;
    }

    if(msg == WM_LBUTTONDOWN) {
        grabx = grabpx = p.x;
        graby = grabpy = p.y;
        grabbing = 1;
        SetCapture(hwnd);
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
            toxvideo_postmessage(VIDEO_SET, 0, 0, (void*)1);
        } else {
            FRIEND *f = sitem->data;
            if(sitem->item == ITEM_FRIEND && f->online) {
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

        DestroyWindow(hwnd);
        return 0;
    }

    if(msg == WM_DESTROY) {
        grabbing = 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void setscale(void)
{
    int i;
    for(i = 0; i != countof(font); i++) {
        if(font[i]) {
            DeleteObject(font[i]);
        }
    }

    LOGFONT lf = {
        .lfWeight = FW_NORMAL,
        //.lfCharSet = ANSI_CHARSET,
        .lfOutPrecision = OUT_TT_PRECIS,
        .lfQuality = CLEARTYPE_QUALITY,
        .lfFaceName = "DejaVu Sans",
    };

    #define F(x) ((-x * SCALE - 1) / 2)
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

    svg_draw(1);
}

static UTOX_SAVE* loadconfig(void)
{
    UTOX_SAVE *r = file_text("utox_save");
    if(r) {
        if(r->version == 1) {
            /* validate values */
            if(r->scale > 4) {
                r->scale = 4;
            }

            if(r->window_x > GetSystemMetrics(SM_CXSCREEN) || r->window_width > GetSystemMetrics(SM_CXSCREEN)
               || r->window_y > GetSystemMetrics(SM_CYSCREEN) || r->window_height > GetSystemMetrics(SM_CYSCREEN)) {
                goto SET_DEFAULTS;
            }
            return r;
        } else {
            free(r);
            r = NULL;
        }
    }

    r = malloc(sizeof(UTOX_SAVE) + 1);
    r->version = 1;
    r->scale = DEFAULT_SCALE - 1;
    r->enableipv6 = 1;
    r->disableudp = 0;
    r->proxy_port = 0;
    r->proxyenable = 0;
    r->proxy_ip[0] = 0;
SET_DEFAULTS:
    r->window_x = (GetSystemMetrics(SM_CXSCREEN) - MAIN_WIDTH) / 2;
    r->window_y = (GetSystemMetrics(SM_CYSCREEN) - MAIN_HEIGHT) / 2;
    r->window_width = MAIN_WIDTH;
    r->window_height = MAIN_HEIGHT;
    return r;
}

#include "dnd.c"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR cmd, int nCmdShow)
{
    /* if opened with argument, check if uTox is already open and pass the argument to the existing process */
    CreateMutex(NULL, 0, "uTox");
    if(*cmd && GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND hwnd = FindWindow("uTox", NULL);
        SetForegroundWindow(hwnd);
        COPYDATASTRUCT data = {
            .cbData = strlen(cmd),
            .lpData = cmd
        };
        SendMessage(hwnd, WM_COPYDATA, (WPARAM)hInstance, (LPARAM)&data);
        return 0;
    }

    /* force the working directory if opened with a command */
    if(*cmd) {
        HMODULE hModule = GetModuleHandle(NULL);
        char path[MAX_PATH];
        int len = GetModuleFileName(hModule, path, MAX_PATH);
        path[len - 10] = 0;//!
        SetCurrentDirectory(path);
    }

    /* */
    MSG msg;
    int x, y;
    wchar_t classname[] = L"uTox", popupclassname[] = L"uToxgrab";

    HICON myicon = LoadIcon(hInstance, MAKEINTRESOURCE(101));
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
        .hIcon = myicon,
        .lpszClassName = classname,
    },

    wc2 = {
        .lpfnWndProc = GrabProc,
        .hInstance = hInstance,
        .hIcon = myicon,
        .lpszClassName = popupclassname,
    };

    NOTIFYICONDATA nid = {
        .uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP,
        .uCallbackMessage = WM_NOTIFYICON,
        .hIcon = myicon,
        .szTip = "Tox - tooltip",
        .cbSize = sizeof(nid),
    };


    OleInitialize(NULL);
    RegisterClassW(&wc);
    RegisterClassW(&wc2);

    UTOX_SAVE *save = loadconfig();

    dropdown_dpi.selected = dropdown_dpi.over = save->scale;
    dropdown_ipv6.selected = dropdown_ipv6.over = !save->enableipv6;
    dropdown_udp.selected = dropdown_udp.over = (save->disableudp != 0);
    dropdown_proxy.selected = dropdown_proxy.over = save->proxyenable <= 2 ? save->proxyenable : 2;

    options.ipv6enabled = save->enableipv6;
    options.udp_disabled = save->disableudp;
    options.proxy_enabled = save->proxyenable;
    options.proxy_port = save->proxy_port;
    strcpy((char*)options.proxy_address, (char*)save->proxy_ip);
    edit_proxy_ip.length = strlen((char*)save->proxy_ip);
    strcpy((char*)edit_proxy_ip.data, (char*)save->proxy_ip);
    if(save->proxy_port) {
        edit_proxy_port.length = sprintf((char*)edit_proxy_port.data, "%u", save->proxy_port);
    }

    hwnd = CreateWindowExW(0, classname, L"Tox", WS_OVERLAPPEDWINDOW, save->window_x, save->window_y, save->window_width, save->window_height, NULL, NULL, hInstance, NULL);

    free(save);

    //start tox thread (hwnd needs to be set first)
    thread(tox_thread, NULL);

    hdc_brush = GetStockObject(DC_BRUSH);

    dropdown_add(&dropdown_video, (uint8_t*)"None", NULL);
    dropdown_add(&dropdown_video, (uint8_t*)"Desktop", (void*)1);

    ShowWindow(hwnd, nCmdShow);

    tme.hwndTrack = hwnd;

    nid.hWnd = hwnd;
    Shell_NotifyIcon(NIM_ADD, &nid);

    SetBkMode(hdc, TRANSPARENT);

    dnd_init(hwnd);

    uint8_t langid = GetUserDefaultUILanguage() & 0xFF;
    switch(langid) {
    default:
    case 0x07:
        LANG = LANG_DE;
        break;
    case 0x09:
        LANG = LANG_EN;
        break;
    case 0x0A:
        LANG = LANG_ES;
        break;
    case 0x0C:
        LANG = LANG_FR;
        break;
    case 0x10:
        LANG = LANG_IT;
        break;
    case 0x11:
        LANG = LANG_JA;
        break;
    case 0x15:
        LANG = LANG_PL;
        break;
    case 0x19:
        LANG = LANG_RU;
        break;
    }

    dropdown_language.selected = dropdown_language.over = LANG;

    //wait for tox_thread init
    while(!tox_thread_init) {
        Sleep(1);
    }

    list_start();

    if(*cmd)
    {
        int len = strlen(cmd);
        parsecmd((uint8_t*)cmd, len);
    }

    draw = 1;
    redraw();

    while(GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    /* kill threads */
    toxaudio_postmessage(AUDIO_KILL, 0, 0, NULL);
    toxvideo_postmessage(VIDEO_KILL, 0, 0, NULL);
    tox_postmessage(TOX_KILL, 0, 0, NULL);

    /* cleanup */

    /* delete tray icon */
    Shell_NotifyIcon(NIM_DELETE, &nid);

    /* wait for threads to exit */
    while(tox_thread_init) {
        yieldcpu(1);
    }

    printf("clean exit\n");

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwn, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static int mx, my;

    if(hwnd && hwn != hwnd) {
        if(msg == WM_DESTROY) {
            if(hwn == video_hwnd[0]) {
                if(video_preview) {
                    video_preview = 0;
                    toxvideo_postmessage(VIDEO_PREVIEW_END, 0, 0, NULL);
                }

                return 0;
            }

            int i;
            for(i = 0; i != countof(friend); i++) {
                if(video_hwnd[i + 1] == hwn) {
                    FRIEND *f = &friend[i];
                    tox_postmessage(TOX_HANGUP, f->callid, 0, NULL);
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
    case WM_DESTROY: {
        FILE *file = fopen("utox_save", "wb");
        if(!file) {
            PostQuitMessage(0);
            return 0;
        }

        RECT wndrect = {0};
        GetWindowRect(hwnd, &wndrect);

        UTOX_SAVE d = {
            .version = 1,
            .scale = SCALE - 1,
            .window_x = wndrect.left < 0 ? 0 : wndrect.left,
            .window_y = wndrect.top < 0 ? 0 : wndrect.top,
            .window_width = (wndrect.right - wndrect.left),
            .window_height = (wndrect.bottom - wndrect.top),
            .enableipv6 = !dropdown_ipv6.selected,
            .disableudp = dropdown_udp.selected,
            .proxyenable = dropdown_proxy.selected,
            .proxy_port = options.proxy_port,
        };

        fwrite(&d, sizeof(d), 1, file);
        fwrite(options.proxy_address, strlen(options.proxy_address), 1, file);
        fclose(file);

        PostQuitMessage(0);
        return 0;
    }

    case WM_GETMINMAXINFO: {
        POINT min = {320 * SCALE, 160 * SCALE};
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
            maximized = 1;
            break;
        }

        case SIZE_RESTORED: {
            maximized = 0;
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

            width = w;
            height = h;

            debug("%u %u\n", w, h);

            ui_scale(dropdown_dpi.selected + 1);
            ui_size(w, h);

            if(hdc_bm) {
                DeleteObject(hdc_bm);
            }

            hdc_bm = CreateCompatibleBitmap(main_hdc, width, height);
            SelectObject(hdc, hdc_bm);
            redraw();
        }
        break;
    }

    case WM_SETFOCUS: {
        if(flashing) {
            FlashWindow(hwnd, 0);
            flashing = 0;
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

    case WM_KEYDOWN: {
        _Bool control = ((GetKeyState(VK_CONTROL) & 0x80) != 0);
        _Bool shift = ((GetKeyState(VK_SHIFT) & 0x80) != 0);

        if(control && wParam == 'C') {
            copy(1);
            return 0;
        }

        if(edit_active()) {
            if(control) {
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

            switch(wParam) {
            case VK_DELETE: {
                list_deletesitem();
                return 0;
            }
            }
        }

        return 0;
    }

    case WM_CHAR: {
        if(edit_active()) {
            if(wParam == KEY_RETURN && (GetKeyState(VK_SHIFT) & 0x80)) {
                wParam = '\n';
            }
            edit_char(wParam, 0, 0);
            return 0;
        }

        return 0;
    }

    case WM_MOUSEWHEEL: {
        panel_mwheel(&panel_main, 0, 0, width, height, (double)((int16_t)HIWORD(wParam)) / (double)(WHEEL_DELTA));
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
        panel_mmove(&panel_main, 0, 0, width, height, x, y, dx, dy);

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
            panel_mmove(&panel_main, 0, 0, width, height, x, y, x - mx, y - my);
            mx = x;
            my = y;
        }

        //double redraw>
        panel_mdown(&panel_main);
        if(msg == WM_LBUTTONDBLCLK) {
            panel_dclick(&panel_main, 0);
        }

        SetCapture(hwn);
        mdown = 1;
        break;
    }

    case WM_RBUTTONDOWN: {
        panel_mright(&panel_main);
        break;
    }

    case WM_RBUTTONUP: {
        break;
    }

    case WM_LBUTTONUP: {
        ReleaseCapture();
        panel_mup(&panel_main);
        mdown = 0;
        break;
    }

    case WM_MOUSELEAVE: {
        panel_mleave(&panel_main);
        mouse_tracked = 0;
        break;
    }


    case WM_COMMAND: {
        int menu = LOWORD(wParam);//, msg = HIWORD(wParam);

        switch(menu) {
        case TRAY_SHOWHIDE: {
            togglehide();
            break;
        }

        case TRAY_EXIT: {
            PostQuitMessage(0);
            break;
        }

#define setstatus(x) if(self.status != x) { \
            tox_postmessage(TOX_SETSTATUS, x, 0, NULL); self.status = x; redraw(); }

        case TRAY_STATUS_AVAILABLE: {
            setstatus(TOX_USERSTATUS_NONE);
            break;
        }

        case TRAY_STATUS_AWAY: {
            setstatus(TOX_USERSTATUS_AWAY);
            break;
        }

        case TRAY_STATUS_BUSY: {
            setstatus(TOX_USERSTATUS_BUSY);
            break;
        }
        }

        break;
    }

    case WM_NOTIFYICON: {
        int msg = LOWORD(lParam);

        switch(msg) {
        case WM_MOUSEMOVE: {
            break;
        }

        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK: {
            togglehide();
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
        COPYDATASTRUCT *data = (void*)lParam;
        parsecmd(data->lpData, data->cbData);
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
        SetWindowPos(video_hwnd[id], 0, 0, 0, r.right - r.left, r.bottom - r.top, SWP_NOZORDER | SWP_NOMOVE);
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

void video_begin(uint32_t id, uint8_t *name, uint16_t name_length, uint16_t width, uint16_t height)
{
    if(video_hwnd[id]) {
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
    *h = CreateWindowExW(0, L"uTox", out, WS_OVERLAPPEDWINDOW, 0, 0, r.right - r.left, r.bottom - r.top, NULL, NULL, hinstance, NULL);



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

IPin* ConnectFilters2(IGraphBuilder *pGraph, IPin *pOut, IBaseFilter *pDest)
{
    IPin *pIn = NULL;

    // Find an input pin on the downstream filter.
    HRESULT hr = FindUnconnectedPin(pDest, PINDIR_INPUT, &pIn);
    if (SUCCEEDED(hr))
    {
        // Try to connect them.
        hr = pGraph->lpVtbl->Connect(pGraph, pOut, pIn);
        pIn->lpVtbl->Release(pIn);
    }
    return SUCCEEDED(hr) ? pIn : NULL;
}

HRESULT ConnectFilters(IGraphBuilder *pGraph, IBaseFilter *pSrc, IBaseFilter *pDest)
{
    IPin *pOut = NULL;

    // Find an output pin on the first filter.
    HRESULT hr = FindUnconnectedPin(pSrc, PINDIR_OUTPUT, &pOut);
    if (SUCCEEDED(hr))
    {
        if(!ConnectFilters2(pGraph, pOut, pDest)) {
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
                postmessage(NEW_VIDEO_DEVICE, 0, 0, data);
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
    } else {
        debug("got bad format\n");
    }

    video_width = bmiHeader->biWidth;
    video_height = bmiHeader->biHeight;
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

int video_getframe(vpx_image_t *image)
{
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
            rgbtoyuv420(image->planes[0], image->planes[1], image->planes[2], dibits, video_width, video_height);
            lasttime = t;
            return 1;
        }
        return 0;
    }

    if(newframe) {
        newframe = 0;
        rgbtoyuv420(image->planes[0], image->planes[1], image->planes[2], frame_data, video_width, video_height);
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
