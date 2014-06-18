#include "main.h"

static EDIT *active_edit;

static struct
{
    uint16_t start, length;
    uint16_t p1, p2, pm;
}edit_sel;
static _Bool edit_select;

void edit_draw(EDIT *edit, int x, int y, int width, int height)
{
    framerect(x, y, x + width, y + height, (edit == active_edit) ? BLUE : (edit->mouseover ? C_GRAY2 : C_GRAY));
    drawrect(x + 1, y + 1, x + width - 1, y + height - 1, WHITE);

    setfont(FONT_TEXT);
    setcolor(COLOR_TEXT);

    drawtextrangecut(x + 5, x + width - 5, y + 5, edit->data, edit->length);

    if(edit == active_edit) {
        int x1 = textwidth(edit->data, edit_sel.start);
        int w = textwidth(edit->data + edit_sel.start, edit_sel.length);

        setcolor(TEXT_HIGHLIGHT);

        if(edit_sel.length) {
            drawrectw(x + 5 + x1, y + 5, w, 14, TEXT_HIGHLIGHT_BG);
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
                uint8_t len = utf8_len(edit->data + fit);
                x1 = textwidth(edit->data, fit);
                x2 = textwidth(edit->data, fit + len);

                if(x2 - extent < extent - x1) {
                    fit += len;
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

        //debug("%u %u\n", edit_sel.start, edit_sel.length);

        redraw = 1;
    } else if(mouseover) {
        int fit = 0, extent = x - 5, x1, x2;

        setfont(FONT_TEXT);
        fit = textfit(edit->data, edit->length, extent);

        if(fit != edit->length) {
            uint8_t len = utf8_len(edit->data + fit);
            x1 = textwidth(edit->data, fit);
            x2 = textwidth(edit->data, fit + len);

            if(x2 - extent < extent - x1) {
                fit += len;
            }
        }

        edit->mouseover_char = fit;
    }

    return redraw;
}

_Bool edit_mdown(EDIT *edit)
{
    if(edit->mouseover_char > edit->length) {
        edit->mouseover_char = edit->length;
    }

    if(edit->mouseover) {
        edit_sel.start = edit_sel.p1 = edit_sel.p2 = edit->mouseover_char;
        edit_sel.length = 0;
        edit_select = 1;

        active_edit = edit;
        return 1;
    } else if(edit == active_edit) {
        edit_resetfocus();
    }

    return 0;
}

_Bool edit_dclick(EDIT *edit, _Bool triclick)
{
    if(edit != active_edit) {
        return 0;
    }

    if(edit->mouseover_char > edit->length) {
        edit->mouseover_char = edit->length;
    }

    uint8_t c = triclick ? '\n' : ' ';

    uint16_t i = edit->mouseover_char;
    while(i != 0 && edit->data[i - 1] != c) {
        i -= utf8_unlen(edit->data + i);
    }
    edit_sel.start = edit_sel.p1 = i;
    i = edit->mouseover_char;
    while(i != edit->length && edit->data[i] != c) {
        i += utf8_len(edit->data + i);
    }
    edit_sel.p2 = i;
    edit_sel.length = i - edit_sel.start;

    return 1;
}

_Bool edit_mright(EDIT *edit)
{
    if(edit->mouseover) {
        EDIT *active = active_edit;
        if(active != edit) {
            active_edit = edit;

            edit_sel.start = edit_sel.p1 = edit_sel.p2 = edit->mouseover_char;
            edit_sel.length = 0;
            edit_select = 1;
        }

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

static uint8_t unicode_to_utf8(uint32_t ch, char_t *dst, uint16_t *len)
{
    if (!dst) {
        if (ch > 0x1FFFFF) {
            return 0;
        }
        return 4 - (ch <= 0xFFFF) - (ch <= 0x7FF) - (ch <= 0x7F);
    }
    uint32_t HB = (uint32_t)0x80;
    uint32_t SB = (uint32_t)0x3F;
    if (ch <= 0x7F) {
        dst[0] = (uint8_t)ch;
        *len += 1;
        return 1;
    }
    if (ch <= 0x7FF) {
        dst[0] = (uint8_t)((ch >> 6) | (uint32_t)0xC0);
        dst[1] = (uint8_t)((ch & SB) | HB);
        *len += 2;
        return 2;
    }
    if (ch <= 0xFFFF) {
        dst[0] = (uint8_t)((ch >> 12) | (uint32_t)0xE0);
        dst[1] = (uint8_t)(((ch >> 6) & SB) | HB);
        dst[2] = (uint8_t)((ch & SB) | HB);
        *len += 3;
        return 3;
    }
    if (ch <= 0x1FFFFF) {
        dst[0] = (uint8_t)((ch >> 18) | (uint32_t)0xF0);
        dst[1] = (uint8_t)(((ch >> 12) & SB) | HB);
        dst[2] = (uint8_t)(((ch >> 6) & SB) | HB);
        dst[3] = (uint8_t)((ch & SB) | HB);
        *len += 4;
        return 4;
    }
    return 0;
}

void edit_char(uint32_t ch, _Bool control)
{
    EDIT *edit = active_edit;

    if(control || ch <= 0x1F || (ch >= 0x7f && ch <= 0x9F)) {
        switch(ch) {
        case KEY_BACK: {
            if(edit_sel.length == 0) {
                if(edit_sel.start != 0) {
                    uint8_t len = utf8_unlen(edit->data + edit_sel.start);
                    memmove(edit->data + edit_sel.start - len, edit->data + edit_sel.start,
                            (edit->length - edit_sel.start) * sizeof(char_t));
                    edit->length -= len;

                    edit_sel.start -= len;
                }
            } else {
                edit_delete();
            }
            break;
        }

        case KEY_LEFT: {
            if(edit_sel.start != 0) {
                edit_sel.start--;
            }
            edit_sel.length = 0;
            break;
        }

        case KEY_RIGHT: {
            if(edit_sel.start != edit->length) {
                edit_sel.start++;
            }
            edit_sel.length = 0;
            break;
        }

        case KEY_RETURN: {
            if(edit->onenter) {
                edit->onenter();
            }
            break;
        }

        case KEY_TAB: {
            if(edit->ontab) {
                edit->ontab();
            }
            break;
        }

        }

        if(edit_sel.start + edit_sel.length > edit->length) {
            if(edit_sel.start > edit->length) {
                edit_sel.start = edit->length;
                edit_sel.length = 0;
            } else {
                edit_sel.length = edit->length - edit_sel.start;
            }
        }

        if(edit_sel.p1 >= edit->length) {
            edit_sel.p1 = edit->length;
        }

        if(edit_sel.p2 >= edit->length) {
            edit_sel.p2 = edit->length;
        }

        redraw();
    } else {
        size_t len = unicode_to_utf8(ch, NULL, NULL);
        if(edit->length - edit_sel.length + len <= edit->maxlength) {
            char_t *p = edit->data + edit_sel.start;
            memmove(p + len, p + edit_sel.length, (edit->length - (edit_sel.start + edit_sel.length)) * sizeof(char_t));
            edit->length -= edit_sel.length;
            unicode_to_utf8(ch, edit->data + edit_sel.start, &edit->length);

            edit_sel.start += len;
            edit_sel.p1 = edit->mouseover_char = edit_sel.start;
            edit_sel.length = 0;

            redraw();
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

void edit_paste(char_t *data, int length)
{
    if(!active_edit) {
        return;
    }

    length = utf8_validate(data, length);

    uint8_t lens[length];
    size_t num_chars = 0;
    //paste only utf8
    size_t i = 0;
    while(i < length) {
        if(data[i] <= 0x1F || data[i] == 0x7F) {
            // Control characters.
            break;
        }

        uint8_t len = utf8_len(data + i);

        if(len == 2 && data[i] == 0xc2 && data[i+1] <= 0x9f) {
            // More control characters.
            break;
        }

        lens[num_chars++] = len;
        i += len;
    }

    // Invariant: active_edit->length <= active_edit->maxlength
    int newlen = active_edit->length + i;
    while(newlen > active_edit->maxlength) {
        i -= lens[--num_chars];
        newlen = active_edit->length + i;
    }

    uint8_t *p = active_edit->data + edit_sel.start;

    memmove(p + i, p + edit_sel.length, (active_edit->length - (edit_sel.start + edit_sel.length)) * sizeof(char_t));
    memcpy(p, data, i * sizeof(char_t));

    active_edit->length += i - edit_sel.length;

    edit_sel.start = edit_sel.start + i;
    edit_sel.length = 0;

    redraw();
}

void edit_delete(void)
{
    uint8_t *p = active_edit->data + edit_sel.start;

    memmove(p, p + edit_sel.length, (active_edit->length - (edit_sel.start + edit_sel.length)) * sizeof(char_t));
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
