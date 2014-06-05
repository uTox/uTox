#include "main.h"

static void textout(int x, int y, wchar_t *str, uint16_t length, int d, int h1, int h2)
{
    h1 -= d;
    h2 -= d;

    if(h1 < 0) {
        h1 = 0;
    }

    if(h2 < 0) {
        h2 = 0;
    }

    if(h1 > length)
    {
        h1 = length;
    }

    if(h2 > length)
    {
        h2 = length;
    }

    SIZE size;

    TextOutW(hdc, x, y, str, h1);
    GetTextExtentPoint32W(hdc, str, h1, &size);
    x += size.cx;

    setbgcolor(TEXT_HIGHLIGHT_BG);
    setcolor(TEXT_HIGHLIGHT);

    TextOutW(hdc, x, y, str + h1, h2 - h1);
    GetTextExtentPoint32W(hdc, str + h1, h2 - h1, &size);
    x += size.cx;

    setbgcolor(~0);
    setcolor(0);

    TextOutW(hdc, x, y, str + h2, length - h2);
}

static int drawmsg(int x, int y, wchar_t *str, uint16_t length, int h1, int h2)
{
    _Bool word = 0;
    int xc = x;
    wchar_t *a = str, *b = str, *end = str + length;
    while(a != end) {
        switch(*a) {
        case '\n': {
            textout(x, y, b, a - b, b - str, h1, h2);
            y += font_msg_lineheight;
            x = xc;
            b = a;

            setcolor(0);
            word = 0;
            break;
        }

        case ' ': {
            if(word) {
                SIZE size;
                int count = a - b;
                textout(x, y, b, count, b - str, h1, h2);
                GetTextExtentPoint32W(hdc, b, count, &size);
                x += size.cx;
                b = a;

                setcolor(0);
                word = 0;
            }

            if((end - a >= 8 && memcmp(a, L" http://", 16) == 0) || (end - a >= 9 && memcmp(a, L" https://", 18) == 0)) {
                SIZE size;
                int count = a - b;
                textout(x, y, b, count,  b - str, h1, h2);
                GetTextExtentPoint32W(hdc, b, count, &size);
                x += size.cx;
                b = a;

                setcolor(0xFF0000);
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

    //RECT r = {x, y, x + width, bottom};
    //y += DrawTextW(hdc, out, length, &r, DT_WORDBREAK | DT_NOPREFIX);

    return y;
}

static uint32_t pmsg(int mx, int my, wchar_t *str, uint16_t length)
{
    wchar_t *a = str, *b = str, *end = str + length;
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
    mx -= 100;
    if(mx > 0) {
        int len = a - b, d[len];
        SIZE size;
        GetTextExtentExPointW(hdc, b, len, mx, &fit, d, &size);
    } else {
        fit = 0;
    }

    return (b - str) + fit;
}

static int heightmsg(wchar_t *str, uint16_t length)
{
    int y = 0;
    wchar_t *a = str, *end = str + length;
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
    setfont(FONT_MESSAGE);

    FRIEND *f = m->data;

    uint8_t lastauthor = 0xFF;

    void **p = f->message;
    int i = 0, n = f->msg;

    while(i != n) {
        MESSAGE *msg = *p++;
        switch(msg->flags) {
        case 0:
        case 1:
        case 2:
        case 3: {
            /* normal message */
            uint8_t author = msg->flags & 1;
            if(author != lastauthor) {
                if(author) {
                    drawtextwidth(x, 90, y, f->name, f->name_length);
                } else {
                    setcolor(0x888888);
                    drawtextwidth(x, 90, y, self.name, self.name_length);
                    setcolor(0);
                }
                lastauthor = author;
            }

            if(i == f->istart) {
                y = drawmsg(x + 100, y, msg->msg, msg->length, f->start, ((i == f->iend) ? f->end : msg->length));
            } else if(i == f->iend) {
                y = drawmsg(x + 100, y, msg->msg, msg->length, 0, f->end);
            } else if(i > f->istart && i < f->iend) {
                y = drawmsg(x + 100, y, msg->msg, msg->length, 0, msg->length);
            } else {
                y = drawmsg(x + 100, y, msg->msg, msg->length, 0, 0);
            }

            break;
        }

        case 4:
        case 5: {
            /* image */
            MSG_IMG *img = (void*)msg;
            SIZE size;

            SelectObject(hdcMem, img->bitmap);
            GetBitmapDimensionEx(img->bitmap, &size);
            BitBlt(hdc, x, y, size.cx, size.cy, hdcMem, 0, 0, SRCCOPY);
            y += img->height;
            break;
        }

        case 6:
        case 7: {
            lastauthor = 0xFF;

            /* file transfer */
            MSG_FILE *file = (void*)msg;
            char text[128];
            char *type[] = {"Incoming", "Outgoing"};
            char *status[] = {"Pending", "Transfering", "Paused", "Disconnected", "Cancelled", "Done"};
            int i = sprintf(text, "File (%s): %s (%s)", type[msg->flags - 6], file->name, status[file->status]);

            TextOut(hdc, x, y, text, i);
            y += font_msg_lineheight;
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

    setfont(FONT_MESSAGE);

    FRIEND *f = m->data;

    void **p = f->message;
    int i = 0, n = f->msg;

    while(i != n) {
        MESSAGE *msg = *p++;

        int dy = msg->height;

        if((my >= 0 && my < dy) || i == n - 1) {
            switch(msg->flags) {
            case 0:
            case 1: {
                /* normal message */
                m->over = pmsg(mx, my, msg->msg, msg->length);
                break;
            }

            case 2:
            case 3: {
                /* action */
                m->over = pmsg(mx, my, msg->msg, msg->length);
                break;
            }

            case 4:
            case 5: {
                /* image */
                //MSG_IMG *img = (void*)msg;

                //SIZE size;
                //GetBitmapDimensionEx(img->bitmap, &size);

                m->over = 0;
                break;
            }

            case 6:
            case 7: {
                /* file transfer */
                //MSG_FILE *file = (void*)msg;
                //y += font_msg_lineheight;
                m->over = 0;
                break;
            }
            }

            m->iover = i;

            if(m->select) {
                if(i > m->idown) {
                    f->istart = m->idown;
                    f->iend = i;

                    f->start = m->down;
                    f->end = m->over;
                } else if(i < m->idown) {
                    f->iend = m->idown;
                    f->istart = i;

                    f->end = m->down;
                    f->start = m->over;
                } else {
                    f->istart = f->iend = i;
                    if(m->over >= m->down) {
                        f->start = m->down;
                        f->end = m->over;
                    } else {
                        f->end = m->down;
                        f->start = m->over;
                    }
                }

                //debug("test: %u %u %u %u\n", f->istart, f->start, f->iend, f->end);
            }
            return 1;
        }

        my -= dy;

        i++;
    }

    return 0;
}

_Bool messages_mdown(MESSAGES *m)
{
    if(m->iover != ~0) {
        FRIEND *f = m->data;
        f->istart = f->iend = m->idown = m->iover;
        f->start = f->end = m->down = m->over;
        m->select = 1;
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

void message_setheight(MESSAGE *msg, MESSAGES *m)
{
    switch(msg->flags) {
    case 0:
    case 1: {
        msg->height = heightmsg(msg->msg, msg->length);
        break;
    }

    case 6:
    case 7: {
        msg->height = font_msg_lineheight;
        break;
    }

    }

    if(m) {
        m->height += msg->height;
        m->panel.content_scroll->content_height = m->height;
    }
}
