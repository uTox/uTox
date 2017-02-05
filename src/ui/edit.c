#include "edit.h"

#include "contextmenu.h"
#include "draw.h"
#include "scrollable.h"
#include "text.h"

#include "../macros.h"
#include "../main.h"
#include "../main_native.h"
#include "../settings.h"
#include "../text.h"
#include "../theme.h"
#include "../ui.h"

static EDIT *active_edit;

static struct {
    uint16_t start, length;
    uint16_t p1, p2;
    // IME mark (underline)
    uint16_t mark_start, mark_length;
} edit_sel;

static bool edit_select;

static void setactive(EDIT *edit) {
    if (edit != active_edit) {
        edit_will_deactivate();

        if (active_edit && active_edit->onlosefocus) {
            active_edit->onlosefocus(active_edit);
        }

        active_edit = edit;
    }
}

void edit_draw(EDIT *edit, int x, int y, int width, int height) {
    if (width - SCALE(8) - SCROLL_WIDTH < 0) { // why?
        return;
    }

    if (settings.window_baseline && y > (int)settings.window_baseline - font_small_lineheight - SCALE(8)) {
        y = settings.window_baseline - font_small_lineheight - SCALE(8);
    }

    edit->width  = width - SCALE(8) - (edit->multiline ? SCROLL_WIDTH : 0);
    edit->height = height - SCALE(8);

    // load colors for this style
    uint32_t color_bg, color_border, color_border_h, color_border_a, color_text;

    switch (edit->style) {
        case AUXILIARY_STYLE:
            color_bg       = COLOR_BKGRND_AUX;
            color_border   = COLOR_AUX_EDGE_NORMAL;
            color_border_h = COLOR_AUX_EDGE_HOVER;
            color_border_a = COLOR_AUX_EDGE_ACTIVE;
            color_text     = COLOR_AUX_TEXT;
            break;
        default:
            color_bg       = COLOR_BKGRND_MAIN;
            color_border   = COLOR_EDGE_NORMAL;
            color_border_h = COLOR_EDGE_HOVER;
            color_border_a = COLOR_EDGE_ACTIVE;
            color_text     = COLOR_MAIN_TEXT;
            break;
    }

    if (!edit->noborder) {
        draw_rect_frame(x, y, width, height,
                        (edit == active_edit) ? color_border_a : (edit->mouseover ? color_border_h : color_border));
    }
    draw_rect_fill(x + 1, y + 1, width - SCALE(2), height - SCALE(2), color_bg);

    setfont(FONT_TEXT);
    setcolor(color_text);

    int yy = y;

    if (edit->multiline) {
        pushclip(x + 1, y + 1, width - 2, height - 2);

        SCROLLABLE *scroll = edit->scroll;
        scroll->content_height =
            text_height(width - SCALE(8) - SCROLL_WIDTH, font_small_lineheight, edit->data, edit->length) + SCALE(8);
        scroll_draw(scroll, x, y, width, height);
        yy -= scroll_gety(scroll, height);
    }

    /* because the search field has a padding of 3.5 SCALEs */
    float top_offset = 2.0;
    if (edit->vcentered && !edit->multiline) {
        top_offset = (height - font_small_lineheight) / (SCALE(4.0));
    }

    // display an edit hint if there's no text in the field
    if (!edit->length && maybe_i18nal_string_is_valid(&edit->empty_str)) {
        STRING *empty_str_text = maybe_i18nal_string_get(&edit->empty_str);
        setcolor(COLOR_MAIN_TEXT_HINT);
        drawtext(x + SCALE(4), yy + SCALE(top_offset* 2), empty_str_text->str, empty_str_text->length);
    }

    bool is_active = (edit == active_edit);
    if (edit->password) {
        /* Generate the stars for this password */
        char star[edit->length];
        memset(star, '*', edit->length);
        utox_draw_text_multiline_within_box(x + SCALE(4), yy + SCALE(top_offset * 2),
                                                x + width - SCALE(4) - (edit->multiline ? SCROLL_WIDTH : 0),
                                                y, y + height, font_small_lineheight, star,
                                                edit->length, is_active ? edit_sel.start : UINT16_MAX,
                                                is_active ? edit_sel.length : UINT16_MAX,
                                                is_active ? edit_sel.mark_start : 0,
                                                is_active ? edit_sel.mark_length : 0, edit->multiline);
    } else {
        utox_draw_text_multiline_within_box(x + SCALE(4), yy + SCALE(top_offset * 2),
                                    x + width - SCALE(4) - (edit->multiline ? SCROLL_WIDTH : 0),
                                    y, y + height, font_small_lineheight, edit->data,
                                    edit->length, is_active ? edit_sel.start : UINT16_MAX,
                                    is_active ? edit_sel.length : UINT16_MAX, is_active ? edit_sel.mark_start : 0,
                                    is_active ? edit_sel.mark_length : 0, edit->multiline);
    }

    if (edit->multiline) {
        popclip();
    }
}

bool edit_mmove(EDIT *edit, int px, int py, int width, int height, int x, int y, int dx, int dy) {
    if (settings.window_baseline && py > (int)settings.window_baseline - font_small_lineheight - SCALE(8)) {
        y += py - (settings.window_baseline - font_small_lineheight - SCALE(8));
        py = settings.window_baseline - font_small_lineheight - SCALE(8);
    }

    bool need_redraw = 0;

    bool mouseover = inrect(x, y, 0, 0, width - (edit->multiline ? SCROLL_WIDTH : 0), height);
    if (mouseover) {
        cursor = CURSOR_TEXT;
    }
    if (mouseover != edit->mouseover) {
        edit->mouseover = mouseover;
        if (edit != active_edit) {
            need_redraw = 1;
        }
    }

    if (edit->multiline) {
        need_redraw |= scroll_mmove(edit->scroll, px, py, width, height, x, y, dx, dy);
        y += scroll_gety(edit->scroll, height);
    }

    if (edit == active_edit && edit_select) {
        if (edit->select_completely) {
            edit_setfocus(edit);
            need_redraw = 1;
            return need_redraw;
        }

        setfont(FONT_TEXT);
        edit_sel.p2 =
            hittextmultiline(x - SCALE(4), width - SCALE(8) - (edit->multiline ? SCROLL_WIDTH : 0), y - SCALE(4),
                             INT_MAX, font_small_lineheight, edit->data, edit->length, edit->multiline);

        uint16_t start, length;
        if (edit_sel.p2 > edit_sel.p1) {
            start  = edit_sel.p1;
            length = edit_sel.p2 - edit_sel.p1;
        } else {
            start  = edit_sel.p2;
            length = edit_sel.p1 - edit_sel.p2;
        }

        if (start != edit_sel.start || length != edit_sel.length) {
            edit_sel.start  = start;
            edit_sel.length = length;
            need_redraw     = 1;
        }
    } else if (mouseover) {
        setfont(FONT_TEXT);
        edit->mouseover_char =
            hittextmultiline(x - SCALE(4), width - SCALE(8) - (edit->multiline ? SCROLL_WIDTH : 0), y - SCALE(4),
                             INT_MAX, font_small_lineheight, edit->data, edit->length, edit->multiline);
    }

    return need_redraw;
}

bool edit_mdown(EDIT *edit) {
    if (edit->mouseover_char > edit->length) {
        edit->mouseover_char = edit->length;
    }

    if (edit->multiline) {
        if (scroll_mdown(edit->scroll)) {
            return 1;
        }
    }

    if (edit->mouseover) {
        edit_sel.start = edit_sel.p1 = edit_sel.p2 = edit->mouseover_char;
        edit_sel.length                            = 0;
        edit_select                                = 1;

        setactive(edit);

        showkeyboard(1);
        return 1;
    } else if (edit == active_edit) {
        edit_resetfocus();
    }

    return 0;
}

bool edit_dclick(EDIT *edit, bool triclick) {
    if (edit != active_edit) {
        return false;
    }

    if (edit->mouseover_char > edit->length) {
        edit->mouseover_char = edit->length;
    }

    char c = triclick ? '\n' : ' ';

    uint16_t i = edit->mouseover_char;
    while (i != 0 && edit->data[i - 1] != c) {
        i -= utf8_unlen(edit->data + i);
    }
    edit_sel.start = edit_sel.p1 = i;
    i                            = edit->mouseover_char;
    while (i != edit->length && edit->data[i] != c) {
        i += utf8_len(edit->data + i);
    }
    edit_sel.p2     = i;
    edit_sel.length = i - edit_sel.start;

    return true;
}

static void contextmenu_edit_onselect(uint8_t i) {
    switch (i) {
        case 0:
            copy(0);
            edit_char(KEY_DEL, 1, 0);
            break;
        case 1: copy(0); break;
        case 2: paste(); break;
        case 3: edit_char(KEY_DEL, 1, 0); break;
        case 4:
            /* Send a ctrl + a to the active edit */
            edit_char('A', 1, 4);
            break;
    }
}

bool edit_mright(EDIT *edit) {
    static UTOX_I18N_STR menu_edit[] = { STR_CUT, STR_COPY, STR_PASTE, STR_DELETE, STR_SELECTALL };
    if (edit->mouseover_char > edit->length) {
        edit->mouseover_char = edit->length;
    }

    if (edit->mouseover) {
        EDIT *active = active_edit;
        if (active != edit) {
            setactive(edit);

            edit_sel.start = edit_sel.p1 = edit_sel.p2 = edit->mouseover_char;
            edit_sel.length                            = 0;
            edit_select                                = 1;
        }

        contextmenu_new(COUNTOF(menu_edit), menu_edit, contextmenu_edit_onselect);

        return true;
    } else if (active_edit == edit) {
        edit_resetfocus(); // lose focus if right mouse button is pressed somewhere else
        return true;          // redraw
    }

    return false;
}

void edit_press(void) {
    edit_sel.start = edit_sel.p1 = edit_sel.p2 = active_edit->mouseover_char;
    edit_sel.length                            = 0;
}

bool edit_mwheel(EDIT *edit, int height, double d, bool smooth) {
    if (edit->multiline) {
        return scroll_mwheel(edit->scroll, height - SCALE(8), d, smooth);
    }
    return false;
}

bool edit_mup(EDIT *edit) {
    if (edit->multiline) {
        if (scroll_mup(edit->scroll)) {
            return true;
        }
    }

    if (edit_select && edit == active_edit) {
        setselection(edit->data + edit_sel.start, edit_sel.length);
        edit_select = 0;
    }

    return false;
}

bool edit_mleave(EDIT *edit) {
    if (edit->mouseover) {
        edit->mouseover = false;
        return true;
    }

    return false;
}

static void edit_redraw(void) {
    redraw();
}

static uint16_t edit_change_do(EDIT *edit, EDIT_CHANGE *c) {
    uint16_t r = c->start;
    if (c->remove) {
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

void edit_do(EDIT *edit, uint16_t start, uint16_t length, bool remove) {
    EDIT_CHANGE *new, **history;

    new = malloc(sizeof(EDIT_CHANGE) + length);
    if (!new) {
        return;
    }

    new->remove = remove;
    new->start  = start;
    new->length = length;
    memcpy(new->data, edit->data + start, length);

    if (edit->history_cur != edit->history_length) {
        uint16_t i = edit->history_cur;
        while (i != edit->history_length) {
            free(edit->history[i]);
            i++;
        }
    }

    history = realloc(edit->history, (edit->history_cur + 1) * sizeof(void *));
    if (!history) {
        // Do something?
    }

    history[edit->history_cur] = new;
    edit->history              = history;

    edit->history_cur++;
    edit->history_length = edit->history_cur;
}

static uint16_t edit_undo(EDIT *edit) {
    uint16_t r = UINT16_MAX;
    if (edit->history_cur) {
        edit->history_cur--;
        r = edit_change_do(edit, edit->history[edit->history_cur]);
    }
    return r;
}

static uint16_t edit_redo(EDIT *edit) {
    uint16_t r = UINT16_MAX;
    if (edit->history_cur != edit->history_length) {
        r = edit_change_do(edit, edit->history[edit->history_cur]);
        edit->history_cur++;
    }
    return r;
}

#define updatesel()                                  \
    if (edit_sel.p1 <= edit_sel.p2) {                \
        edit_sel.start  = edit_sel.p1;               \
        edit_sel.length = edit_sel.p2 - edit_sel.p1; \
    } else {                                         \
        edit_sel.start  = edit_sel.p2;               \
        edit_sel.length = edit_sel.p1 - edit_sel.p2; \
    }

enum {
    EMOD_SHIFT = (1 << 0),
    EMOD_CTRL  = (1 << 2),
};

void edit_char(uint32_t ch, bool control, uint8_t flags) {
    /* shift: flags & 1
     * control: flags & 4 */
    EDIT *edit = active_edit; // TODO this is bad

    if (control || (ch <= 0x1F && (!edit->multiline || ch != '\n')) || (ch >= 0x7f && ch <= 0x9F)) {
        bool modified = false;

        switch (ch) {
            case KEY_BACK: {
                if (edit->readonly) {
                    return;
                }

                if (edit_sel.length == 0) {
                    uint16_t p = edit_sel.start;
                    if (p == 0) {
                        break;
                    }

                    modified = true;

                    /* same as ctrl+left */
                    if (flags & EMOD_CTRL) {
                        while (p != 0 && edit->data[p - 1] == ' ') {
                            p--;
                        }
                    }

                    if (p != 0) {
                        do {
                            p -= utf8_unlen(&edit->data[p]);
                        } while ((flags & EMOD_CTRL) && p != 0 && edit->data[p - 1] != ' ' && edit->data[p - 1] != '\n');
                    }

                    uint16_t len = edit_sel.start - p;
                    edit_do(edit, edit_sel.start - len, len, 1);
                    memmove(edit->data + edit_sel.start - len, edit->data + edit_sel.start,
                            edit->length - edit_sel.start);
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
                if (edit->readonly) {
                    return;
                }

                char *p = active_edit->data + edit_sel.start;
                if (edit_sel.length) {
                    edit_do(edit, edit_sel.start, edit_sel.length, 1);
                    memmove(p, p + edit_sel.length, active_edit->length - (edit_sel.start + edit_sel.length));
                    active_edit->length -= edit_sel.length;
                } else if (edit_sel.start < active_edit->length) {
                    uint8_t len = utf8_len(p);
                    edit_do(edit, edit_sel.start, len, 1);
                    memmove(p, p + len, active_edit->length - edit_sel.start - len);
                    active_edit->length -= len;
                }
                edit_sel.p1     = edit_sel.start;
                edit_sel.p2     = edit_sel.start;
                edit_sel.length = 0;
                modified        = true;
                break;
            }

            case KEY_LEFT: {
                uint16_t p = edit_sel.p2;
                if (p != 0) {
                    if (flags & EMOD_CTRL) {
                        while (p != 0 && edit->data[p - 1] == ' ') {
                            p--;
                        }
                    }

                    if (p != 0) {
                        do {
                            p -= utf8_unlen(&edit->data[p]);
                        } while ((flags & EMOD_CTRL) && p != 0 && edit->data[p - 1] != ' ' && edit->data[p - 1] != '\n');
                    }
                }

                if (flags & EMOD_SHIFT) {
                    edit_sel.p2 = p;
                    updatesel();
                } else {
                    if (edit_sel.length) {
                        p = edit_sel.start;
                    }
                    edit_sel.p1     = p;
                    edit_sel.p2     = p;
                    edit_sel.start  = p;
                    edit_sel.length = 0;
                }
                break;
            }

            case KEY_RIGHT: {
                uint16_t p = edit_sel.p2;
                if (flags & EMOD_CTRL) {
                    while (p != edit->length && edit->data[p] == ' ') {
                        p++;
                    }
                }

                do {
                    if (p == edit->length) {
                        break;
                    }
                    p += utf8_len(&edit->data[p]);
                } while ((flags & EMOD_CTRL) && edit->data[p] != ' ' && edit->data[p] != '\n');

                if (flags & EMOD_SHIFT) {
                    edit_sel.p2 = p;
                    updatesel();
                } else {
                    if (edit_sel.length) {
                        p = edit_sel.start + edit_sel.length;
                    }
                    edit_sel.p1     = p;
                    edit_sel.p2     = p;
                    edit_sel.start  = p;
                    edit_sel.length = 0;
                }
                break;
            }

            case KEY_UP: {
                if (!edit->multiline) {
                    break;
                }

                setfont(FONT_TEXT);
                edit_sel.p2 = text_lineup(edit->width, edit->height, edit_sel.p2, font_small_lineheight, edit->data,
                                          edit->length, edit->scroll);
                if (!(flags & EMOD_SHIFT)) {
                    edit_sel.p1 = edit_sel.p2;
                }
                updatesel();
                break;
            }

            case KEY_DOWN: {
                if (!edit->multiline) {
                    break;
                }

                setfont(FONT_TEXT);
                edit_sel.p2 = text_linedown(edit->width, edit->height, edit_sel.p2, font_small_lineheight, edit->data,
                                            edit->length, edit->scroll);
                if (!(flags & EMOD_SHIFT)) {
                    edit_sel.p1 = edit_sel.p2;
                }
                updatesel();
                break;
            }

            case KEY_PAGEUP: {
                if (!edit->multiline) {
                    break;
                }

                edit->scroll->d = 0.0;
                break;
            }

            case KEY_PAGEDOWN: {
                if (!edit->multiline) {
                    break;
                }

                edit->scroll->d = 1.0;
                break;
            }

            case KEY_HOME: {
                if (flags & EMOD_SHIFT) {
                    edit_sel.p2     = 0;
                    edit_sel.start  = 0;
                    edit_sel.length = edit_sel.p1;
                    break;
                }
                edit_sel.p1 = edit_sel.p2 = edit_sel.start = edit_sel.length = 0;
                break;
            }

            case KEY_END: {
                if (edit->length == 0) {
                    break;
                }

                if (flags & EMOD_CTRL) {
                    edit_sel.p1 = edit_sel.p2 = edit_sel.start = edit->length;
                    edit_sel.length                            = 0;
                    break;
                }

                uint32_t p = edit_sel.p2;
                while (p != edit->length && edit->data[p] != '\n') {
                    p++;
                }
                --p;

                do {
                    if (p == edit->length) {
                        break;
                    }
                    p += utf8_len(&edit->data[p]);
                } while ((flags & EMOD_CTRL) && edit->data[p] != '\n');

                if (flags & EMOD_SHIFT) {
                    edit_sel.p2 = p;
                    updatesel();
                } else {
                    if (edit_sel.length) {
                        p = edit_sel.start + edit_sel.length;
                    }
                    edit_sel.p1     = p;
                    edit_sel.p2     = p;
                    edit_sel.start  = p;
                    edit_sel.length = 0;
                }
                break;
            }

            case 'a':
            case 'A': {
                edit_sel.p1     = 0;
                edit_sel.p2     = active_edit->length;
                edit_sel.start  = 0;
                edit_sel.length = active_edit->length;
                setselection(active_edit->data, active_edit->length);
                break;
            }

            case 'z':
            case 'Z': {
                if (!(flags & EMOD_SHIFT)) {
                    uint16_t p = edit_undo(edit);
                    if (p != UINT16_MAX) {
                        edit_sel.p1     = p;
                        edit_sel.p2     = p;
                        edit_sel.start  = p;
                        edit_sel.length = 0;
                        modified        = true;
                    }
                    break;
                } else {
                    /* ctrl+shift+z, fall to ctrl+y*/
                }
            }

            case 'y':
            case 'Y': {
                uint16_t p = edit_redo(edit);
                if (p != UINT16_MAX) {
                    edit_sel.p1     = p;
                    edit_sel.p2     = p;
                    edit_sel.start  = p;
                    edit_sel.length = 0;
                    modified        = false;
                }
                break;
            }

            case KEY_RETURN: {
                modified = true;

                if (edit->onenter && !(flags & EMOD_CTRL)) {
                    edit->onenter(edit);
                    /*dirty*/
                    if (edit->length == 0) {
                        for (uint16_t i = 0; i != edit->history_length; i++) {
                            free(edit->history[i]);
                        }
                        free(edit->history);
                        edit->history        = NULL;
                        edit->history_cur    = 0;
                        edit->history_length = 0;

                        edit_sel.p1     = 0;
                        edit_sel.p2     = 0;
                        edit_sel.start  = 0;
                        edit_sel.length = 0;
                    }
                }
                break;
            }

            case KEY_TAB: {
                if ((flags & EMOD_SHIFT) && !(flags & EMOD_CTRL) && edit->onshifttab) {
                    edit->onshifttab(edit);
                } else if (!(flags & EMOD_CTRL) && edit->ontab) {
                    edit->ontab(edit);
                }

                break;
            }
        }

        edit_select = 0;
        if (modified && edit->onchange) {
            edit->onchange(edit);
        }

        edit_redraw();
    } else if (!edit->readonly) {
        uint8_t len = unicode_to_utf8_len(ch);
        if (edit->length - edit_sel.length + len <= edit->maxlength) {
            char *p = edit->data + edit_sel.start;

            if (edit_sel.length) {
                edit_do(edit, edit_sel.start, edit_sel.length, 1);
            }

            memmove(p + len, p + edit_sel.length, edit->length - (edit_sel.start + edit_sel.length));
            edit->length -= edit_sel.length;
            unicode_to_utf8(ch, edit->data + edit_sel.start);
            edit->length += len;

            edit_do(edit, edit_sel.start, len, 0);

            edit_sel.start += len;
            edit_sel.p1     = edit_sel.start;
            edit_sel.p2     = edit_sel.p1;
            edit_sel.length = 0;

            if (edit->onchange) {
                edit->onchange(edit);
            }

            edit_redraw();
        }
    }
}

int edit_selection(EDIT *edit, char *data, int UNUSED(len)) {
    if (data) {
        memcpy(data, edit->data + edit_sel.start, edit_sel.length);
    }
    return edit_sel.length;
}

int edit_copy(char *data, int len) {
    return edit_selection(active_edit, data, len);
}

void edit_paste(char *data, int length, bool select) {
    if (!active_edit) {
        return;
    }

    if (active_edit->readonly) {
        return;
    }

    length = utf8_validate((uint8_t *)data, length);

    const int maxlen = active_edit->maxlength - active_edit->length + edit_sel.length;
    int newlen = 0, i = 0;
    while (i < length) {
        uint8_t len = utf8_len(data + i);
        // Ignore and strip these control characters.
        if ((((!active_edit->multiline || data[i] != '\n')                               // not multiline or '\n'
              && data[i] <= 0x1F)                                                        // and is a control char
             || data[i] == 0x7F)                                                         // or is delete
            || (len == 2 && (uint8_t)data[i] == 0xC2 && (uint8_t)data[i + 1] <= 0xBF)) { // or is utf8
        } else {
            if (newlen + len > maxlen) {
                break;
            }
            if (newlen != i) {
                memcpy(data + newlen, data + i, len);
            }
            newlen += len;
        }
        i += len;
    }

    if (newlen == 0) {
        return;
    }

    char *p = active_edit->data + edit_sel.start;

    if (edit_sel.length) {
        edit_do(active_edit, edit_sel.start, edit_sel.length, 1);
    }

    memmove(p + newlen, p + edit_sel.length, active_edit->length - (edit_sel.start + edit_sel.length));
    memcpy(p, data, newlen);

    edit_do(active_edit, edit_sel.start, newlen, 0);

    active_edit->length += newlen - edit_sel.length;

    if (select) {
        edit_sel.length = newlen;
        setselection(active_edit->data + edit_sel.start, newlen);
    } else {
        edit_sel.start  = edit_sel.start + newlen;
        edit_sel.length = 0;
    }

    edit_sel.p1 = edit_sel.start;
    edit_sel.p2 = edit_sel.start + edit_sel.length;

    if (active_edit->onchange) {
        active_edit->onchange(active_edit);
    }

    edit_redraw();
}

void edit_resetfocus(void) {
    edit_select = 0;
    setactive(NULL);
}

void edit_setfocus(EDIT *edit) {
    if (active_edit == edit) {
        return;
    }
    edit_select    = 0;
    edit_sel.start = edit_sel.p1 = 0;
    edit_sel.length = edit_sel.p2 = edit->length;
    edit_sel.mark_start           = 0;
    edit_sel.mark_length          = 0;
    setactive(edit);
}

bool edit_active(void) {
    return (active_edit != NULL);
}

EDIT *edit_get_active(void) {
    return active_edit;
}

void edit_setstr(EDIT *edit, char *str, uint16_t length) {
    if (length >= edit->maxlength) {
        length = edit->maxlength;
    }

    edit->length = length;
    memcpy(edit->data, str, length);

    if (edit->onchange) {
        edit->onchange(edit);
    }
}

void edit_setcursorpos(EDIT *edit, uint16_t pos) {
    if (pos <= edit->length) {
        edit_sel.p1 = pos;
    } else {
        edit_sel.p1 = edit->length;
    }

    edit_sel.p2 = edit_sel.start = edit_sel.p1;
    edit_sel.length              = 0;
}

uint16_t edit_getcursorpos(void) {
    return edit_sel.p1 < edit_sel.p2 ? edit_sel.p1 : edit_sel.p2;
}

bool edit_getmark(uint16_t *outloc, uint16_t *outlen) {
    if (outloc) {
        *outloc = edit_sel.mark_start;
    }
    if (outlen) {
        *outlen = edit_sel.mark_length;
    }

    return (active_edit && edit_sel.mark_length) ? 1 : 0;
}

void edit_setmark(uint16_t loc, uint16_t len) {
    edit_sel.mark_start  = loc;
    edit_sel.mark_length = len;
}

void edit_setselectedrange(uint16_t loc, uint16_t len) {
    edit_sel.start = edit_sel.p1 = loc;
    edit_sel.length              = len;
    edit_sel.p2                  = loc + len;
}
