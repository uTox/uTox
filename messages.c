#include "main.h"

static int textout(int x, int y, char_t *str, uint16_t length, int d, int h1, int h2)
{
    h1 -= d;
    h2 -= d;

    if(h1 < 0) {
        h1 = 0;
    }

    if(h2 < 0) {
        h2 = 0;
    }

    if(h1 > length) {
        h1 = length;
    }

    if(h2 > length) {
        h2 = length;
    }

    int width;

    width = drawtext_getwidthW(x, y, str, h1);

    uint32_t color = setcolor(TEXT_HIGHLIGHT);

    int w = textwidthW(str + h1, h2 - h1);
    drawrect(x + width, y, w, font_msg_lineheight, TEXT_HIGHLIGHT_BG);
    drawtextW(x + width, y, str + h1, h2 - h1);
    width += w;

    setcolor(color);

    width += drawtext_getwidthW(x + width, y, str + h2, length - h2);

    return width;
}

static int drawmsg(int x, int y, char_t *str, uint16_t length, int h1, int h2)
{
    _Bool word = 0;
    int xc = x;
    char_t *a = str, *b = str, *end = str + length;
    while(a != end) {
        switch(*a) {
        case '\n': {
            textout(x, y, b, a - b, b - str, h1, h2);
            y += font_msg_lineheight;
            x = xc;
            b = a + 1;

            setcolor(0);
            setfont(FONT_MSG);
            word = 0;
            break;
        }

        case ' ': {
            if(word) {
                int count = a - b;
                x += textout(x, y, b, count, b - str, h1, h2);
                b = a;

                setcolor(0);
                setfont(FONT_MSG);
                word = 0;
            }
            break;
        }

        case 'h': {
            if((end - a >= 7 && strcmp2(a, "http://") == 0) || (end - a >= 8 && strcmp2(a, "https://") == 0)) {
                int count = a - b;
                x += textout(x, y, b, count,  b - str, h1, h2);
                b = a;

                setcolor(COLOR_LINK);
                setfont(FONT_MSG_LINK);
                word = 1;
            }
            break;
        }
        }
        a++;
    }

    textout(x, y, b, a - b, b - str, h1, h2);
    y += font_msg_lineheight;
    setcolor(0);
    setfont(FONT_MSG);

    //RECT r = {x, y, x + width, bottom};
    //y += DrawTextW(hdc, out, length, &r, DT_WORDBREAK | DT_NOPREFIX);

    return y;
}

static uint32_t pmsg(int mx, int my, char_t *str, uint16_t length)
{
    char_t *a = str, *b = str, *end = str + length;
    while(a != end) {
        if(*a == '\n') {
            if(my >= 0 && my <= font_msg_lineheight) {
                break;
            }
            b = a;
            my -= font_msg_lineheight;
        }
        a++;
    }

    int fit;
    mx -= 110;
    if(mx > 0) {
        int len = a - b;
        fit = textfitW(b, len, mx);
    } else {
        fit = 0;
    }

    return (b - str) + fit;
}

static int heightmsg(char_t *str, uint16_t length)
{
    int y = 0;
    char_t *a = str, *end = str + length;
    while(a != end) {
        if(*a == '\n') {
            y += font_msg_lineheight;
        }
        a++;
    }

    y += font_msg_lineheight;

    return y;
}

void messages_draw(MESSAGES *m, int x, int y, int width, int height)
{
    setcolor(0);
    setfont(FONT_MSG);

    uint8_t lastauthor = 0xFF;

    void **p = m->data->data;
    int i = 0, n = m->data->n;

    while(i != n) {
        MESSAGE *msg = *p++;

        if(m->type) {
            /* group */
            setfont(FONT_MSG_NAME);
            drawtextwidth_rightW(x, 95, y, &msg->msg[msg->length] + 1, (uint16_t)msg->msg[msg->length]);
            setfont(FONT_MSG);
        } else {
            FRIEND *f = &friend[m->data->id];
            uint8_t author = msg->flags & 1;
            if(author != lastauthor) {
                if(!author) {
                    setcolor(0);
                    setfont(FONT_MSG_NAME);
                    drawtextwidth_right(x, 95, y, f->name, f->name_length);
                    setfont(FONT_MSG);
                } else {
                    setcolor(0x888888);
                    drawtextwidth_right(x, 95, y, self.name, self.name_length);
                    setcolor(0);
                }
                lastauthor = author;
            }
        }
        /**/

        switch(msg->flags) {
        case 0:
        case 1:
        case 2:
        case 3: {
            /* normal message */
            setcolor(0);
            if(i == m->data->istart) {
                y = drawmsg(x + 110, y, msg->msg, msg->length, m->data->start, ((i == m->data->iend) ? m->data->end : msg->length));
            } else if(i == m->data->iend) {
                y = drawmsg(x + 110, y, msg->msg, msg->length, 0, m->data->end);
            } else if(i > m->data->istart && i < m->data->iend) {
                y = drawmsg(x + 110, y, msg->msg, msg->length, 0, msg->length);
            } else {
                y = drawmsg(x + 110, y, msg->msg, msg->length, 0, 0);
            }

            break;
        }

        case 4:
        case 5: {
            /* image */
            MSG_IMG *img = (void*)msg;
            //SIZE size;

            //SelectObject(hdcMem, img->bitmap);
            //GetBitmapDimensionEx(img->bitmap, &size);
            //BitBlt(hdc, x, y, size.cx, size.cy, hdcMem, 0, 0, SRCCOPY);
            y += img->height;
            break;
        }

        case 6:
        case 7: {
            MSG_FILE *file = (void*)msg;
            int dx = 110;

            uint8_t size[16];
            int sizelen = sprint_bytes(size, file->size);

            switch(file->status) {
            case FILE_PENDING: {
                if(msg->flags == 6) {
                    setcolor(0x888888);
                    dx += drawstr_getwidth(x + dx, y, "wants to share file ");
                    setcolor(0);
                    dx += drawtext_getwidth(x + dx, y, file->name, file->name_length);
                    setcolor(0x888888);
                    dx += drawstr_getwidth(x + dx, y, " (");
                    setcolor(0);
                    dx += drawtext_getwidth(x + dx, y, size, sizelen);
                    setcolor(0x888888);
                    dx += drawstr_getwidth(x + dx, y, ") ");
                    setcolor(COLOR_LINK);
                    setfont(FONT_MSG_LINK);
                    dx += drawstr_getwidth(x + dx, y, "Accept");
                    drawstr(x + dx + 10, y, "Decline");
                    setfont(FONT_MSG);
                } else {
                    setcolor(0x888888);
                    dx += drawstr_getwidth(x + dx, y, "offering file ");
                    setcolor(0);
                    dx += drawtext_getwidth(x + dx, y, file->name, file->name_length);
                    setcolor(0x888888);
                    dx += drawstr_getwidth(x + dx, y, " (");
                    setcolor(0);
                    dx += drawtext_getwidth(x + dx, y, size, sizelen);
                    setcolor(0x888888);
                    dx += drawstr_getwidth(x + dx, y, ") ");
                    setcolor(COLOR_LINK);
                    setfont(FONT_MSG_LINK);
                    drawstr(x + dx, y, "Cancel");
                    setfont(FONT_MSG);
                }

                break;
            }

            case FILE_OK: {
                setcolor(0x888888);
                dx += drawstr_getwidth(x + dx, y, "transferring file ");
                setcolor(0);
                dx += drawtext_getwidth(x + dx, y, file->name, file->name_length);

                setcolor(COLOR_LINK);
                setfont(FONT_MSG_LINK);
                dx += drawstr_getwidth(x + dx + 10, y, "Pause");
                drawstr(x + dx + 20, y, "Cancel");
                setfont(FONT_MSG);
                break;
            }

            case FILE_PAUSED: {
                setcolor(0x888888);
                dx += drawstr_getwidth(x + dx, y, "transferring file (paused) ");
                setcolor(0);
                dx += drawtext_getwidth(x + dx, y, file->name, file->name_length);

                setcolor(COLOR_LINK);
                setfont(FONT_MSG_LINK);
                dx += drawstr_getwidth(x + dx + 10, y, "Resume");
                drawstr(x + dx + 20, y, "Cancel");
                setfont(FONT_MSG);
                break;
            }

            case FILE_BROKEN: {
                setcolor(0x888888);
                dx += drawstr_getwidth(x + dx, y, "transferring file (disconnected) ");
                setcolor(0);
                dx += drawtext_getwidth(x + dx, y, file->name, file->name_length);

                setcolor(COLOR_LINK);
                setfont(FONT_MSG_LINK);
                drawstr(x + dx + 10, y, "Cancel");
                setfont(FONT_MSG);
                break;
            }

            case FILE_KILLED: {
                setcolor(0x888888);
                dx += drawstr_getwidth(x + dx, y, "cancelled file ");
                setcolor(0);
                dx += drawtext_getwidth(x + dx, y, file->name, file->name_length);

                break;
            }

            case FILE_DONE: {
                if(msg->flags == 6) {
                    setcolor(0x888888);
                    dx += drawstr_getwidth(x + dx, y, "transferred file ");
                    setcolor(0);
                    dx += drawtext_getwidth(x + dx, y, file->name, file->name_length);

                    setcolor(COLOR_LINK);
                    setfont(FONT_MSG_LINK);
                    drawstr(x + dx + 10, y, "Open");
                    setfont(FONT_MSG);

                } else {
                    setcolor(0x888888);
                    dx += drawstr_getwidth(x + dx, y, "transferred file ");
                    setcolor(0);
                    dx += drawtext_getwidth(x + dx, y, file->name, file->name_length);
                    setcolor(0x888888);
                    dx += drawstr_getwidth(x + dx, y, " (");
                    setcolor(0);
                    dx += drawtext_getwidth(x + dx, y, size, sizelen);
                    setcolor(0x888888);
                    dx += drawstr_getwidth(x + dx, y, ") ");
                }

                break;
            }
            }

            y += font_msg_lineheight;



            if(file->status != FILE_PENDING && file->status < FILE_KILLED) {
                uint64_t progress = (file->progress > file->size) ? file->size : file->progress;
                uint32_t x1 = (uint64_t)400 * progress / file->size;
                RECT r = {x + 110, y, x + 110 + x1, y + font_msg_lineheight};
                fillrect(&r, BLUE);
                r.left = r.right;
                r.right = x + 110 + 400;
                fillrect(&r, 0x999999);

                //SetTextAlign(hdc, TA_CENTER | TA_TOP | TA_NOUPDATECP);

                //char text[128];
                //int textlen = sprintf(text, "%"PRIu64"/%"PRIu64, file->progress, file->size);
                //TextOut(hdc, x + 110 + 200, y, text, textlen);

                //SetTextAlign(hdc, TA_LEFT | TA_TOP | TA_NOUPDATECP);

                y += font_msg_lineheight;
            }



            break;
        }
        }

        i++;
    }
}

_Bool messages_mmove(MESSAGES *m, int mx, int my, int dy, int width, int height)
{
    if(my < 0) {
        my = 0;
    }

    if(mx < 0 || mx >= width) {
        m->iover = ~0;
        return 0;
    }

    setfont(FONT_MSG);

    void **p = m->data->data;
    int i = 0, n = m->data->n;

    while(i != n) {
        MESSAGE *msg = *p++;

        int dy = msg->height;

        if((my >= 0 && my < dy) || i == n - 1) {
            m->over = 0;
            switch(msg->flags) {
            case 0:
            case 1:
            case 2:
            case 3: {
                /* normal message */
                m->over = pmsg(mx, my, msg->msg, msg->length);
                m->urlover = 0xFFFF;

                if(my >= dy || mx < 110 || m->over == msg->length)
                {
                    break;
                }

                char_t *str = msg->msg + m->over;
                while(str != msg->msg)
                {
                    str--;
                    if(*str == ' ' || *str == '\n')
                    {
                        str++;
                        break;
                    }
                }

                char_t *end = msg->msg + msg->length;
                while(str != end && *str != ' ' && *str != '\n')
                {
                    if(end - str >= 7 && strcmp2(str, "http://") == 0) {
                        hand = 1;
                        m->urlover = str - msg->msg;
                    }

                    if(end - str >= 8 && strcmp2(str, "https://") == 0) {
                        hand = 1;
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
                /* image */
                //MSG_IMG *img = (void*)msg;

                //SIZE size;
                //GetBitmapDimensionEx(img->bitmap, &size);
                break;
            }

            case 6:
            case 7: {
                if(my >= font_msg_lineheight) {break;}
                /* file transfer */
                MSG_FILE *file = (void*)msg;
                mx -= 110;

                uint8_t size[16];
                int sizelen = sprint_bytes(size, file->size);

                switch(file->status) {
                case FILE_PENDING: {
                    if(msg->flags == 6) {
                        uint8_t str[64];
                        int x1, x2, strlen;

                        strlen = sprintf((char*)str, "wants to share file %.*s (%.*s) ", file->name_length, file->name, sizelen, size);
                        x1 = textwidth(str, strlen);
                        x2 = x1 + strwidth("Accept");

                        if(mx >= x1 && mx < x2) {
                            hand = 1;
                            m->over = 1;
                            break;
                        }

                        x1 = x2 + 10;
                        x2 = x1 + strwidth("Decline");

                        if(mx >= x1 && mx < x2) {
                            hand = 1;
                            m->over = 2;
                            break;
                        }
                    } else {
                        uint8_t str[64];
                        int x1, x2, strlen;

                        strlen = sprintf((char*)str, "offering file %.*s (%.*s) ", file->name_length, file->name, sizelen, size);
                        x1 = textwidth(str, strlen);
                        x2 = x1 + strwidth("Cancel");

                        if(mx >= x1 && mx < x2) {
                            hand = 1;
                            m->over = 1;
                            break;
                        }
                    }

                    break;
                }

                case FILE_OK: {
                    uint8_t str[64];
                    int x1, x2, strlen;

                    strlen = sprintf((char*)str, "transferring file %.*s", file->name_length, file->name);
                    x1 = textwidth(str, strlen) + 10;
                    x2 = x1 + strwidth("Pause");

                    if(mx >= x1 && mx < x2) {
                        hand = 1;
                        m->over = 1;
                        break;
                    }

                    x1 = x2 + 10;
                    x2 = x1 + strwidth("Cancel");

                    if(mx >= x1 && mx < x2) {
                        hand = 1;
                        m->over = 2;
                        break;
                    }
                    break;
                }

                case FILE_PAUSED: {
                    uint8_t str[64];
                    int x1, x2, strlen;

                    strlen = sprintf((char*)str, "transferring file (paused) %.*s", file->name_length, file->name);
                    x1 = textwidth(str, strlen) + 10;
                    x2 = x1 + strwidth("Resume");

                    if(mx >= x1 && mx < x2) {
                        hand = 1;
                        m->over = 1;
                        break;
                    }

                    x1 = x2 + 10;
                    x2 = x1 + strwidth("Cancel");

                    if(mx >= x1 && mx < x2) {
                        hand = 1;
                        m->over = 2;
                        break;
                    }
                    break;
                }

                case FILE_BROKEN: {
                    //"cancelled file winTox.png (150KiB)"
                    uint8_t str[64];
                    int x1, x2, strlen;

                    strlen = sprintf((char*)str, "transferring file %.*s", file->name_length, file->name);
                    x1 = textwidth(str, strlen) + 10;
                    x2 = x1 + strwidth("Cancel");

                    if(mx >= x1 && mx < x2) {
                        hand = 1;
                        m->over = 1;
                        break;
                    }

                    break;
                }

                case FILE_KILLED: {
                    break;
                }

                case FILE_DONE: {
                    if(msg->flags == 6) {
                        uint8_t str[64];
                        int x1, x2, strlen;

                        strlen = sprintf((char*)str, "transferred file %.*s", file->name_length, file->name);
                        x1 = textwidth(str, strlen) + 10;
                        x2 = x1 + strwidth("Open");

                        if(mx >= x1 && mx < x2) {
                            hand = 1;
                            m->over = 1;
                            break;
                        }


                    }

                    break;
                }
                }
                break;
            }
            }

            m->iover = i;

            if(m->select) {
                if(i > m->idown) {
                    m->data->istart = m->idown;
                    m->data->iend = i;

                    m->data->start = m->down;
                    m->data->end = m->over;
                } else if(i < m->idown) {
                    m->data->iend = m->idown;
                    m->data->istart = i;

                    m->data->end = m->down;
                    m->data->start = m->over;
                } else {
                    m->data->istart = m->data->iend = i;
                    if(m->over >= m->down) {
                        m->data->start = m->down;
                        m->data->end = m->over;
                    } else {
                        m->data->end = m->down;
                        m->data->start = m->over;
                    }
                }

                setselection();

                //debug("test: %u %u %u %u\n", f->istart, f->start, f->iend, f->end);
                return 1;
            }
            return 0;
        }

        my -= dy;

        i++;
    }

    return 0;
}

_Bool messages_mdown(MESSAGES *m)
{
    if(m->iover != ~0) {
        MESSAGE *msg = m->data->data[m->iover];
        switch(msg->flags)
        {
            case 0 ... 3:
            {
                if(m->urlover != 0xFFFF)
                {
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

            case 6 ... 7:
            {
                MSG_FILE *file = (void*)msg;
                if(m->over == 0)
                {
                    break;
                }

                switch(file->status) {
                    case FILE_PENDING:
                    {
                        if(msg->flags == 6)
                        {
                            if(m->over == 1) {
                                savefilerecv(m->data->id, file);
                            } else {
                                //decline
                                tox_postmessage(TOX_FILE_IN_CANCEL, m->data->id, file->filenumber, NULL);
                            }
                        } else
                        {
                            //cancel
                            tox_postmessage(TOX_FILE_OUT_CANCEL, m->data->id, file->filenumber, NULL);
                        }


                        break;
                    }

                    case FILE_OK:
                    {
                        if(m->over == 1) {
                            //pause
                            tox_postmessage(TOX_FILE_IN_PAUSE + (msg->flags & 1), m->data->id, file->filenumber, NULL);
                        } else {
                            //cancel
                            tox_postmessage(TOX_FILE_IN_CANCEL + (msg->flags & 1), m->data->id, file->filenumber, NULL);
                        }
                        break;
                    }

                    case FILE_PAUSED:
                    {
                        if(m->over == 1) {
                            //resume
                            tox_postmessage(TOX_FILE_IN_RESUME + (msg->flags & 1), m->data->id, file->filenumber, NULL);
                        } else {
                            //cancel
                            tox_postmessage(TOX_FILE_IN_CANCEL + (msg->flags & 1), m->data->id, file->filenumber, NULL);
                        }
                        break;
                    }

                    case FILE_BROKEN:
                    {
                        //cancel
                        tox_postmessage(TOX_FILE_IN_CANCEL + (msg->flags & 1), m->data->id, file->filenumber, NULL);
                        break;
                    }

                    case FILE_DONE:
                    {
                        if(msg->flags == 6) {
                            //open the file
                        }
                        break;
                    }
                }
                break;
            }
        }

        return 1;
    }

    return 0;
}

_Bool messages_mright(MESSAGES *m)
{
    return 0;
}

_Bool messages_mwheel(MESSAGES *m, int height, double d)
{
    return 0;
}

_Bool messages_mup(MESSAGES *m)
{
    m->select = 0;
    return 0;
}

_Bool messages_mleave(MESSAGES *m)
{
    return 0;
}

int messages_selection(MESSAGES *m, void *data, uint32_t len)
{
    int i = m->data->istart, n = m->data->iend + 1;
    void **dp = &m->data->data[i];

    char_t *p = data;

    while(i != n) {
        MESSAGE *msg = *dp++;

        if(m->type) {
            memcpy(p, &msg->msg[msg->length + 1], (uint16_t)msg->msg[msg->length] * 2);
            p += (uint16_t)msg->msg[msg->length];
        } else {
            FRIEND *f = &friend[m->data->id];
            uint8_t author = msg->flags & 1;

            if(!author) {
                memcpy(p, f->name, f->name_length);
                p += f->name_length;
            } else {
                memcpy(p, self.name, self.name_length);
                p += self.name_length;
            }
        }

        strcpy2(p, ": ");
        p += 2;

        switch(msg->flags)
        {
            case 0:
            case 1:
            case 2:
            case 3: {
                memcpy(p, msg->msg, msg->length * 2);
                p += msg->length;
                break;
            }
        }

        strcpy2(p, "\r\n");
        p += 2;

        i++;
    }
    *p = 0;

    return (void*)p - data;
}

static int msgheight(MESSAGE *msg)
{
    switch(msg->flags) {
    case 0:
    case 1:
    case 2:
    case 3: {
        return heightmsg(msg->msg, msg->length);
    }

    case 6:
    case 7: {
        MSG_FILE *file = (void*)msg;
        return (file->status != FILE_PENDING && file->status < FILE_KILLED) ? font_msg_lineheight * 2 : font_msg_lineheight;
    }

    }

    return 0;
}

void message_setheight(MESSAGES *m, MESSAGE *msg, MSG_DATA *p)
{
    msg->height = msgheight(msg);
    p->height += msg->height;
    if(m->data == p) {
        m->panel.content_scroll->content_height = p->height;
    }
}

void message_updateheight(MESSAGES *m, MESSAGE *msg, MSG_DATA *p)
{
    int newheight = msgheight(msg);
    p->height += newheight - msg->height;
    msg->height = newheight;
    if(m->data == p) {
        m->panel.content_scroll->content_height = p->height;
    }
}

void message_add(MESSAGES *m, MESSAGE *msg, MSG_DATA *p)
{
    p->data = realloc(p->data, (p->n + 1) * sizeof(void*));
    p->data[p->n++] = msg;

    message_setheight(m, msg, p);
}
