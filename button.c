

static _Bool inbutton(BUTTON *b, int x, int y)
{
    x -= b->x;
    y -= b->y;
    return (x >= 0 && x < b->width && y >= 0 && y < b->height);
}

void button_copyid_onpress(void)
{
    address_to_clipboard();
}

void button_addfriend_onpress(void)
{
    uint8_t id[TOX_FRIEND_ADDRESS_SIZE];
    if(edit_addid.length != TOX_FRIEND_ADDRESS_SIZE * 2 || !strtoid(id, edit_addid_data))
    {
        addfriend_status = 2;
        main_draw();
        return;
    }

    core_postmessage2(CMSG_ADDFRIEND, edit_addmsg.length, TOX_FRIEND_ADDRESS_SIZE, id, edit_addmsg_data);

    EDIT *edit = sedit;
    sedit = NULL;
    if(edit)
    {
        edit_draw(edit);
    }
}

void button_newgroup_onpress(void)
{
    core_postmessage3(CMSG_NEWGROUP, 0);
}


void button_draw(BUTTON *b)
{
    RECT r = {b->x, b->y, b->x + b->width, b->y + b->height};
    FillRect(hdc, &r, white);

    SetTextColor(hdc, b->mouseover ? 0x222222 : 0x555555);
    SetBkColor(hdc, WHITE);

    SelectObject(hdcMem, bm_plus2);
    BitBlt(hdc, b->x + 4, b->y + 3, 16, 11, hdcMem, 0, 0, SRCCOPY);

    SelectObject(hdc, font_med2);
    drawtext(b->x + 20, b->y, b->text, b->text_length);

    commitdraw(b->x, b->y, b->width, b->height);
}

void button_mousemove(BUTTON *b, int x, int y)
{
    _Bool mouseover = inbutton(b, x, y);
    if(mouseover != b->mouseover)
    {
        b->mouseover = mouseover;
        button_draw(b);
    }
}

void button_mousedown(BUTTON *b, int x, int y)
{
    if(inbutton(b, x, y))
    {
        if(!b->mousedown)
        {
            b->mousedown = 1;
            button_draw(b);
        }
    }
}

void button_mouseup(BUTTON *b)
{
    if(b->mousedown)
    {
        if(b->mouseover)
        {
            b->onpress();
        }

         b->mousedown = 0;
         button_draw(b);
    }
}

void button_mouseleave(BUTTON *b)
{
    if(b->mouseover)
    {
         b->mouseover = 0;
         button_draw(b);
    }
}
