#include "main.h"

static EDIT *active_edit;

static struct
{
    uint16_t start, length;
    uint16_t p1, p2, pm;
}edit_sel;
static _Bool edit_select;

static void setactive(EDIT *edit)
{
    if(edit != active_edit) {
        active_edit = edit;

        if(active_edit && active_edit->onlosefocus) {
            active_edit->onlosefocus();
        }
    }

}

void edit_draw(EDIT *edit, int x, int y, int width, int height)
{
    if(baseline && y > baseline - font_small_lineheight - 4 * SCALE) {
        y = baseline - font_small_lineheight - 4 * SCALE;
    }

    edit->width = width -4 * SCALE - (edit->multiline ? SCROLL_WIDTH : 0);
    edit->height = height - 4 * SCALE;

    if(!edit->noborder) {
        framerect(x, y, x + width, y + height, (edit == active_edit) ? BLUE : (edit->mouseover ? C_GRAY2 : C_GRAY));
    }
    drawrect(x + 1, y + 1, x + width - 1, y + height - 1, WHITE);

    setfont(FONT_TEXT);
    setcolor(COLOR_TEXT);

    int yy = y;

    if(edit->multiline) {
        pushclip(x + 1, y + 1, width - 2, height - 2);

        SCROLLABLE *scroll = edit->scroll;
        scroll->content_height = text_height(width - 4 * SCALE - SCROLL_WIDTH, font_small_lineheight, edit->data, edit->length) + 4 * SCALE;
        scroll_draw(scroll, x, y, width, height);
        yy -= scroll_gety(scroll, height);
    }

    _Bool a = (edit == active_edit);
    drawtextmultiline(x + 2 * SCALE, x + width - 2 * SCALE - (edit->multiline ? SCROLL_WIDTH : 0), yy + 2 * SCALE, y, y + height, font_small_lineheight, edit->data, edit->length,
                      a ? edit_sel.start : 0xFFFF, a ? edit_sel.length : 0xFFFF, edit->multiline);

    if(edit->multiline) {
        popclip();
    }
}

_Bool edit_mmove(EDIT *edit, int px, int py, int width, int height, int x, int y, int dy)
{
    if(baseline && py > baseline - font_small_lineheight - 4 * SCALE) {
        y += py - (baseline - font_small_lineheight - 4 * SCALE);
        py = baseline - font_small_lineheight - 4 * SCALE;
    }

    _Bool redraw = 0;

    _Bool mouseover = inrect(x, y, 0, 0, width - (edit->multiline ? SCROLL_WIDTH : 0), height);
    if(mouseover) {
        cursor = CURSOR_TEXT;
    }
    if(mouseover != edit->mouseover) {
        edit->mouseover = mouseover;
        if(edit != active_edit) {
            redraw = 1;
        }
    }

    if(edit->multiline) {
        redraw |= scroll_mmove(edit->scroll, px, py, width, height, x, y, dy);
        y += scroll_gety(edit->scroll, height);
    }

    if(edit == active_edit && edit_select) {
        setfont(FONT_TEXT);
        edit_sel.p2 = hittextmultiline(x - 2 * SCALE, width - 4 * SCALE - (edit->multiline ? SCROLL_WIDTH : 0), y - 2 * SCALE, INT_MAX, font_small_lineheight, edit->data, edit->length, edit->multiline);

        uint16_t start, length;
        if(edit_sel.p2 > edit_sel.p1) {
            start = edit_sel.p1;
            length = edit_sel.p2 - edit_sel.p1;
        } else {
            start = edit_sel.p2;
            length = edit_sel.p1 - edit_sel.p2;
        }

        if(start != edit_sel.start || length != edit_sel.length) {
            edit_sel.start = start;
            edit_sel.length = length;
            setselection(0, edit);
            redraw = 1;
        }
    } else if(mouseover) {
        setfont(FONT_TEXT);
        edit->mouseover_char = hittextmultiline(x - 2 * SCALE, width - 4 * SCALE - (edit->multiline ? SCROLL_WIDTH : 0), y - 2 * SCALE, INT_MAX, font_small_lineheight, edit->data, edit->length, edit->multiline);
    }

    return redraw;
}

_Bool edit_mdown(EDIT *edit)
{
    if(edit->mouseover_char > edit->length) {
        edit->mouseover_char = edit->length;
    }

    if(edit->multiline) {
        if(scroll_mdown(edit->scroll)) {
            return 1;
        }
    }

    if(edit->mouseover) {
        edit_sel.start = edit_sel.p1 = edit_sel.p2 = edit->mouseover_char;
        edit_sel.length = 0;
        edit_select = 1;

        setactive(edit);

        showkeyboard(1);
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
    setselection(0, edit);

    return 1;
}

_Bool edit_mright(EDIT *edit)
{
    if(edit->mouseover_char > edit->length) {
        edit->mouseover_char = edit->length;
    }

    if(edit->mouseover) {
        EDIT *active = active_edit;
        if(active != edit) {
            setactive(edit);

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

void edit_press(void)
{
    edit_sel.start = edit_sel.p1 = edit_sel.p2 = active_edit->mouseover_char;
    edit_sel.length = 0;
}

_Bool edit_mwheel(EDIT *edit, int height, double d)
{
    if(edit->multiline) {
        return scroll_mwheel(edit->scroll, height - SCALE * 4, d);
    }
    return 0;
}

_Bool edit_mup(EDIT *edit)
{
    if(edit->multiline) {
        if(scroll_mup(edit->scroll)) {
            return 1;
        }
    }

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

static void edit_redraw(void)
{
    redraw();
}

static uint16_t edit_change_do(EDIT *edit, EDIT_CHANGE *c)
{
    uint16_t r = c->start;
    if(c->remove) {
        memmove(edit->data + c->start + c->length, edit->data + c->start, c->length);
        memcpy(edit->data + c->start, c->data, c->length);
        edit->length += c->length;
        r += c->length;
    } else {
        memmove(edit->data + c->start, edit->data + c->start + c->length, c->length);
        edit->length -= c->length;
    }

    c->remove = !c->remove;
    return r;
}

static void edit_do(EDIT *edit, uint16_t start, uint16_t length, _Bool remove)
{
    EDIT_CHANGE *new, **history;

    new = malloc(sizeof(EDIT_CHANGE) + length);
    if(!new) {
        return;
    }

    new->remove = remove;
    new->start = start;
    new->length = length;
    memcpy(new->data, edit->data + start, length);


    if(edit->history_cur != edit->history_length) {
        uint16_t i = edit->history_cur;
        while(i != edit->history_length) {
            free(edit->history[i]);
            i++;
        }
    }

    history = realloc(edit->history, (edit->history_cur + 1) * sizeof(void*));
    if(!history) {
        //
    }

    history[edit->history_cur] = new;
    edit->history = history;

    edit->history_cur++;
    edit->history_length = edit->history_cur;
}

static uint16_t edit_undo(EDIT *edit)
{
    uint16_t r = 0xFFFF;
    if(edit->history_cur) {
        edit->history_cur--;
        r = edit_change_do(edit, edit->history[edit->history_cur]);
    }
    return r;
}

static uint16_t edit_redo(EDIT *edit)
{
    uint16_t r = 0xFFFF;
    if(edit->history_cur != edit->history_length) {
        r = edit_change_do(edit, edit->history[edit->history_cur]);
        edit->history_cur++;
    }
    return r;
}

#define updatesel() if(edit_sel.p1 <= edit_sel.p2) {edit_sel.start = edit_sel.p1; edit_sel.length = edit_sel.p2 - edit_sel.p1;} \
                    else {edit_sel.start = edit_sel.p2; edit_sel.length = edit_sel.p1 - edit_sel.p2;}

/* shift: flags & 1
 * control: flags & 4
*/
void edit_char(uint32_t ch, _Bool control, uint8_t flags)
{
    EDIT *edit = active_edit;

    if(control || (ch <= 0x1F && (!edit->multiline || ch != '\n')) || (ch >= 0x7f && ch <= 0x9F)) {
        switch(ch) {
        case KEY_BACK: {
            if(edit->readonly) {
                return;
            }

            if(edit_sel.length == 0) {
                uint16_t p = edit_sel.start;
                if(p == 0) {
                    break;
                }

                /* same as ctrl+left */
                if(flags & 4) {
                    while(p != 0 && edit->data[p - 1] == ' ') {
                        p--;
                    }
                }

                do {
                    p -= utf8_unlen(&edit->data[p]);
                } while((flags & 4) && p != 0 && edit->data[p - 1] != ' ' && edit->data[p - 1] != '\n');

                uint16_t len = edit_sel.start - p;
                edit_do(edit, edit_sel.start - len, len, 1);
                memmove(edit->data + edit_sel.start - len, edit->data + edit_sel.start, edit->length - edit_sel.start);
                edit->length -= len;

                edit_sel.start -= len;
                edit_sel.p1 = edit_sel.start;
                edit_sel.p2 = edit_sel.start;
                break;
            } else {
                /* fall through to KEY_DEL */
            }
        }

        case KEY_DEL: {
            if(edit->readonly) {
                return;
            }

            uint8_t *p = active_edit->data + edit_sel.start;
            if(edit_sel.length) {
                edit_do(edit, edit_sel.start, edit_sel.length, 1);
                memmove(p, p + edit_sel.length, active_edit->length - (edit_sel.start + edit_sel.length));
                active_edit->length -= edit_sel.length;
            }
            else if(edit_sel.start < active_edit->length) {
                uint8_t len = utf8_len(p);
                edit_do(edit, edit_sel.start, len, 1);
                memmove(p, p + len, active_edit->length - edit_sel.start - len);
                active_edit->length -= len;
            }
            edit_sel.p1 = edit_sel.start;
            edit_sel.p2 = edit_sel.start;
            edit_sel.length = 0;
            break;
        }

        case KEY_LEFT: {
            uint16_t p = edit_sel.p2;
            if(p != 0) {
                if(flags & 4) {
                    while(p != 0 && edit->data[p - 1] == ' ') {
                        p--;
                    }
                }

                do {
                    p -= utf8_unlen(&edit->data[p]);
                } while((flags & 4) && p != 0 && edit->data[p - 1] != ' ' && edit->data[p - 1] != '\n');
            }

            if(flags & 1) {
                edit_sel.p2 = p;
                updatesel();
            } else {
                if(edit_sel.length) {
                    p = edit_sel.start;
                }
                edit_sel.p1 = p;
                edit_sel.p2 = p;
                edit_sel.start = p;
                edit_sel.length = 0;
            }
            break;
        }

        case KEY_RIGHT: {
            uint16_t p = edit_sel.p2;
            if(flags & 4) {
                while(p != edit->length && edit->data[p] == ' ') {
                    p++;
                }
            }

            do {
                if(p == edit->length) {
                    break;
                }
                p += utf8_len(&edit->data[p]);
            } while((flags & 4) && edit->data[p] != ' ' && edit->data[p] != '\n');

            if(flags & 1) {
                edit_sel.p2 = p;
                updatesel();
            } else {
                if(edit_sel.length) {
                    p = edit_sel.start + edit_sel.length;
                }
                edit_sel.p1 = p;
                edit_sel.p2 = p;
                edit_sel.start = p;
                edit_sel.length = 0;
            }
            break;
        }

        case KEY_UP: {
            if(!edit->multiline) {
                break;
            }

            setfont(FONT_TEXT);
            edit_sel.p2 = text_lineup(edit->width, edit->height, edit_sel.p2, font_small_lineheight, edit->data, edit->length, edit->scroll);
            if(!(flags & 1)) {
                edit_sel.p1 = edit_sel.p2;
            }
            updatesel();
            break;
        }

        case KEY_DOWN: {
            if(!edit->multiline) {
                break;
            }

            setfont(FONT_TEXT);
            edit_sel.p2 = text_linedown(edit->width, edit->height, edit_sel.p2, font_small_lineheight, edit->data, edit->length, edit->scroll);
            if(!(flags & 1)) {
                edit_sel.p1 = edit_sel.p2;
            }
            updatesel();
            break;
        }

        case KEY_PAGEUP: {
            if(!edit->multiline) {
                break;
            }

            edit->scroll->d = 0.0;
            break;
        }

        case KEY_PAGEDOWN: {
            if(!edit->multiline) {
                break;
            }

            edit->scroll->d = 1.0;
            break;
        }

        case KEY_HOME: {
            if(flags & 1) {
                edit_sel.p2 = 0;
                edit_sel.start = 0;
                edit_sel.length = edit_sel.p1;
                break;
            }
            edit_sel.p1 = edit_sel.p2 = edit_sel.start = edit_sel.length = 0;
            break;
        }

        case KEY_END: {
            if(flags & 1) {
                edit_sel.p2 = edit->length;
                edit_sel.start = edit_sel.p1;
                edit_sel.length = edit_sel.p2 - edit_sel.p1;
                break;
            }
            edit_sel.p1 = edit_sel.p2 = edit_sel.start = edit->length;
            edit_sel.length = 0;
            break;
        }

        case KEY('A'): {
            edit_sel.p1 = 0;
            edit_sel.p2 = active_edit->length;
            edit_sel.start = 0;
            edit_sel.length = active_edit->length;
            setselection(0, edit);
            break;
        }

        case KEY('Z'): {
            if(!(flags & 1)) {
                uint16_t p = edit_undo(edit);
                if(p != 0xFFFF) {
                    edit_sel.p1 = p;
                    edit_sel.p2 = p;
                    edit_sel.start = p;
                    edit_sel.length = 0;
                }
                break;
            } else {
                /* ctrl+shift+z, fall to ctrl+y*/
            }
        }

        case KEY('Y'): {
            uint16_t p = edit_redo(edit);
            if(p != 0xFFFF) {
                edit_sel.p1 = p;
                edit_sel.p2 = p;
                edit_sel.start = p;
                edit_sel.length = 0;
            }
            break;
        }

        case KEY_RETURN: {
            if(edit->onenter) {
                edit->onenter();
                /*dirty*/
                if(edit->length == 0) {
                    uint16_t i = 0;
                    while(i != edit->history_length) {
                        free(edit->history[i]);
                        i++;
                    }
                    free(edit->history);
                    edit->history = NULL;
                    edit->history_cur = 0;
                    edit->history_length = 0;

                    edit_sel.p1 = 0;
                    edit_sel.p2 = 0;
                    edit_sel.start = 0;
                    edit_sel.length = 0;
                }
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

        edit_select = 0;
        if(edit->onchange) {
            edit->onchange();
        }

        edit_redraw();
    } else if(!edit->readonly) {
        uint8_t len = unicode_to_utf8_len(ch);
        if(edit->length - edit_sel.length + len <= edit->maxlength) {
            uint8_t *p = edit->data + edit_sel.start;

            if(edit_sel.length) {
                edit_do(edit, edit_sel.start, edit_sel.length, 1);
            }

            memmove(p + len, p + edit_sel.length, edit->length - (edit_sel.start + edit_sel.length));
            edit->length -= edit_sel.length;
            unicode_to_utf8(ch, edit->data + edit_sel.start);
            edit->length += len;

            edit_do(edit, edit_sel.start, len, 0);

            edit_sel.start += len;
            edit_sel.p1 = edit_sel.start;
            edit_sel.p2 = edit_sel.p1;
            edit_sel.length = 0;

            if(edit->onchange) {
                edit->onchange();
            }

            edit_redraw();
        }
    }
}

int edit_selection(EDIT *edit, char_t *data, int len)
{
    memcpy(data, edit->data + edit_sel.start, edit_sel.length);
    return edit_sel.length;
}

int edit_copy(char_t *data, int len)
{
    return edit_selection(active_edit, data, len);
}

void edit_paste(char_t *data, int length, _Bool select)
{
    if(!active_edit) {
        return;
    }

    if(active_edit->readonly) {
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

    if(newlen == 0) {
        return;
    }

    uint8_t *p = active_edit->data + edit_sel.start;

    if(edit_sel.length) {
        edit_do(active_edit, edit_sel.start, edit_sel.length, 1);
    }

    memmove(p + newlen, p + edit_sel.length, active_edit->length - (edit_sel.start + edit_sel.length));
    memcpy(p, data, newlen);

    edit_do(active_edit, edit_sel.start, newlen, 0);

    active_edit->length += newlen - edit_sel.length;

    if(select) {
        edit_sel.start = edit_sel.start;
        edit_sel.length = newlen;
        setselection(0, active_edit);
    } else {
        edit_sel.start = edit_sel.start + newlen;
        edit_sel.length = 0;
    }

    edit_sel.p1 = edit_sel.start;
    edit_sel.p2 = edit_sel.start + edit_sel.length;

    edit_redraw();
}

void edit_resetfocus(void)
{
    edit_select = 0;
    setactive(NULL);
}

void edit_setfocus(EDIT *edit)
{
    edit_select = 0;
    edit_sel.start = edit_sel.p1 = 0;
    edit_sel.length = edit_sel.p2 = edit->length;
    setactive(edit);
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
