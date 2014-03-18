
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

        f->message = realloc(f->message, (f->msg + 1) * sizeof(void*));
        f->message[f->msg++] = msg;


    }
    else
    {
        GROUPCHAT *g = sitem->data;

        core_postmessage(CMSG_SENDMESSAGEGROUP, (g - group), length, edit_msg_data);
    }

    main_draw();

    edit_clear();
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
            GetTextExtentPoint32(hdc, (char*)edit->data, edit_sel.start, &size);
            x1 = size.cx;
            GetTextExtentPoint32(hdc, (char*)edit->data, edit_sel.start + edit_sel.length, &size);
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

        edit_sel.p2 = fit;
        if(edit_sel.p2 > edit_sel.p1)
        {
            edit_sel.start = edit_sel.p1;
            edit_sel.length = edit_sel.p2 - edit_sel.p1;
        }
        else
        {
            edit_sel.start = edit_sel.p2;
            edit_sel.length = edit_sel.p1 - edit_sel.p2;
        }

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
        edit_sel.start = edit_sel.p1 = edit_sel.p2 = fit;
        edit_sel.length = 0;
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

void edit_rightclick(EDIT *edit, int x, int y)
{
    if(inedit(edit, x, y))
    {
        if(sedit != edit)
        {
            edit_mousedown(edit, x, y);
        }

        POINT p;
        GetCursorPos(&p);

        HMENU hMenu = CreatePopupMenu();
        if(hMenu)
        {
            _Bool emptysel = (edit_sel.length == 0);

            InsertMenu(hMenu, -1, MF_BYPOSITION | (emptysel ? MF_GRAYED : 0), EDIT_CUT, "Cut");
            InsertMenu(hMenu, -1, MF_BYPOSITION | (emptysel ? MF_GRAYED : 0), EDIT_COPY, "Copy");
            InsertMenu(hMenu, -1, MF_BYPOSITION, EDIT_PASTE, "Paste");
            InsertMenu(hMenu, -1, MF_BYPOSITION | (emptysel ? MF_GRAYED : 0), EDIT_DELETE, "Delete");
            InsertMenu(hMenu, -1, MF_BYPOSITION, EDIT_SELECTALL, "Select All");

            SetForegroundWindow(hwnd);

            TrackPopupMenu(hMenu, TPM_TOPALIGN, p.x, p.y, 0, hwnd, NULL);
            DestroyMenu(hMenu);
        }

    }
}

void edit_char(uint32_t ch)
{
    if(!sedit)
    {
        return;
    }

    EDIT *edit = sedit;

    if(ch >= ' ' && ch <= 126)
    {
        if(edit->length != edit->maxlength || edit_sel.length > 0)
        {
            uint8_t *p = edit->data + edit_sel.start;
            if(edit_sel.length == 0)
            {
                memmove(p + 1, p, edit->length - edit_sel.start);

                edit->data[edit_sel.start] = ch;
                edit->length++;
            }
            else
            {
                edit->data[edit_sel.start] = ch;

                memmove(p + 1, p + edit_sel.length, edit->length - (edit_sel.start + edit_sel.length));
                edit->length -= edit_sel.length - 1;
            }

            edit_sel.start++;
            edit_sel.length = 0;

            edit_draw(edit);
        }
    }
    else
    {
        switch(ch)
        {
            case VK_BACK:
            {
                if(edit_sel.length == 0)
                {
                    if(edit_sel.start != 0)
                    {
                        memmove(edit->data + edit_sel.start - 1, edit->data + edit_sel.start, edit->length - edit_sel.start);
                        edit->length--;

                        edit_sel.start--;
                    }
                }
                else
                {
                    edit_delete();
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

void edit_cut(void)
{
    edit_copy();
    edit_delete();
}

void edit_copy(void)
{
    uint16_t length = edit_sel.length;

    if(!sedit || length == 0)
    {
        return;
    }

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, length + 1);
    uint8_t *p = GlobalLock(hMem);
    memcpy(p, sedit->data + edit_sel.start, length);
    p[length] = 0;
    GlobalUnlock(hMem);
    OpenClipboard(0);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();
}

void edit_paste(void)
{
    if(!sedit)
    {
        return;
    }

    OpenClipboard(NULL);
    char *cd = GetClipboardData(CF_TEXT);
    int length = strlen(cd);
    char str[length], *s = str, c;
    //paste only allowed characters
    while((c = *cd++))
    {
        if(c >= ' ' && c <= 126)
        {
            *s++ = c;
        }
        else
        {
            length--;
        }
    }
    CloseClipboard();

    int newlen = (int)sedit->length + length;
    if(newlen > sedit->maxlength)
    {
        //for now just return if paste doesnt fit
        return;
    }

    uint8_t *p = sedit->data + edit_sel.start;

    memmove(p + length, p + edit_sel.length, sedit->length - (edit_sel.start + edit_sel.length));
    memcpy(p, str, length);

    sedit->length += length - edit_sel.length;

    edit_sel.start = edit_sel.start + length;
    edit_sel.length = 0;

    edit_draw(sedit);
}

void edit_delete(void)
{
    if(!sedit)
    {
        return;
    }

    uint8_t *p = sedit->data + edit_sel.start;

    memmove(p, p + edit_sel.length, sedit->length - (edit_sel.start + edit_sel.length));
    sedit->length -= edit_sel.length;

    edit_sel.length = 0;
}

void edit_selectall(void)
{
    if(!sedit || edit_select)
    {
        return;
    }

    edit_sel.start = 0;
    edit_sel.length = sedit->length;
}

void edit_clear(void)
{
    if(!sedit)
    {
        return;
    }

    sedit->length = 0;
    edit_sel.start = 0;
    edit_sel.length = 0;
}
