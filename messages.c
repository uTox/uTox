
int messages_height(void **message, uint32_t msg, _Bool friend)
{
    int lines = 0;

    void **m = message, **end = m + msg;
    while(m != end)
    {
        uint16_t *msg = *m++;
        uint8_t *str, *end;
        if(friend)
        {
            str = (uint8_t*)(msg + 1);
            end = str + (msg[0] >> 2);
        }
        else
        {
            str = (uint8_t*)(msg + 2);
            end = str + msg[1];
        }

        lines++;
        while(str != end)
        {
            char c = *str++;
            if(c == '\n')
            {
                lines++;
            }
        }
    }

    return (lines * font_small_lineheight);
}

int messages_y(void **message, uint32_t msg, double scroll, _Bool friend)
{
    uint32_t c = messages_height(message, msg, friend);
    uint32_t h = MESSAGES_BOTTOM - MESSAGES_Y;

    int dy = 0;
    if(c > h)
    {
        dy = (scroll * (double)(c - h)) + 0.5;
    }

    return -dy;
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

    void **m = f->message, **end = m + f->msg;

    SelectObject(hdc, font_small);

    int yy = messages_y(f->message, f->msg, 1.0, 1);
    uint8_t a = 0xFF;

    while(m != end)
    {
        uint16_t *msg = *m++;

        uint16_t length = msg[0] >> 2;
        uint8_t author = msg[0] & 1;

        if(a != author)
        {
            a = author;

            SetTextColor(hdc, 0x999999);

            if(!author)
            {
                drawtextrange(x, x + 100, y + yy, name, name_length);
            }
            else
            {
                drawtextrange(x, x + 100, y + yy, f->name, f->name_length);
            }
        }

        SetTextColor(hdc, 0x333333);
        yy += drawtextrect(x + 100, y + yy, width - 24, height - 152, (uint8_t*)(msg + 1), length);
    }

    SelectClipRgn(hdc, NULL);
}

void draw_groupmessages(int x, int y, GROUPCHAT *g)
{
    void **m = g->message, **end = m + g->msg;
    if(!m)
    {
        return;
    }

    HRGN rgn = CreateRectRgn(x, y, width - 24 - 100, height - 152);
    SelectClipRgn (hdc, rgn);
    DeleteObject(rgn);


    SelectObject(hdc, font_small);

    int yy = messages_y(g->message, g->msg, 1.0, 0);

    while(m != end)
    {
        uint16_t *msg = *m++;
        //uint16_t id = msg[0] >> 9;
        uint16_t length = msg[1];
        uint8_t namelen = msg[0] & 0xFF;

        void *str = (void*)msg + 4, *name = str + length;

        SetTextColor(hdc, 0x999999);
        drawtextrange(x, x + 100, y + yy, name, namelen);

        SetTextColor(hdc, 0x333333);
        yy += drawtextrect(x + 100, y + yy, width - 24 - 100, height - 152, (uint8_t*)(msg + 2), length);

    }

    SelectClipRgn(hdc, NULL);

}
