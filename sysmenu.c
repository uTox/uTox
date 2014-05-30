#include "main.h"

enum {
    SYSMENU_NONE,
    SYSMENU_EXIT,
    SYSMENU_SIZE,
    SYSMENU_MINI,
};

static _Bool sysmenu_down;
static uint8_t sysmenu_active, sysmenu_over;

static void sysmenu_button(RECT *area, int bm, uint32_t bg_hover, uint32_t bg_down, uint32_t fg_hover, uint32_t fg_down, uint8_t menu)
{
    if(sysmenu_active == menu) {
        if(sysmenu_down) {
            fillrect(area, bg_down);
            setcolor(bg_down);
            setbkcolor(fg_down);
        } else {
            fillrect(area, bg_hover);
            setcolor(bg_hover);
            setbkcolor(fg_hover);
        }
    } else {
        fillrect(area, COLOR_BG);
        setcolor(COLOR_BG);
        setbkcolor(COLOR_SYSMENU);
    }

    drawbitmap(bm, area->left + 7, area->top + 8, 16, 10);
}

uint8_t sysmenu_hit(int x, int y)
{
    y = y - 1;
    x = (width - BORDER) - x - 1;
    if(y >= 26 || y < 0 || x < 0) {
        return 0;
    }

    x /= 30;
    if(x >= SYSMENU_MINI) {
        return 0;
    }

    return x + 1;
}

void sysmenu_draw(void)
{
    RECT r = {width - BORDER - 30, BORDER, width - BORDER, BORDER + 26};


    sysmenu_button(&r, BM_EXIT, RED, RED2, WHITE, WHITE, SYSMENU_EXIT);
    r.left -= 30;
    r.right -= 30;

    sysmenu_button(&r, maximized ? BM_RESTORE : BM_MAXIMIZE, GRAY, BLUE, 0x333333, WHITE, SYSMENU_SIZE);
    r.left -= 30;
    r.right -= 30;

    sysmenu_button(&r, BM_MINIMIZE, GRAY, BLUE, 0x333333, WHITE, SYSMENU_MINI);

    commitdraw(width - BORDER - 90, BORDER, width - BORDER, BORDER + 26);
}

void sysmenu_mousemove(int x, int y)
{
    uint8_t sm = sysmenu_hit(x, y);

    if(!(GetKeyState(VK_LBUTTON) & 0x80)) {
        if(sm != sysmenu_active) {
            sysmenu_active = sm;
            sysmenu_draw();
        }
    } else {
        _Bool md = (sm == sysmenu_active);
        if(md != sysmenu_down) {
            sysmenu_down = md;
            sysmenu_draw();
        }
    }

    sysmenu_over = sm;
}

void sysmenu_mousedown(void)
{
    if(sysmenu_active) {
        if(!sysmenu_down) {
            sysmenu_down = 1;
            sysmenu_draw();
        }
    }
}

void sysmenu_mouseup(void)
{
    if(sysmenu_active == sysmenu_over) {
        switch(sysmenu_active) {
        case SYSMENU_EXIT: {
            PostQuitMessage(0);
            break;
        }

        case SYSMENU_SIZE: {
            ShowWindow(hwnd, maximized ? SW_RESTORE : SW_MAXIMIZE);
            break;
        }

        case SYSMENU_MINI: {
            ShowWindow(hwnd, SW_MINIMIZE);
            break;
        }
        }
    }

    if(sysmenu_active || sysmenu_over) {
        sysmenu_active = sysmenu_over;
        sysmenu_draw();
    }

    sysmenu_down = 0;
}

void sysmenu_mouseleave(void)
{
    if(sysmenu_active) {
        sysmenu_active = 0;
        sysmenu_draw();
    }
}

