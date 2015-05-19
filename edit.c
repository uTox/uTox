#include "main.h"

static EDIT *active_edit;

static struct
{
    STRING_IDX start, length;
    STRING_IDX p1, p2;
    // IME mark (underline)
    STRING_IDX mark_start, mark_length;
    //TODO: pm field doesn't seem to be used. Remove? done
}edit_sel;
static _Bool edit_select;

static void setactive(EDIT *edit)
{
    if(edit != active_edit) {
        edit_will_deactivate();

        if(active_edit && active_edit->onlosefocus) {
            active_edit->onlosefocus(active_edit);
        }

        active_edit = edit;
    }

}

void edit_draw(EDIT *edit, int x, int y, int width, int height)
{
    if((width - 4 * SCALE - SCROLL_WIDTH) < 0) {
        return;
    }

    if(utox_window_baseline && y > utox_window_baseline - font_small_lineheight - 4 * SCALE) {
        y = utox_window_baseline - font_small_lineheight - 4 * SCALE;
    }

    edit->width = width -4 * SCALE - (edit->multiline ? SCROLL_WIDTH : 0);
    edit->height = height - 4 * SCALE;

    // load colours for this style
    uint32_t color_bg,
             color_border,
             color_border_h,
             color_border_a,
             color_text;

    switch(edit->style) {
        case AUXILIARY_STYLE:
            color_bg = COLOR_AUX_BACKGROUND;
            color_border = COLOR_AUX_EDGE_NORMAL;
            color_border_h = COLOR_AUX_EDGE_HOVER;
            color_border_a = COLOR_AUX_EDGE_ACTIVE;
            color_text = COLOR_AUX_TEXT;
            break;
        default:
            color_bg = COLOR_MAIN_BACKGROUND;
            color_border = COLOR_EDGE_NORMAL;
            color_border_h = COLOR_EDGE_HOVER;
            color_border_a = COLOR_EDGE_ACTIVE;
            color_text = COLOR_MAIN_TEXT;
            break;
    }

    if(!edit->noborder) {
        framerect(x, y, x + width, y + height, (edit == active_edit) ? color_border_a : (edit->mouseover ? color_border_h : color_border));
    }
    drawrect(x + 1, y + 1, x + width - 1, y + height - 1, color_bg);

    setfont(FONT_TEXT);
    setcolor(color_text);

    int yy = y;

    if(edit->multiline) {
        pushclip(x + 1, y + 1, width - 2, height - 2);

        SCROLLABLE *scroll = edit->scroll;
        scroll->content_height = text_height(width - 4 * SCALE - SCROLL_WIDTH, font_small_lineheight, edit->data, edit->length) + 4 * SCALE;
        scroll_draw(scroll, x, y, width, height);
        yy -= scroll_gety(scroll, height);
    }

    // display an edit hint if there's no text in the field
    if(!edit->length && maybe_i18nal_string_is_valid(&edit->empty_str)) {
        STRING* empty_str_text = maybe_i18nal_string_get(&edit->empty_str);
        setcolor(COLOR_MAIN_HINTTEXT);
        drawtext(x + 2 * SCALE, yy + 2 * SCALE, empty_str_text->str, empty_str_text->length);
    }

    _Bool a = (edit == active_edit);
    drawtextmultiline(x + 2 * SCALE, x + width - 2 * SCALE - (edit->multiline ? SCROLL_WIDTH : 0), yy + 2 * SCALE, y, y + height, font_small_lineheight, edit->data, edit->length,
                      a ? edit_sel.start : STRING_IDX_MAX, a ? edit_sel.length : STRING_IDX_MAX,
                      a ? edit_sel.mark_start : 0, a ? edit_sel.mark_length : 0, edit->multiline);

    if(edit->multiline) {
        popclip();
    }
}

_Bool edit_mmove(EDIT *edit, int px, int py, int width, int height, int x, int y, int dx, int dy)
{
    if(utox_window_baseline && py > utox_window_baseline - font_small_lineheight - 4 * SCALE) {
        y += py - (utox_window_baseline - font_small_lineheight - 4 * SCALE);
        py = utox_window_baseline - font_small_lineheight - 4 * SCALE;
    }

    _Bool need_redraw = 0;

    _Bool mouseover = inrect(x, y, 0, 0, width - (edit->multiline ? SCROLL_WIDTH : 0), height);
    if(mouseover) {
        cursor = CURSOR_TEXT;
    }
    if(mouseover != edit->mouseover) {
        edit->mouseover = mouseover;
        if(edit != active_edit) {
            need_redraw = 1;
        }
    }

    if(edit->multiline) {
        need_redraw |= scroll_mmove(edit->scroll, px, py, width, height, x, y, dx, dy);
        y += scroll_gety(edit->scroll, height);
    }

    if(edit == active_edit && edit_select) {
        if (edit->select_completely) {
            edit_setfocus(edit);
            need_redraw = 1;
            return need_redraw;
        }

        setfont(FONT_TEXT);
        edit_sel.p2 = hittextmultiline(x - 2 * SCALE, width - 4 * SCALE - (edit->multiline ? SCROLL_WIDTH : 0), y - 2 * SCALE, INT_MAX, font_small_lineheight, edit->data, edit->length, edit->multiline);

        STRING_IDX start, length;
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
            need_redraw = 1;
        }
    } else if(mouseover) {
        setfont(FONT_TEXT);
        edit->mouseover_char = hittextmultiline(x - 2 * SCALE, width - 4 * SCALE - (edit->multiline ? SCROLL_WIDTH : 0), y - 2 * SCALE, INT_MAX, font_small_lineheight, edit->data, edit->length, edit->multiline);
    }

    return need_redraw;
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

    char_t c = triclick ? '\n' : ' ';

    STRING_IDX i = edit->mouseover_char;
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

static void contextmenu_edit_onselect(uint8_t i)
{
    switch(i) {
    case 0:
        copy(0);
        edit_char(KEY_DEL, 1, 0);
        break;
    case 1:
        copy(0);
        break;
    case 2:
        paste();
        break;
    case 3:
        edit_char(KEY_DEL, 1, 0);
        break;
    case 4:
        edit_char(KEY('A'), 1, 4);
        break;
    }
}

_Bool edit_mright(EDIT *edit)
{
    static UI_STRING_ID menu_edit[] = {STR_CUT, STR_COPY, STR_PASTE, STR_DELETE, STR_SELECTALL};
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

        contextmenu_new(countof(menu_edit), menu_edit, contextmenu_edit_onselect);

        return 1;
    } else if (active_edit == edit) {
        edit_resetfocus(); // lose focus if right mouse button is pressed somewhere else
        return 1; // redraw
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

    if(edit_select && edit == active_edit) {
        setselection(edit->data + edit_sel.start, edit_sel.length);
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

static STRING_IDX edit_change_do(EDIT *edit, EDIT_CHANGE *c)
{
    STRING_IDX r = c->start;
    if(c->remove) {
        memmove(edit->data + c->start + c->length, edit->data + c->start, edit->length - c->start);
        memcpy(edit->data + c->start, c->data, c->length);
        edit->length += c->length;
        r += c->length;
    } else {
        edit->length -= c->length;
        memmove(edit->data + c->start, edit->data + c->start + c->length, edit->length - c->start);
    }

    c->remove = !c->remove;
    return r;
}

void edit_do(EDIT *edit, STRING_IDX start, STRING_IDX length, _Bool remove)
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

static STRING_IDX edit_undo(EDIT *edit)
{
    STRING_IDX r = STRING_IDX_MAX;
    if(edit->history_cur) {
        edit->history_cur--;
        r = edit_change_do(edit, edit->history[edit->history_cur]);
    }
    return r;
}

static STRING_IDX edit_redo(EDIT *edit)
{
    STRING_IDX r = STRING_IDX_MAX;
    if(edit->history_cur != edit->history_length) {
        r = edit_change_do(edit, edit->history[edit->history_cur]);
        edit->history_cur++;
    }
    return r;
}

#define updatesel() if(edit_sel.p1 <= edit_sel.p2) {edit_sel.start = edit_sel.p1; edit_sel.length = edit_sel.p2 - edit_sel.p1;} \
                    else {edit_sel.start = edit_sel.p2; edit_sel.length = edit_sel.p1 - edit_sel.p2;}

void edit_char(uint32_t ch, _Bool control, uint8_t flags){
    /* shift: flags & 1
     * control: flags & 4 */
    EDIT *edit = active_edit;

    if(control || (ch <= 0x1F && (!edit->multiline || ch != '\n')) || (ch >= 0x7f && ch <= 0x9F)) {
        _Bool modified = 0;

        switch(ch) {
        case KEY_BACK: {
            if(edit->readonly) {
                return;
            }

            if(edit_sel.length == 0) {
                STRING_IDX p = edit_sel.start;
                if(p == 0) {
                    break;
                }

                modified = 1;

                /* same as ctrl+left */
                if(flags & 4) {
                    while(p != 0 && edit->data[p - 1] == ' ') {
                        p--;
                    }
                }

                if (p != 0) {
                    do {
                        p -= utf8_unlen(&edit->data[p]);
                    } while((flags & 4) && p != 0 && edit->data[p - 1] != ' ' && edit->data[p - 1] != '\n');
                }

                STRING_IDX len = edit_sel.start - p;
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

            char_t *p = active_edit->data + edit_sel.start;
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
            modified = 1;
            break;
        }

        case KEY_LEFT: {
            STRING_IDX p = edit_sel.p2;
            if(p != 0) {
                if(flags & 4) {
                    while(p != 0 && edit->data[p - 1] == ' ') {
                        p--;
                    }
                }

                if (p != 0) {
                    do {
                        p -= utf8_unlen(&edit->data[p]);
                    } while((flags & 4) && p != 0 && edit->data[p - 1] != ' ' && edit->data[p - 1] != '\n');
                }
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
            STRING_IDX p = edit_sel.p2;
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
            setselection(active_edit->data, active_edit->length);
            break;
        }

        case KEY('Z'): {
            if(!(flags & 1)) {
                STRING_IDX p = edit_undo(edit);
                if(p != STRING_IDX_MAX) {
                    edit_sel.p1 = p;
                    edit_sel.p2 = p;
                    edit_sel.start = p;
                    edit_sel.length = 0;
                    modified = 1;
                }
                break;
            } else {
                /* ctrl+shift+z, fall to ctrl+y*/
            }
        }

        case KEY('Y'): {
            STRING_IDX p = edit_redo(edit);
            if(p != STRING_IDX_MAX) {
                edit_sel.p1 = p;
                edit_sel.p2 = p;
                edit_sel.start = p;
                edit_sel.length = 0;
                modified = 1;
            }
            break;
        }

        case KEY_RETURN: {
            modified = 1;

            if(edit->onenter && !(flags & 4)) {
                edit->onenter(edit);
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
            if ((flags & 1) && !(flags & 4) && edit->onshifttab) {
                edit->onshifttab(edit);
            } else if (!(flags & 4) && edit->ontab) {
                edit->ontab(edit);
            }

            break;
        }

        }

        edit_select = 0;
        if(modified && edit->onchange) {
            edit->onchange(edit);
        }

        edit_redraw();
    } else if(!edit->readonly) {
        uint8_t len = unicode_to_utf8_len(ch);
        if(edit->length - edit_sel.length + len <= edit->maxlength) {
            char_t *p = edit->data + edit_sel.start;

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
                edit->onchange(edit);
            }

            edit_redraw();
        }
    }
}

int edit_selection(EDIT *edit, char_t *data, int len)
{
    if (data)
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

    char_t *p = active_edit->data + edit_sel.start;

    if(edit_sel.length) {
        edit_do(active_edit, edit_sel.start, edit_sel.length, 1);
    }

    memmove(p + newlen, p + edit_sel.length, active_edit->length - (edit_sel.start + edit_sel.length));
    memcpy(p, data, newlen);

    edit_do(active_edit, edit_sel.start, newlen, 0);

    active_edit->length += newlen - edit_sel.length;

    if(select) {
        edit_sel.length = newlen;
        setselection(active_edit->data + edit_sel.start, newlen);
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
    edit_sel.mark_start = 0;
    edit_sel.mark_length = 0;
    setactive(edit);
}

_Bool edit_active(void)
{
    return (active_edit != NULL);
}

EDIT *edit_get_active(void)
{
    return active_edit;
}

void edit_setstr(EDIT *edit, char_t *str, STRING_IDX length)
{
    if(length >= edit->maxlength) {
        length = edit->maxlength;
    }

    edit->length = length;
    memcpy(edit->data, str, length);
}

void edit_setcursorpos(EDIT *edit, STRING_IDX pos)
{
    if (pos <= edit->length) {
        edit_sel.p1 = pos;
    } else {
        edit_sel.p1 = edit->length;
    }

    edit_sel.p2 = edit_sel.start = edit_sel.p1;
    edit_sel.length = 0;
}

STRING_IDX edit_getcursorpos(void)
{
    return edit_sel.p1 < edit_sel.p2 ? edit_sel.p1 : edit_sel.p2;
}

_Bool edit_getmark(STRING_IDX *outloc, STRING_IDX *outlen)
{
    if (outloc)
        *outloc = edit_sel.mark_start;
    if (outlen)
        *outlen = edit_sel.mark_length;

    return (active_edit && edit_sel.mark_length)? 1 : 0;
}

void edit_setmark(STRING_IDX loc, STRING_IDX len)
{
    edit_sel.mark_start = loc;
    edit_sel.mark_length = len;
}

void edit_setselectedrange(STRING_IDX loc, STRING_IDX len)
{
    edit_sel.start = edit_sel.p1 = loc;
    edit_sel.length = len;
    edit_sel.p2 = loc + len;
}
