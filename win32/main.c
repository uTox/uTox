#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x500
#endif

#ifndef WINVER
#define WINVER 0x410
#endif

#include <windows.h>
#include <windowsx.h>

#include <process.h>
#define CLEARTYPE_QUALITY 5

#define WM_NOTIFYICON   (WM_APP + 0)
#define WM_TOX          (WM_APP + 1)

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

_Bool draw = 0;

float scale = 1.0;
_Bool connected = 0;

enum {
    MENU_TEXTINPUT = 101,
    MENU_MESSAGES = 102,
};

HBITMAP bitmap[16];
HFONT font[16];
HCURSOR cursor_arrow, cursor_hand;

HWND hwnd;
HDC main_hdc, hdc, hdcMem;
HBRUSH hdc_brush;
HBITMAP hdc_bm;


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
    EDIT_CUT,
    EDIT_COPY,
    EDIT_PASTE,
    EDIT_DELETE,
    EDIT_SELECTALL,
    LIST_DELETE,
    LIST_ACCEPT
};

static uint16_t utf8tonative(char_t *str, wchar_t *out, uint16_t length)
{
    return MultiByteToWideChar(CP_UTF8, 0, str, length, out, 65536);
}

void postmessage(uint32_t msg, uint16_t param1, uint16_t param2, void *data)
{
    PostMessage(hwnd, WM_TOX + (msg), ((param1) << 16) | (param2), (LPARAM)data);
}

void drawbitmap(int bm, int x, int y, int width, int height)
{
    SelectObject(hdcMem, bitmap[bm]);
    BitBlt(hdc, x, y, width, height, hdcMem, 0, 0, SRCCOPY);
}

void drawbitmapalpha(int bm, int x, int y, int width, int height)
{
    BLENDFUNCTION ftn = {
        .BlendOp = AC_SRC_OVER,
        .BlendFlags = 0,
        .SourceConstantAlpha = 0xFF,
        .AlphaFormat = AC_SRC_ALPHA
    };

    SelectObject(hdcMem, bitmap[bm]);
    AlphaBlend(hdc, x, y, width, height, hdcMem, 0, 0, width, height, ftn);
}

void drawtext(int x, int y, uint8_t *str, uint16_t length)
{
    TextOut(hdc, x, y, (char*)str, length);
}

void drawtextW(int x, int y, char_t *str, uint16_t length)
{
    wchar_t out[length];
    length = utf8tonative(str, out, length);

    TextOutW(hdc, x, y, out, length);
}

int drawtext_getwidth(int x, int y, uint8_t *str, uint16_t length)
{
    SIZE size;
    TextOut(hdc, x, y, (char*)str, length);
    GetTextExtentPoint32(hdc, (char*)str, length, &size);
    return size.cx;
}

int drawtext_getwidthW(int x, int y, char_t *str, uint16_t length)
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
    RECT r = {x, y, x + width, y + 256};
    DrawText(hdc, (char*)str, length, &r, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
}

void drawtextwidth_right(int x, int width, int y, uint8_t *str, uint16_t length)
{
    RECT r = {x, y, x + width, y + 256};
    DrawText(hdc, (char*)str, length, &r, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX | DT_RIGHT);
}

void drawtextwidth_rightW(int x, int width, int y, char_t *str, uint16_t length)
{
    wchar_t out[length];
    length = utf8tonative(str, out, length);

    RECT r = {x, y, x + width, y + 256};
    DrawTextW(hdc, out, length, &r, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX | DT_RIGHT);
}

void drawtextrange(int x, int x2, int y, uint8_t *str, uint16_t length)
{
    RECT r = {x, y, x2, y + 256};
    DrawText(hdc, (char*)str, length, &r, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
}

void drawtextrangecut(int x, int x2, int y, uint8_t *str, uint16_t length)
{
    RECT r = {x, y, x2, y + 256};
    DrawText(hdc, (char*)str, length, &r, DT_SINGLELINE | DT_NOPREFIX);
}

void drawtextrangecutW(int x, int x2, int y, char_t *str, uint16_t length)
{
    wchar_t out[length];
    length = utf8tonative(str, out, length);

    RECT r = {x, y, x2, y + 256};
    DrawTextW(hdc, out, length, &r, DT_SINGLELINE | DT_NOPREFIX);
}

int textwidth(uint8_t *str, uint16_t length)
{
    SIZE size;
    GetTextExtentPoint32(hdc, (char*)str, length, &size);
    return size.cx;
}

int textwidthW(char_t *str, uint16_t length)
{
    wchar_t out[length];
    length = utf8tonative(str, out, length);

    SIZE size;
    GetTextExtentPoint32W(hdc, out, length, &size);
    return size.cx;
}

int textfit(uint8_t *str, uint16_t length, int width)
{
    int fit;
    SIZE size;
    GetTextExtentExPoint(hdc, str, length, width, &fit, NULL, &size);

    return fit;
}

int textfitW(char_t *str, uint16_t length, int width)
{
    wchar_t out[length];
    length = utf8tonative(str, out, length);

    int fit;
    SIZE size;
    GetTextExtentExPointW(hdc, out, length, width, &fit, NULL, &size);

    return WideCharToMultiByte(CP_UTF8, 0, out, fit, str, 65536, NULL, 0);
}

void drawrect(int x, int y, int width, int height, uint32_t color)
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

void fillrect(RECT *r, uint32_t color)
{
    SetDCBrushColor(hdc, color);
    FillRect(hdc, r, hdc_brush);
}

void framerect(RECT *r, uint32_t color)
{
    SetDCBrushColor(hdc, color);
    FrameRect(hdc, r, hdc_brush);
}

void setfont(int id)
{
    SelectObject(hdc, font[id]);
}

uint32_t setcolor(uint32_t color)
{
    return SetTextColor(hdc, color);
}

void setbkcolor(uint32_t color)
{
    SetBkColor(hdc, color);
}

void setbgcolor(uint32_t color)
{
    if(color == ~0) {
        SetBkMode(hdc, TRANSPARENT);
    } else {
        SetBkMode(hdc, OPAQUE);
        SetBkColor(hdc, color);
    }
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

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size + 1);
    uint8_t *p = GlobalLock(hMem);
    memcpy(p, self.id, size + 1);
    p[size] = 0;
    GlobalUnlock(hMem);
    OpenClipboard(0);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();

#undef size
}

void editpopup(void)
{
    POINT p;
    GetCursorPos(&p);

    HMENU hMenu = CreatePopupMenu();
    if(hMenu) {
        _Bool emptysel = (edit_sel.length == 0);

        InsertMenu(hMenu, -1, MF_BYPOSITION | (emptysel ? MF_GRAYED : 0), EDIT_CUT, "Cut");
        InsertMenu(hMenu, -1, MF_BYPOSITION | (emptysel ? MF_GRAYED : 0), EDIT_COPY, "Copy");
        InsertMenu(hMenu, -1, MF_BYPOSITION, EDIT_PASTE, "Paste");///gray out if clipboard empty
        InsertMenu(hMenu, -1, MF_BYPOSITION | (emptysel ? MF_GRAYED : 0), EDIT_DELETE, "Delete");
        InsertMenu(hMenu, -1, MF_BYPOSITION, EDIT_SELECTALL, "Select All");

        SetForegroundWindow(hwnd);

        TrackPopupMenu(hMenu, TPM_TOPALIGN, p.x, p.y, 0, hwnd, NULL);
        DestroyMenu(hMenu);
    }
}

void listpopup(uint8_t item)
{
    POINT p;
    GetCursorPos(&p);

    HMENU hMenu = CreatePopupMenu();
    if(hMenu) {
        switch(item) {
        case ITEM_SELF: {
            InsertMenu(hMenu, -1, MF_BYPOSITION | ((self.status == TOX_USERSTATUS_NONE) ? MF_CHECKED : 0), TRAY_STATUS_AVAILABLE, "Available");
            InsertMenu(hMenu, -1, MF_BYPOSITION | ((self.status == TOX_USERSTATUS_AWAY) ? MF_CHECKED : 0), TRAY_STATUS_AWAY, "Away");
            InsertMenu(hMenu, -1, MF_BYPOSITION | ((self.status == TOX_USERSTATUS_BUSY) ? MF_CHECKED : 0), TRAY_STATUS_BUSY, "Busy");
            break;
        }

        case ITEM_FRIEND: {
            InsertMenu(hMenu, -1, MF_BYPOSITION, LIST_DELETE, "Remove");
            break;
        }

        case ITEM_GROUP: {
            InsertMenu(hMenu, -1, MF_BYPOSITION, LIST_DELETE, "Leave");
            break;
        }

        case ITEM_FRIEND_ADD: {
            InsertMenu(hMenu, -1, MF_BYPOSITION, LIST_ACCEPT, "Accept");
            InsertMenu(hMenu, -1, MF_BYPOSITION, LIST_DELETE, "Ignore");
            break;
        }
        }

        SetForegroundWindow(hwnd);

        TrackPopupMenu(hMenu, TPM_TOPALIGN, p.x, p.y, 0, hwnd, NULL);
        DestroyMenu(hMenu);
    }
}

void openurl(char_t *str)
{
    //!convert
    ShellExecute(NULL, "open", str, NULL, NULL, SW_SHOW);
}

void openfilesend(void)
{
    char *filepath = malloc(1024);
    filepath[0] = 0;

    OPENFILENAME ofn = {
        .lStructSize = sizeof(OPENFILENAME),
        .hwndOwner = hwnd,
        .lpstrFile = filepath,
        .nMaxFile = 1024,
        .Flags = OFN_EXPLORER | OFN_ALLOWMULTISELECT | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR,
    };

    if(GetOpenFileName(&ofn)) {
        tox_postmessage(TOX_SENDFILES, (FRIEND*)sitem->data - friend, ofn.nFileOffset, filepath);
    } else {
        debug("GetOpenFileName() failed\n");
    }
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

void sysmexit(void)
{
    PostQuitMessage(0);
}

void sysmsize(void)
{
    ShowWindow(hwnd, maximized ? SW_RESTORE : SW_MAXIMIZE);
}

void sysmmini(void)
{
    ShowWindow(hwnd, SW_MINIMIZE);
}

void setselection(void)
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

LRESULT nc_hit(int x, int y)
{
    uint8_t row, col;

    row = 1;
    col = 1;

    if(x < BORDER) {
        col = 0;
    } else if(x >= width - BORDER) {
        col = 2;
    }

    if(y < CAPTION + BORDER) {
        if(y < BORDER) {
            row = 0;
        } else if(col == 1) {
            if(x >= LIST_X && x < LIST_X + ITEM_WIDTH) {
                return (y >= LIST_Y) ? HTCLIENT : HTCAPTION;
            }

            return inrect(x, y, width - 91, 1, 90, 26) ? HTCLIENT : HTCAPTION;
        }
    } else if(y >= height - BORDER) {
        row = 2;
    }

    if(x >= width - 10 && y >= height - 10) {
        return HTBOTTOMRIGHT;
    }

    const LRESULT result[3][3] = {
        {HTTOPLEFT, HTTOP, HTTOPRIGHT},
        {HTLEFT, HTCLIENT, HTRIGHT},
        {HTBOTTOMLEFT, HTBOTTOM, HTBOTTOMRIGHT},
    };

    return result[row][col];
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
    const char classname[] = "uTox";

    HICON myicon = LoadIcon(hInstance, MAKEINTRESOURCE(101));
    cursor_arrow = LoadCursor(NULL, IDC_ARROW);
    cursor_hand = LoadCursor(NULL, IDC_HAND);

    WNDCLASS wc = {
        .style = CS_OWNDC,
        .lpfnWndProc = WindowProc,
        .hInstance = hInstance,
        .hIcon = myicon,
        .lpszClassName = classname,
    };

    NOTIFYICONDATA nid = {
        .uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP,
        .uCallbackMessage = WM_NOTIFYICON,
        .hIcon = myicon,
        .szTip = "Tox - tooltip",
        .cbSize = sizeof(nid),
    };

    LOGFONT lf = {
        .lfWeight = FW_NORMAL,
        //.lfCharSet = ANSI_CHARSET,
        .lfOutPrecision = OUT_TT_PRECIS,
        .lfQuality = CLEARTYPE_QUALITY,
        .lfFaceName = "DejaVu Sans",
    };

    BITMAP bm = {
        .bmWidth = 16,
        .bmHeight = 10,
        .bmWidthBytes = 2,
        .bmPlanes = 1,
        .bmBitsPixel = 1
    };

    BITMAP bm2 = {
        .bmWidth = 8,
        .bmHeight = 8,
        .bmWidthBytes = 32,
        .bmPlanes = 1,
        .bmBitsPixel = 32
    };

    //start tox thread
    thread(tox_thread, NULL);

    RegisterClass(&wc);

    x = (GetSystemMetrics(SM_CXSCREEN) - MAIN_WIDTH) / 2;
    y = (GetSystemMetrics(SM_CYSCREEN) - MAIN_HEIGHT) / 2;
    hwnd = CreateWindowEx(WS_EX_APPWINDOW, classname, "Tox", WS_POPUP | WS_MINIMIZEBOX | WS_MAXIMIZEBOX, x, y, MAIN_WIDTH, MAIN_HEIGHT, NULL, NULL, hInstance, NULL);

    hdc_brush = GetStockObject(DC_BRUSH);

    ShowWindow(hwnd, nCmdShow);

    tme.hwndTrack = hwnd;

    nid.hWnd = hwnd;
    Shell_NotifyIcon(NIM_ADD, &nid);

    lf.lfHeight = -20;
    font[FONT_TITLE] = CreateFontIndirect(&lf);
    lf.lfHeight = -18;
    font[FONT_SUBTITLE] = CreateFontIndirect(&lf);
    lf.lfHeight = -16;
    font[FONT_MED] = CreateFontIndirect(&lf);
    lf.lfHeight = -14;
    font[FONT_TEXT_LARGE] = CreateFontIndirect(&lf);
    lf.lfHeight = -12;
    font[FONT_TEXT] = CreateFontIndirect(&lf);

    //memcpy(lf.lfFaceName, "DejaVu Sans", sizeof("DejaVu Sans"));
    lf.lfHeight = 16;
    font[FONT_MSG] = CreateFontIndirect(&lf);
    lf.lfWeight = FW_BOLD;
    font[FONT_MSG_NAME] = CreateFontIndirect(&lf);
    lf.lfWeight = FW_NORMAL;
    lf.lfUnderline = 1;
    font[FONT_MSG_LINK] = CreateFontIndirect(&lf);

    bm.bmBits = bm_minimize_bits;
    bitmap[BM_MINIMIZE] = CreateBitmapIndirect(&bm);
    bm.bmBits = bm_restore_bits;
    bitmap[BM_RESTORE] = CreateBitmapIndirect(&bm);
    bm.bmBits = bm_maximize_bits;
    bitmap[BM_MAXIMIZE] = CreateBitmapIndirect(&bm);
    bm.bmBits = bm_exit_bits;
    bitmap[BM_EXIT] = CreateBitmapIndirect(&bm);

    //153, 182, 224

    bitmap[BM_ONLINE] = CreateBitmap(10, 10, 1, 32, bm_online_bits);
    bitmap[BM_AWAY] = CreateBitmap(10, 10, 1, 32, bm_away_bits);
    bitmap[BM_BUSY] = CreateBitmap(10, 10, 1, 32, bm_busy_bits);
    bitmap[BM_OFFLINE] = CreateBitmap(10, 10, 1, 32, bm_offline_bits);
    bitmap[BM_CONTACT] = CreateBitmap(48, 48, 1, 32, bm_contact_bits);
    bitmap[BM_GROUP] = CreateBitmap(48, 48, 1, 32, bm_group_bits);
    bitmap[BM_FILE] = CreateBitmap(48, 48, 1, 32, bm_file_bits);

    uint32_t test[64];
    int xx = 0;
    while(xx < 8) {
        int y = 0;
        while(y < 8) {
            uint32_t value = 0xFFFFFF;
            if(xx + y >= 7) {
                int a = xx % 3, b = y % 3;
                if(a == 1) {
                    if(b == 0) {
                        value = 0xB6B6B6;
                    } else if(b == 1) {
                        value = 0x999999;
                    }
                } else if(a == 0 && b == 1) {
                    value = 0xE0E0E0;
                }
            }

            test[y * 8 + xx] = value;
            y++;
        }
        xx++;
    }

    bm2.bmBits = test;
    bitmap[BM_CORNER] = CreateBitmapIndirect(&bm2);

    TEXTMETRIC tm;
    SelectObject(hdc, font[FONT_TEXT]);
    GetTextMetrics(hdc, &tm);
    font_small_lineheight = tm.tmHeight + tm.tmExternalLeading;
    SelectObject(hdc, font[FONT_MSG]);
    GetTextMetrics(hdc, &tm);
    font_msg_lineheight = tm.tmHeight + tm.tmExternalLeading;

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

    tox_postmessage(TOX_KILL, 0, 0, NULL);

    //cleanup

    //delete tray icon
    Shell_NotifyIcon(NIM_DELETE, &nid);

    //wait for tox_thread cleanup
    while(1) {
        if(GetMessage(&msg, NULL, 0, 0)) {
            if(msg.message == WM_TOX + TOX_DONE) {
                break;
            }
        }
    }

    printf("clean exit\n");

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {
    case WM_DESTROY: {
        PostQuitMessage(0);
        return 0;
    }

    case WM_GETMINMAXINFO: {
        POINT min = {480, 320};
        ((MINMAXINFO*)lParam)->ptMinTrackSize = min;

        break;
    }

    case WM_CREATE: {
        main_hdc = GetDC(hwnd);
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
            width = w;
            height = h;

            panel_update(&panel_main, 0, 0, width, height);

            if(hdc_bm) {
                DeleteObject(hdc_bm);
            }

            hdc_bm = CreateCompatibleBitmap(main_hdc, width, height);
            SelectObject(hdc, hdc_bm);
            redraw();
        }
        break;
    }

    case WM_ERASEBKGND: {
        return 1;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;

        BeginPaint(hwnd, &ps);

        RECT r = ps.rcPaint;
        BitBlt(main_hdc, r.left, r.top, r.right - r.left, r.bottom - r.top, hdc, r.left, r.top, SRCCOPY);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_NCHITTEST: {
        POINT p = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

        ScreenToClient(hwnd, &p);

        return nc_hit(p.x, p.y);
    }

    case WM_KEYDOWN: {
        switch(wParam) {

        case VK_ESCAPE: {
            edit_resetfocus();
            return 0;
        }

        case VK_DELETE: {
            if(edit_active()) {
                edit_delete();
                break;
            }

            list_deletesitem();
            return 0;
        }
        }

        if(GetKeyState(VK_CONTROL) & 0x80) {
            switch(wParam) {
            case 'C': {
                if(edit_active()) {
                    edit_copy();
                    break;
                }

                HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 65536);//! calculate this number
                char_t *data = GlobalLock(hMem);
                data[0] = 0;

                if(sitem->item == ITEM_FRIEND) {
                    messages_selection(&messages_friend, data, 65536);
                }

                if(sitem->item == ITEM_GROUP) {
                    messages_selection(&messages_group, data, 65536);
                }

                GlobalUnlock(hMem);
                OpenClipboard(0);
                EmptyClipboard();
                SetClipboardData(CF_UNICODETEXT, hMem);
                CloseClipboard();

                break;
            }

            case 'V': {
                if(edit_active()) {
                    OpenClipboard(NULL);
                    char *cd = GetClipboardData(CF_TEXT);
                    int length = strlen(cd);
                    edit_paste(cd, length);
                    CloseClipboard();
                    break;
                }
                break;
            }

            case 'A': {
                if(edit_active()) {
                    edit_selectall();
                    break;
                }
                break;
            }
            }
        }

        break;
    }

    case WM_CHAR: {
        if(edit_active()) {
            edit_char(wParam, 0);
            return 0;
        }

        return 0;
    }

    case WM_MOUSEWHEEL: {
        panel_mwheel(&panel_main, 0, 0, width, height, (double)((int16_t)HIWORD(wParam)) / (double)(WHEEL_DELTA));
        return 0;
    }

    case WM_MOUSEMOVE: {
        static int my;
        int x, y, dy;

        x = GET_X_LPARAM(lParam);
        y = GET_Y_LPARAM(lParam);

        dy = y - my;
        my = y;

        hand = 0;

        panel_mmove(&panel_main, 0, 0, width, height, x, y, dy);

        SetCursor(hand ? cursor_hand : cursor_arrow);

        if(!mouse_tracked) {
            TrackMouseEvent(&tme);
            mouse_tracked = 1;
        }

        return 0;
    }

    case WM_LBUTTONDOWN: {
        panel_mdown(&panel_main);
        SetCapture(hwnd);
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
        panel_mup(&panel_main);
        ReleaseCapture();
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


        case EDIT_CUT: {
            edit_cut();
            break;
        }

        case EDIT_COPY: {
            edit_copy();
            break;
        }

        case EDIT_PASTE: {
            if(edit_active()) {
                OpenClipboard(NULL);
                char *cd = GetClipboardData(CF_TEXT);
                int length = strlen(cd);
                edit_paste(cd, length);
                CloseClipboard();
                break;
            }
            break;
        }

        case EDIT_DELETE: {
            edit_delete();
            break;
        }

        case EDIT_SELECTALL: {
            edit_selectall();
            break;
        }

        case LIST_DELETE: {
            list_deleteritem();
            break;
        }

        case LIST_ACCEPT: {
            FRIENDREQ *req = ritem->data;
            tox_postmessage(TOX_ACCEPTFRIEND, 0, 0, req);
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

    return DefWindowProc(hwnd, msg, wParam, lParam);
}
