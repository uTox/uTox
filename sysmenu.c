
enum
{
    SYSMENU_NONE,
    SYSMENU_EXIT,
    SYSMENU_SIZE,
    SYSMENU_MINI,
};

static _Bool sysmenu_down;
static uint8_t sysmenu_active, sysmenu_over;

static void sysmenu_button(RECT *r, HBITMAP bm, uint32_t c1, uint32_t c2, uint32_t c3, uint32_t c4, uint8_t menu)
{
    if(sysmenu_active == menu)
    {
        if(sysmenu_down)
        {
            fillrect(r, c2);
            SetTextColor(hdc, c2);
            SetBkColor(hdc, c4);
        }
        else
        {
            fillrect(r, c1);
            SetTextColor(hdc, c1);
            SetBkColor(hdc, c3);
        }
    }
    else
    {
        fillrect(r, WHITE);
        SetBkColor(hdc, 0xCCCCCC);
        SetTextColor(hdc, WHITE);
    }

    SelectObject(hdcMem, bm);
    BitBlt(hdc, r->left + 7, r->top + 8, 16, 10, hdcMem, 0, 0, SRCCOPY);
}

static uint8_t sysmenu_hit(int x, int y)
{
    y = y - 1;
    x = (width - BORDER) - x - 1;
    if(y >= 26 || y < 0 || x < 0)
    {
        return 0;
    }

    x /= 30;
    if(x >= SYSMENU_MINI)
    {
        return 0;
    }

    return x + 1;
}

void sysmenu_draw(void)
{
    RECT r = {width - BORDER - 30, BORDER, width - BORDER, BORDER + 26};


    sysmenu_button(&r, bm_exit, RED, RED2, WHITE, WHITE, SYSMENU_EXIT);
    r.left -= 30; r.right -= 30;

    sysmenu_button(&r, maximized ? bm_restore : bm_maximize, GRAY, BLUE, 0x333333, WHITE, SYSMENU_SIZE);
    r.left -= 30; r.right -= 30;

    sysmenu_button(&r, bm_minimize, GRAY, BLUE, 0x333333, WHITE, SYSMENU_MINI);

    commitdraw(width - BORDER - 90, BORDER, width - BORDER, BORDER + 26);
}

void sysmenu_mousemove(int x, int y)
{
    uint8_t sm = sysmenu_hit(x, y);

    if(!(GetKeyState(VK_LBUTTON) & 0x80))
    {
        if(sm != sysmenu_active)
        {
            sysmenu_active = sm;
            sysmenu_draw();
        }
    }
    else
    {
        _Bool md = (sm == sysmenu_active);
        if(md != sysmenu_down)
        {
            sysmenu_down = md;
            sysmenu_draw();
        }
    }

    sysmenu_over = sm;
}

void sysmenu_mousedown(void)
{
    if(sysmenu_active)
    {
        if(!sysmenu_down)
        {
            sysmenu_down = 1;
            sysmenu_draw();
        }
    }
}

void sysmenu_mouseup(void)
{
    if(sysmenu_active == sysmenu_over)
    {
        switch(sysmenu_active)
        {
            case SYSMENU_EXIT:
            {
                PostQuitMessage(0);
                break;
            }

            case SYSMENU_SIZE:
            {
                ShowWindow(hwnd, maximized ? SW_RESTORE : SW_MAXIMIZE);
                break;
            }

            case SYSMENU_MINI:
            {
                ShowWindow(hwnd, SW_MINIMIZE);
                break;
            }
        }
    }

    if(sysmenu_active || sysmenu_over)
    {
        sysmenu_active = sysmenu_over;
        sysmenu_draw();
    }

    sysmenu_down = 0;
}

void sysmenu_mouseleave(void)
{
    if(sysmenu_active)
    {
        sysmenu_active = 0;
        sysmenu_draw();
    }
}

