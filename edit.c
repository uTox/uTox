
#include "main.h"

static _Bool inedit(EDIT *edit, int x, int y)
{
    return (x >= edit->x && x <= edit->right && y >= edit->y && y <= edit->bottom);
}

void edit_name_onenter(void)
{
    uint8_t *data = edit_name_data;
    uint16_t length = edit_name.length;

    memcpy(name, data, length);
    name_length = length;

    list_draw();

    core_postmessage(CMSG_SETNAME, 0, length, data);
}

void edit_status_onenter(void)
{
    uint8_t *data = edit_status_data;
    uint16_t length = edit_status.length;

    memcpy(statusmsg, data, length);
    status_length = length;

    list_draw();

    core_postmessage(CMSG_SETSTATUSMSG, 0, length, data);
}

void edit_msg_onenter(void)
{
    uint16_t length = edit_msg.length;
    if(length == 0)
    {
        return;
    }

    if(sitem->item == ITEM_FRIEND)
    {
        FRIEND *f = sitem->data;

        core_postmessage(CMSG_SENDMESSAGE, (f - friend), length, edit_msg_data);

        void *msg = malloc(length + 2);
        *(uint16_t*)msg = length << 2;
        memcpy(msg + 2, edit_msg_data, length);

        f->message[f->msg++] = msg;


    }
    else
    {
        GROUPCHAT *g = sitem->data;

        core_postmessage(CMSG_SENDMESSAGEGROUP, (g - group), length, edit_msg_data);

        /*uint16_t *msg = malloc(length + 4);
        msg[0] = (0xFFFF ^ 1);
        msg[1] = length;
        memcpy(msg + 2, edit_msg_data, length);

        g->message[g->msg++] = msg;*/
    }

    main_draw();

    edit_msg.length = 0;
    edit_selstart = 0;
    edit_selend = 0;

    edit_draw(&edit_msg);

}

void edit_draw(EDIT *edit)
{

    RECT outline = {edit->x, edit->y, edit->right, edit->bottom};
    FrameRect(hdc, &outline, (edit == sedit) ? blue : (edit->mouseover ? gray6 : gray5));
    outline.left++;
    outline.top++;
    outline.right--;
    outline.bottom--;

    FillRect(hdc, &outline, white);

    SelectObject(hdc, font_small);

    SetTextColor(hdc, 0x333333);

    if(edit->multiline)
    {
        drawtextrect(edit->x + 5, edit->y + 5, edit->right - 1, edit->bottom - 5, edit->data, edit->length);
    }
    else
    {
        drawtextrange(edit->x + 5, edit->right - 5, edit->y + 5, edit->data, edit->length);

        if(edit == sedit)
        {
            int x1, x2;
            SIZE size;
            GetTextExtentPoint32(hdc, (char*)edit->data, edit_selstart, &size);
            x1 = size.cx;
            GetTextExtentPoint32(hdc, (char*)edit->data, edit_selend, &size);
            x2 = size.cx;

            if(x1 > x2)
            {
                RECT r = {edit->x + 5 + x2, edit->y + 5, edit->x + 5 + x1 + 1, edit->y + 19};
                InvertRect(hdc, &r);
            }
            else
            {
                RECT r = {edit->x + 5 + x1, edit->y + 5, edit->x + 5 + x2 + 1, edit->y + 19};
                InvertRect(hdc, &r);
            }

        }
    }

    commitdraw(edit->x, edit->y, edit->right - edit->x, edit->bottom - edit->y);
}

void edit_mousemove(EDIT *edit, int x, int y)
{
    _Bool mouseover = inedit(edit, x, y);
    if(mouseover != edit->mouseover)
    {
        edit->mouseover = mouseover;
        edit_draw(edit);
    }

    if(edit == sedit && edit_select)
    {
        int fit;
        SIZE size;
        SelectObject(hdc, font_small);
        GetTextExtentExPoint(hdc, (char*)edit->data, edit->length, x - edit->x - 5, &fit, NULL, &size);
        edit_selend = fit;

        edit_draw(edit);
    }
}

void edit_mousedown(EDIT *edit, int x, int y)
{
    if(inedit(edit, x, y))
    {
        if(sedit != edit)
        {
            EDIT *old = sedit;
            sedit = edit;
            if(old)
            {
                edit_draw(old);
            }
        }

        int fit;
        SIZE size;
        SelectObject(hdc, font_small);
        GetTextExtentExPoint(hdc, (char*)edit->data, edit->length, x - edit->x - 5, &fit, NULL, &size);
        edit_selstart = edit_selend = fit;
        edit_select = 1;

        edit_draw(edit);
    }
}

void edit_mouseup(EDIT *edit)
{
    if(edit->mousedown)
    {
         edit->mousedown = 0;
         edit_draw(edit);
    }

    if(edit_select)
    {
        edit_select = 0;
    }

}

void edit_mouseleave(EDIT *edit)
{
    if(edit->mouseover)
    {
         edit->mouseover = 0;
         edit_draw(edit);
    }
}

void edit_char(EDIT *edit, uint32_t ch)
{
    if(edit->locked)
    {
        return;
    }

    if(edit == sedit)
    {
        if(ch >= ' ' && ch <= 126)
        {
            if(edit->length != edit->maxlength || edit_selstart != edit_selend)
            {
                if(edit_selstart > edit_selend)
                {
                    uint16_t temp;
                    temp = edit_selstart;
                    edit_selstart = edit_selend;
                    edit_selend = temp;
                }

                if(edit_selstart == edit_selend)
                {
                    uint8_t *start = edit->data + edit_selstart, *end = edit->data + edit->length;
                    while(end != start)
                    {
                        *end = *(end - 1);
                        end--;
                    }

                    edit->data[edit_selstart] = ch;
                    edit->length++;

                    edit_selstart++;
                    edit_selend++;
                }
                else
                {
                    edit->data[edit_selstart] = ch;
                    memcpy(edit->data + edit_selstart + 1, edit->data + edit_selend, edit->length - edit_selend);
                    edit->length -= (edit_selend - edit_selstart - 1);

                    edit_selstart = edit_selend = edit_selstart + 1;
                }


                edit_draw(edit);
            }
        }
        else
        {
            switch(ch)
            {
                case VK_BACK:
                {
                    if(edit_selstart == edit_selend)
                    {
                        if(edit_selstart != 0)
                        {
                            memcpy(edit->data + edit_selstart - 1, edit->data + edit_selstart, edit->length - edit_selstart);
                            edit->length--;
                            edit->data[edit->length] = 0;



                            edit_selstart--;
                            edit_selend--;
                        }
                    }
                    else
                    {
                        if(edit_selstart > edit_selend)
                        {
                            uint16_t temp;
                            temp = edit_selstart;
                            edit_selstart = edit_selend;
                            edit_selend = temp;
                        }

                        printf("%u %u %u\n", edit_selstart, edit_selend, edit->length);

                        memcpy(edit->data + edit_selstart, edit->data + edit_selend, edit->length - edit_selend);
                        edit->length -= (edit_selend - edit_selstart);

                        edit_selend = edit_selstart;
                    }

                    edit_draw(edit);
                    break;
                }

                case VK_RETURN:
                {
                    if(edit->onenter)
                    {
                        edit->onenter();
                    }
                    break;
                }
            }
        }
    }
}

void edit_paste(EDIT *edit, uint8_t *data, uint16_t length)
{
    if(edit->locked)
    {
        return;
    }

    int len = (int)edit->length + length;
    if(len > edit->maxlength)
    {
        len = edit->maxlength - edit->length;
    }
    else
    {
        len = length;
    }

    memcpy(edit->data + edit->length, data, len);
    edit->length += len;

    edit_selstart = edit_selend = edit->length;

    edit_draw(edit);
}
