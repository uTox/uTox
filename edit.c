#include "main.h"

static EDIT *active_edit;

static _Bool inedit(EDIT *edit, int x, int y)
{
    return (x >= edit->x && x <= edit->right && y >= edit->y && y <= edit->bottom);
}

void edit_draw(EDIT *edit)
{
    RECT outline = {edit->x, edit->y, edit->right, edit->bottom};
    framerect(&outline, (edit == active_edit) ? BLUE : (edit->mouseover ? GRAY6 : GRAY5));

    RECT area = {edit->x + 1, edit->y + 1, edit->right - 1, edit->bottom - 1};
    fillrect(&area, WHITE);

    setfont(FONT_TEXT);
    setcolor(COLOR_TEXT);

    drawtextrangecut(edit->x + 5, edit->right - 5, edit->y + 5, edit->data, edit->length);

    if(edit == active_edit) {
        SIZE size;
        GetTextExtentPoint32(hdc, (char*)edit->data, edit_sel.start, &size);

        setbgcolor(TEXT_HIGHLIGHT_BG);
        setcolor(TEXT_HIGHLIGHT);

        if(edit_sel.length)
        {
            drawtextrangecut(edit->x + 5 + size.cx, edit->right - 5, edit->y + 5, edit->data + edit_sel.start, edit_sel.length);
        }
        else
        {
            drawvline(edit->x + 5 + size.cx, edit->y + 5, edit->y + 19, BLACK);
        }

        setbgcolor(~0);
    }

    commitdraw(edit->x, edit->y, edit->right - edit->x, edit->bottom - edit->y);
}

void edit_mousemove(EDIT *edit, int x, int y)
{
    _Bool mouseover = inedit(edit, x, y);
    if(mouseover != edit->mouseover) {
        edit->mouseover = mouseover;
        if(edit != active_edit)
        {
            edit->onredraw();
        }
    }

    if(edit == active_edit && edit_select) {
        int fit = 0, extent;
        SIZE size;
        SelectObject(hdc, font_small);

        extent = x - (edit->x + 5);
        if(extent > 0) {
            GetTextExtentExPoint(hdc, (char*)edit->data, edit->length, extent, &fit, NULL, &size);

            GetTextExtentPoint32(hdc, (char*)edit->data, fit, &size);
            int sx = size.cx;
            GetTextExtentPoint32(hdc, (char*)edit->data, fit + 1, &size);
            if(fit != edit->length && size.cx - extent < extent - sx)
            {
                fit += 1;
            }
        }

        edit_sel.p2 = fit;
        if(edit_sel.p2 > edit_sel.p1) {
            edit_sel.start = edit_sel.p1;
            edit_sel.length = edit_sel.p2 - edit_sel.p1;
        } else {
            edit_sel.start = edit_sel.p2;
            edit_sel.length = edit_sel.p1 - edit_sel.p2;
        }

        edit->onredraw();
    }
}

void edit_mousedown(EDIT *edit, int x, int y)
{
    if(inedit(edit, x, y)) {
        int fit, extent = x - (edit->x + 5);
        SIZE size;
        SelectObject(hdc, font_small);
        GetTextExtentExPoint(hdc, (char*)edit->data, edit->length, extent, &fit, NULL, &size);

        GetTextExtentPoint32(hdc, (char*)edit->data, fit, &size);
        int sx = size.cx;
        GetTextExtentPoint32(hdc, (char*)edit->data, fit + 1, &size);
        if(fit != edit->length && size.cx - extent < extent - sx)
        {
            fit += 1;
        }

        edit_sel.start = edit_sel.p1 = edit_sel.p2 = fit;
        edit_sel.length = 0;
        edit_select = 1;

        if(active_edit != edit) {
            edit_setfocus(edit);
        } else {
            edit->onredraw();
        }
    } else if(active_edit == edit) {
        edit_setfocus(NULL);
    }
}

void edit_mouseup(EDIT *edit)
{
    if(edit_select) {
        edit_select = 0;
    }

}

void edit_mouseleave(EDIT *edit)
{
    if(edit->mouseover) {
        edit->mouseover = 0;
        edit->onredraw();
    }
}

void edit_rightclick(EDIT *edit, int x, int y)
{
    if(inedit(edit, x, y)) {
        if(active_edit != edit) {
            edit_mousedown(edit, x, y);
        }

        POINT p;
        GetCursorPos(&p);

        HMENU hMenu = CreatePopupMenu();
        if(hMenu) {
            _Bool emptysel = (edit_sel.length == 0);

            InsertMenu(hMenu, -1, MF_BYPOSITION | (emptysel ? MF_GRAYED : 0), EDIT_CUT, "Cut");
            InsertMenu(hMenu, -1, MF_BYPOSITION | (emptysel ? MF_GRAYED : 0), EDIT_COPY, "Copy");
            InsertMenu(hMenu, -1, MF_BYPOSITION, EDIT_PASTE, "Paste");
            InsertMenu(hMenu, -1, MF_BYPOSITION | (emptysel ? MF_GRAYED : 0), EDIT_DELETE, "Delete");
            InsertMenu(hMenu, -1, MF_BYPOSITION, EDIT_SELECTALL, "Select All");

            SetForegroundWindow(hwnd);

            TrackPopupMenu(hMenu, TPM_TOPALIGN, p.x, p.y, 0, hwnd, NULL);
            DestroyMenu(hMenu);
        }

    }
}

void edit_char(uint32_t ch)
{
    EDIT *edit = active_edit;

    if(ch >= ' ' && ch <= 126) {
        if(edit->length != edit->maxlength || edit_sel.length > 0) {
            uint8_t *p = edit->data + edit_sel.start;
            if(edit_sel.length == 0) {
                memmove(p + 1, p, edit->length - edit_sel.start);

                edit->data[edit_sel.start] = ch;
                edit->length++;
            } else {
                edit->data[edit_sel.start] = ch;

                memmove(p + 1, p + edit_sel.length, edit->length - (edit_sel.start + edit_sel.length));
                edit->length -= edit_sel.length - 1;
            }

            edit_sel.start++;
            edit_sel.length = 0;

            edit->onredraw();;
        }
    } else {
        switch(ch) {
        case VK_BACK: {
            if(edit_sel.length == 0) {
                if(edit_sel.start != 0) {
                    memmove(edit->data + edit_sel.start - 1, edit->data + edit_sel.start, edit->length - edit_sel.start);
                    edit->length--;

                    edit_sel.start--;
                }
            } else {
                edit_delete();
            }

            edit->onredraw();
            break;
        }

        case VK_RETURN: {
            if(edit->onenter) {
                edit->onenter();
            }
            break;
        }
        }
    }
}

void edit_cut(void)
{
    edit_copy();
    edit_delete();
}

void edit_copy(void)
{
    uint16_t length = edit_sel.length;

    if(!active_edit || length == 0) {
        return;
    }

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, length + 1);
    uint8_t *p = GlobalLock(hMem);
    memcpy(p, active_edit->data + edit_sel.start, length);
    p[length] = 0;
    GlobalUnlock(hMem);
    OpenClipboard(0);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();
}

void edit_paste(void)
{
    if(!active_edit) {
        return;
    }

    OpenClipboard(NULL);
    char *cd = GetClipboardData(CF_TEXT);
    int length = strlen(cd);
    char str[length], *s = str, c;
    //paste only allowed characters
    while((c = *cd++)) {
        if(c >= ' ' && c <= 126) {
            *s++ = c;
        } else {
            length--;
        }
    }
    CloseClipboard();

    int newlen = (int)active_edit->length + length;
    if(newlen > active_edit->maxlength) {
        //for now just return if paste doesnt fit
        return;
    }

    uint8_t *p = active_edit->data + edit_sel.start;

    memmove(p + length, p + edit_sel.length, active_edit->length - (edit_sel.start + edit_sel.length));
    memcpy(p, str, length);

    active_edit->length += length - edit_sel.length;

    edit_sel.start = edit_sel.start + length;
    edit_sel.length = 0;

    active_edit->onredraw();
}

void edit_delete(void)
{
    uint8_t *p = active_edit->data + edit_sel.start;

    memmove(p, p + edit_sel.length, active_edit->length - (edit_sel.start + edit_sel.length));
    active_edit->length -= edit_sel.length;

    edit_sel.length = 0;

    active_edit->onredraw();
}

void edit_selectall(void)
{
    edit_sel.start = 0;
    edit_sel.length = active_edit->length;
    edit_select = 0;

    active_edit->onredraw();
}

void edit_clear(void)
{
    active_edit->length = 0;
    edit_sel.start = 0;
    edit_sel.length = 0;

    active_edit->onredraw();
}

void edit_setfocus(EDIT *edit)
{
    EDIT *s = active_edit;
    active_edit = edit;
    if(s) {
        //onlosefocus()
        if(s == &edit_name || s == &edit_status) {
            s->onenter();
        }

        s->onredraw();
    }

    if(edit && edit != s) {
        edit->onredraw();
    }
}

_Bool edit_active(void)
{
    return (active_edit != NULL);
}

void edit_setstr(EDIT *edit, uint8_t *str, uint16_t length)
{
    if(length >= edit->maxlength) {
        length = edit->maxlength;
    }

    edit->length = length;
    memcpy(edit->data, str, length);
}
