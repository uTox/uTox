
#include "main.h"

static ITEM item[256], *mitem, *nitem;
static uint32_t itemcount;

static double listscroll = 0.0;

static _Bool scroll_mousedown, sitem_mousedown;
static uint8_t scroll_mouseover;

static int sitem_dy;

static void drawitembox(ITEM *i, int x, int y)
{
    RECT r = {x, y, x + ITEM_WIDTH, y + ITEM_HEIGHT - 1};
    //RECT line = {x, y + ITEM_HEIGHT - 1, x + ITEM_WIDTH, y + ITEM_HEIGHT};

    if(i == sitem)
    {
        fillrect(&r, BLUE);
        SetBkColor(hdc, BLUE);
    }
    else if(i == nitem)
    {
        if((i->item == ITEM_FRIEND && (sitem->item == ITEM_FRIEND || sitem->item == ITEM_GROUP)) ||
           (i->item == ITEM_GROUP && sitem->item == ITEM_GROUP))
        {
            fillrect(&r, YELLOW);
            SetBkColor(hdc, YELLOW);
        }
        else if(i->item == ITEM_GROUP && sitem->item == ITEM_FRIEND)
        {
            fillrect(&r, BLUE);
            SetBkColor(hdc, BLUE);
        }
        else
        {
            fillrect(&r, GRAY);
            SetBkColor(hdc, GRAY);
        }
    }
    else if(nitem == NULL && i == mitem)
    {
        fillrect(&r, GRAY);
        SetBkColor(hdc, GRAY);
    }
    else
    {
        fillrect(&r, WHITE);
        SetBkColor(hdc, WHITE);
    }

    drawhline(x, y + ITEM_HEIGHT - 1, ITEM_WIDTH, GRAY);
}

static void drawstatusrect(RECT *r, uint8_t status)
{
    uint32_t color;
    uint32_t colors[] = {GREEN, YELLOW, BLUE, RED};
    color = colors[status];

    fillrect(r, color);
}

static void drawitem(ITEM *i, int x, int y)
{
    drawitembox(i, x, y);

    SetBkMode(hdc, TRANSPARENT);

    switch(i->item)
    {
        case ITEM_FRIEND:
        {
            FRIEND *f = i->data;

            //
            RECT r = {x, y, x + ITEM_HEIGHT - 1, y + ITEM_HEIGHT - 1};
            fillrect(&r, RED);

            r.right = x + 16;
            r.bottom = y + 16;

            if(f->online)
            {
                drawstatusrect(&r, f->status);
            }
            else
            {
                fillrect(&r, RED);
            }


            SetTextColor(hdc, (sitem == i) ? WHITE : 0x333333);
            SelectObject(hdc, font_med);
            drawtextwidth(x + 60, ITEM_WIDTH - 60, y + 8, f->name, f->name_length);

            SetTextColor(hdc, (sitem == i) ? 0xFFE6C5 : 0x999999);
            SelectObject(hdc, font_med2);
            drawtextwidth(x + 60, ITEM_WIDTH - 60, y + 28,  f->status_message, f->status_length);
            break;
        }

        case ITEM_GROUP:
        {
            GROUPCHAT *g = i->data;

            //
            RECT r = {x, y, x + ITEM_HEIGHT - 1, y + ITEM_HEIGHT - 1};
            fillrect(&r, RED2);

            SetTextColor(hdc, (sitem == i) ? WHITE : 0x333333);
            SelectObject(hdc, font_med);
            drawtextwidth(x + 60, ITEM_WIDTH - 60, y + 8, g->name, g->name_length);

            SetTextColor(hdc, (sitem == i) ? 0xFFE6C5 : 0x999999);
            SelectObject(hdc, font_med2);
            drawtextwidth(x + 60, ITEM_WIDTH - 60, y + 28,  g->topic, g->topic_length);
            break;
        }

        case ITEM_SELF:
        {
            RECT r = {x, y, x + ITEM_HEIGHT - 1, y + ITEM_HEIGHT - 1};
            fillrect(&r, GREEN);

            r.right = x + 16;
            r.bottom = y + 16;

            drawstatusrect(&r, status);

            SetTextColor(hdc, (sitem == i) ? WHITE : 0x333333);
            SelectObject(hdc, font_med);
            drawtextwidth(x + 60, ITEM_WIDTH - 60, y + 8, name, name_length);

            SetTextColor(hdc, (sitem == i) ? 0xFFE6C5 : 0x999999);
            SelectObject(hdc, font_med2);
            drawtextwidth(x + 60, ITEM_WIDTH - 60, y + 28, statusmsg, status_length);
            break;
        }

        case ITEM_ADDFRIEND:
        {
            SetTextColor(hdc, (sitem == i) ? WHITE : 0x999999);

            SelectObject(hdc, font_big);
            drawstr(x + 50, y + 16, "Add Friend");

            SelectObject(hdcMem, bm_plus);
            BitBlt(hdc, x + 18, y + 18, 16, 16, hdcMem, 0, 0, SRCCOPY);

            break;
        }

        case ITEM_FRIENDREQUESTS:
        {
            SetTextColor(hdc, (sitem == i) ? WHITE : 0x999999);

            SelectObject(hdc, font_big);
            drawstr(x + 50, y + 16, "Accept Friends");

            SelectObject(hdcMem, bm_plus);
            BitBlt(hdc, x + 18, y + 18, 16, 16, hdcMem, 0, 0, SRCCOPY);


             if(requests)
            {
                int c;
                char numreq[16];
                c = sprintf(numreq, "%u", requests);

                SIZE size;

                GetTextExtentPoint32(hdc, numreq, c, &size);

                RECT r = {x + ITEM_WIDTH - 30 - size.cx / 2 - 6, y + 16, x + ITEM_WIDTH - 30 + size.cx / 2 + 7, y + 38};
                framerect(&r, (sitem == i) ? GRAY_BORDER : GRAY);

                r.left++;
                r.top++;
                r.right--;
                r.bottom--;

                fillrect(&r, WHITE);

                SetTextColor(hdc, 0x999999);
                drawtext(x + ITEM_WIDTH - 30 - size.cx / 2, y + 16, numreq, c);
            }

            break;
        }

        case ITEM_NEWGROUP:
        {
            SetTextColor(hdc, (sitem == i) ? WHITE : 0x999999);

            SelectObject(hdc, font_big);
            drawstr(x + 50, y + 16, "New Group");

            SelectObject(hdcMem, bm_plus);
            BitBlt(hdc, x + 18, y + 18, 16, 16, hdcMem, 0, 0, SRCCOPY);
            break;
        }
    }
}

static ITEM* newitem(void)
{
    return &item[itemcount++];
}

static uint8_t scroll_hit(int x, int y)
{
    x -= SCROLL_X;
    if(x < 0 || x >= SCROLL_WIDTH)
    {
        return 0;
    }

    if(y < SCROLL_Y || y >= SCROLL_BOTTOM)
    {
        return 0;
    }

    return 2;
}

static ITEM* item_hit(int x, int y)
{
    x -= LIST_X;
    if(x < 0 || x >= ITEM_WIDTH + 1)
    {
        return NULL;
    }

    uint32_t h = SCROLL_BOTTOM - SCROLL_Y;

    y -= LIST_Y;
    if(y < 0 || y >= h)
    {
        return NULL;
    }

    uint32_t c = itemcount * ITEM_HEIGHT;


    if(c > h)
    {
        y += (listscroll * (double)(c - h)) + 0.5;
    }

    y /= ITEM_HEIGHT;
    if(y >= itemcount)
    {
        return NULL;
    }

    ITEM *i = item;
    int k = 0;
    while(k != y)
    {
        if(i->item) {k++;}
        i++;
    }

    while(!i->item){i++;}

    return i;
}

static void selectitem(ITEM *i)
{
    if(sitem->item == ITEM_FRIEND)
    {
        FRIEND *f = sitem->data;

        free(f->typed);
        f->typed_length = edit_msg.length;
        f->typed = malloc(edit_msg.length);
        memcpy(f->typed, edit_msg_data, edit_msg.length);

        msg_sel.m = f->sel;
    }

    if(sitem->item == ITEM_GROUP)
    {
        GROUPCHAT *g = sitem->data;

        free(g->typed);
        g->typed_length = edit_msg.length;
        g->typed = malloc(edit_msg.length);
        memcpy(g->typed, edit_msg_data, edit_msg.length);

        msg_sel.m = g->sel;
    }

    if(i->item == ITEM_FRIEND)
    {
        FRIEND *f = i->data;

        memcpy(edit_msg_data, f->typed, f->typed_length);
        edit_msg.length = f->typed_length;

        msg_sel.m = f->sel;
    }

    if(i->item == ITEM_GROUP)
    {
        GROUPCHAT *g = i->data;

        memcpy(edit_msg_data, g->typed, g->typed_length);
        edit_msg.length = g->typed_length;

        msg_sel.m = g->sel;
    }

    if(i->item == ITEM_FRIENDREQUESTS)
    {
        sreq = request[0];
        sreqq = &request[0];
    }
    else
    {
        sreq = NULL;
        sreqq = NULL;
    }

    sitem = i;
    sedit = NULL;

    addfriend_status = 0;

    main_draw();
}

void list_init(void)
{
    ITEM *i = item;

    sitem = i;

    i->item = ITEM_SELF;
    i++;

    i->item = ITEM_FRIENDREQUESTS;
    i++;

    i->item = ITEM_ADDFRIEND;
    i++;

    i->item = ITEM_NEWGROUP;
    i++;

    FRIEND *f = friend, *end = f + friends;
    while(f != end)
    {
        i->item = ITEM_FRIEND;
        i->data = f;
        i++;
        f++;
    }

    itemcount = i - item;
}

void list_addfriend(FRIEND *f)
{
    ITEM *i = newitem();
    i->item = ITEM_FRIEND;
    i->data = f;

    list_draw();
}

void list_addgroup(GROUPCHAT *g)
{
    ITEM *i = newitem();
    i->item = ITEM_GROUP;
    i->data = g;

    list_draw();
}

void list_draw(void)
{
    HRGN rgn = CreateRectRgn(LIST_X, LIST_Y, LIST_X + ITEM_WIDTH + SCROLL_WIDTH + 1, SCROLL_BOTTOM);
    SelectClipRgn (hdc, rgn);
    DeleteObject(rgn);

    RECT area = {LIST_X, LIST_Y, LIST_X + ITEM_WIDTH + SCROLL_WIDTH + 1, SCROLL_BOTTOM};
    fillrect(&area, WHITE);

    uint32_t c = itemcount * ITEM_HEIGHT;
    uint32_t h = SCROLL_BOTTOM - SCROLL_Y;

    int y = LIST_Y, my, dy = 0;

    if(c > h)
    {
        dy = (listscroll * (double)(c - h)) + 0.5;
    }

    ITEM *i = item, *mi = NULL;

    i += dy / ITEM_HEIGHT;
    y -= dy % ITEM_HEIGHT;
    while(i != &item[itemcount])
    {
        if(i == sitem && (sitem_dy >= 5 || sitem_dy <= -5))
        {
            mi = i;
            my = y + sitem_dy;

            RECT r = {LIST_X, y, LIST_X + ITEM_WIDTH, y + ITEM_HEIGHT};
            fillrect(&r, WHITE);
        }
        else
        {
            drawitem(i, LIST_X, y);
        }

        y += ITEM_HEIGHT;
        i++;
    }

    if(mi)
    {
        drawitem(mi, LIST_X, my);
    }

    SetTextColor(hdc, BLACK);
    SetBkColor(hdc, WHITE);

    RECT r = {SCROLL_X, SCROLL_Y, SCROLL_X + SCROLL_WIDTH, SCROLL_BOTTOM};

    if(c > h)
    {
        fillrect(&r, (scroll_mouseover) ? GRAY : WHITE);

        uint32_t m = (h * h) / c;
        double d = (h - m);
        uint32_t y = (listscroll * d) + 0.5;

        r.top += y;
        r.bottom = r.top + m;

    }

    fillrect(&r, (scroll_mouseover == 2) ? GRAY3 : GRAY2);

    //RECT f = {LIST_X, LIST_Y - ITEM_HEIGHT, LIST_X + ITEM_WIDTH, LIST_Y};
    //fillrect(&f, WHITE);

    //f.top = SCROLL_BOTTOM;
    //f.bottom = height - 1;

    //fillrect(&f, WHITE);

    SelectClipRgn(hdc, NULL);

    commitdraw(LIST_X, LIST_Y, ITEM_WIDTH + SCROLL_WIDTH + 1, SCROLL_BOTTOM - LIST_Y);

}

void list_deletesitem(void)
{
    itemcount--;

    int size = (&item[itemcount] - sitem) * sizeof(ITEM);
    memcpy(sitem, sitem + 1, size);

    int x = LIST_X, y = LIST_Y + itemcount * ITEM_HEIGHT;
    RECT r = {x, y, x + ITEM_WIDTH, y + ITEM_HEIGHT};
    fillrect(&r, WHITE);

    if(!size)
    {
        sitem = &item[0];
        drawitem(&item[0], LIST_X, LIST_Y);

        main_draw();
    }
    else
    {
        list_draw();
        main_draw();
    }
}

void list_mousemove(int x, int y, int dy)
{
    ITEM *i = item_hit(x, y);

    uint8_t sc;

    if(scroll_mousedown)
    {
        sc = scroll_mouseover;

        uint32_t c = itemcount * ITEM_HEIGHT;
        uint32_t h = SCROLL_BOTTOM - SCROLL_Y;

        if(c > h)
        {
            uint32_t m = (h * h) / c;
            double d = (h - m);

            listscroll = ((listscroll * d) + (double)dy) / d;

            if(listscroll < 0.0)
            {
                listscroll = 0.0;
            }
            else if(listscroll >= 1.0)
            {
                listscroll = 1.0;
            }

            list_draw();
        }
    }
    else
    {
        sc = scroll_hit(x, y);
    }

    _Bool draw = 0;

    if(i != mitem)
    {
        mitem = i;
        draw = 1;
    }

    if(scroll_mouseover != sc)
    {
        scroll_mouseover = sc;
        draw = 1;
    }

    if(sitem_mousedown)
    {
        sitem_dy += dy;
        nitem = NULL;
        if(abs(sitem_dy) >= ITEM_HEIGHT / 2)
        {
            int d;
            if(sitem_dy > 0)
            {
                d = (sitem_dy + ITEM_HEIGHT / 2) / ITEM_HEIGHT;
            }
            else
            {
                d = (sitem_dy - ITEM_HEIGHT / 2) / ITEM_HEIGHT;
            }

            ITEM *i = sitem + d;
            if(d != 0 && i >= item && i < &item[itemcount])
            {
                nitem = i;
            }
        }

        list_draw();
    }

    if(draw)
    {
        list_draw();
    }


}

void list_mousedown(void)
{
    _Bool draw = 0;

    if(scroll_mouseover)
    {
        if(scroll_mouseover == 1)
        {

        }
        else
        {
            if(!scroll_mousedown)
            {
                scroll_mousedown = 1;
                draw = 1;
            }
        }
    }

    if(mitem)
    {
        if(mitem != sitem)
        {
            selectitem(mitem);
            draw = 1;
        }

        sitem_mousedown = 1;
    }

    if(draw)
    {
        list_draw();
    }
}

void list_mouseup(void)
{
    if(scroll_mousedown)
    {
        scroll_mousedown = 0;
        list_draw();
    }

    _Bool draw = 0;
    if(sitem_mousedown && abs(sitem_dy) >= 5)
    {
        if(nitem)
        {
            if(sitem->item == ITEM_FRIEND)
            {
                if(nitem->item == ITEM_FRIEND)
                {
                    ITEM temp;

                    temp = *sitem;
                    *sitem = *nitem;
                    *nitem = temp;

                    sitem = nitem;
                }

                if(nitem->item == ITEM_GROUP)
                {
                    FRIEND *f = sitem->data;
                    GROUPCHAT *g = nitem->data;

                    core_postmessage3(CMSG_GROUPINVITE, ((f - friend) << 16) | (g - group));
                }

            }

            if(sitem->item == ITEM_GROUP)
            {
                if(nitem->item == ITEM_FRIEND || nitem->item == ITEM_GROUP)
                {
                    ITEM temp;

                    temp = *sitem;
                    *sitem = *nitem;
                    *nitem = temp;

                    sitem = nitem;
                }
            }

            nitem = NULL;
        }


        draw = 1;
    }

    sitem_mousedown = 0;
    sitem_dy = 0;

    if(draw)
    {
        list_draw();
    }

}

void list_mouseleave(void)
{
    if(scroll_mouseover || mitem)
    {
        scroll_mouseover = 0;
        mitem = NULL;

        list_draw();
    }
}
