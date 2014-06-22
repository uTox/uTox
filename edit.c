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

    _Bool a = (edit == active_edit);
    drawtextmultiline(x + 2 * SCALE, x + width - 2 * SCALE, y + 2 * SCALE, font_small_lineheight, edit->data, edit->length,
                      a ? edit_sel.start : 0xFFFF, a ? edit_sel.length : 0xFFFF, edit->multiline);
}

_Bool edit_mmove(EDIT *edit, int x, int y, int dy, int width, int height)
{
    _Bool redraw = 0;

    _Bool mouseover = inrect(x, y, 0, 0, width, height);
    if(mouseover) {
        overtext = 1;
    }
    if(mouseover != edit->mouseover) {
        edit->mouseover = mouseover;
        if(edit != active_edit) {
            redraw = 1;
        }
    }

    if(edit == active_edit && edit_select) {
        setfont(FONT_TEXT);

        edit_sel.p2 = hittextmultiline(x - 2 * SCALE, width - 4 * SCALE, y - 2 * SCALE, height, font_small_lineheight, edit->data, edit->length, edit->multiline);
        if(edit_sel.p2 > edit_sel.p1) {
            edit_sel.start = edit_sel.p1;
            edit_sel.length = edit_sel.p2 - edit_sel.p1;
        } else {
            edit_sel.start = edit_sel.p2;
            edit_sel.length = edit_sel.p1 - edit_sel.p2;
        }

        redraw = 1;
    } else if(mouseover) {
        setfont(FONT_TEXT);

        edit->mouseover_char = hittextmultiline(x - 2 * SCALE, width - 4 * SCALE, y - 2 * SCALE, height, font_small_lineheight, edit->data, edit->length, edit->multiline);
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

    if(control || (ch <= 0x1F && (!edit->multiline || ch != '\n')) || (ch >= 0x7f && ch <= 0x9F)) {
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
                edit_sel.start -= utf8_unlen(edit->data + edit_sel.start);
            }
            edit_sel.length = 0;
            break;
        }

        case KEY_RIGHT: {
            if(edit_sel.start != edit->length) {
                edit_sel.start += utf8_len(edit->data + edit_sel.start);
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

int edit_copy(char_t *data, int len)
{
    memcpy(data, active_edit->data + edit_sel.start, edit_sel.length);
    data[edit_sel.length] = 0;
    return edit_sel.length;
}

void edit_paste(char_t *data, int length)
{
    if(!active_edit) {
        return;
    }

    length = utf8_validate(data, length);

    int maxlen = active_edit->maxlength - active_edit->length + edit_sel.length;
    int newlen = 0, i = 0;
    while(i < length) {
        uint8_t len = utf8_len(data + i);
        if((((!active_edit->multiline || data[i] != '\n') && data[i] <= 0x1F) || data[i] == 0x7F) || (len == 2 && data[i] == 0xc2 && data[i + 1] <= 0x9f)) {
            //control characters.
        } else {
            if(newlen + len > maxlen) {
                break;
            }
            if(newlen != i) {
                memcpy(data + newlen, data + i, len);
            }
            newlen += len;
        }
        i += len;
    }

    uint8_t *p = active_edit->data + edit_sel.start;

    memmove(p + newlen, p + edit_sel.length, (active_edit->length - (edit_sel.start + edit_sel.length)) * sizeof(char_t));
    memcpy(p, data, newlen * sizeof(char_t));

    active_edit->length += newlen - edit_sel.length;

    edit_sel.start = edit_sel.start + newlen;
    edit_sel.length = 0;

    redraw();
}

void edit_delete(void)
{
    uint8_t *p = active_edit->data + edit_sel.start;

    if (edit_sel.length)
    {
        memmove(p, p + edit_sel.length, (active_edit->length - (edit_sel.start + edit_sel.length)) * sizeof(char_t));
        active_edit->length -= edit_sel.length;
    }
    else if (edit_sel.start < active_edit->length)
    {
        unsigned len = utf8_len(p);
        memmove(p, p + len, active_edit->length - edit_sel.start - len * sizeof(char_t));
        active_edit->length -= len;
    }

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
