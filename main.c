
#include "main.h"

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

_Bool draw = 0;

float scale = 1.0;
_Bool connected = 0;

enum
{
    MENU_TEXTINPUT = 101,
    MENU_MESSAGES = 102,
};

static int mx, my;

static TRACKMOUSEEVENT tme = {sizeof(TRACKMOUSEEVENT), TME_LEAVE, 0, 0};
static _Bool mouse_tracked = 0;

static _Bool hidden;

void togglehide(void)
{
    if(hidden)
    {
        ShowWindow(hwnd, SW_RESTORE);
        SetForegroundWindow(hwnd);
        drawall();
        hidden = 0;
    }
    else
    {
        ShowWindow(hwnd, SW_HIDE);
        hidden = 1;
    }
}

void ShowContextMenu(void)
{
	POINT pt;
	GetCursorPos(&pt);
	HMENU hMenu = CreatePopupMenu();
	if(hMenu)
	{
	    InsertMenu(hMenu, -1, MF_BYPOSITION, TRAY_SHOWHIDE, hidden ? "Restore" : "Hide");
		InsertMenu(hMenu, -1, MF_BYPOSITION, TRAY_EXIT, "Exit");

		// note:	must set window to the foreground or the
		//			menu won't disappear when it should
		SetForegroundWindow(hwnd);

		TrackPopupMenu(hMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, NULL);
		DestroyMenu(hMenu);
	}
}

RECT rect_new(RECT r, int left, int top, int right, int bottom)
{
    r.left += left;
    r.top += top;
    r.right += right;
    r.bottom += bottom;
    return r;
}

void drawfriendmain(int x, int y, FRIEND *f)
{
    SetTextColor(hdc, 0x333333);
    SelectObject(hdc, font_big);
    drawtextrange(x + 8, width - 24, y + 2, f->name, f->name_length);

    SetTextColor(hdc, 0x999999);
    SelectObject(hdc, font_med);
    drawtextrange(x + 8, width - 24, y + 26, f->status_message, f->status_length);


    RECT send = {width - 100 , height - 48, width - 24, height - 24};
    FillRect(hdc, &send, red);

    draw_messages(x + 1, y + 47, f);

    RECT r = {x, y + 46, width - 24, height - 152};

    if(f->online)
    {
        FrameRect(hdc, &r, border);
    }
}

void drawgroupmain(int x, int y, GROUPCHAT *g)
{
    RECT r = {x, height - 128, width - 24, height - 48};
    //FrameRect(hdc, &r, border);

    r.top = y + 46;
    r.bottom = height - 152;
    FrameRect(hdc, &r, border);

    r.left = width - 24 - 100;
    FrameRect(hdc, &r, border);

    RECT send = {width - 100 , height - 48, width - 24, height - 24};
    FillRect(hdc, &send, red);

    draw_groupmessages(x + 1, y + 47, g);

    uint8_t **np = g->peername;
    int i = 0;
    while(i < g->peers)
    {
        uint8_t *n = *np++;
        if(n)
        {
            drawtextrange(width - 122, width - 24, y + 47 + i * 12, n + 1, n[0]);
            i++;
        }
    }
}

void drawselfmain(int x, int y)
{
    x += 32;

    SetTextColor(hdc, 0x333333);

    SelectObject(hdc, font_big);
    drawstr(x, y + 2, "User settings");

    SetTextColor(hdc, 0x555555);

    SelectObject(hdc, font_big2);
    drawstr(x, y + 50, "Name");
    drawstr(x, y + 100, "Status message");
    drawstr(x, y + 150, "Tox ID");

    SelectObject(hdc, font_med2);
    drawtextrange(x, edit_name.right, y + 176, tox_address_string, sizeof(tox_address_string));
}

void drawaddmain(int x, int y)
{
    x += 32;

    SetTextColor(hdc, 0x333333);

    SelectObject(hdc, font_big);
    drawstr(x, y + 2, "Add a friend");

    SetTextColor(hdc, 0x555555);

    SelectObject(hdc, font_big2);
    drawstr(x, y + 50, "Tox ID");
    drawstr(x, y + 100, "Message");

    if(addfriend_status)
    {
        char *message[] = {"Friend request sent", "Invalid ID format", "Error"};
        uint16_t length[] = {sizeof("Friend request sent") - 1, sizeof("Invalid ID format") - 1, sizeof("Error") - 1};

        drawtext(x, y + 250, message[addfriend_status - 1], length[addfriend_status - 1]);
    }
}

uint8_t scrollfr_mouseover, scrollfr_mousedown;
_Bool freqaccept_mouseover, freqaccept_mousedown, freqignore_mouseover, freqignore_mousedown;

void friendreqs_mouseleave(void)
{
    if(!requests)
    {
        return;
    }

    if(scrollfr_mouseover)
    {
        scrollfr_mouseover = 0;
        main_draw();
    }

    if(mreq)
    {
        mreq = NULL;
        main_draw();
    }
}

void remove_sfreq(void)
{
    free(sreq);

    FRIENDREQ **r = sreqq;

    requests--;
    int l = (void*)(&request[requests]) - (void*)(r);

    //printf("%i %i %u\n", l, r - request, r);

    memcpy(r, r + 1, l);

    mreqq = NULL;
    mreq = NULL;

    if(l != 0)
    {
        sreq = *r;
    }
    else
    {
        sreq = NULL;
        sreqq = NULL;
    }

}

void friendreqs_mouseup(void)
{
    if(!requests)
    {
        return;
    }

    if(scrollfr_mousedown)
    {
        scrollfr_mousedown = 0;
        main_draw();
    }

    if(freqaccept_mousedown)
    {
        if(freqaccept_mouseover)
        {
            core_postmessage(CMSG_ACCEPTFRIEND, 0, sizeof(sreq->id), sreq->id);

            remove_sfreq();
        }

        freqaccept_mousedown = 0;
        main_draw();
    }

    if(freqignore_mousedown)
    {
        if(freqignore_mouseover)
        {
            remove_sfreq();

        }

        freqignore_mousedown = 0;
        main_draw();
    }
}

void friendreqs_mousedown(void)
{
    if(!requests)
    {
        return;
    }

    if(scrollfr_mouseover)
    {
        scrollfr_mousedown = 1;
        main_draw();
    }

    if(freqaccept_mouseover)
    {
        freqaccept_mousedown = 1;
    }

    if(freqignore_mouseover)
    {
        freqignore_mousedown = 1;
    }

    if(mreq && mreq != sreq)
    {
        sreq = mreq;
        main_draw();
    }
}

_Bool inrect(int x, int y, int rx, int ry, int width, int height)
{
    x -= rx;
    y -= ry;

    return (x >= 0 && x < width && y >= 0 && y < height);
}

void friendreqs_mousemove(int x, int y)
{
    if(!requests)
    {
        return;
    }

    if(inrect(x, y, MAIN_X + 12 + REQ_WIDTH + 36, MAIN_Y + 222, 50, 18))
    {
        if(!freqignore_mouseover)
        {
            freqignore_mouseover = 1;
            main_draw();
        }
    }
    else
    {
        if(freqignore_mouseover)
        {
            freqignore_mouseover = 0;
            main_draw();
        }
    }

    if(inrect(x, y, width - 120, MAIN_Y + 222, 50, 18))
    {
        if(!freqaccept_mouseover)
        {
            freqaccept_mouseover = 1;
            main_draw();
        }
    }
    else
    {
        if(freqaccept_mouseover)
        {
            freqaccept_mouseover = 0;
            main_draw();
        }
    }

    if(x >= MAIN_X + 12 && x < MAIN_X + 12 + REQ_WIDTH)
    {
        int i = (y - (MAIN_Y + 40));
        if(i >= 0)
        {
            i /= REQ_HEIGHT;
            if(i < requests)
            {
                FRIENDREQ **m = &request[i];
                if(m != mreqq)
                {
                    mreqq = m;
                    mreq = *m;

                    main_draw();
                }

                goto SKIP;
            }
        }
    }

    if(mreq)
    {
        mreq = NULL;
        mreqq = NULL;

        main_draw();
    }

    SKIP:


    x -= MAIN_X + 12 + REQSCROLL_X;
    if(x >= 0 && x < SCROLL_WIDTH && y >= MAIN_Y + 40 && y < height - 24)
    {
        if(!scrollfr_mouseover)
        {
            scrollfr_mouseover = 2;
            main_draw();
        }
    }
    else
    {
        if(scrollfr_mouseover)
        {
            scrollfr_mouseover = 0;
            main_draw();
        }
    }
}

void drawrequest(int x, int y, FRIENDREQ *f)
{
    RECT rmain = {x, y, x + REQ_WIDTH, y + REQ_HEIGHT - 1};
    FillRect(hdc, &rmain, (f == sreq) ? blue : ((f == mreq) ? gray : white));

    RECT r = {x, y + REQ_HEIGHT - 1, x + REQ_WIDTH, y + REQ_HEIGHT};
    FillRect(hdc, &r, gray);

    SetTextColor(hdc, (f == sreq) ? WHITE : 0x333333);
    SelectObject(hdc, font_med);


    uint8_t id[TOX_FRIEND_ADDRESS_SIZE * 2];
    sprint_address(id, f->id);

    drawtextwidth(x + 6, REQ_WIDTH - 7, y + 8, id, TOX_FRIEND_ADDRESS_SIZE * 2);

    SetTextColor(hdc, (f == sreq) ? 0xFFE6C5 : 0x999999);
    SelectObject(hdc, font_med2);
    drawtextwidth(x + 6, REQ_WIDTH - 7, y + 28, f->msg, f->length);

}

void drawacceptmain(int x, int y)
{
    x += 12;

    SetTextColor(hdc, 0x333333);

    SelectObject(hdc, font_big);
    drawstr(x + 20, y + 2, "Accept friends");

    if(requests)
    {
        int i = 0;
        while(i < requests)
        {
            drawrequest(x, y + 40 + i * 52, request[i]);
            i++;
        }

        RECT scroll = {x + REQSCROLL_X, y + 40, x + REQSCROLL_X + SCROLL_WIDTH, height - 48};

        FillRect(hdc, &scroll, (scrollfr_mouseover == 2) ? gray3 : gray2);

        scroll.top = scroll.bottom - 100;
        FillRect(hdc, &scroll, (scrollfr_mouseover) ? gray : white);

        uint8_t id[TOX_FRIEND_ADDRESS_SIZE * 2];
        sprint_address(id, sreq->id);

        SetTextColor(hdc, 0x333333);
        drawtextrange(x + REQ_WIDTH + 20, width - 24, y + 40, id, TOX_FRIEND_ADDRESS_SIZE * 2);

        SetTextColor(hdc, 0x999999);
        drawtextrect(x + REQ_WIDTH + 20, y + 60, width - 24, y + 220, sreq->msg, sreq->length);

        SetTextColor(hdc, (freqaccept_mouseover) ? 0x222222 : 0x555555);

        SelectObject(hdcMem, bm_plus2);
        SetBkColor(hdc, WHITE);
        BitBlt(hdc, width - 116, y + 225, 16, 11, hdcMem, 0, 0, SRCCOPY);

        SelectObject(hdc, font_med2);
        drawstr(width - 100, y + 222, "accept");

        SetTextColor(hdc, (freqignore_mouseover) ? 0x222222 : 0x555555);

        drawrect(x + REQ_WIDTH + 40, y + 229, 11, 3, (freqignore_mouseover) ? 0x222222 : 0x555555);

        drawstr(x + REQ_WIDTH + 54, y + 222, "ignore");


    }
    else
    {
        SetTextColor(hdc, 0x555555);
        SelectObject(hdc, font_med);
        drawstr(x, y + 50, "No new friend requests");
    }
}

void main_draw(void)
{
    int x = MAIN_X, y = MAIN_Y;

    RECT r = {MAIN_X, MAIN_Y, width - 24, height - 24};
    //FrameRect(hdc, &r, border);

    //r.left++;
    //r.top++;
    //r.right--;
    //r.bottom--;

    FillRect(hdc, &r, white);

    SetBkMode(hdc, TRANSPARENT);

    switch(sitem->item)
    {
        case ITEM_FRIEND:
        {
            drawfriendmain(x, y, sitem->data);
            break;
        }

        case ITEM_GROUP:
        {
            drawgroupmain(x, y, sitem->data);
            break;
        }

        case ITEM_SELF:
        {
            drawselfmain(x, y);
            break;
        }

        case ITEM_ADDFRIEND:
        {
            drawaddmain(x, y);
            break;
        }

        case ITEM_FRIENDREQUESTS:
        {
            drawacceptmain(x, y);
            break;
        }
    }

    edit_func(edit_draw);
    button_func(button_draw);

    commitdraw(MAIN_X, MAIN_Y, width - MAIN_X, height - MAIN_Y);
}

void drawbackground(void)
{
    RECT r = {1, 1, width - 1, height - 1};
    RECT window = {0, 0, width, height};

    FrameRect(hdc, &window, border);
    FillRect(hdc, &r, white);

    SelectObject(hdcMem, bm_corner);
    BitBlt(hdc, width - 10, height - 10, 8, 8, hdcMem, 0, 0, SRCCOPY);

    commitdraw(0, 0, width, height);
}

void drawall(void)
{
    if(!draw)
    {
        return;
    }

    drawbackground();
    sysmenu_draw();
    list_draw();
    main_draw();
}

LRESULT nc_hit(int x, int y)
{
    uint8_t row, col;

    row = 1;
    col = 1;

    if(x < BORDER)
    {
        col = 0;
    }
    else if(x >= width - BORDER)
    {
        col = 2;
    }

    if(y < CAPTION + BORDER)
    {
        if(y < BORDER)
        {
            row = 0;
        }
        else if(col == 1)
        {
            return sysmenu_hit(x, y) ? HTCLIENT : HTCAPTION;
        }
    }
    else if(y >= height - BORDER)
    {
        row = 2;
    }

    if(x >= width - 10 && y >= height - 10)
    {
        return HTBOTTOMRIGHT;
    }

    const LRESULT result[3][3] =
    {
        {HTTOPLEFT, HTTOP, HTTOPRIGHT},
        {HTLEFT, HTCLIENT, HTRIGHT},
        {HTBOTTOMLEFT, HTBOTTOM, HTBOTTOMRIGHT},
    };

    return result[row][col];
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
{
    MSG msg;
    int x, y;
    const char classname[] = "winTox";

    HICON myicon = LoadIcon(hInstance, MAKEINTRESOURCE(101));

    WNDCLASS wc = {
        .style = CS_OWNDC,
        .lpfnWndProc = WindowProc,
        .hInstance = hInstance,
        .hIcon = myicon,
        .lpszClassName = classname,
        .hCursor = LoadCursor(NULL, IDC_ARROW),
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
        .lfFaceName = "Arial",
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
    thread(core_thread, NULL);

    RegisterClass(&wc);

    white = CreateSolidBrush(WHITE);
    border = CreateSolidBrush(0x999999);
    black = CreateSolidBrush(BLACK);

    gray = CreateSolidBrush(GRAY);
    gray2 = CreateSolidBrush(GRAY2);
    gray3 = CreateSolidBrush(GRAY3);
    gray5 = CreateSolidBrush(GRAY5);
    gray6 = CreateSolidBrush(GRAY6);

    blue = CreateSolidBrush(BLUE);
    red = CreateSolidBrush(RED);
    red2 = CreateSolidBrush(RED2);

    green = CreateSolidBrush(GREEN);

    //mouseover friend 0xF1F1F1
    //mouseover top 0xEEEEEE 0x333333

//    INITCOMMONCONTROLSEX iccx;
//    iccx.dwSize = sizeof(INITCOMMONCONTROLSEX);
//    iccx.dwICC = ICC_ANIMATE_CLASS;
//    InitCommonControlsEx(&iccx);

    x = (GetSystemMetrics(SM_CXSCREEN) - MAIN_WIDTH) / 2;
    y = (GetSystemMetrics(SM_CYSCREEN) - MAIN_HEIGHT) / 2;
    hwnd = CreateWindowEx(WS_EX_APPWINDOW, classname, "Tox", WS_POPUP | WS_MINIMIZEBOX | WS_MAXIMIZEBOX, x, y, MAIN_WIDTH, MAIN_HEIGHT, NULL, NULL, hInstance, NULL);

    hdc_brush = GetStockObject(DC_BRUSH);

    ShowWindow(hwnd, nCmdShow);

    tme.hwndTrack = hwnd;

    nid.hWnd = hwnd;
    Shell_NotifyIcon(NIM_ADD, &nid);

    lf.lfHeight = -20;
    font_big = CreateFontIndirect(&lf);
    lf.lfHeight = -18;
    font_big2 = CreateFontIndirect(&lf);
    lf.lfHeight = -16;
    font_med = CreateFontIndirect(&lf);
    lf.lfHeight = -14;
    font_med2 = CreateFontIndirect(&lf);
    lf.lfHeight = -12;
    font_small = CreateFontIndirect(&lf);

    bm.bmBits = bm_minimize_bits;
    bm_minimize = CreateBitmapIndirect(&bm);
    bm.bmBits = bm_restore_bits;
    bm_restore = CreateBitmapIndirect(&bm);
    bm.bmBits = bm_maximize_bits;
    bm_maximize = CreateBitmapIndirect(&bm);
    bm.bmBits = bm_exit_bits;
    bm_exit = CreateBitmapIndirect(&bm);

    bm.bmBits = bm_plus_bits;
    bm.bmHeight = 16;
    bm_plus = CreateBitmapIndirect(&bm);

    bm.bmBits = bm_plus2_bits;
    bm.bmHeight = 11;
    bm_plus2 = CreateBitmapIndirect(&bm);

    //153, 182, 224

    uint32_t test[64];
    int xx = 0;
    while(xx < 8)
    {
        int y = 0;
        while(y < 8)
        {
            uint32_t value = 0xFFFFFF;
            if(xx + y >= 7)
            {
                int a = xx % 3, b = y % 3;
                if(a == 1)
                {
                    if(b == 0)
                    {
                        value = 0xB6B6B6;
                    }
                    else if(b == 1)
                    {
                        value = 0x999999;
                    }
                }
                else if(a == 0 && b == 1)
                {
                    value = 0xE0E0E0;
                }
            }

            test[y * 8 + xx] = value;
            y++;
        }
        xx++;
    }

    bm2.bmBits = test;
    bm_corner = CreateBitmapIndirect(&bm2);

    TEXTMETRIC tm;
    SelectObject(hdc, font_small);
    GetTextMetrics(hdc, &tm);

    font_small_lineheight = tm.tmHeight + tm.tmExternalLeading;


    //wait for tox_thread init
    while(!tox_thread_b) {Sleep(1);}

    list_init();
    draw = 1;
    drawall();

    while(GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    tox_thread_b = 0;

    //cleanup

    //delete tray icon
    Shell_NotifyIcon(NIM_DELETE, &nid);



    //wait for tox_thread cleanup
    while(!tox_thread_b) {Sleep(1);}


    printf("exit\n");

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }

        case WM_GETMINMAXINFO:
        {
            POINT min = {800, 320};
            ((MINMAXINFO*)lParam)->ptMinTrackSize = min;

            break;
        }

        case WM_CREATE:
        {
            main_hdc = GetDC(hwnd);
            hdc = CreateCompatibleDC(main_hdc);
            hdcMem = CreateCompatibleDC(hdc);

            return 0;
        }

        case WM_SIZE:
        {
            switch(wParam)
            {
                case SIZE_MAXIMIZED:
                {
                    maximized = 1;
                    break;
                }

                case SIZE_RESTORED:
                {
                    maximized = 0;
                    break;
                }
            }

            int w, h;

            w = GET_X_LPARAM(lParam);
            h = GET_Y_LPARAM(lParam);

            width = w;
            height = h;

            int x2 = (MAIN_X + 32 + 600) < (width - 24) ? MAIN_X + 32 + 600 : width - 24;

            edit_name.right = x2;
            edit_status.right = x2;

            edit_addid.right = x2;
            edit_addmsg.right = x2;

            button_addfriend.x = edit_addmsg.right - 50;
            button_addfriend.y = MAIN_Y + 222;

            edit_msg.y = height - 128;
            edit_msg.bottom = edit_msg.y + 80;
            edit_msg.right = width - 24;

            if(hdc_bm)
            {
                DeleteObject(hdc_bm);
            }

            hdc_bm = CreateCompatibleBitmap(main_hdc, width, height);
            SelectObject(hdc, hdc_bm);

            drawall();
            //commitdraw(0, 0, width, height);

            break;
        }

        case WM_ERASEBKGND:
        {
            return 1;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT ps;

            BeginPaint(hwnd, &ps);

            RECT r = ps.rcPaint;
            commitdraw(r.left, r.top, r.right - r.left, r.bottom - r.top);;

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_NCHITTEST:
        {
            POINT p = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

            ScreenToClient(hwnd, &p);

            return nc_hit(p.x, p.y);
        }

        case WM_KEYDOWN:
        {
            switch(wParam)
            {
                case VK_ESCAPE:
                {
                    if(sedit)
                    {
                        EDIT *edit = sedit;
                        sedit = NULL;
                        edit_draw(edit);
                    }
                    else
                    {
                        PostQuitMessage(0);
                    }
                    return 0;
                }

                case VK_DELETE:
                {
                    if(!sedit)
                    {
                        if(sitem)
                        {

                            if(sitem->item == ITEM_FRIEND)
                            {
                                FRIEND *f = sitem->data;

                                core_postmessage3(CMSG_DELFRIEND, (f - friend));

                                free(f->name);
                                free(f->status_message);
                                free(f->typed);

                                int i = 0;
                                while(i < f->msg)
                                {
                                    free(f->message[i]);
                                    i++;
                                }

                                free(f->message);

                                memset(f, 0, sizeof(FRIEND));//

                                friends--;

                                list_deletesitem();

                            }
                            else if(sitem->item == ITEM_GROUP)
                            {
                                GROUPCHAT *g = sitem->data;

                                core_postmessage3(CMSG_LEAVEGROUP, (g - group));

                                uint8_t **np = g->peername;
                                int i = 0;
                                while(i < g->peers)
                                {
                                    uint8_t *n = *np++;
                                    if(n)
                                    {
                                        free(n);
                                        i++;
                                    }
                                }

                                i = 0;
                                while(i < g->msg)
                                {
                                    free(g->message[i]);
                                    i++;
                                }

                                free(g->message);

                                memset(g, 0, sizeof(GROUP));//

                                list_deletesitem();
                            }
                        }
                    }

                    return 0;
                }
            }

            if(GetKeyState(VK_CONTROL) & 0x80)
            {
                switch(wParam)
                {
                    case 'C':
                    {
                        edit_copy();
                        break;
                    }

                    case 'V':
                    {
                        edit_paste();
                        break;
                    }

                    case 'A':
                    {
                        edit_selectall();
                        break;
                    }
                }
            }

            break;
        }

        case WM_CHAR:
        {
            edit_char(wParam);

            return 0;
        }

        case WM_MOUSELEAVE:
        {
            sysmenu_mouseleave();
            list_mouseleave();

            edit_func(edit_mouseleave);
            button_func(button_mouseleave);

            if(sitem && sitem->item == ITEM_FRIENDREQUESTS)
            {
                friendreqs_mouseleave();
            }

            mouse_tracked = 0;
            break;
        }

        case WM_MOUSEMOVE:
        {
            int x, y, dx, dy;

            x = GET_X_LPARAM(lParam);
            y = GET_Y_LPARAM(lParam);

            dx = x - mx;
            dy = y - my;

            mx = x;
            my = y;

            sysmenu_mousemove(x, y);
            list_mousemove(x, y, dy);

            edit_func(edit_mousemove, x, y);
            button_func(button_mousemove, x, y);

            if(sitem && sitem->item == ITEM_FRIENDREQUESTS)
            {
                friendreqs_mousemove(x, y);
            }

            if(!mouse_tracked)
            {
                TrackMouseEvent(&tme);
                mouse_tracked = 1;
            }

            break;
        }

        case WM_LBUTTONDOWN:
        {
            int x, y;

            x = GET_X_LPARAM(lParam);
            y = GET_Y_LPARAM(lParam);

            sysmenu_mousedown();
            list_mousedown();

            edit_func(edit_mousedown, x, y);
            button_func(button_mousedown, x, y);

            if(sitem && sitem->item == ITEM_FRIENDREQUESTS)
            {
                friendreqs_mousedown();
            }

            SetCapture(hwnd);

            break;
        }

        case WM_LBUTTONUP:
        {
            sysmenu_mouseup();
            list_mouseup();

            edit_func(edit_mouseup);
            button_func(button_mouseup);

            if(sitem && sitem->item == ITEM_FRIENDREQUESTS)
            {
                friendreqs_mouseup();
            }

            ReleaseCapture();

            break;
        }

        case WM_RBUTTONDOWN:
        {
            int x, y;

            x = GET_X_LPARAM(lParam);
            y = GET_Y_LPARAM(lParam);

            edit_func(edit_rightclick, x, y);

            break;
        }

        case WM_COMMAND:
        {
            int menu = LOWORD(wParam);//, msg = HIWORD(wParam);

            switch(menu)
            {
                case TRAY_SHOWHIDE:
                {
                    togglehide();
                    break;
                }

                case TRAY_EXIT:
                {
                    PostQuitMessage(0);
                    break;
                }

                case EDIT_CUT:
                {
                    edit_cut();
                    break;
                }

                case EDIT_COPY:
                {
                    edit_copy();
                    break;
                }

                case EDIT_PASTE:
                {
                    edit_paste();
                    break;
                }

                case EDIT_DELETE:
                {
                    edit_delete();
                    break;
                }

                case EDIT_SELECTALL:
                {
                    edit_selectall();
                    break;
                }
            }

            break;
        }

        case WM_NOTIFYICON:
        {
            int msg = LOWORD(lParam);

            switch(msg)
            {
                case WM_MOUSEMOVE:
                {
                    break;
                }

                case WM_LBUTTONDOWN:
                {
                    break;
                }

                case WM_LBUTTONUP:
                {
                    break;
                }

                case WM_RBUTTONDOWN:
                {
                    break;
                }

                case WM_RBUTTONUP:
                case WM_CONTEXTMENU:
                {
                    ShowContextMenu();
                    break;
                }

                case WM_LBUTTONDBLCLK:
                {
                    togglehide();
                    break;
                }


            }
            break;
        }

        case WM_FREQUEST:
        {
            FRIENDREQ **f = newfriendreq((void*)lParam + 2);
            if(!f)
            {
                break;
            }

            *f = (void*)lParam;

            list_draw();

            if(sitem && sitem->item == ITEM_FRIENDREQUESTS)
            {
                if(!sreq)
                {
                    sreq = *f;
                }
                main_draw();
            }

            break;
        }

        case WM_FADD:
        {
            int r = wParam;
            uint8_t *id = (void*)lParam;

            if(r >= 0)
            {
                edit_addid.length = 0;
                edit_addmsg.length = 0;
                edit_draw(&edit_addid);
                edit_draw(&edit_addmsg);

                FRIEND *f = &friend[r];
                friends++;

                f->name_length = TOX_FRIEND_ADDRESS_SIZE * 2;
                f->name = malloc(TOX_FRIEND_ADDRESS_SIZE * 2);
                sprint_address(f->name, id);

                list_addfriend(f);

                addfriend_status = 1;
                main_draw();
            }
            else
            {
                switch(r)
                {
                    default:
                    {
                        addfriend_status = 3;
                        main_draw();
                        break;
                    }
                }
            }

            free(id);

            break;
        }

        case WM_FACCEPT:
        {
            int r = wParam;
            if(r != -1)
            {
                FRIEND *f = &friend[r];
                friends++;

                uint8_t *id = (uint8_t*)lParam;

                f->name_length = TOX_FRIEND_ADDRESS_SIZE * 2;
                f->name = malloc(TOX_FRIEND_ADDRESS_SIZE * 2);
                sprint_address(f->name, id);

                free(id);

                list_addfriend(f);
            }

            break;
        }

        case WM_FMESSAGE:
        case WM_FACTION:
        {
            FRIEND *f = &friend[wParam];

            f->message = realloc(f->message, (f->msg + 1) * sizeof(void*));
            f->message[f->msg++] = (void*)lParam;

            if(sitem && f == sitem->data)
            {
                main_draw();
            }

            break;
        }

        case WM_FNAME:
        case WM_FSTATUSMSG:
        {
            uint8_t *str;
            uint16_t length, fid;
            FRIEND *f;

            length = wParam;
            fid = wParam >> 16;
            str = (uint8_t*)lParam;

            f = &friend[fid];

            if(msg == WM_FNAME)
            {
                free(f->name);
                f->name_length = length;
                f->name = str;
            }
            else
            {
                free(f->status_message);
                f->status_length = length;
                f->status_message = str;
            }

            updatefriend(f);

            break;
        }

        case WM_FSTATUS:
        {
            FRIEND *f = &friend[wParam];

            f->status = lParam;

            updatefriend(f);

            break;
        }

        case WM_FTYPING:
        {
            FRIEND *f = &friend[wParam];

            f->typing = lParam;

            updatefriend(f);

            break;
        }

        case WM_FRECEIPT:
        {
            //receipt = lParam
            break;
        }

        case WM_FONLINE:
        {
            FRIEND *f = &friend[wParam];

            f->online = lParam;

            updatefriend(f);

            break;
        }

        case WM_GADD:
        {
            GROUPCHAT *g = &group[wParam];

            g->name_length = sprintf((char*)g->name, "Groupchat #%u", wParam);

            list_addgroup(g);
            break;
        }

        case WM_GMESSAGE:
        case WM_GACTION:
        {
            GROUPCHAT *g = &group[wParam];

            g->message = realloc(g->message, (g->msg + 1) * sizeof(void*));
            g->message[g->msg++] = (void*)lParam;

            if(sitem && g == sitem->data)
            {
                main_draw();
            }

            break;
        }

        case WM_GPEERNAME:
        {
            int groupnumber = wParam >> 16, peernumber = wParam & 0xFFFF;

            GROUPCHAT *g = &group[groupnumber];

            free(g->peername[peernumber]);
            g->peername[peernumber] = (uint8_t*)lParam;

            //printf("name: %s\n", lParam);

            if(sitem && g == sitem->data)
            {
                main_draw();
            }
            break;
        }

        case WM_GPEERADD:
        {
            int groupnumber = wParam >> 16, peernumber = wParam & 0xFFFF;

            GROUPCHAT *g = &group[groupnumber];

            if(g->peername[peernumber])
            {
                free(g->peername[peernumber]);
            }
            else
            {
                g->peers++;
            }

            g->topic_length = sprintf((char*)g->topic, "%u users in chat", g->peers);

            uint8_t *n = malloc(10);
            n[0] = 9;
            memcpy(n + 1, "<unknown>", 9);
            g->peername[peernumber] = n;

            updategroup(g);

            break;
        }

        case WM_GPEERDEL:
        {
            int groupnumber = wParam >> 16, peernumber = wParam & 0xFFFF;

            GROUPCHAT *g = &group[groupnumber];

            if(g->peername[peernumber])
            {
                g->peers--;
            }

            free(g->peername[peernumber]);
            g->peername[peernumber] = NULL;

            g->topic_length = sprintf((char*)g->topic, "%u users in chat", g->peers);

            updategroup(g);

            break;
        }
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}
