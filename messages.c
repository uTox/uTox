
void draw_messages(int x, int y, FRIEND *f)
{
    uint16_t **m = (uint16_t**)f->message;

    SelectObject(hdc, font_small);

    int yy = 0;
    uint16_t *msg;
    uint8_t a = 0xFF;

    while((msg = *m++) != NULL)
    {
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
        yy += drawtextrect(x + 100, y + yy, width - 24 - 100, height - 152, (uint8_t*)(msg + 1), length);
    }
}

void draw_groupmessages(int x, int y, GROUPCHAT *g)
{
    uint16_t **m = (uint16_t**)g->message;

    SelectObject(hdc, font_small);

    int yy = 0;
    uint16_t *msg;

    while((msg = *m++) != NULL)
    {
        //uint16_t id = msg[0] >> 9;
        uint16_t length = msg[1];
        uint8_t namelen = msg[0] & 0xFF;

        void *str = (void*)msg + 4, *name = str + length;

        SetTextColor(hdc, 0x999999);
        drawtextrange(x, x + 100, y + yy, name, namelen);

        SetTextColor(hdc, 0x333333);
        yy += drawtextrect(x + 100, y + yy, width - 24 - 100, height - 152, (uint8_t*)(msg + 2), length);

    }


}
