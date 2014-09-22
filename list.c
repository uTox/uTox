#include "main.h"

static ITEM item_add, item_settings, item_transfer;
static ITEM item[1024], *mitem, *nitem;
ITEM *sitem = &item_add;
static uint32_t itemcount, searchcount;

static _Bool sitem_mousedown;

static int sitem_dy;

static void drawitembox(ITEM *i, int y)
{
    if(sitem == i) {
        drawpixel(LIST_X, y, LIST_EDGE6);
        drawhline(LIST_X + 1, y, LIST_RIGHT, LIST_EDGE7);
        drawpixel(LIST_RIGHT, y, LIST_EDGE5);
        drawvline(LIST_X, y + 1, y + ITEM_HEIGHT, LIST_EDGE4);
        drawrect(LIST_X + 1, y + 1, LIST_RIGHT + 1, y + ITEM_HEIGHT, LIST_SELECTED);

        //drawrectw(LIST_X + 5 * SCALE / 2, y + 5 * SCALE / 2, 40, 40, LIST_MAIN);
    } else if(mitem == i) {
        drawrect(LIST_X + 1, y + 1, LIST_RIGHT, y + ITEM_HEIGHT, LIST_HIGHLIGHT);
    }
}

static void drawname(ITEM *i, int y, uint8_t *name, uint8_t *msg, uint16_t name_length, uint16_t msg_length)
{
    setcolor((sitem == i) ? LIST_DARK : LIST_SELECTED);
    setfont(FONT_LIST_NAME);
    drawtextwidth(LIST_NAME_X, LIST_RIGHT - LIST_NAME_X - SCALE * 16, y + LIST_NAME_Y, name, name_length);

    setcolor((sitem == i) ? LIST_MAIN : C_STATUS);
    setfont(FONT_STATUS);
    drawtextwidth(LIST_STATUS_X, LIST_RIGHT - LIST_STATUS_X - SCALE * 16, y + LIST_STATUS_Y,  msg, msg_length);
}

static void drawitem(ITEM *i, int x, int y)
{
    drawitembox(i, y);

    switch(i->item) {
    case ITEM_FRIEND: {
        FRIEND *f = i->data;

        drawalpha(BM_CONTACT, LIST_AVATAR_X, y + LIST_AVATAR_Y, BM_CONTACT_WIDTH, BM_CONTACT_WIDTH, (sitem == i) ? LIST_MAIN : WHITE);

        drawname(i, y, f->name, f->status_message, f->name_length, f->status_length);

        uint8_t status = f->online ? f->status : 3;
        drawalpha(BM_ONLINE + status, LIST_RIGHT - SCALE * 12, y + ITEM_HEIGHT / 2 - BM_STATUS_WIDTH / 2, BM_STATUS_WIDTH, BM_STATUS_WIDTH, status_color[status]);
        if(f->notify) {
            drawalpha(BM_STATUS_NOTIFY, LIST_RIGHT - SCALE * 13, y + ITEM_HEIGHT / 2 - BM_STATUS_NOTIFY_WIDTH / 2, BM_STATUS_NOTIFY_WIDTH, BM_STATUS_NOTIFY_WIDTH, status_color[status]);
        }
        break;
    }

    case ITEM_GROUP: {
        GROUPCHAT *g = i->data;
        drawalpha(BM_GROUP, LIST_AVATAR_X, y + LIST_AVATAR_Y, BM_CONTACT_WIDTH, BM_CONTACT_WIDTH, (sitem == i) ? LIST_MAIN : WHITE);
        drawname(i, y, g->name, g->topic, g->name_length, g->topic_length);
        break;
    }

    case ITEM_FRIEND_ADD: {
        FRIENDREQ *f = i->data;

        uint8_t name[TOX_FRIEND_ADDRESS_SIZE * 2];
        id_to_string(name, f->id);

        drawalpha(BM_CONTACT, LIST_AVATAR_X, y + LIST_AVATAR_Y, BM_CONTACT_WIDTH, BM_CONTACT_WIDTH, (sitem == i) ? LIST_MAIN : WHITE);
        drawname(i, y, name, f->msg, sizeof(name), f->length);
        break;
    }
    }
}

static ITEM* newitem(void)
{
    ITEM *i = &item[itemcount++];
    //TODO: ..
    scroll_list.content_height = searchcount * ITEM_HEIGHT;
    return i;
}

void list_scale(void)
{
    scroll_list.content_height = searchcount * ITEM_HEIGHT;
}

static ITEM* item_hit(int mx, int my, int height)
{
    if(mx < LIST_X || mx >= LIST_RIGHT) {
        return NULL;
    }

    if(my < 0) {
        return NULL;
    }

    my /= ITEM_HEIGHT;
    if(my >= searchcount) {
        return NULL;
    }

    ITEM *i;

    i = &item[my + search_offset[my]];
    return i;
}

static void selectitem(ITEM *i)
{
    panel_item[sitem->item - 1].disabled = 1;
    panel_item[i->item - 1].disabled = 0;

    if(sitem->item == ITEM_FRIEND) {
        FRIEND *f = sitem->data;

        free(f->typed);
        f->typed_length = edit_msg.length;
        f->typed = malloc(edit_msg.length);
        memcpy(f->typed, edit_msg.data, edit_msg.length);

        f->msg.scroll = messages_friend.panel.content_scroll->d;

        f->edit_history = edit_msg.history;
        f->edit_history_cur = edit_msg.history_cur;
        f->edit_history_length = edit_msg.history_length;
    }

    if(sitem->item == ITEM_GROUP) {
        GROUPCHAT *g = sitem->data;

        free(g->typed);
        g->typed_length = edit_msg.length;
        g->typed = malloc(edit_msg.length);
        memcpy(g->typed, edit_msg.data, edit_msg.length);

        g->msg.scroll = messages_group.panel.content_scroll->d;

        g->edit_history = edit_msg.history;
        g->edit_history_cur = edit_msg.history_cur;
        g->edit_history_length = edit_msg.history_length;
    }

    if(sitem->item == ITEM_SETTINGS) {
        button_settings.disabled = 0;
    }

    if(sitem->item == ITEM_ADD) {
        button_add.disabled = 0;
    }

    if(sitem->item == ITEM_TRANSFER) {
        button_transfer.disabled = 0;
    }

    if(i->item == ITEM_FRIEND) {
        FRIEND *f = i->data;

        memcpy(edit_msg.data, f->typed, f->typed_length);
        edit_msg.length = f->typed_length;

        messages_friend.data = &f->msg;
        messages_updateheight(&messages_friend);

        messages_friend.iover = ~0;
        messages_friend.panel.content_scroll->content_height = f->msg.height;
        messages_friend.panel.content_scroll->d = f->msg.scroll;

        f->msg.id = f - friend;

        f->notify = 0;

        edit_msg.history = f->edit_history;
        edit_msg.history_cur = f->edit_history_cur;
        edit_msg.history_length = f->edit_history_length;
    }

    if(i->item == ITEM_GROUP) {
        GROUPCHAT *g = i->data;

        memcpy(edit_msg.data, g->typed, g->typed_length);
        edit_msg.length = g->typed_length;

        messages_group.data = &g->msg;
        messages_updateheight(&messages_group);

        messages_group.iover = ~0;
        messages_group.panel.content_scroll->content_height = g->msg.height;
        messages_group.panel.content_scroll->d = g->msg.scroll;

        g->msg.id = g - group;

        edit_msg.history = g->edit_history;
        edit_msg.history_cur = g->edit_history_cur;
        edit_msg.history_length = g->edit_history_length;
    }

    if(i->item == ITEM_SETTINGS) {
        button_settings.disabled = 1;
    }

    if(i->item == ITEM_ADD) {
        button_add.disabled = 1;
    }

    if(i->item == ITEM_TRANSFER) {
        button_transfer.disabled = 1;
    }

    sitem = i;

    edit_resetfocus();

    addfriend_status = 0;

    redraw();
}

void list_start(void)
{
    ITEM *i = item;

    item_add.item = ITEM_ADD;
    button_add.disabled = 1;

    item_settings.item = ITEM_SETTINGS;
    item_transfer.item = ITEM_TRANSFER;
    //sitem = &item_settings;

    FRIEND *f = friend, *end = f + friends;
    while(f != end) {
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
    while(i < itemcount) {
        if(item[i].data == req) {
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
    int my, j, k;

    ITEM *i = item, *mi = NULL;
    FRIEND *f;
    j = 0;
    k = 0;
    //TODO: only draw visible
    while(i != &item[itemcount]) {
        f = i->data;
        if(i->item != ITEM_FRIEND ||
           ((!FILTER || f->online) && (!SEARCH || strstr_case((char*)f->name, (char*)search_data)))) {
            if(i == sitem && (sitem_dy >= 5 || sitem_dy <= -5)) {
                mi = i;
                my = y + sitem_dy;

                //RECT r = {LIST_X, y, LIST_X + ITEM_WIDTH, y + ITEM_HEIGHT};
                //fillrect(&r, WHITE);
            } else {
                drawitem(i, LIST_X, y);
            }
            search_offset[j] = k - j;
            search_unset[k] = j - k;
            j++;
            y += ITEM_HEIGHT;
        } else {
            search_offset[j] = INT_MAX;
        }
        k++;
        i++;
    }

    searchcount = j;
    scroll_list.content_height = searchcount * ITEM_HEIGHT;

    if(mi) {
        drawitem(mi, LIST_X, my);
    }
}

static void deleteitem(ITEM *i)
{
    ritem = NULL;

    if(i == sitem) {
        if(i == &item[itemcount] - 1) {
            if(i == item) {
                selectitem(&item_add);
            } else {
                selectitem(i - 1);
            }
        } else {
            selectitem(i + 1);
        }
    }

    switch(i->item) {
    case ITEM_FRIEND: {
        FRIEND *f = i->data;
        tox_postmessage(TOX_DELFRIEND, (f - friend), 0, f);
        break;
    }

    case ITEM_GROUP: {
        GROUPCHAT *g = i->data;

        tox_postmessage(TOX_LEAVEGROUP, (g - group), 0, NULL);

        group_free(g);
        break;
    }

    case ITEM_FRIEND_ADD: {
        free(i->data);
        break;
    }

    default: {
        return;
    }
    }

    itemcount--;
    scroll_list.content_height = itemcount * ITEM_HEIGHT;

    int size = (&item[itemcount] - i) * sizeof(ITEM);
    memmove(i, i + 1, size);

    if(i != sitem && sitem > i && sitem >= item && sitem < item + countof(item)) {
        sitem--;
    }

    redraw();//list_draw();
}

void list_deletesitem(void)
{
    if(sitem >= item && sitem < item + countof(item)) {
        deleteitem(sitem);
    }
}

void list_deleteritem(void)
{
    if(ritem >= item && ritem < item + countof(item)) {
        deleteitem(ritem);
    }
}

void list_freeall(void)
{
    ITEM *i;
    for(i = item; i != item + itemcount; i++) {
        switch(i->item) {
        case ITEM_FRIEND:
            friend_free(i->data);
            break;
        case ITEM_GROUP:
            group_free(i->data);
            break;
        case ITEM_FRIEND_ADD:
            free(i->data);
            break;
        }
    }
}

void list_selectsettings(void)
{
    selectitem(&item_settings);
}

void list_selectaddfriend(void)
{
    selectitem(&item_add);
}

void list_selectswap(void)
{
    selectitem(&item_transfer);
}


_Bool list_mmove(void *n, int x, int y, int width, int height, int mx, int my, int dx, int dy)
{
    ITEM *i = item_hit(mx, my, height);

    _Bool draw = 0;

    if(i != mitem) {
        mitem = i;
        draw = 1;
    }

    if(sitem_mousedown) {
        sitem_dy += dy;
        nitem = NULL;
        if(abs(sitem_dy) >= ITEM_HEIGHT / 2) {
            int d;
            if(sitem_dy > 0) {
                d = (sitem_dy + ITEM_HEIGHT / 2) / ITEM_HEIGHT;
            } else {
                d = (sitem_dy - ITEM_HEIGHT / 2) / ITEM_HEIGHT;
            }
            int index = (sitem - item) + search_unset[sitem - item] + d;
            int offset = search_offset[index];
            if(offset != INT_MAX) {
                index += offset;
                if(index >= 0 && index < itemcount) {
                    nitem = item + index;
                }
            }
        }

        draw = 1;
    }

    return draw;
}

_Bool list_mdown(void *n)
{
    _Bool draw = 0;

    if(mitem) {
        if(mitem != sitem) {
            selectitem(mitem);
            draw = 1;
        }

        sitem_mousedown = 1;
    }

    return draw;
}

static void contextmenu_list_onselect(uint8_t i)
{
    if(ritem->item == ITEM_FRIEND_ADD && i == 0) {
        FRIENDREQ *req = ritem->data;
        tox_postmessage(TOX_ACCEPTFRIEND, 0, 0, req);
        return;
    }

    list_deleteritem();
}

_Bool list_mright(void *n)
{
    if(mitem) {
        ritem = mitem;
        if(mitem->item == ITEM_FRIEND) {
            uint8_t *remove = (uint8_t*)"Remove";
            contextmenu_new(&remove, 1, contextmenu_list_onselect);
        } else if(mitem->item == ITEM_GROUP) {
            uint8_t *leave = (uint8_t*)"Leave";
            contextmenu_new(&leave, 1, contextmenu_list_onselect);
        } else {
            uint8_t *names[] = {(uint8_t*)"Accept", (uint8_t*)"Ignore"};
            contextmenu_new(names, 2, contextmenu_list_onselect);
        }
        return 1;
        //listpopup(mitem->item);
    }

    return 0;
}

_Bool list_mwheel(void *UNUSED(n), int UNUSED(height), double UNUSED(d))
{
    return 0;
}

_Bool list_mup(void *n)
{
    _Bool draw = 0;
    if(sitem_mousedown && abs(sitem_dy) >= 5) {
        if(nitem) {
            if(sitem->item == ITEM_FRIEND) {
                if(nitem->item == ITEM_FRIEND) {
                    ITEM temp;

                    temp = *sitem;
                    *sitem = *nitem;
                    *nitem = temp;

                    sitem = nitem;
                }

                if(nitem->item == ITEM_GROUP) {
                    FRIEND *f = sitem->data;
                    GROUPCHAT *g = nitem->data;

                    tox_postmessage(TOX_GROUPINVITE, (g - group), (f - friend), NULL);
                }

            }

            if(sitem->item == ITEM_GROUP) {
                if(nitem->item == ITEM_FRIEND || nitem->item == ITEM_GROUP) {
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
    if(mitem) {
        mitem = NULL;

        return 1;
    }

    return 0;
}
