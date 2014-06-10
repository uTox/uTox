#include "main.h"

static EDIT *active_edit;

void edit_draw(EDIT *edit, int x, int y, int width, int height)
{
    RECT outline = {x, y, x + width, y + height};
    framerect(&outline, (edit == active_edit) ? BLUE : (edit->mouseover ? GRAY6 : GRAY5));

    RECT area = {x + 1, y + 1, x + width - 1, y + height - 1};
    fillrect(&area, WHITE);

    setfont(FONT_TEXT);
    setcolor(COLOR_TEXT);

    drawtextrangecut(x + 5, x + width - 5, y + 5, edit->data, edit->length);

    if(edit == active_edit) {
        int x1 = textwidth(edit->data, edit_sel.start);
        int w = textwidth(edit->data + edit_sel.start, edit_sel.length);

        setcolor(TEXT_HIGHLIGHT);

        if(edit_sel.length) {
            drawrect(x + 5 + x1, y + 5, w, 14, TEXT_HIGHLIGHT_BG);
            drawtextrangecut(x + 5 + x1, x + width - 5, y + 5, edit->data + edit_sel.start, edit_sel.length);
        } else {
            drawvline(x + 5 + x1, y + 5, y + 19, BLACK);
        }
    }
}

_Bool edit_mmove(EDIT *edit, int x, int y, int dy, int width, int height)
{
    _Bool redraw = 0;

    _Bool mouseover = inrect(x, y, 0, 0, width, height);
    if(mouseover != edit->mouseover) {
        edit->mouseover = mouseover;
        if(edit != active_edit) {
            redraw = 1;
        }
    }

    if(edit == active_edit && edit_select) {
        int fit = 0, extent = x - 5, x1, x2;

        setfont(FONT_TEXT);

        if(extent > 0) {
            fit = textfit(edit->data, edit->length, extent);

            if(fit != edit->length) {
                x1 = textwidth(edit->data, fit);
                x2 = textwidth(edit->data, fit + 1);

                if(x2 - extent < extent - x1) {
                    fit++;
                }
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

        redraw = 1;
    } else if(mouseover) {
        int fit = 0, extent = x - 5, x1, x2;

        setfont(FONT_TEXT);
        fit = textfit(edit->data, edit->length, extent);

        if(fit != edit->length) {
            x1 = textwidth(edit->data, fit);
            x2 = textwidth(edit->data, fit + 1);

            if(x2 - extent < extent - x1) {
                fit++;
            }
        }

        /*GetTextExtentExPoint(hdc, (char*)edit->data, edit->length, extent, &fit, NULL, &size);

        GetTextExtentPoint32(hdc, (char*)edit->data, fit, &size);
        int sx = size.cx;

        GetTextExtentPoint32(hdc, (char*)edit->data, fit + 1, &size);
        if(fit != edit->length && size.cx - extent < extent - sx) {
            fit += 1;
        }*/

        edit->mouseover_char = fit;
    }

    return redraw;
}

_Bool edit_mdown(EDIT *edit)
{
    if(edit->mouseover) {
        edit_sel.start = edit_sel.p1 = edit_sel.p2 = edit->mouseover_char;
        edit_sel.length = 0;
        edit_select = 1;

        active_edit = edit;
        return 1;
    } else if(edit == active_edit)
    {
        edit_resetfocus();
        return 1;
    }

    return 0;
}

_Bool edit_mright(EDIT *edit)
{
    if(edit->mouseover) {
        EDIT *active = active_edit;
        active_edit = edit;

        editpopup();

        if(active != edit) {
            return 1;
        }

    }

    return 0;
}

_Bool edit_mwheel(EDIT *edit, int height, double d)
{
    return 0;
}

_Bool edit_mup(EDIT *edit)
{
    if(edit_select) {
        edit_select = 0;
    }

    return 0;
}

_Bool edit_mleave(EDIT *edit)
{
    if(edit->mouseover) {
        edit->mouseover = 0;
        return 1;
    }

    return 0;
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

            redraw();
        }
    } else {
        switch(ch) {
        case KEY_BACK: {
            if(edit_sel.length == 0) {
                if(edit_sel.start != 0) {
                    memmove(edit->data + edit_sel.start - 1, edit->data + edit_sel.start, edit->length - edit_sel.start);
                    edit->length--;

                    edit_sel.start--;
                }
            } else {
                edit_delete();
            }

            redraw();
            break;
        }

        case KEY_RETURN: {
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

    /*HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, length + 1);
    uint8_t *p = GlobalLock(hMem);
    memcpy(p, active_edit->data + edit_sel.start, length);
    p[length] = 0;
    GlobalUnlock(hMem);
    OpenClipboard(0);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();*/
}

void edit_paste(uint8_t *data, int length)
{
    if(!active_edit) {
        return;
    }

    uint8_t str[length], *s = str, c;
    //paste only allowed characters
    while((c = *data++)) {
        if(c >= ' ' && c <= 126) {
            *s++ = c;
        } else {
            length--;
        }
    }

    int newlen = active_edit->length + length;
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

    redraw();
}

void edit_delete(void)
{
    uint8_t *p = active_edit->data + edit_sel.start;

    memmove(p, p + edit_sel.length, active_edit->length - (edit_sel.start + edit_sel.length));
    active_edit->length -= edit_sel.length;

    edit_sel.length = 0;

    redraw();
}

void edit_selectall(void)
{
    edit_sel.start = 0;
    edit_sel.length = active_edit->length;
    edit_select = 0;

    redraw();
}

void edit_clear(void)
{
    active_edit->length = 0;
    edit_sel.start = 0;
    edit_sel.length = 0;

    redraw();
}

void edit_resetfocus(void)
{
    edit_select = 0;
    active_edit = NULL;
}

void edit_setfocus(EDIT *edit)
{
    edit_select = 0;
    active_edit = NULL;
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
