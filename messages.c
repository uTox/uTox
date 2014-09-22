#include "main.h"

// Ideally this function would have returned an array of simultaneously
// typing friends, e.g. in a groupchat. But since groupchats don't support
// notifications, it simply returns the friend associated with
// given MESSAGES, or NULL if he is not typing.
static FRIEND* get_typers(MESSAGES *m) {
    // Currently only friends support typing notifications
    if(sitem->item == ITEM_FRIEND) {
        FRIEND *f = sitem->data;
        // check just in case that we're given the messages panel for
        // the currently selected friend
        if(&f->msg == m->data) {
            if(f->typing) return f;
        }
    }
    return NULL;
}

void messages_draw(MESSAGES *m, int x, int y, int width, int height)
{
    setcolor(0);
    setfont(FONT_TEXT);

    uint8_t lastauthor = 0xFF;

    void **p = m->data->data;
    int i, n = m->data->n;

    for(i = 0; i != n; i++) {
        MESSAGE *msg = *p++;

	if(msg->height == 0) {
            return;
	}

        if(y + msg->height <= 0) { //! NOTE: should not be constant 0
            y += msg->height;
            continue;
        }

        if(y >= height + 50 * SCALE) { //! NOTE: should not be constant 100
            break;
        }

        setcolor(LIST_MAIN);
        setfont(FONT_MISC);
        char timestr[6];
        int len;
        len = snprintf(timestr, sizeof(timestr), "%u:%.2u", msg->time / 60, msg->time % 60);
        drawtext(x + width - TIME_WIDTH, y, (uint8_t*)timestr, len);

        if(m->type) {
            /* group */
            setcolor(0);
            setfont(FONT_TEXT);
            drawtextwidth_right(x, MESSAGES_X - NAME_OFFSET, y, &msg->msg[msg->length] + 1, msg->msg[msg->length]);
        } else {
            FRIEND *f = &friend[m->data->id];
            uint8_t author = msg->flags & 1;
            if(author != lastauthor) {
                setfont(FONT_TEXT);
                if(!author) {
                    setcolor(0);
                    drawtextwidth_right(x, MESSAGES_X - NAME_OFFSET, y, f->name, f->name_length);
                } else {
                    setcolor(CHAT_SELF);
                    drawtextwidth_right(x, MESSAGES_X - NAME_OFFSET, y, self.name, self.name_length);
                }
                lastauthor = author;
            } else {
                if(!author) {
                    setcolor(0);
                } else {
                    setcolor(CHAT_SELF);
                }
            }
        }
        /**/

        switch(msg->flags) {
        case 0:
        case 1:
        case 2:
        case 3: {
            /* normal message */
            int h1 = 0xFFFF, h2 = 0xFFFF;
            if(i == m->data->istart) {
                h1 = m->data->start;
                h2 = ((i == m->data->iend) ? m->data->end : msg->length);
            } else if(i == m->data->iend) {
                h1 = 0;
                h2 = m->data->end;
            } else if(i > m->data->istart && i < m->data->iend) {
                h1 = 0;
                h2 = msg->length;
            }

            if((m->data->istart == m->data->iend && m->data->start == m->data->end) || h1 == h2) {
                h1 = 0xFFFF;
                h2 = 0xFFFF;
            }

            setfont(FONT_TEXT);
            int ny = drawtextmultiline(x + MESSAGES_X, x + width - TIME_WIDTH, y, y, y + msg->height, font_small_lineheight, msg->msg, msg->length, h1, h2 - h1, 1);
            if(ny - y != msg->height - MESSAGES_SPACING) {
                debug("error101 %u %u\n", ny -y, msg->height - MESSAGES_SPACING);
            }
            y = ny;

            break;
        }

        case 4:
        case 5: {
            /* image */
            MSG_IMG *img = (void*)msg;
            int maxwidth = width - MESSAGES_X - TIME_WIDTH;
            drawimage(img->data, x + MESSAGES_X, y, img->w, img->h, maxwidth, img->zoom, img->position);
            y += (img->zoom || img->w <= maxwidth) ? img->h : img->h * maxwidth / img->w;
            break;
        }

        case 6:
        case 7: {
            MSG_FILE *file = (void*)msg;
            int dx = MESSAGES_X;
            int xx = x + dx;
            _Bool mo = (m->iover == i);

            uint8_t size[16];
            int sizelen = sprint_bytes(size, sizeof(size), file->size);

            setcolor(WHITE);
            setfont(FONT_MISC);

            if(file->status == FILE_DONE) {
                drawalpha(BM_FT, xx, y, BM_FT_WIDTH, BM_FT_HEIGHT, (mo && m->over) ? C_GREEN_LIGHT : C_GREEN);
                drawalpha(BM_YES, xx + BM_FTM_WIDTH + SCALE + (BM_FTB_WIDTH - BM_FB_WIDTH) / 2, y + SCALE * 4, BM_FB_WIDTH, BM_FB_HEIGHT, WHITE);
                if(file->inline_png) {
                    drawstr(xx + 5 * SCALE, y + 17 * SCALE, CLICKTOSAVE);
                } else {
                    drawstr(xx + 5 * SCALE, y + 17 * SCALE, CLICKTOOPEN);
                }
            } else if(file->status == FILE_KILLED) {
                drawalpha(BM_FT, xx, y, BM_FT_WIDTH, BM_FT_HEIGHT, C_RED);
                drawalpha(BM_NO, xx + BM_FTM_WIDTH + SCALE + (BM_FTB_WIDTH - BM_FB_WIDTH) / 2, y + SCALE * 4, BM_FB_WIDTH, BM_FB_HEIGHT, WHITE);
                drawstr(xx + 5 * SCALE, y + 17 * SCALE, CANCELLED);
            } else {
                if(file->status == FILE_BROKEN) {
                    drawalpha(BM_FTM, xx, y, BM_FTM_WIDTH, BM_FT_HEIGHT, C_RED);
                } else if(file->status == FILE_OK) {
                    drawalpha(BM_FTM, xx, y, BM_FTM_WIDTH, BM_FT_HEIGHT, BLUE);
                } else {
                    drawalpha(BM_FTM, xx, y, BM_FTM_WIDTH, BM_FT_HEIGHT, C_GRAY);
                    setcolor(GRAY(98));
                }

                int x = xx + BM_FTM_WIDTH + SCALE;
                drawalpha(BM_FTB1, x, y, BM_FTB_WIDTH, BM_FTB_HEIGHT + SCALE, (mo && m->over == 1) ? C_GREEN_LIGHT : C_GREEN);
                drawalpha(BM_NO, x + (BM_FTB_WIDTH - BM_FB_WIDTH) / 2, y + SCALE * 4, BM_FB_WIDTH, BM_FB_HEIGHT, WHITE);

                uint32_t color = ((msg->flags == 7 && file->status == FILE_PENDING) || file->status == FILE_BROKEN || file->status == FILE_PAUSED_OTHER) ? C_GRAY: ((mo && m->over == 2) ? C_GREEN_LIGHT : C_GREEN);
                drawalpha(BM_FTB2, x, y + BM_FTB_HEIGHT + SCALE * 2, BM_FTB_WIDTH, BM_FTB_HEIGHT, color);
                drawalpha((msg->flags == 6 && file->status ==  FILE_PENDING) ? BM_YES : (file->status == FILE_PAUSED ? BM_RESUME : BM_PAUSE), x + (BM_FTB_WIDTH - BM_FB_WIDTH) / 2, y + BM_FTB_HEIGHT + SCALE * 5, BM_FB_WIDTH, BM_FB_HEIGHT, color == C_GRAY ? LIST_MAIN : WHITE);


                uint64_t progress = file->progress;
                if(progress > file->size) {
                    progress = file->size;
                }

                uint32_t w = (file->size == 0) ? 0 : (progress * (uint64_t)106 * SCALE) / file->size;

                color = (file->status == FILE_PENDING || file->status == FILE_PAUSED || file->status == FILE_PAUSED_OTHER) ? LIST_MAIN : WHITE;
                framerect(xx + 5 * SCALE, y + 17 * SCALE, xx + 111 * SCALE, y + 24 * SCALE, color);
                drawrectw(xx + 5 * SCALE, y + 17 * SCALE, w, 7 * SCALE, color);

                if(file->status == FILE_OK) {
                    uint8_t text[16];
                    int len;

                    len = sprint_bytes(text, sizeof(text), file->speed);
                    text[len++] = '/';
                    text[len++] = 's';

                    drawtext(xx + 5 * SCALE + 53 * SCALE - textwidth(text, len) / 2, y + 10 * SCALE, text, len);

                    uint64_t etasec = 0;
                    if(file->speed) {
                        etasec = (file->size - progress) / file->speed;
                    }

                    len = snprintf((char*)text, sizeof(text), "%us", (uint32_t)etasec);

                    drawtext(xx + 5 * SCALE + 106 * SCALE - textwidth(text, len), y + 10 * SCALE, text, len);
                }
            }


            drawtextwidth(xx + 5 * SCALE, 106 * SCALE, y + 3 * SCALE, file->name, file->name_length);
            drawtext(xx + 5 * SCALE, y + 10 * SCALE, size, sizelen);

            y += BM_FT_HEIGHT;

            break;
        }
        }

        y += MESSAGES_SPACING;
    }

    if(i == n) {
        // Last message is visible. Append typing notifications, if needed.
        FRIEND *f = get_typers(m);
        if(f) {
            setfont(FONT_TEXT);
            setcolor(C_GRAY2);
            drawtextwidth_right(x, MESSAGES_X - NAME_OFFSET, y, f->name, f->name_length);
            drawtextwidth(x + MESSAGES_X, x + width, y, S(IS_TYPING), SLEN(IS_TYPING));
        }
    }
}

_Bool messages_mmove(MESSAGES *m, int UNUSED(px), int UNUSED(py), int width, int UNUSED(height), int mx, int my, int dx, int UNUSED(dy))
{
    if(m->idown < m->data->n) {
        int maxwidth = width - MESSAGES_X - TIME_WIDTH;
        MSG_IMG *img_down = m->data->data[m->idown];
        if((img_down->flags & (~1)) == 4 && img_down->w  > maxwidth) {
            img_down->position -= (double)dx / (double)(img_down->w - maxwidth);
            if(img_down->position > 1.0) {
                img_down->position = 1.0;
            } else if(img_down->position < 0.0) {
                img_down->position = 0.0;
            }
            cursor = CURSOR_ZOOM_OUT;
            return 1;
        }
    }

    if(mx < 0 || my > m->data->height || my < 0) {
        if(m->iover != ~0) {
            m->iover = ~0;
            return 1;
        }
        return 0;
    }

    setfont(FONT_TEXT);

    void **p = m->data->data;
    int i = 0, n = m->data->n;
    _Bool redraw = 0;

    while(i != n) {
        MESSAGE *msg = *p++;

        int dy = msg->height;

        if(my >= 0 && my < dy) {
            switch(msg->flags) {
            case 0:
            case 1:
            case 2:
            case 3: {
                /* normal message */
                m->over = hittextmultiline(mx - MESSAGES_X, width - MESSAGES_X - TIME_WIDTH, my < 0 ? 0 : my, msg->height, font_small_lineheight, msg->msg, msg->length, 1);
                m->urlover = 0xFFFF;

                if(my < 0 || my >= dy || mx < MESSAGES_X || m->over == msg->length) {
                    break;
                }

                cursor = CURSOR_TEXT;

                char_t *str = msg->msg + m->over;
                while(str != msg->msg) {
                    str--;
                    if(*str == ' ' || *str == '\n') {
                        str++;
                        break;
                    }
                }

                char_t *end = msg->msg + msg->length;
                while(str != end && *str != ' ' && *str != '\n') {
                    if(m->urlover == 0xFFFF && end - str >= 7 && strcmp2(str, "http://") == 0) {
                        cursor = CURSOR_HAND;
                        m->urlover = str - msg->msg;
                    }

                    if(m->urlover == 0xFFFF && end - str >= 8 && strcmp2(str, "https://") == 0) {
                        cursor = CURSOR_HAND;
                        m->urlover = str - msg->msg;
                    }

                    str++;
                }

                if(m->urlover != 0xFFFF) {
                    m->urllen = (str - msg->msg) - m->urlover;
                }

                break;
            }

            case 4:
            case 5: {
                m->over = 0;

                MSG_IMG *img = (void*)msg;
                int maxwidth = width - MESSAGES_X - TIME_WIDTH;

                if(img->w > maxwidth) {
                    mx -= MESSAGES_X;
                    int w = img->w > maxwidth ? maxwidth : img->w;
                    int h = (img->zoom || img->w <= maxwidth) ? img->h : img->h * maxwidth / img->w;
                    if(mx >= 0 && my >= 0 && mx < w && my < h) {
                        m->over = 1;
                        cursor = CURSOR_ZOOM_IN + img->zoom;
                    }
                }
                break;
            }

            case 6:
            case 7: {
                uint8_t over = 0;
                MSG_FILE *file = (void*)msg;

                mx -= MESSAGES_X;
                if(mx >= 0 && mx < BM_FT_WIDTH && my >= 0 && my < BM_FT_HEIGHT) {
                    over = 3;
                    mx -= BM_FTM_WIDTH + SCALE;
                    if(mx >= 0) {
                        if(my < BM_FTB_HEIGHT + SCALE) {
                            over = 1;
                        } else if(my >= BM_FTB_HEIGHT + SCALE * 2) {
                            over = 2;
                        }
                    }
                }

                static const uint8_t f[14] = {
                    0b011,
                    0b001,

                    0b011,
                    0b011,

                    0b011,
                    0b011,

                    0b001,
                    0b001,

                    0b001,
                    0b001,

                    0b000,
                    0b000,

                    0b111,
                    0b111,
                };

                if(over && f[file->status * 2 + (file->flags == 7)] & (1 << (over - 1))) {
                    cursor = CURSOR_HAND;
                }

                if(over != m->over) {
                    redraw = 1;
                    m->over = over;
                }

                break;
            }
            }

            if(i != m->iover && m->iover != ~0 && ((msg->flags & 0xFFFE) == 6 || (((MESSAGE*)(m->data->data[m->iover]))->flags & 0xFFFE) == 6)) {
                redraw = 1;
            }

            m->iover = i;

            if(m->select) {
                int start, end, istart, iend;
                if(i > m->idown) {
                    istart = m->idown;
                    iend = i;

                    start = m->down;
                    end = m->over;
                } else if(i < m->idown) {
                    iend = m->idown;
                    istart = i;

                    end = m->down;
                    start = m->over;
                } else {
                    istart = iend = i;
                    if(m->over >= m->down) {
                        start = m->down;
                        end = m->over;
                    } else {
                        end = m->down;
                        start = m->over;
                    }
                }

                if(start != m->data->start || istart != m->data->istart || end != m->data->end || iend != m->data->iend) {
                    m->data->start = start;
                    m->data->end = end;
                    m->data->istart = istart;
                    m->data->iend = iend;
                    redraw = 1;
                }

            }
            return redraw;
        }

        my -= dy;

        i++;
    }

    return 0;
}

_Bool messages_mdown(MESSAGES *m)
{
    m->idown = 0xFFFF;
    if(m->iover != ~0) {
        MESSAGE *msg = m->data->data[m->iover];
        switch(msg->flags) {
        case 0 ... 3: {
            if(m->urlover != 0xFFFF) {
                char_t url[m->urllen + 1];
                memcpy(url, msg->msg + m->urlover, m->urllen * sizeof(char_t));
                url[m->urllen] = 0;

                openurl(url);
            }

            m->data->istart = m->data->iend = m->idown = m->iover;
            m->data->start = m->data->end = m->down = m->over;
            m->select = 1;
            break;
        }

        case 4:
        case 5: {
            MSG_IMG *img = (void*)msg;
            if(m->over) {
                if(!img->zoom) {
                    img->zoom = 1;
                    message_updateheight(m, msg, m->data);
                } else {
                    m->idown = m->iover;
                }
            }
            break;
        }

        case 6 ... 7: {
            MSG_FILE *file = (void*)msg;
            if(m->over == 0) {
                break;
            }

            switch(file->status) {
            case FILE_PENDING: {
                if(msg->flags == 6) {
                    if(m->over == 2) {
                        savefilerecv(m->data->id, file);
                    } else if(m->over == 1) {
                        //decline
                        tox_postmessage(TOX_FILE_IN_CANCEL, m->data->id, file->filenumber, NULL);
                    }
                } else if(m->over == 1) {
                    //cancel
                    tox_postmessage(TOX_FILE_OUT_CANCEL, m->data->id, file->filenumber, NULL);
                }


                break;
            }

            case FILE_OK: {
                if(m->over == 2) {
                    //pause
                    tox_postmessage(TOX_FILE_IN_PAUSE + (msg->flags & 1), m->data->id, file->filenumber, NULL);
                } else if(m->over == 1) {
                    //cancel
                    tox_postmessage(TOX_FILE_IN_CANCEL + (msg->flags & 1), m->data->id, file->filenumber, NULL);
                }
                break;
            }

            case FILE_PAUSED: {
                if(m->over == 2) {
                    //resume
                    tox_postmessage(TOX_FILE_IN_RESUME + (msg->flags & 1), m->data->id, file->filenumber, NULL);
                } else if(m->over == 1) {
                    //cancel
                    tox_postmessage(TOX_FILE_IN_CANCEL + (msg->flags & 1), m->data->id, file->filenumber, NULL);
                }
                break;
            }

            case FILE_PAUSED_OTHER:
            case FILE_BROKEN: {
                //cancel
                if(m->over == 1) {
                    tox_postmessage(TOX_FILE_IN_CANCEL + (msg->flags & 1), m->data->id, file->filenumber, NULL);
                }
                break;
            }

            case FILE_DONE: {
                if(m->over) {
                    if(file->inline_png) {
                        savefiledata(file);
                    } else {
                        openurl(file->path);
                    }
                }
                break;
            }
            }
            break;
        }
        }

        return 1;
    } else {
        if(m->data->istart != m->data->iend || m->data->start != m->data->end) {
            m->data->istart = 0;
            m->data->iend = 0;
            m->data->start = 0;
            m->data->end = 0;
            return 1;
        }
    }

    return 0;
}

_Bool messages_dclick(MESSAGES *m, _Bool triclick)
{
    if(m->iover != ~0) {
        MESSAGE *msg = m->data->data[m->iover];
        if(msg->flags >= 0 && msg->flags <= 3) {
            m->data->istart = m->data->iend = m->iover;

            uint8_t c = triclick ? '\n' : ' ';

            uint16_t i = m->over;
            while(i != 0 && msg->msg[i - 1] != c) {
                i -= utf8_unlen(msg->msg + i);
            }
            m->data->start = i;
            i = m->over;
            while(i != msg->length && msg->msg[i] != c) {
                i += utf8_len(msg->msg + i);
            }
            m->data->end = i;
            return 1;
        } else if(msg->flags >= 4 && msg->flags <= 5) {
            MSG_IMG *img = (void*)msg;
            if(m->over) {
                if(img->zoom) {
                    img->zoom = 0;
                    message_updateheight(m, msg, m->data);
                }
            }
            return 1;
        }
    }
    return 0;
}

static void contextmenu_messages_onselect(uint8_t i)
{
    switch(i) {
    case 0:
        copy(1);
        break;
    case 1:
        copy(0);
        break;
    }
}

_Bool messages_mright(MESSAGES *m)
{
    static UI_STRING_ID menu_copy[] = {STR_COPY, STR_COPYWITHOUTNAMES};
    if(m->iover != ~0 && ((MESSAGE*)m->data->data[m->iover])->flags <= 3) {
        contextmenu_new(countof(menu_copy), menu_copy, contextmenu_messages_onselect);
        return 1;
    }
    return 0;
}

_Bool messages_mwheel(MESSAGES *UNUSED(m), int UNUSED(height), double UNUSED(d))
{
    return 0;
}


_Bool messages_mup(MESSAGES *m)
{
    //temporary, change this
    if(m->select) {
        uint8_t *lel = malloc(65536);
        setselection(lel, messages_selection(m, lel, 65536, 0));
        free(lel);


        m->select = 0;
    }

    m->idown = 0xFFFF;

    return 0;
}

_Bool messages_mleave(MESSAGES *UNUSED(m))
{
    return 0;
}

int messages_selection(MESSAGES *m, void *data, uint32_t len, _Bool names)
{
    if(m->data->n == 0) {
        *(uint8_t*)data = 0;
        return 0;
    }

    int i = m->data->istart, n = m->data->iend + 1;
    void **dp = &m->data->data[i];

    char_t *p = data;

    while(i != n) {
        MESSAGE *msg = *dp++;

        if(names && (i != m->data->istart || m->data->start == 0)) {
            if(m->type) {
                uint8_t l = (uint8_t)msg->msg[msg->length];
                if(len <= l) {
                    break;
                }

                memcpy(p, &msg->msg[msg->length + 1], l);
                p += l;
                len -= l;
            } else {
                FRIEND *f = &friend[m->data->id];
                uint8_t author = msg->flags & 1;

                if(!author) {
                    if(len <= f->name_length) {
                        break;
                    }

                    memcpy(p, f->name, f->name_length);
                    p += f->name_length;
                    len -= f->name_length;
                } else {
                    if(len <= self.name_length) {
                        break;
                    }

                    memcpy(p, self.name, self.name_length);
                    p += self.name_length;
                    len -= self.name_length;
                }
            }

            if(len <= 2) {
                break;
            }

            strcpy2(p, ": ");
            p += 2;
            len -= 2;
        }

        switch(msg->flags) {
        case 0:
        case 1:
        case 2:
        case 3: {
            uint8_t *data;
            uint16_t length;
            if(i == m->data->istart) {
                if(i == m->data->iend) {
                    data = msg->msg + m->data->start;
                    length = m->data->end - m->data->start;
                } else {
                    data = msg->msg + m->data->start;
                    length = msg->length - m->data->start;
                }
            } else if(i == m->data->iend) {
                data = msg->msg;
                length = m->data->end;
            } else {
                data = msg->msg;
                length = msg->length;
            }

            if(len <= length) {
                goto BREAK;
            }

            memcpy(p, data, length);
            p += length;
            len -= length;
            break;
        }
        }

        i++;

        if(i != n) {
            #ifdef __WIN32__
            if(len <= 2) {
                break;
            }
            *p++ = '\r';
            *p++ = '\n';
            len -= 2;
            #else
            if(len <= 1) {
                break;
            }
            *p++ = '\n';
            len--;
            #endif
        }
    }
    BREAK:
    *p = 0;

    return (void*)p - data;
}

static int msgheight(MESSAGE *msg, int width)
{
    switch(msg->flags) {
    case 0:
    case 1:
    case 2:
    case 3: {
        int theight = text_height(width - MESSAGES_X - TIME_WIDTH, font_small_lineheight, msg->msg, msg->length);
        return (theight == 0) ? 0 : theight + MESSAGES_SPACING;
    }

    case 4:
    case 5: {
        MSG_IMG *img = (void*)msg;
        int maxwidth = width - MESSAGES_X - TIME_WIDTH;
        return ((img->zoom || img->w <= maxwidth) ? img->h : img->h * maxwidth / img->w) + MESSAGES_SPACING;
    }

    case 6:
    case 7: {
        return BM_FT_HEIGHT + MESSAGES_SPACING;
    }

    }

    return 0;
}

void messages_updateheight(MESSAGES *m)
{
    MSG_DATA *data = m->data;
    if(!data) {
        return;
    }

    setfont(FONT_TEXT);

    uint32_t height = 0;
    int i = 0;
    while(i < data->n) {
        MESSAGE *msg = data->data[i];
        msg->height = msgheight(msg, m->width);
        height += msg->height;
        i++;
    }
    if(get_typers(m)) {
        // Add space for typing notification
        height += font_small_lineheight + MESSAGES_SPACING;
    }

    m->height = height;
    data->height = height;
    data->width = m->width;
    m->panel.content_scroll->content_height = height;
}

static void message_setheight(MESSAGES *m, MESSAGE *msg, MSG_DATA *p)
{
    if(m->width == 0) {
        return;
    }

    setfont(FONT_TEXT);

    msg->height = msgheight(msg, m->width);
    p->height += msg->height;
    if(m->data == p) {
        m->panel.content_scroll->content_height = p->height;
    }
}

void message_updateheight(MESSAGES *m, MESSAGE *msg, MSG_DATA *p)
{
    if(m->width == 0) {
        return;
    }

    setfont(FONT_TEXT);

    p->height -= msg->height;
    msg->height = msgheight(msg, m->width);
    p->height += msg->height;
    if(m->data == p) {
        m->panel.content_scroll->content_height = p->height;
    }
}

void message_add(MESSAGES *m, MESSAGE *msg, MSG_DATA *p)
{
    time_t rawtime;
    struct tm *ti;
    time(&rawtime);
    ti = localtime(&rawtime);

    msg->time = ti->tm_hour * 60 + ti->tm_min;

    if(p->n != 128) {
        p->data = realloc(p->data, (p->n + 1) * sizeof(void*));
        p->data[p->n++] = msg;
    } else {
        p->height -= ((MESSAGE*)p->data[0])->height;
        message_free(p->data[0]);
        memmove(p->data, p->data + 1, 127 * sizeof(void*));
        p->data[127] = msg;
        if(p->start != 0xFFFF) {
            if(!p->istart) {
                if(!p->iend) {
                    p->istart = p->iend = 0xFFFF;
                } else {
                    p->start = 0;
                }
            } else {
                p->istart--;
                p->iend--;
            }
        }
    }

    message_setheight(m, msg, p);
}

void messages_set_typing(MESSAGES *m, MSG_DATA *p, int UNUSED(typing)) {
    if(m->data == p) {
        // MSG_DATA associated with typing notification
        // corresponds to given MESSAGES, so update their height.
        messages_updateheight(m);
    }
}

_Bool messages_char(uint32_t ch)
{
    MESSAGES *m;
    if(sitem->item == ITEM_FRIEND) {
        m = &messages_friend;
    } else if(sitem->item == ITEM_GROUP) {
        m = &messages_group;
    } else {
        return 0;
    }

    switch(ch) {
        //!TODO: not constant 0.25
        case KEY_PAGEUP: {
            SCROLLABLE *scroll = m->panel.content_scroll;
            scroll->d -= 0.25;
            if(scroll->d < 0.0) {
                scroll->d = 0.0;
            }
            redraw();
            return 1;
        }

        case KEY_PAGEDOWN: {
            SCROLLABLE *scroll = m->panel.content_scroll;
            scroll->d += 0.25;
            if(scroll->d > 1.0) {
                scroll->d = 1.0;
            }
            redraw();
            return 1;
        }
    }

    return 0;
}

void message_free(MESSAGE *msg)
{
    switch(msg->flags >> 1) {
    case 2:
        //TODO: freeimage
        break;
    case 3:
        //already gets free()d
        //free(((MSG_FILE*)msg)->path);
        break;
    }
    free(msg);
}
