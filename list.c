#include "main.h"

static ITEM item[256], *mitem, *nitem;
static uint32_t itemcount;

static _Bool sitem_mousedown;

static int sitem_dy;

static void drawitembox(ITEM *i, int x, int y)
{
    RECT r = {x, y, x + ITEM_WIDTH, y + ITEM_HEIGHT - 1};

    if(i == sitem)
    {
        fillrect(&r, BLUE);
    }
    else if(i == nitem)
    {
        if((i->item == ITEM_FRIEND && (sitem->item == ITEM_FRIEND || sitem->item == ITEM_GROUP)) ||
           (i->item == ITEM_GROUP && sitem->item == ITEM_GROUP))
        {
            fillrect(&r, YELLOW);
        }
        else if(i->item == ITEM_GROUP && sitem->item == ITEM_FRIEND)
        {
            fillrect(&r, BLUE);
        }
        else
        {
            fillrect(&r, GRAY);
        }
    }
    else if(nitem == NULL && i == mitem)
    {
        fillrect(&r, GRAY);
    }
    else
    {
        fillrect(&r, WHITE);
    }

    drawhline(x, y + ITEM_HEIGHT - 1, x + ITEM_WIDTH, GRAY);
}

static void drawname(ITEM *i, int x, int y, uint8_t *name, uint8_t *msg, uint16_t name_length, uint16_t msg_length)
{
    SetTextColor(hdc, (sitem == i) ? WHITE : 0x333333);
    SelectObject(hdc, font_med);
    drawtextwidth(x + 50, ITEM_WIDTH - 50, y + 6, name, name_length);

    SetTextColor(hdc, (sitem == i) ? 0xFFE6C5 : 0x999999);
    SelectObject(hdc, font_med2);
    drawtextwidth(x + 50, ITEM_WIDTH - 50, y + 25,  msg, msg_length);
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

            drawbitmap(BM_CONTACT, x, y, 48, 48);

            if(f->online)
            {
                drawbitmapalpha(BM_ONLINE + f->status, x + 3, y + 3, 10, 10);
            }
            else
            {
                drawbitmapalpha(BM_OFFLINE, x + 3, y + 3, 10, 10);
            }

            drawname(i, x, y, f->name, f->status_message, f->name_length, f->status_length);

            break;
        }

        case ITEM_GROUP:
        {
            GROUPCHAT *g = i->data;

            drawbitmap(BM_GROUP, x, y, 48, 48);

            drawname(i, x, y, g->name, g->topic, g->name_length, g->topic_length);
            break;
        }

        case ITEM_SELF:
        {
            drawbitmap(BM_CONTACT, x, y, 48, 48);

            drawbitmapalpha(tox_connected ? (BM_ONLINE + self.status) : BM_OFFLINE, x + 3, y + 3, 10, 10);

            drawname(i, x, y, self.name, self.statusmsg, self.name_length, self.statusmsg_length);
            break;
        }

        case ITEM_ADDFRIEND:
        {
            SetTextColor(hdc, (sitem == i) ? WHITE : 0x999999);

            SelectObject(hdc, font_big);
            drawstr(x + 50, y + 14, "Add Friend");

            drawbitmaptrans(BM_PLUS, x + 16, y + 16, 16, 16);

            break;
        }

        case ITEM_FRIEND_ADD:
        {
            drawbitmap(BM_CONTACT, x, y, 48, 48);

            setcolor(0x939393);
            drawbitmaptrans(BM_PLUS, x + 27, y + 5, 16, 16);

            FRIENDREQ *f = i->data;

            uint8_t name[TOX_FRIEND_ADDRESS_SIZE * 2];
            id_to_string(name, f->id);

            drawname(i, x, y, name, f->msg, sizeof(name), f->length);

            break;
        }
    }
}

static ITEM* newitem(void)
{
    ITEM *i = &item[itemcount++];
    scroll_list.content_height = itemcount * ITEM_HEIGHT;
    return i;
}

static ITEM* item_hit(int x, int y)
{
    x -= LIST_X;
    if(x < 0 || x >= (scrolls(&scroll_list) ? (ITEM_WIDTH - SCROLL_WIDTH) : ITEM_WIDTH))
    {
        return NULL;
    }

    uint32_t h = SCROLL_BOTTOM - SCROLL_Y;

    y -= LIST_Y;
    if(y < 0 || y >= h)
    {
        return NULL;
    }

    y += scroll_gety(&scroll_list);

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
        memcpy(f->typed, edit_msg.data, edit_msg.length);

        msg_sel.m = f->sel;
    }

    if(sitem->item == ITEM_GROUP)
    {
        GROUPCHAT *g = sitem->data;

        free(g->typed);
        g->typed_length = edit_msg.length;
        g->typed = malloc(edit_msg.length);
        memcpy(g->typed, edit_msg.data, edit_msg.length);

        msg_sel.m = g->sel;
    }

    if(i->item == ITEM_FRIEND)
    {
        FRIEND *f = i->data;

        memcpy(edit_msg.data, f->typed, f->typed_length);
        edit_msg.length = f->typed_length;

        msg_sel.m = f->sel;
    }

    if(i->item == ITEM_GROUP)
    {
        GROUPCHAT *g = i->data;

        memcpy(edit_msg.data, g->typed, g->typed_length);
        edit_msg.length = g->typed_length;

        msg_sel.m = g->sel;
    }

    sitem = i;
    edit_setfocus(NULL);

    addfriend_status = 0;

    ui_drawmain();
}

void list_start(void)
{
    ITEM *i = item;

    sitem = i;

    i->item = ITEM_SELF;
    i++;

    i->item = ITEM_ADDFRIEND;
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

    scroll_list.content_height = itemcount * ITEM_HEIGHT;
}

void list_addfriend(FRIEND *f)
{
    ITEM *i = newitem();
    i->item = ITEM_FRIEND;
    i->data = f;

    list_draw();
}

void list_addfriend2(FRIEND *f, FRIENDREQ *req)
{
    int i = 0;
    while(i < itemcount)
    {
        if(item[i].data == req)
        {
            item[i].item = ITEM_FRIEND;
            item[i].data = f;

            list_draw();
            return;
        }
        i++;
    }
}

void list_addgroup(GROUPCHAT *g)
{
    ITEM *i = newitem();
    i->item = ITEM_GROUP;
    i->data = g;

    list_draw();
}

void list_addfriendreq(FRIENDREQ *f)
{
    ITEM *i = newitem();
    i->item = ITEM_FRIEND_ADD;
    i->data = f;

    list_draw();
}

void list_draw(void)
{
    int left = LIST_X, right = LIST_X + ITEM_WIDTH, top = LIST_Y, bottom = SCROLL_BOTTOM;

    begindraw(left, top, right, bottom);

    int y = LIST_Y, my, dy = scroll_gety(&scroll_list);

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

    enddraw();
    commitdraw(LIST_X, LIST_Y, ITEM_WIDTH, SCROLL_BOTTOM - LIST_Y);

    scroll_draw(&scroll_list);
}

static void deleteitem(ITEM *i)
{
    switch(i->item)
    {
        case ITEM_FRIEND:
        {
            FRIEND *f = i->data;

            tox_postmessage(TOX_DELFRIEND, (f - friend), 0, NULL);

            free(f->name);
            free(f->status_message);
            free(f->typed);

            int i = 0;
            while(i < f->msg)
            {
                free(f->message[i]);
                i++;
            }

            free(f->message);

            memset(f, 0, sizeof(FRIEND));//

            friends--;
            break;
        }

        case ITEM_GROUP:
        {
            GROUPCHAT *g = i->data;

            tox_postmessage(TOX_LEAVEGROUP, (g - group), 0, NULL);

            uint8_t **np = g->peername;
            int i = 0;
            while(i < g->peers)
            {
                uint8_t *n = *np++;
                if(n)
                {
                    free(n);
                    i++;
                }
            }

            i = 0;
            while(i < g->msg)
            {
                free(g->message[i]);
                i++;
            }

            free(g->message);

            memset(g, 0, sizeof(GROUPCHAT));//
            break;
        }

        case ITEM_FRIEND_ADD:
        {
            free(i->data);
            break;
        }

        default:
        {
            return;
        }
    }

    itemcount--;
    scroll_list.content_height = itemcount * ITEM_HEIGHT;

    int size = (&item[itemcount] - i) * sizeof(ITEM);
    memmove(i, i + 1, size);

    if(i == sitem)
    {
        if(sitem == &item[itemcount])
        {
            sitem--;
        }
        ui_drawmain();
    }
    else if(sitem > i)
    {
        sitem--;
    }

    list_draw();
}

void list_deletesitem(void)
{
    if(sitem)
    {
        deleteitem(sitem);
    }
}

void list_mousemove(int x, int y, int dy)
{
    ITEM *i = item_hit(x, y);

    _Bool draw = 0;

    if(i != mitem)
    {
        mitem = i;
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

                    tox_postmessage(TOX_GROUPINVITE, (f - friend), (g - group), NULL);
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
    if(mitem)
    {
        mitem = NULL;

        list_draw();
    }
}
