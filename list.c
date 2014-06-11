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
    setcolor((sitem == i) ? WHITE : 0x333333);
    setfont(FONT_MED);
    drawtextwidth(x + 50, ITEM_WIDTH - 50, y + 6, name, name_length);

    setcolor((sitem == i) ? RGB(0xC5, 0xE6, 0xFF) : 0x999999);
    setfont(FONT_TEXT_LARGE);
    drawtextwidth(x + 50, ITEM_WIDTH - 50, y + 25,  msg, msg_length);
}

static void drawitem(ITEM *i, int x, int y)
{
    drawitembox(i, x, y);

    setbgcolor(~0);

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

        case ITEM_FRIEND_ADD:
        {
            drawbitmap(BM_CONTACT, x, y, 48, 48);

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

static ITEM* item_hit(int x, int y, int height)
{
    if(x < 0 || x >= ((scroll_list.content_height > height) ? (ITEM_WIDTH - SCROLL_WIDTH) : ITEM_WIDTH))
    {
        return NULL;
    }

    if(y < 0 || y >= height)
    {
        return NULL;
    }

    y += scroll_gety(&scroll_list, height);

    y /= ITEM_HEIGHT;
    if(y >= itemcount)
    {
        return NULL;
    }

    ITEM *i = &item[y];

    return i;
}

static void selectitem(ITEM *i)
{
    panel_item[sitem->item - 1].disabled = 1;
    panel_item[i->item - 1].disabled = 0;

    if(sitem->item == ITEM_FRIEND)
    {
        FRIEND *f = sitem->data;

        free(f->typed);
        f->typed_length = edit_msg.length;
        f->typed = malloc(edit_msg.length);
        memcpy(f->typed, edit_msg.data, edit_msg.length);

        f->msg.scroll = messages_friend.panel.content_scroll->d;
    }

    if(sitem->item == ITEM_GROUP)
    {
        GROUPCHAT *g = sitem->data;

        free(g->typed);
        g->typed_length = edit_msg.length;
        g->typed = malloc(edit_msg.length);
        memcpy(g->typed, edit_msg.data, edit_msg.length);

        g->msg.scroll = messages_group.panel.content_scroll->d;
    }

    if(i->item == ITEM_FRIEND)
    {
        FRIEND *f = i->data;

        memcpy(edit_msg.data, f->typed, f->typed_length);
        edit_msg.length = f->typed_length;

        messages_friend.data = &f->msg;
        messages_updateheight(&messages_friend);

        messages_friend.iover = ~0;
        messages_friend.panel.content_scroll->content_height = f->msg.height;
        messages_friend.panel.content_scroll->d = f->msg.scroll;

        f->msg.id = f - friend;
    }

    if(i->item == ITEM_GROUP)
    {
        GROUPCHAT *g = i->data;

        memcpy(edit_msg.data, g->typed, g->typed_length);
        edit_msg.length = g->typed_length;

        messages_group.data = &g->msg;
        messages_updateheight(&messages_group);

        messages_group.iover = ~0;
        messages_group.panel.content_scroll->content_height = g->msg.height;
        messages_group.panel.content_scroll->d = g->msg.scroll;

        g->msg.id = g - group;
    }

    sitem = i;

    edit_resetfocus();

    addfriend_status = 0;

    redraw();
}

void list_start(void)
{
    ITEM *i = item;

    sitem = i;

    i->item = ITEM_SELF;
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
}

void list_addfriend2(FRIEND *f, FRIENDREQ *req)
{
    int i = 0;
    while(i < itemcount)
    {
        if(item[i].data == req)
        {
            if(&item[i] == sitem) {
                panel_item[sitem->item - 1].disabled = 1;
                panel_item[ITEM_FRIEND - 1].disabled = 0;

                messages_friend.data = &f->msg;
                messages_friend.iover = ~0;
                messages_friend.panel.content_scroll->content_height = f->msg.height;
                messages_friend.panel.content_scroll->d = f->msg.scroll;

                f->msg.id = f - friend;
            }

            item[i].item = ITEM_FRIEND;
            item[i].data = f;
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
}

void list_addfriendreq(FRIENDREQ *f)
{
    ITEM *i = newitem();
    i->item = ITEM_FRIEND_ADD;
    i->data = f;
}

void list_draw(void *n, int x, int y, int width, int height)
{
    int my, dy = scroll_gety(&scroll_list, height);

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
}

static void deleteitem(ITEM *i)
{
    if(i == sitem) {
        if(i == &item[itemcount] - 1) {
            selectitem(i - 1);
        } else {
            selectitem(i + 1);
        }
    }

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
            while(i < f->msg.n)
            {
                free(f->msg.data[i]);
                i++;
            }

            free(f->msg.data);

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
            while(i < g->msg.n)
            {
                free(g->msg.data[i]);
                i++;
            }

            free(g->msg.data);

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
            //sitem--;
        }
        //ui_drawmain();
    }
    else if(sitem > i)
    {
        sitem--;
    }

    redraw();//list_draw();
}

void list_deletesitem(void)
{
    if(sitem)
    {
        deleteitem(sitem);
    }
}

void list_deleteritem(void)
{
    if(ritem)
    {
        deleteitem(ritem);
    }
}

void list_selectaddfriend(void)
{
    selectitem(&item[1]);
}

_Bool list_mmove(void *n, int x, int y, int dy, int width, int height)
{
    ITEM *i = item_hit(x, y, height);

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

        draw = 1;
    }

    return draw;
}

_Bool list_mdown(void *n)
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

    return draw;
}

_Bool list_mright(void *n)
{
    if(mitem)
    {
        ritem = mitem;
        listpopup(mitem->item);
    }

    return 0;
}

_Bool list_mwheel(void *n, int height, double d)
{
    return 0;
}

_Bool list_mup(void *n)
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

                    tox_postmessage(TOX_GROUPINVITE, (g - group), (f - friend), NULL);
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

    return draw;
}

_Bool list_mleave(void *n)
{
    if(mitem)
    {
        mitem = NULL;

        return 1;
    }

    return 0;
}
