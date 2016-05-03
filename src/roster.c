#include "main.h"
#ifdef UNITY
#include "xlib/mmenu.h"
extern _Bool unity_running;
#endif

/* I think these are pointers to the panel's they're named after. */
static ITEM item_add, item_settings, item_transfer;


// full list of friends and group chats
static ITEM item[1024]; /* TODO MAGIC NUMBER*/
static uint32_t itemcount;

static uint32_t shown_list[1024]; // list of chats actually shown in the GUI after filtering(actually indices pointing to chats in the chats array)
static uint32_t showncount;

// search and filter stuff
static char_t* search_string;
static uint8_t filter;

static ITEM *mouseover_item;
static ITEM *nitem; // item that selected_item is being dragged over
ITEM *selected_item = &item_add;

static _Bool mouse_in_list,
             selected_item_mousedown;

static int selected_item_dy; // y offset of selected item being dragged from its original position

static void roster_draw_itembox(ITEM *i, int y) {
    int height = 0;

    if (settings.use_mini_roster) {
        height = ROSTER_BOX_HEIGHT / 2;
    } else {
        height = ROSTER_BOX_HEIGHT;
    }

    if(selected_item == i) {
        drawrect(ROSTER_BOX_LEFT, y + 1, SIDEBAR_WIDTH, y + height, COLOR_BACKGROUND_MAIN);
    } else if(mouseover_item == i) {
        drawrect(ROSTER_BOX_LEFT, y + 1, SIDEBAR_WIDTH, y + height, COLOR_BACKGROUND_LIST_HOVER);
    }
}

static void roster_draw_name(ITEM *i, int y, char_t *name, char_t *msg, uint16_t name_length, uint16_t msg_length,
                     _Bool color_overide, uint32_t color)
{
    if (!color_overide) {
        color = (selected_item == i) ? COLOR_MAIN_TEXT : COLOR_LIST_TEXT;
    }
    setcolor(color);
    setfont(FONT_LIST_NAME);

    if (settings.use_mini_roster) {
        /* Name only*/
        drawtextwidth(ROSTER_NAME_LEFT / 2 + SCALE(5), SIDEBAR_WIDTH - ROSTER_NAME_LEFT / 2 - SCALE(32),
                      y + ROSTER_NAME_TOP / 2, name, name_length);
    } else {
        /* Name + user status msg*/
        drawtextwidth(ROSTER_NAME_LEFT, SIDEBAR_WIDTH - ROSTER_NAME_LEFT - SCALE(32),
                      y + ROSTER_NAME_TOP, name, name_length);

        if (!color_overide) {
            color = (selected_item == i) ? COLOR_MAIN_SUBTEXT : COLOR_LIST_SUBTEXT;
        }
        setcolor(color);
        setfont(FONT_STATUS);
        drawtextwidth(ROSTER_NAME_LEFT, SIDEBAR_WIDTH - ROSTER_NAME_LEFT - SCALE(32),
                      y + ROSTER_STATUS_MSG_TOP, msg, msg_length);

    }
}

static void roster_draw_status_icon(uint8_t status, int y, _Bool notify) {
    int notify_y = y;
    if (settings.use_mini_roster) {
        y        += (ROSTER_BOX_HEIGHT / 4) - (BM_STATUS_WIDTH / 2);
        notify_y += (ROSTER_BOX_HEIGHT / 4) - (BM_STATUS_NOTIFY_WIDTH / 2);
    } else {
        y        += (ROSTER_BOX_HEIGHT / 2) - (BM_STATUS_WIDTH / 2);
        notify_y += (ROSTER_BOX_HEIGHT / 2) - (BM_STATUS_NOTIFY_WIDTH / 2);
    }

    drawalpha(BM_ONLINE + status,   SIDEBAR_WIDTH - SCALE(24),        y, BM_STATUS_WIDTH,        BM_STATUS_WIDTH, status_color[status]);
    if (notify) {
        drawalpha(BM_STATUS_NOTIFY, SIDEBAR_WIDTH - SCALE(26), notify_y, BM_STATUS_NOTIFY_WIDTH, BM_STATUS_NOTIFY_WIDTH, status_color[status]);
    }
}

static void drawitem(ITEM *i, int UNUSED(x), int y) {
    roster_draw_itembox(i, y);

    int default_w      = 0;
    int ava_top        = 0;
    int group_bitmap   = 0;
    int contact_bitmap = 0;

    if (settings.use_mini_roster) {
        default_w      = BM_CONTACT_WIDTH  / 2;
        ava_top        = ROSTER_AVATAR_TOP / 2;
        group_bitmap   = BM_GROUP_MINI;
        contact_bitmap = BM_CONTACT_MINI;
    } else {
        default_w      = BM_CONTACT_WIDTH;
        ava_top        = ROSTER_AVATAR_TOP;
        group_bitmap   = BM_GROUP;
        contact_bitmap = BM_CONTACT;
    }

    switch(i->item) {
        case ITEM_FRIEND: {
            FRIEND *f = i->data;
            uint8_t status = f->online ? f->status : 3;

            // draw avatar or default image
            if (friend_has_avatar(f)) {
                draw_avatar_image(f->avatar.image, ROSTER_AVATAR_LEFT, y + ava_top, f->avatar.width, f->avatar.height, default_w, default_w);
            } else {
                drawalpha(contact_bitmap, ROSTER_AVATAR_LEFT, y + ava_top, default_w, default_w, (selected_item == i) ? COLOR_MAIN_TEXT : COLOR_LIST_TEXT);
            }

            roster_draw_name(i, y, UTOX_FRIEND_NAME(f), f->status_message, UTOX_FRIEND_NAME_LENGTH(f), f->status_length, 0, 0);

            roster_draw_status_icon(status, y, f->unread_msg);
            break;
        }

        case ITEM_GROUP: {
            GROUPCHAT *g = i->data;
            drawalpha(group_bitmap, ROSTER_AVATAR_LEFT, y + ava_top, default_w, default_w, (selected_item == i) ? COLOR_MAIN_TEXT : COLOR_LIST_TEXT);
            _Bool color_overide = 0;
            uint32_t color = 0;

            if (g->muted) {
                color_overide = 1;
                color = COLOR_GROUP_MUTED;
            } else {
                uint64_t time = get_time();
                unsigned int j;
                for (j = 0; j < g->peer_count; ++j) {
                    if (time - g->last_recv_audio[j] <= (uint64_t)1 * 1000 * 1000 * 1000) {
                        color_overide = 1;
                        color = COLOR_GROUP_AUDIO;
                        break;
                    }
                }
            }

            roster_draw_name(i, y, g->name, g->topic, g->name_length, g->topic_length, color_overide, color);

            roster_draw_status_icon(0, y, g->notify);
            break;
        }

        case ITEM_FRIEND_ADD: {
            FRIENDREQ *f = i->data;

            char_t name[TOX_FRIEND_ADDRESS_SIZE * 2];
            id_to_string(name, f->id);

            drawalpha(contact_bitmap, ROSTER_AVATAR_LEFT, y + ROSTER_AVATAR_TOP, default_w, default_w, (selected_item == i) ? COLOR_MAIN_TEXT : COLOR_LIST_TEXT);
            roster_draw_name(i, y, name, f->msg, sizeof(name), f->length, 0, 0);
            break;
        }

        case ITEM_CREATE_GROUP:{
            drawalpha(group_bitmap, ROSTER_AVATAR_LEFT, y + ROSTER_AVATAR_TOP, default_w, default_w, (selected_item == i) ? COLOR_MAIN_TEXT : COLOR_LIST_TEXT);
            roster_draw_name(i, y, S(CREATEGROUPCHAT),    S(CURSOR_CLICK_RIGHT),
                                   SLEN(CREATEGROUPCHAT), SLEN(CURSOR_CLICK_RIGHT), 1, (selected_item == i) ? COLOR_MAIN_TEXT : COLOR_LIST_TEXT);
            break;
        }
    }
}

// find index of given item in shown_list, or INT_MAX if it can't be found
static int find_item_shown_index(ITEM *it) {
    int i = 0;
    while (i < showncount) {
        if (shown_list[i] == it - item) { // (it - item) returns the index of the item in the full items list
            return i;
        }
        i++;
    }
    return INT_MAX; // can't be found!
}

void roster_re_scale(void) {
    scrollbar_roster.content_height = showncount * (ROSTER_BOX_HEIGHT / (!!settings.use_mini_roster + 1));
}

_Bool friend_matches_search_string(FRIEND *f, char_t *str) {
    return !str ||
            strstr_case((char*)f->name, (char*)str) ||
            (f->alias && strstr_case((char*)f->alias, (char*)str));
}

void update_shown_list(void) {
    FRIEND *f;
    uint32_t i; // index in item array
    uint32_t j; // index in shown_list array
    for (i = j = 0; i < itemcount; i++) {
        ITEM *it = &item[i];
        f = it->data;
        if(it->item != ITEM_FRIEND ||
                ((!filter || f->online) && friend_matches_search_string(f, search_string))) {
            shown_list[j++] = i;
        }
    }

    showncount = j;
    roster_re_scale();
}

static ITEM* newitem(void) {
    ITEM *i = &item[itemcount - 1];
    item[itemcount].item = ITEM_CREATE_GROUP;
    item[itemcount].data = NULL;
    itemcount++;
    update_shown_list();
    return i;
}

// return item that the user is mousing over
static ITEM* item_hit(int mx, int my, int height) {
    int real_height = 0;
    if (settings.use_mini_roster) {
        real_height = ROSTER_BOX_HEIGHT / 2;
    } else {
        real_height = ROSTER_BOX_HEIGHT;
    }

    /* Mouse is outsite the list */
    if (mx < ROSTER_BOX_LEFT || mx >= SIDEBAR_WIDTH ||
        my < 0               || my >= showncount * real_height) { /* TODO: Height is a bit buggy, Height needs /2
                                                                      * figure out why!  */
        mouse_in_list = 0;
        return NULL;
    }

    uint32_t item_idx = my;
    item_idx /= real_height;

    /* mouse is below the last item */
    if(item_idx >= showncount) {
        mouse_in_list = 1;
        return NULL;
    }

    ITEM *i;
    i = &item[shown_list[item_idx]];
    mouse_in_list = 1;
    return i;
}

uint8_t list_get_filter(void) {
    return filter;
}

void list_set_filter(uint8_t new_filter) {
    filter = new_filter;
    update_shown_list();
}

void list_search(char_t *str) {
    search_string = str;
    update_shown_list();
}

// change the selected item by [offset] items in the shown list
static void change_tab(int offset) {
    /* Pg-Up/Dn broke on the create group icon,
     * remoing this if seems to work but I don't know what it was doing here
     * so I commented it incase it breaks stuff...  */
    // if (selected_item->item == ITEM_FRIEND ||
        // selected_item->item == ITEM_GROUP) {
        int index = find_item_shown_index(selected_item);
        if (index != INT_MAX) {
            // list_selectchat will check if out of bounds
            list_selectchat((index + offset + showncount) % showncount);
        }
    // }
}

void previous_tab(void) {
    change_tab(-1);
}

void next_tab(void) {
    change_tab(1);
}

/* TODO: move this out of here!
 * maybe to ui.c ? */
static void show_page(ITEM *i) {
    // TODO!!
    // panel_item[selected_item->item - 1].disabled = 1;
    // panel_item[i->item - 1].disabled = 0;
    int current_width;

    edit_resetfocus();

    /* First things first, we need to deselect and store the old data. */
    switch (selected_item->item){
        case ITEM_FRIEND: {
            FRIEND *f = selected_item->data;

            current_width = f->msg.width;

            free(f->typed);
            f->typed_length = edit_msg.length;
            f->typed = malloc(edit_msg.length);
            memcpy(f->typed, edit_msg.data, edit_msg.length);

            f->msg.scroll = messages_friend.content_scroll->d;

            f->edit_history = edit_msg.history;
            f->edit_history_cur = edit_msg.history_cur;
            f->edit_history_length = edit_msg.history_length;


            panel_chat.disabled            = 1;
            panel_friend.disabled          = 1;
            panel_friend_chat.disabled     = 1;
            panel_friend_video.disabled    = 1;
            panel_friend_settings.disabled = 1;
            break;
        }
        case ITEM_FRIEND_ADD: {
            panel_chat.disabled           = 1;
            panel_friend_request.disabled = 1;
            break;
        }
        case ITEM_GROUP: {
            GROUPCHAT *g = selected_item->data;

            current_width = g->msg.width;

            free(g->typed);
            g->typed_length = edit_msg_group.length;
            g->typed = malloc(edit_msg_group.length);
            memcpy(g->typed, edit_msg_group.data, edit_msg_group.length);

            g->msg.scroll = messages_group.content_scroll->d;

            g->edit_history = edit_msg_group.history;
            g->edit_history_cur = edit_msg_group.history_cur;
            g->edit_history_length = edit_msg_group.history_length;

            panel_chat.disabled  = 1;
            panel_group.disabled = 1;
            break;
        }
        case ITEM_SETTINGS: {
            if (panel_profile_password.disabled) {
                button_settings.disabled       = 0;
                panel_settings_master.disabled = 1;
                panel_overhead.disabled        = 1;
                panel_profile_password_settings.disabled = 1;
            }
            break;
        }
        case ITEM_ADD: {
            button_add_new_contact.disabled = 0;
            panel_add_friend.disabled       = 1;
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

            f->msg.width  = current_width;
            f->msg.id     = f - friend;
            f->unread_msg = 0;
            /* We use the MESSAGES struct from the friend, but we need the info from the panel. */
            messages_friend.object = ((void**)&f->msg);
            messages_updateheight((MESSAGES*)messages_friend.object, current_width);

            ((MESSAGES*)messages_friend.object)->cursor_over_msg = UINT32_MAX;
            ((MESSAGES*)messages_friend.object)->cursor_over_uri = UINT32_MAX;
            scrollbar_friend.content_height = f->msg.height;
            messages_friend.content_scroll->d = f->msg.scroll;

            edit_msg.history = f->edit_history;
            edit_msg.history_cur = f->edit_history_cur;
            edit_msg.history_length = f->edit_history_length;
            edit_setfocus(&edit_msg);

            panel_chat.disabled            = 0;
            panel_friend.disabled          = 0;
            panel_friend_chat.disabled     = 0;
            panel_friend_video.disabled    = 1;
            panel_friend_settings.disabled = 1;
            break;
        }
        case ITEM_GROUP: {
            GROUPCHAT *g = i->data;

            memcpy(edit_msg_group.data, g->typed, g->typed_length);
            edit_msg_group.length = g->typed_length;

            g->msg.width = current_width;
            g->msg.id = g - group;
            g->notify = 0;
            /* We use the MESSAGES struct from the group, but we need the info from the panel. */
            messages_group.object = ((void*)&g->msg);
            messages_updateheight((MESSAGES*)messages_group.object, current_width);

            ((MESSAGES*)messages_group.object)->cursor_over_msg = UINT32_MAX;
            messages_group.content_scroll->content_height = g->msg.height;
            messages_group.content_scroll->d = g->msg.scroll;
            edit_setfocus(&edit_msg_group);

            edit_msg_group.history = g->edit_history;
            edit_msg_group.history_cur = g->edit_history_cur;
            edit_msg_group.history_length = g->edit_history_length;

            panel_chat.disabled           = 0;
            panel_group.disabled          = 0;
            panel_group_chat.disabled     = 0;
            panel_group_video.disabled    = 1;
            panel_group_settings.disabled = 1;
            break;
        }
        case ITEM_SETTINGS: {
            if (panel_profile_password.disabled) {
                button_settings.disabled        = 1;
                panel_overhead.disabled         = 0;
                panel_settings_master.disabled  = 0;
            }
            break;
        }
        case ITEM_ADD: {
            button_add_new_contact.disabled = 1;
            panel_overhead.disabled         = 0;
            panel_add_friend.disabled       = 0;
            edit_setfocus(&edit_add_id);
            break;
        }
        case ITEM_CREATE_GROUP: {
            // postmessage_toxcore(TOX_GROUP_CREATE, 0, 0, NULL);
            break;
        }

    } // Switch

    selected_item = i;

    addfriend_status = 0;
}

void list_start(void) {
    ITEM *i = item;

    item_add.item       = ITEM_ADD;
    item_settings.item  = ITEM_SETTINGS;

    button_settings.disabled = 1;
    selected_item = &item_settings;

    FRIEND *f = friend, *end = f + friends;

    while (f != end) {
        i->item = ITEM_FRIEND;
        i->data = f;
        i++;
        f++;
    }

    itemcount = i - item;

    newitem(); /* Called alone will create the group bar */

    search_string = NULL;
    update_shown_list();
}

void list_addfriend(FRIEND *f) {
    ITEM *i = newitem();
    i->item = ITEM_FRIEND;
    i->data = f;
}

void list_addfriend2(FRIEND *f, FRIENDREQ *req) {
    uint32_t i = 0;
    while(i < itemcount) {
        if(item[i].data == req) {
            if(&item[i] == selected_item) {
                // panel_item[selected_item->item - 1].disabled = 1;
                // panel_item[ITEM_FRIEND - 1].disabled = 0;

                messages_friend.object = (void*)&f->msg;
                ((MESSAGES*)messages_friend.object)->cursor_over_msg = UINT32_MAX;
                messages_friend.content_scroll->content_height = f->msg.height;
                messages_friend.content_scroll->d = f->msg.scroll;

                f->msg.id = f - friend;
            }

            item[i].item = ITEM_FRIEND;
            item[i].data = f;
            return;
        }
        i++;
    }
}

void list_addgroup(GROUPCHAT *g) {
    ITEM *i = newitem();
    i->item = ITEM_GROUP;
    i->data = g;
}

void list_addfriendreq(FRIENDREQ *f) {
    ITEM *i = newitem();
    i->item = ITEM_FRIEND_ADD;
    i->data = f;
}

void list_draw(void *UNUSED(n), int UNUSED(x), int y, int UNUSED(width), int UNUSED(height)) {
    int real_height = 0;
    if (settings.use_mini_roster) {
        real_height = ROSTER_BOX_HEIGHT / 2;
    } else {
        real_height = ROSTER_BOX_HEIGHT;
    }

    ITEM *mi = NULL; // item being dragged
    int my; // y of item being dragged

    for (int i = 0; i < showncount; i++) {
        ITEM *it = &item[shown_list[i]];
        if(it == selected_item && (selected_item_dy >= 5 || selected_item_dy <= -5)) {
            mi = it;
            my = y + selected_item_dy;
        } else {
            drawitem(it, ROSTER_BOX_LEFT, y);
        }
        y += real_height;
    }

    if (mi) {
        drawitem(mi, ROSTER_BOX_LEFT, my);
    }
}

void group_av_peer_remove(GROUPCHAT *g, int peernumber);


// TODO  removing multiple items without moving the mouse causes asan neg-size-param error on memmove!
static void deleteitem(ITEM *i) {
    right_mouse_item = NULL;

    if (i == selected_item) {
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
            postmessage_toxcore(TOX_FRIEND_DELETE, (f - friend), 0, f);
            break;
        }

        case ITEM_GROUP: {
            GROUPCHAT *g = i->data;
            postmessage_toxcore(TOX_GROUP_PART, (g - group), 0, NULL);
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

    int size = (&item[itemcount] - i) * sizeof(ITEM);
    memmove(i, i + 1, size);

    if(i != selected_item && selected_item > i && selected_item >= item && selected_item < item + countof(item)) {
        selected_item--;
    }

    update_shown_list();
    redraw();//list_draw();
}

void list_deletesitem(void) {
    if(selected_item >= item && selected_item < item + countof(item)) {
        deleteitem(selected_item);
    }
}

void roster_delete_rmouse_item(void) {
    if(right_mouse_item >= item && right_mouse_item < item + countof(item)) {
        deleteitem(right_mouse_item);
    }
}

void list_freeall(void) {
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
        i->item = ITEM_NONE;
    }
    itemcount  = 0;
    showncount = 0;
}

void list_selectchat(int index) {
    if (index >= 0 && index < showncount) {
        show_page(&item[shown_list[index]]);
    }
}

void list_reselect_current(void) {
    show_page(selected_item);
}

void list_selectsettings(void) {
    show_page(&item_settings);
}

void list_selectaddfriend(void) {
    show_page(&item_add);
}

void list_selectswap(void) {
    show_page(&item_transfer);
}

void roster_select_last(void) {
    /* -2 should be the last, -1 is the create group */
    show_page(&item[itemcount-2]);
}

_Bool list_mmove(void *UNUSED(n), int UNUSED(x), int UNUSED(y), int UNUSED(width), int height, int mx, int my, int UNUSED(dx), int dy) {
    int real_height = 0;
    if (settings.use_mini_roster) {
        real_height = ROSTER_BOX_HEIGHT / 2;
    } else {
        real_height = ROSTER_BOX_HEIGHT;
    }


    ITEM *i = item_hit(mx, my, height);

    _Bool draw = 0;

    if(i != mouseover_item) {
        mouseover_item = i;
        draw = 1;
    }

    if(selected_item_mousedown) {
        // drag item
        selected_item_dy += dy;
        nitem = NULL;
        if(abs(selected_item_dy) >= real_height / 2) {
            int d; // offset, in number of items, of where the dragged item is compared to where it started
            if(selected_item_dy > 0) {
                d = (selected_item_dy + real_height / 2) / real_height;
            } else {
                d = (selected_item_dy - real_height / 2) / real_height;
            }
            int index = find_item_shown_index(selected_item);
            if (index != INT_MAX) { // selected_item was found in shown list
                index += d; // get item being dragged over

                // set item being dragged over
                if(index >= 0 && ((uint32_t) index) < itemcount) {
                    nitem = &item[shown_list[index]];
                }
            }
        }

        draw = 1;
    } else {
        tooltip_draw();
    }

    return draw;
}

_Bool list_mdown(void *UNUSED(n)) {
    _Bool draw = 0;
    tooltip_mdown(); /* may need to return on true */
    if (mouseover_item) {
        show_page(mouseover_item);
        draw = 1;
        selected_item_mousedown = 1;
    }

    return draw;
}

static void roster_init_settings_page(void) {
    FRIEND *f = right_mouse_item->data;

    panel_friend_chat.disabled     = 1;
    panel_friend_video.disabled    = 1;
    panel_friend_settings.disabled = 0;

    maybe_i18nal_string_set_plain(&edit_friend_alias.empty_str, f->name, f->name_length);
    edit_setstr(&edit_friend_alias, f->alias, f->alias_length);

    dropdown_friend_autoaccept_ft.over = dropdown_friend_autoaccept_ft.selected = f->ft_autoaccept;
}

static void contextmenu_list_onselect(uint8_t i) {
    if (right_mouse_item) {
        switch (right_mouse_item->item) {
            case ITEM_FRIEND:{
                panel_friend_chat.disabled     = 0;
                panel_friend_video.disabled    = 1;
                panel_friend_settings.disabled = 1;
                if (i == 0) {
                    roster_init_settings_page();

                } else if (i == 1) {
                    friend_history_clear((FRIEND*)right_mouse_item->data);
                } else {
                    roster_delete_rmouse_item();
                }
                return;
            }
            case ITEM_GROUP: {
                panel_group_chat.disabled = 0;
                GROUPCHAT *g = right_mouse_item->data;
                if (i == 0) {
                    if(right_mouse_item != selected_item) {
                        show_page(right_mouse_item);
                    }

                    char str[g->name_length + 7];
                    strcpy(str, "/topic ");
                    memcpy(str + 7, g->name, g->name_length);
                    edit_setfocus(&edit_msg_group);
                    edit_paste((char_t*)str, sizeof(str), 0);
                } else if (i == 1 && g->av_group) {
                    g->muted = !g->muted;
                } else {
                    roster_delete_rmouse_item();
                }
                return;
            }
            case ITEM_FRIEND_ADD: {
                if (i == 0) {
                    FRIENDREQ *req = right_mouse_item->data;
                    postmessage_toxcore(TOX_FRIEND_ACCEPT, 0, 0, req);
                }
                return;
            }

            case ITEM_CREATE_GROUP: {
                if (i) {
                    postmessage_toxcore(TOX_GROUP_CREATE, 0, 1, NULL);
                } else {
                    postmessage_toxcore(TOX_GROUP_CREATE, 0, 0, NULL);
                }
            }
            default: {
                debug("blerg\n");
                return;
            }
        }
    } else {
        if (i) {
            postmessage_toxcore(TOX_GROUP_CREATE, 0, 0, NULL);
        } else {
            show_page(&item_add);
        }
    }
}

_Bool list_mright(void *UNUSED(n)) {
    static UI_STRING_ID menu_friend[]           = {STR_FRIEND_SETTINGS,     STR_CLEAR_HISTORY,  STR_REMOVE_FRIEND};
    static UI_STRING_ID menu_group_unmuted[]    = {STR_CHANGE_GROUP_TOPIC,  STR_MUTE,           STR_REMOVE_GROUP};
    static UI_STRING_ID menu_group_muted[]      = {STR_CHANGE_GROUP_TOPIC,  STR_UNMUTE,         STR_REMOVE_GROUP};

    static UI_STRING_ID menu_group[]            = {STR_CHANGE_GROUP_TOPIC,  STR_REMOVE_GROUP};
    static UI_STRING_ID menu_create_group[]     = {STR_GROUP_CREATE_TEXT,   STR_GROUP_CREATE_VOICE};
    static UI_STRING_ID menu_request[]          = {STR_REQ_ACCEPT,          STR_REQ_DECLINE};

    if (mouseover_item) {
        right_mouse_item = mouseover_item;
        switch (mouseover_item->item) {
            case ITEM_FRIEND: {
                contextmenu_new(countof(menu_friend), menu_friend, contextmenu_list_onselect);
                show_page(mouseover_item);
                break;
            }

            case ITEM_GROUP: {
                GROUPCHAT *g = mouseover_item->data;
                if (g->av_group) {
                    if (g->muted) {
                        contextmenu_new(countof(menu_group_muted), menu_group_muted, contextmenu_list_onselect);
                    } else {
                        contextmenu_new(countof(menu_group_unmuted), menu_group_unmuted, contextmenu_list_onselect);
                    }
                } else {
                    contextmenu_new(countof(menu_group), menu_group, contextmenu_list_onselect);
                }
                show_page(mouseover_item);
                break;
            }

            case ITEM_CREATE_GROUP: {
                contextmenu_new(countof(menu_create_group), menu_create_group, contextmenu_list_onselect);
                break;
            }

            case ITEM_FRIEND_ADD: {
                contextmenu_new(countof(menu_request), menu_request, contextmenu_list_onselect);
                break;
            }
        }

        return 1;
    } else if (mouse_in_list) {
        right_mouse_item = NULL; /* Unset right_mouse_item so that we don't interact with the incorrect context menu
                                  * I'm not sure if this belongs here or in list_mmove, or maybe item_hit. */
    }
    return 0;
}

_Bool list_mwheel(void *UNUSED(n), int UNUSED(height), double UNUSED(d), _Bool UNUSED(smooth)) {
    return 0;
}

_Bool list_mup(void *UNUSED(n)) {
    _Bool draw = 0;
    tooltip_mup(); /* may need to return one true */
    if(selected_item_mousedown && abs(selected_item_dy) >= 5) {
        if(nitem && find_item_shown_index(nitem) != INT_MAX) {
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
                        postmessage_toxcore(TOX_GROUP_SEND_INVITE, (g - group), (f - friend), NULL);
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
