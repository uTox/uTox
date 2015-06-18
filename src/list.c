#include "main.h"
#ifdef UNITY
#include "xlib/mmenu.h"
extern _Bool unity_running;
#endif

static ITEM item_add, item_settings, item_transfer; /* I think these are pointers to the panel's they're named after. */
static ITEM item[1024], *mouseover_item, *nitem;
ITEM *selected_item = &item_add;
static uint32_t itemcount, searchcount;

static _Bool selected_item_mousedown;

static int selected_item_dy; // selected_item_delta-y??

static void drawitembox(ITEM *i, int y)
{
    if(selected_item == i) {
        drawrect(LIST_X + 1, y + 1, LIST_RIGHT + 1, y + ITEM_HEIGHT, COLOR_MAIN_BACKGROUND);

        //drawrectw(LIST_X + 5 * SCALE / 2, y + 5 * SCALE / 2, 40, 40, COLOR_LIST_BACKGROUND);
    } else if(mouseover_item == i) {
        drawrect(LIST_X + 1, y + 1, LIST_RIGHT, y + ITEM_HEIGHT, COLOR_LIST_HOVER_BACKGROUND);
    }
}

static void drawname(ITEM *i, int y, char_t *name, char_t *msg, STRING_IDX name_length, STRING_IDX msg_length, _Bool color_overide, uint32_t color)
{
    if (!color_overide) {
        color = (selected_item == i) ? COLOR_MAIN_TEXT : COLOR_LIST_TEXT;
    }

    setcolor(color);
    setfont(FONT_LIST_NAME);
    drawtextwidth(LIST_NAME_X, LIST_RIGHT - LIST_NAME_X - SCALE * 16, y + LIST_NAME_Y, name, name_length);

    if (!color_overide) {
        color = (selected_item == i) ? COLOR_MAIN_SUBTEXT : COLOR_LIST_SUBTEXT;
    }

    setcolor(color);
    setfont(FONT_STATUS);
    drawtextwidth(LIST_STATUS_X, LIST_RIGHT - LIST_STATUS_X - SCALE * 16, y + LIST_STATUS_Y,  msg, msg_length);
}

static void drawitem(ITEM *i, int UNUSED(x), int y)
{
    drawitembox(i, y);
    switch(i->item) {
    case ITEM_FRIEND: {
        FRIEND *f = i->data;

        // draw avatar or default image
        if (friend_has_avatar(f)) {
            draw_avatar_image(f->avatar.image, LIST_AVATAR_X, y + LIST_AVATAR_Y, f->avatar.width, f->avatar.height, BM_CONTACT_WIDTH, BM_CONTACT_WIDTH);
        } else {
            drawalpha(BM_CONTACT, LIST_AVATAR_X, y + LIST_AVATAR_Y, BM_CONTACT_WIDTH, BM_CONTACT_WIDTH, (selected_item == i) ? COLOR_MAIN_TEXT : COLOR_LIST_TEXT);
        }

        if(f->alias){
            drawname(i, y, f->alias, f->status_message, f->alias_length, f->status_length, 0, 0);
        } else {
            drawname(i, y, f->name, f->status_message, f->name_length, f->status_length, 0, 0);
        }

        uint8_t status = f->online ? f->status : 3;
        drawalpha(BM_ONLINE + status, LIST_RIGHT - SCALE * 12, y + ITEM_HEIGHT / 2 - BM_STATUS_WIDTH / 2, BM_STATUS_WIDTH, BM_STATUS_WIDTH, status_color[status]);
        if(f->notify) {
            drawalpha(BM_STATUS_NOTIFY, LIST_RIGHT - SCALE * 13, y + ITEM_HEIGHT / 2 - BM_STATUS_NOTIFY_WIDTH / 2, BM_STATUS_NOTIFY_WIDTH, BM_STATUS_NOTIFY_WIDTH, status_color[status]);
        }
        // tooltip_new(utf8tonative(snprint_t(f->name, sizeof(char_t)*8));
        break;
    }

    case ITEM_GROUP: {
        GROUPCHAT *g = i->data;
        drawalpha(BM_GROUP, LIST_AVATAR_X, y + LIST_AVATAR_Y, BM_CONTACT_WIDTH, BM_CONTACT_WIDTH, (selected_item == i) ? COLOR_MAIN_TEXT : COLOR_LIST_TEXT);
        _Bool color_overide = 0;
        uint32_t color = 0;

        if (g->muted) {
            color_overide = 1;
            color = COLOR_GROUP_MUTED;
        } else {
            uint64_t time = get_time();
            unsigned int j;
            for (j = 0; j < g->peers; ++j) {
                if (time - g->last_recv_audio[j] <= (uint64_t)1 * 1000 * 1000 * 1000) {
                    color_overide = 1;
                    color = COLOR_GROUP_AUDIO;
                    break;
                }
            }
        }

        drawname(i, y, g->name, g->topic, g->name_length, g->topic_length, color_overide, color);
        break;
    }

    case ITEM_FRIEND_ADD: {
        FRIENDREQ *f = i->data;

        char_t name[TOX_FRIEND_ADDRESS_SIZE * 2];
        id_to_string(name, f->id);

        drawalpha(BM_CONTACT, LIST_AVATAR_X, y + LIST_AVATAR_Y, BM_CONTACT_WIDTH, BM_CONTACT_WIDTH, (selected_item == i) ? COLOR_MAIN_TEXT : COLOR_LIST_TEXT);
        drawname(i, y, name, f->msg, sizeof(name), f->length, 0, 0);
        break;
    }
    }
}

static ITEM* newitem(void)
{
    ITEM *i = &item[itemcount++];
    //TODO: ..
    scrollbar_roster.content_height = searchcount * ITEM_HEIGHT;
    return i;
}

void list_scale(void)
{
    scrollbar_roster.content_height = searchcount * ITEM_HEIGHT;
}

static ITEM* item_hit(int mx, int my, int UNUSED(height))
{
    /* Mouse is outsite the list */
    if(mx < LIST_X || mx >= LIST_RIGHT) {
        return NULL;
    }

    /* Mouse is above the list */
    if(my < 0) {
        return NULL;
    }

    uint32_t item_idx = my;
    item_idx /= ITEM_HEIGHT;

    if(item_idx >= searchcount) {
        return NULL;
    }

    ITEM *i;

    i = &item[item_idx + search_offset[item_idx]];
    return i;
}


// TODO move this out of here!
static void show_page(ITEM *i){
    // TODO!!
    // panel_item[selected_item->item - 1].disabled = 1;
    // panel_item[i->item - 1].disabled = 0;

    edit_resetfocus();

    /* First things first, we need to deselect and store the old data. */
    switch (selected_item->item){
    case ITEM_FRIEND: {
        FRIEND *f = selected_item->data;

        free(f->typed);
        f->typed_length = edit_msg.length;
        f->typed = malloc(edit_msg.length);
        memcpy(f->typed, edit_msg.data, edit_msg.length);

        f->msg.scroll = messages_friend.panel.content_scroll->d;

        f->edit_history = edit_msg.history;
        f->edit_history_cur = edit_msg.history_cur;
        f->edit_history_length = edit_msg.history_length;


        panel_friend_chat.disabled = 1;
    }
    case ITEM_FRIEND_ADD: {
        panel_chat.disabled           = 1;
        panel_friend_request.disabled = 1;
        break;
    }
    case ITEM_GROUP: {
        GROUPCHAT *g = selected_item->data;

        free(g->typed);
        g->typed_length = edit_msg_group.length;
        g->typed = malloc(edit_msg_group.length);
        memcpy(g->typed, edit_msg_group.data, edit_msg_group.length);

        g->msg.scroll = messages_group.panel.content_scroll->d;

        g->edit_history = edit_msg_group.history;
        g->edit_history_cur = edit_msg_group.history_cur;
        g->edit_history_length = edit_msg_group.history_length;

        panel_chat.disabled       = 1;
        panel_group_chat.disabled = 1;
        break;
    }
    case ITEM_SETTINGS: {
        button_settings.disabled       = 0;
        panel_settings_master.disabled = 1;
        panel_overhead.disabled        = 1;
        break;
    }
    case ITEM_ADD: {
        button_add.disabled       = 0;
        panel_add_friend.disabled = 1;
        break;
    }

    case ITEM_TRANSFER: {
        button_transfer.disabled      = 0;
        panel_change_profile.disabled = 1;
        break;
    } /* End of last case */
    } /* End of switch    */


    /* Now we activate/select the new page, and load stored data */
    switch (i->item) {
    case ITEM_FRIEND_ADD: {
        panel_chat.disabled           = 0;
        panel_friend_request.disabled = 0;
        break;
    }
    case ITEM_FRIEND: {
        FRIEND *f = i->data;

        #ifdef UNITY
        if (unity_running) {
            mm_rm_entry(f->cid);
        }
        #endif

        memcpy(edit_msg.data, f->typed, f->typed_length);
        edit_msg.length = f->typed_length;

        messages_friend.data = &f->msg;
        messages_updateheight(&messages_friend);

        messages_friend.iover = MSG_IDX_MAX;
        messages_friend.panel.content_scroll->content_height = f->msg.height;
        messages_friend.panel.content_scroll->d = f->msg.scroll;

        f->msg.id = f - friend;

        f->notify = 0;

        edit_msg.history = f->edit_history;
        edit_msg.history_cur = f->edit_history_cur;
        edit_msg.history_length = f->edit_history_length;
        edit_setfocus(&edit_msg);

        panel_chat.disabled        = 0;
        panel_friend_chat.disabled = 0;
        break;
    }
    case ITEM_GROUP: {
        GROUPCHAT *g = i->data;

        memcpy(edit_msg_group.data, g->typed, g->typed_length);
        edit_msg_group.length = g->typed_length;

        messages_group.data = &g->msg;
        messages_updateheight(&messages_group);

        messages_group.iover = MSG_IDX_MAX;
        messages_group.panel.content_scroll->content_height = g->msg.height;
        messages_group.panel.content_scroll->d = g->msg.scroll;
        edit_setfocus(&edit_msg_group);

        g->msg.id = g - group;

        edit_msg_group.history = g->edit_history;
        edit_msg_group.history_cur = g->edit_history_cur;
        edit_msg_group.history_length = g->edit_history_length;

        panel_chat.disabled       = 0;
        panel_group_chat.disabled = 0;
        break;
    }
    case ITEM_SETTINGS: {
        button_settings.disabled        = 1;

        panel_overhead.disabled         = 0;
        panel_settings_master.disabled  = 0;
        break;
    }
    case ITEM_ADD: {
        button_add.disabled       = 1;

        panel_overhead.disabled   = 0;
        panel_add_friend.disabled = 0;
        edit_setfocus(&edit_add_id);
        break;
    }
    case ITEM_TRANSFER: {
        button_transfer.disabled      = 1;

        panel_overhead.disabled       = 0;
        panel_change_profile.disabled = 0;
        break;
    } // Last case
    } // Switch

    selected_item = i;

    addfriend_status = 0;

    // I think we shouldn't call this just here, redrawing should only be done when panel_draw is called, and now, we
    // don't even need to call the root tree, we can call subtrees/roots and should be able to increase performance.
    // redraw();
}

void list_start(void)
{
    ITEM *i = item;

    item_add.item = ITEM_ADD;
    item_settings.item = ITEM_SETTINGS;
    item_transfer.item = ITEM_TRANSFER;


    button_settings.disabled = 1;
    selected_item = &item_settings;
    // edit_setfocus(&edit_add_id);


    FRIEND *f = friend, *end = f + friends;
    while(f != end) {
        i->item = ITEM_FRIEND;
        i->data = f;
        i++;
        f++;
    }

    itemcount = i - item;

    scrollbar_roster.content_height = itemcount * ITEM_HEIGHT;
}

void list_addfriend(FRIEND *f)
{
    ITEM *i = newitem();
    i->item = ITEM_FRIEND;
    i->data = f;
}

void list_addfriend2(FRIEND *f, FRIENDREQ *req)
{
    uint32_t i = 0;
    while(i < itemcount) {
        if(item[i].data == req) {
            if(&item[i] == selected_item) {
                // panel_item[selected_item->item - 1].disabled = 1;
                // panel_item[ITEM_FRIEND - 1].disabled = 0;

                messages_friend.data = &f->msg;
                messages_friend.iover = MSG_IDX_MAX;
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

void list_draw(void *UNUSED(n), int UNUSED(x), int y, int UNUSED(width), int UNUSED(height))
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
            if(i == selected_item && (selected_item_dy >= 5 || selected_item_dy <= -5)) {
                mi = i;
                my = y + selected_item_dy;
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
    scrollbar_roster.content_height = searchcount * ITEM_HEIGHT;

    if(mi) {
        drawitem(mi, LIST_X, my);
    }
}

void group_av_peer_remove(GROUPCHAT *g, int peernumber);

static void deleteitem(ITEM *i)
{
    ritem = NULL;

    if(i == selected_item) {
        if(i == &item[itemcount] - 1) {
            if(i == item) {
                show_page(&item_add);
            } else {
                show_page(i - 1);
            }
        } else {
            show_page(i + 1);
        }
    }

    switch (i->item) {
    case ITEM_FRIEND: {
        FRIEND *f = i->data;
        tox_postmessage(TOX_DELFRIEND, (f - friend), 0, f);
        break;
    }

    case ITEM_GROUP: {
        GROUPCHAT *g = i->data;

        tox_postmessage(TOX_LEAVEGROUP, (g - group), 0, NULL);

        unsigned int j;
        for (j = 0; j < g->peers; ++j) {
            if(g->peername[j]) {
                free(g->peername[j]);
                g->peername[j] = NULL;
            }

            group_av_peer_remove(g, j);
        }

        toxaudio_postmessage(GROUP_AUDIO_CALL_END, (g - group), 0, NULL);

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
    scrollbar_roster.content_height = itemcount * ITEM_HEIGHT;

    int size = (&item[itemcount] - i) * sizeof(ITEM);
    memmove(i, i + 1, size);

    if(i != selected_item && selected_item > i && selected_item >= item && selected_item < item + countof(item)) {
        selected_item--;
    }

    redraw();//list_draw();
}

void list_deletesitem(void)
{
    if(selected_item >= item && selected_item < item + countof(item)) {
        deleteitem(selected_item);
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

static void selectchat_filtered(int index){
    int i;
    for (i = 0; i < itemcount; ++i) {
        ITEM *it = &item[i];
        FRIEND *f = it->data;
        if (it->item != ITEM_FRIEND ||
            ((!FILTER || f->online) && (!SEARCH || strstr_case((char*)f->name, (char*)search_data)))) {
            --index;
        }

        if (index == -1) {
            show_page(it);
            break;
        }
    }
}

static void selectchat_notfiltered(int index){
   if (index < itemcount) {
        show_page(&item[index]);
    }
}

void list_selectchat(int index){
    if (FILTER || SEARCH) {
        selectchat_filtered(index);
    } else {
        selectchat_notfiltered(index);
    }
}

void list_reselect_current(void){
    show_page(selected_item);
}

void list_selectsettings(void){
    show_page(&item_settings);
}

void list_selectaddfriend(void){
    show_page(&item_add);
}

void list_selectswap(void){
    show_page(&item_transfer);
}


_Bool list_mmove(void *UNUSED(n), int UNUSED(x), int UNUSED(y), int UNUSED(width), int height, int mx, int my, int UNUSED(dx), int dy)
{
    ITEM *i = item_hit(mx, my, height);

    _Bool draw = 0;

    if(i != mouseover_item) {
        mouseover_item = i;
        draw = 1;
    }

    if(selected_item_mousedown) {
        selected_item_dy += dy;
        nitem = NULL;
        if(abs(selected_item_dy) >= ITEM_HEIGHT / 2) {
            int d;
            if(selected_item_dy > 0) {
                d = (selected_item_dy + ITEM_HEIGHT / 2) / ITEM_HEIGHT;
            } else {
                d = (selected_item_dy - ITEM_HEIGHT / 2) / ITEM_HEIGHT;
            }
            int index = (selected_item - item) + search_unset[selected_item - item] + d;
            int offset = search_offset[index];
            if(offset != INT_MAX) {
                index += offset;
                if(index >= 0 && ((uint32_t) index) < itemcount) {
                    nitem = item + index;
                }
            }
        }

        draw = 1;
    } else {
        tooltip_draw();
    }

    return draw;
}

_Bool list_mdown(void *UNUSED(n))
{
    _Bool draw = 0;
    tooltip_mdown(); /* may need to return on true */
    if(mouseover_item) {
        if(mouseover_item != selected_item) {
            show_page(mouseover_item);
            draw = 1;
        }

        selected_item_mousedown = 1;
    }

    return draw;
}

static void contextmenu_list_onselect(uint8_t i){
    if(ritem->item == ITEM_FRIEND_ADD && i == 0) {
        FRIENDREQ *req = ritem->data;
        tox_postmessage(TOX_ACCEPTFRIEND, 0, 0, req);
        return;
    }

    if (ritem->item == ITEM_FRIEND && i == 0) {
        friend_history_clear((FRIEND*)ritem->data);
        return;
    }

    if (ritem->item == ITEM_GROUP && i == 1) {
        GROUPCHAT *g = ritem->data;
        if (g->type == TOX_GROUPCHAT_TYPE_AV) {
            g->muted = !g->muted;
            return;
        }
    }

    if (ritem->item == ITEM_GROUP && i == 0) {
        GROUPCHAT *g = ritem->data;
        if(ritem != selected_item) {
            show_page(ritem);
        }

        char str[g->name_length + 7];
        strcpy(str, "/topic ");
        memcpy(str + 7, g->name, g->name_length);
        edit_setfocus(&edit_msg_group);
        edit_paste((char_t*)str, sizeof(str), 0);
        return;
    }

    list_deleteritem();
}

_Bool list_mright(void *UNUSED(n))
{
    static UI_STRING_ID menu_friend[] = {STR_CLEAR_HISTORY, STR_REMOVE_FRIEND};
    static UI_STRING_ID menu_group_unmuted[] = {STR_CHANGE_GROUP_TOPIC, STR_MUTE, STR_REMOVE_GROUP};
    static UI_STRING_ID menu_group_muted[] = {STR_CHANGE_GROUP_TOPIC, STR_UNMUTE, STR_REMOVE_GROUP};
    static UI_STRING_ID menu_group[] = {STR_CHANGE_GROUP_TOPIC, STR_REMOVE_GROUP};
    static UI_STRING_ID menu_request[] = {STR_REQ_ACCEPT, STR_REQ_DECLINE};

    if(mouseover_item) {
        ritem = mouseover_item;
        if(mouseover_item->item == ITEM_FRIEND) {
            contextmenu_new(countof(menu_friend), menu_friend, contextmenu_list_onselect);
        } else if(mouseover_item->item == ITEM_GROUP) {
            GROUPCHAT *g = mouseover_item->data;

            if (g->type == TOX_GROUPCHAT_TYPE_AV) {
                if (g->muted) {
                    contextmenu_new(countof(menu_group_muted), menu_group_muted, contextmenu_list_onselect);
                } else {
                    contextmenu_new(countof(menu_group_unmuted), menu_group_unmuted, contextmenu_list_onselect);
                }
            } else {
                contextmenu_new(countof(menu_group), menu_group, contextmenu_list_onselect);
            }
        } else {
            contextmenu_new(countof(menu_request), menu_request, contextmenu_list_onselect);
        }
        return 1;
        //listpopup(mouseover_item->item);
    }

    return 0;
}

_Bool list_mwheel(void *UNUSED(n), int UNUSED(height), double UNUSED(d))
{
    return 0;
}

_Bool list_mup(void *UNUSED(n))
{
    _Bool draw = 0;
    tooltip_mup(); /* may need to return one true */
    if(selected_item_mousedown && abs(selected_item_dy) >= 5) {
        if(nitem) {
            if(selected_item->item == ITEM_FRIEND) {
                if(nitem->item == ITEM_FRIEND) {
                    ITEM temp;

                    temp = *selected_item;
                    *selected_item = *nitem;
                    *nitem = temp;

                    selected_item = nitem;
                }

                if(nitem->item == ITEM_GROUP) {
                    FRIEND *f = selected_item->data;
                    GROUPCHAT *g = nitem->data;

                    if(f->online) {
                        tox_postmessage(TOX_GROUPINVITE, (g - group), (f - friend), NULL);
                    }
                }

            }

            if(selected_item->item == ITEM_GROUP) {
                if(nitem->item == ITEM_FRIEND || nitem->item == ITEM_GROUP) {
                    ITEM temp;

                    temp = *selected_item;
                    *selected_item = *nitem;
                    *nitem = temp;

                    selected_item = nitem;
                }
            }

            nitem = NULL;
        }


        draw = 1;
    }

    selected_item_mousedown = 0;
    selected_item_dy = 0;

    return draw;
}

_Bool list_mleave(void *UNUSED(n)){
    if (mouseover_item) {
        mouseover_item = NULL;
        return 1;
    }

    return 0;
}
