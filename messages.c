#include "main.h"

static _Bool msg_select;

int drawmessage(int x, int y, int right, int bottom, uint8_t *str, uint16_t length)
{
    int ry = y;

    uint8_t *a = str, *end = a + length;
    while(1)
    {
        if(a == end || *a == '\n')
        {
            if(str == a)
            {
                y += font_small_lineheight;
            }

            while(str != a)
            {
                int fit;
                SIZE size;
                GetTextExtentExPoint(hdc, (char*)str, a - str, right - x, &fit, NULL, &size);
                TextOut(hdc, x, y, (char*)str, fit);

                y += font_small_lineheight;
                str += fit;

            }

            if(a == end)
            {
                break;
            }

            str++;
            a++;
        }
        a++;
    }

    return y - ry;
}

int drawmessage2(int x, int y, int right, int bottom, uint8_t *str, uint16_t length, uint16_t hstart, uint16_t hend)
{
    int ry = y;
    uint8_t *pstr = str;

    int lx = x, cx = x;
    uint8_t *a = str, *end = a + hstart;
    while(hstart)
    {
        if(a == end || *a == '\n')
        {
            if(str == a)
            {
                y += font_small_lineheight;
                cx = x;
            }

            while(str != a)
            {
                int fit;
                SIZE size;
                GetTextExtentExPoint(hdc, (char*)str, a - str, right - x, &fit, NULL, &size);
                TextOut(hdc, x, y, (char*)str, fit);
                cx = x + size.cx;

                y += font_small_lineheight;
                str += fit;

            }

            if(a == end)
            {
                lx = cx;
                y -= font_small_lineheight;
                break;
            }

            str++;
            a++;
        }
        a++;
    }

    SetBkMode(hdc, OPAQUE);
    SetBkColor(hdc, BLACK);
    SetTextColor(hdc, WHITE);

    end = pstr + hend;

    while(hend != hstart)
    {
        if(a == end || *a == '\n')
        {
            if(str == a)
            {
                y += font_small_lineheight;
                cx = x;
                lx = x;
            }

            while(str != a)
            {
                int fit;
                SIZE size;
                GetTextExtentExPoint(hdc, (char*)str, a - str, right - lx, &fit, NULL, &size);
                TextOut(hdc, lx, y, (char*)str, fit);
                cx = lx + size.cx;
                lx = x;

                y += font_small_lineheight;
                str += fit;

            }

            if(a == end)
            {
                lx = cx;
                y -= font_small_lineheight;
                break;
            }

            str++;
            a++;
        }
        a++;
    }

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, 0x333333);

    end = pstr + length;

    while(length != hend)
    {
        if(a == end || *a == '\n')
        {
            if(str == a)
            {
                y += font_small_lineheight;
                lx = x;
            }

            while(str != a)
            {
                int fit;
                SIZE size;
                GetTextExtentExPoint(hdc, (char*)str, a - str, right - lx, &fit, NULL, &size);
                TextOut(hdc, lx, y, (char*)str, fit);
                lx = x;

                y += font_small_lineheight;
                str += fit;

            }

            if(a == end)
            {
                break;
            }

            str++;
            a++;
        }
        a++;
    }

    if(length == hend)
    {
        y += font_small_lineheight;
    }

    return y - ry;
}

int heightmessage(uint8_t *str, uint16_t length, int width)
{
    int y = 0;

    uint8_t *a = str, *end = a + length;
    while(1)
    {
        if(a == end || *a == '\n')
        {
            if(str == a)
            {
                y += font_small_lineheight;
            }

            while(str != a)
            {
                int fit;
                SIZE size;
                GetTextExtentExPoint(hdc, (char*)str, a - str, width, &fit, NULL, &size);
                //TextOut(hdc, x, y, (char*)str, fit);

                y += font_small_lineheight;
                str += fit;

            }

            if(a == end)
            {
                break;
            }

            str++;
            a++;
        }
        a++;
    }

    return y;
}

int messages_height(void **message, uint32_t msg, int width)
{
    int y = 0;

    void **m = message, **end = m + msg;
    while(m != end)
    {
        uint16_t *msg = *m++;

        uint8_t *str = (uint8_t*)(msg + 2);
        uint16_t length = msg[1];

        y += heightmessage(str, length, width);
    }

    return y;
}

int messages_y(void **message, uint32_t msg, double scroll, int width)
{
    uint32_t c = messages_height(message, msg, width);
    uint32_t h = MESSAGES_BOTTOM - MESSAGES_Y;

    int dy = 0;
    if(c > h)
    {
        dy = (scroll * (double)(c - h)) + 0.5;
    }

    return -dy;
}

void somefunc(int x, int y, void **message, uint32_t msg, double scroll, int width)
{
    if(msg == 0)
    {
        return;
    }

    x -= MESSAGES_X + 100;
    if(x < 0 || x >= width)
    {
        return;
    }

    y -= MESSAGES_Y;
    if(y < 0 || y >= MESSAGES_BOTTOM - MESSAGES_Y)
    {
        return;
    }

    SelectObject(hdc, font_small);

    int yy = messages_y(message, msg, scroll, width);

    void **m = message, **end = m + msg;
    while(m != end)
    {
        uint16_t *msg = *m;

        uint8_t *str = (uint8_t*)(msg + 2);
        uint16_t length = msg[1];

        int p = -1;

        uint8_t *a = str, *end = a + length, *pstr = str;
        while(1)
        {
            if(a == end || *a == '\n')
            {
                if(str == a)
                {
                    if(y >= yy && y < yy + font_small_lineheight)
                    {
                        p = (str - pstr);
                        break;
                    }
                    yy += font_small_lineheight;
                }

                while(str != a)
                {
                    if(y >= yy && y < yy + font_small_lineheight)
                    {
                        int fit;
                        SIZE size;
                        GetTextExtentExPoint(hdc, (char*)str, a - str, x, &fit, NULL, &size);

                        p = fit + (str - pstr);

                        a = end;
                        break;
                    }
                    else
                    {
                        int fit;
                        SIZE size;
                        GetTextExtentExPoint(hdc, (char*)str, a - str, width, &fit, NULL, &size);
                        //TextOut(hdc, x, y, (char*)str, fit);

                        yy += font_small_lineheight;
                        str += fit;
                    }
                }

                if(a == end)
                {
                    break;
                }

                str++;
                a++;
            }
            a++;
        }

        if(p != -1)
        {
            int sstr = m - message;

            if(!msg_select)
            {
                msg_sel.mstart = msg_sel.mend = msg_sel.mp = sstr;
                msg_sel.start = msg_sel.end = msg_sel.p = p;

            }
            else
            {
                if(sstr > msg_sel.mp)
                {
                    msg_sel.mstart = msg_sel.mp;
                    msg_sel.start = msg_sel.p;

                    msg_sel.mend = sstr;
                    msg_sel.end = p;
                }
                else if(sstr < msg_sel.mp)
                {
                    msg_sel.mstart = sstr;
                    msg_sel.start = p;

                    msg_sel.mend = msg_sel.mp;
                    msg_sel.end = msg_sel.p;
                }
                else
                {
                    msg_sel.mend = msg_sel.mp;
                    msg_sel.mstart = msg_sel.mp;

                    if(p > msg_sel.p)
                    {
                        msg_sel.start = msg_sel.p;
                        msg_sel.end = p;
                    }
                    else
                    {
                        msg_sel.start = p;
                        msg_sel.end = msg_sel.p;
                    }
                }

            }

            ui_drawmain();

            return;
        }

        m++;

    }
}

void draw_messages(int x, int y, FRIEND *f)
{
    if(!f->message)
    {
        return;
    }

    HRGN rgn = CreateRectRgn(x, y, width - 24, height - 152);
    SelectClipRgn (hdc, rgn);
    DeleteObject(rgn);

    void **m = f->message;

    SelectObject(hdc, font_small);
    SetTextColor(hdc, 0x333333);

    int yy = messages_y(f->message, f->msg, 1.0 - f->scroll, (width - 24 - 100) - x);
    uint8_t a = 0xFF;

    int i = 0;
    while(i != f->msg)
    {
        uint16_t *msg = *m++;

        uint16_t length = msg[1];
        uint8_t author = msg[0] & 1;

        if(a != author)
        {
            a = author;

            SetTextColor(hdc, 0x999999);

            if(!author)
            {
                drawtextrange(x, x + 100, y + yy, self.name, self.name_length);
            }
            else
            {
                drawtextrange(x, x + 100, y + yy, f->name, f->name_length);
            }

            SetTextColor(hdc, 0x333333);
        }

        if(i == msg_sel.mstart)
        {
            if(i == msg_sel.mend)
            {
                yy += drawmessage2(x + 100, y + yy, width - 24, height - 152, (uint8_t*)(msg + 2), length, msg_sel.start, msg_sel.end);
            }
            else
            {
                yy += drawmessage2(x + 100, y + yy, width - 24, height - 152, (uint8_t*)(msg + 2), length, msg_sel.start, length);
            }


        }
        else if(i == msg_sel.mend)
        {
            yy += drawmessage2(x + 100, y + yy, width - 24, height - 152, (uint8_t*)(msg + 2), length, 0, msg_sel.end);
        }
        else
        {
            _Bool h = (i > msg_sel.mstart && i < msg_sel.mend);

            if(h)
            {
                SetBkMode(hdc, OPAQUE);
                SetBkColor(hdc, BLACK);
                SetTextColor(hdc, WHITE);
            }
            yy += drawmessage(x + 100, y + yy, width - 24, height - 152, (uint8_t*)(msg + 2), length);
            if(h)
            {
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, 0x333333);
            }
        }


        /*if(i == msg_sel.mend)
        {
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, 0x333333);
        }*/

        i++;
    }

    SelectClipRgn(hdc, NULL);
}

void draw_groupmessages(int x, int y, GROUPCHAT *g)
{
    void **m = g->message;
    if(!m)
    {
        return;
    }

    HRGN rgn = CreateRectRgn(x, y, width - 24 - 100, height - 152);
    SelectClipRgn (hdc, rgn);
    DeleteObject(rgn);


    SelectObject(hdc, font_small);

    int yy = messages_y(g->message, g->msg, 1.0 - g->scroll, (width - 24 - 100 - 100) - x);

    int i = 0;
    while(i != g->msg)
    {
        uint16_t *msg = *m++;
        //uint16_t id = msg[0] >> 9;
        uint16_t length = msg[1];
        uint8_t namelen = msg[0] & 0xFF;

        void *str = (void*)msg + 4, *name = str + length;

        SetTextColor(hdc, 0x999999);
        drawtextrange(x, x + 100, y + yy, name, namelen);

        SetTextColor(hdc, 0x333333);
       // yy += drawmessage(x + 100, y + yy, width - 24 - 100, height - 152, (uint8_t*)(msg + 2), length);

        if(i == msg_sel.mstart)
        {
            if(i == msg_sel.mend)
            {
                yy += drawmessage2(x + 100, y + yy, GMESSAGES_RIGHT, height - 152, (uint8_t*)(msg + 2), length, msg_sel.start, msg_sel.end);
            }
            else
            {
                yy += drawmessage2(x + 100, y + yy, GMESSAGES_RIGHT, height - 152, (uint8_t*)(msg + 2), length, msg_sel.start, length);
            }


        }
        else if(i == msg_sel.mend)
        {
            yy += drawmessage2(x + 100, y + yy, GMESSAGES_RIGHT, height - 152, (uint8_t*)(msg + 2), length, 0, msg_sel.end);
        }
        else
        {
            _Bool h = (i > msg_sel.mstart && i < msg_sel.mend);

            if(h)
            {
                SetBkMode(hdc, OPAQUE);
                SetBkColor(hdc, BLACK);
                SetTextColor(hdc, WHITE);
            }
            yy += drawmessage(x + 100, y + yy, GMESSAGES_RIGHT, height - 152, (uint8_t*)(msg + 2), length);
            if(h)
            {
                SetBkMode(hdc, TRANSPARENT);
            }
        }

        i++;

    }

    SelectClipRgn(hdc, NULL);

}

void messages_mousemove(int x, int y, void **message, uint32_t msg, double scroll, int width)
{
    if(msg_select)
    {
        somefunc(x, y, message, msg, 1.0 - scroll, width);
    }
}

void messages_mousedown(int x, int y, void **message, uint32_t msg, double scroll, int width)
{
    somefunc(x, y, message, msg, 1.0 - scroll, width);
    msg_select = 1;
}

void messages_mouseup(void)
{
    msg_select = 0;
}

void messages_mousewheel(int x, int y, double d, double *scroll)
{
    if(x < MESSAGES_X || x >= MESSAGES_RIGHT || y < MESSAGES_Y || y >= MESSAGES_BOTTOM)
    {
        return;
    }

    //uint32_t c = itemcount * ITEM_HEIGHT;
    //uint32_t h = MESSAGES_BOTTOM - MESSAGES_Y;

    //if(c > h)
    {
        //uint32_t m = (h * h) / c;
        //double dd = (h - m);

        //*scroll -= 16.0 * d / dd;;

        *scroll += d / 16.0;
        if(*scroll > 1.0)
        {
            *scroll = 1.0;
        }
        else if(*scroll < 0.0)
        {
            *scroll = 0.0;
        }

        ui_drawmain();
    }
}

void messages_copy(void **message)
{
    int length = 0;
    int i = msg_sel.mstart;
    while(i != msg_sel.mend + 1)
    {
        uint16_t *msg = message[i];
        length += msg[1] + 1;
        i++;
    }
    //note: approximate length

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, length + 1);
    uint8_t *p = GlobalLock(hMem), *s = p;
    i = msg_sel.mstart;
    while(1)
    {
        uint16_t *msg = message[i];
        uint16_t length = msg[1];

        if(i == msg_sel.mstart)
        {
            if(i == msg_sel.mend)
            {
                memcpy(s, (uint8_t*)(msg + 2) + msg_sel.start, msg_sel.end - msg_sel.start);
                s += msg_sel.end - msg_sel.start;
                break;
            }
            else
            {
                memcpy(s, (uint8_t*)(msg + 2) + msg_sel.start, length - msg_sel.start);
                s += length - msg_sel.start;
            }
        }
        else if(i == msg_sel.mend)
        {
            memcpy(s, (msg + 2), msg_sel.end);
            s += msg_sel.end;
            break;
        }
        else
        {
            memcpy(s, (msg + 2), length);
            s += length;
        }

        *s++ = '\n';

        i++;
    }
    *s++ = 0;

    GlobalUnlock(hMem);
    OpenClipboard(0);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();

}
