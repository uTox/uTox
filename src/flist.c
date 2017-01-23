#include "flist.h"

// FIXME: Separate from UI or include in UI.

#include "friend.h"
#include "groups.h"
#include "logging_native.h"
#include "macros.h"
#include "main_native.h"
#include "self.h"
#include "settings.h"
#include "text.h"
#include "theme.h"
#include "tox.h"
#include "utox.h"

#include "ui/buttons.h"
#include "ui/contextmenu.h"
#include "ui/draw.h"
#include "ui/dropdowns.h"
#include "ui/edits.h"
#include "ui/scrollable.h"
#include "ui/svg.h"
#include "ui/tooltip.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#ifdef UNITY
#include "xlib/mmenu.h"
extern bool unity_running;
#endif

/* I think these are pointers to the panels they're named after. */
static ITEM item_add, item_settings, item_transfer;

// full list of friends and group chats
static ITEM     item[1024]; /* TODO MAGIC NUMBER*/
static uint32_t itemcount;

static uint32_t shown_list[1024]; // list of chats actually shown in the GUI after filtering(actually indices pointing
                                  // to chats in the chats array)
static uint32_t showncount;

// search and filter stuff
static char *  search_string;
static uint8_t filter;

static ITEM *mouseover_item;
static ITEM *nitem; // item that selected_item is being dragged over
static ITEM *selected_item = &item_add;

static bool mouse_in_list;
static bool selected_item_mousedown;
static bool selected_item_mousedown_move_pend;

static int selected_item_dy; // y offset of selected item being dragged from its original position

static void flist_draw_itembox(ITEM *i, int y) {
    int height = 0;

    if (settings.use_mini_flist) {
        height = ROSTER_BOX_HEIGHT / 2;
    } else {
        height = ROSTER_BOX_HEIGHT;
    }

    if (selected_item == i) {
        drawrect(ROSTER_BOX_LEFT, y + 1, SIDEBAR_WIDTH, y + height, COLOR_BKGRND_MAIN);
    } else if (mouseover_item == i) {
        drawrect(ROSTER_BOX_LEFT, y + 1, SIDEBAR_WIDTH, y + height, COLOR_BKGRND_LIST_HOVER);
    }
}

static void flist_draw_name(ITEM *i, int y, char *name, char *msg, uint16_t name_length, uint16_t msg_length,
                            bool color_overide, uint32_t color) {
    if (!color_overide) {
        color = (selected_item == i) ? COLOR_MAIN_TEXT : COLOR_LIST_TEXT;
    }
    setcolor(color);
    setfont(FONT_LIST_NAME);

    if (settings.use_mini_flist) {
        /* Name only*/
        drawtextwidth(ROSTER_NAME_LEFT / 2 + SCALE(5), SIDEBAR_WIDTH - ROSTER_NAME_LEFT / 2 - SCALE(32),
                      y + ROSTER_NAME_TOP / 2, name, name_length);
    } else {
        /* Name + user status msg*/
        drawtextwidth(ROSTER_NAME_LEFT, SIDEBAR_WIDTH - ROSTER_NAME_LEFT - SCALE(32), y + ROSTER_NAME_TOP, name,
                      name_length);

        if (!color_overide) {
            color = (selected_item == i) ? COLOR_MAIN_TEXT_SUBTEXT : COLOR_LIST_TEXT_SUBTEXT;
        }
        setcolor(color);
        setfont(FONT_STATUS);
        drawtextwidth(ROSTER_NAME_LEFT, SIDEBAR_WIDTH - ROSTER_NAME_LEFT - SCALE(32), y + ROSTER_STATUS_MSG_TOP, msg,
                      msg_length);
    }
}

static void flist_draw_status_icon(uint8_t status, int y, bool notify) {
    int y_n = y;
    if (settings.use_mini_flist) {
        y += (ROSTER_BOX_HEIGHT / 4) - (BM_STATUS_WIDTH / 2);
        y_n += (ROSTER_BOX_HEIGHT / 4) - (BM_STATUS_NOTIFY_WIDTH / 2);
    } else {
        y += (ROSTER_BOX_HEIGHT / 2) - (BM_STATUS_WIDTH / 2);
        y_n += (ROSTER_BOX_HEIGHT / 2) - (BM_STATUS_NOTIFY_WIDTH / 2);
    }

    int xpos   = SIDEBAR_WIDTH - SCALE(15) - BM_STATUS_WIDTH / 2;
    int xpos_n = SIDEBAR_WIDTH - SCALE(15) - BM_STATUS_NOTIFY_WIDTH / 2;

    drawalpha(BM_ONLINE + status, xpos, y, BM_STATUS_WIDTH, BM_STATUS_WIDTH, status_color[status]);
    if (notify) {
        drawalpha(BM_STATUS_NOTIFY, xpos_n, y_n, BM_STATUS_NOTIFY_WIDTH, BM_STATUS_NOTIFY_WIDTH, status_color[status]);
    }
}

static void drawitem(ITEM *i, int UNUSED(x), int y) {
    flist_draw_itembox(i, y);

    int default_w      = 0;
    int ava_top        = 0;
    int group_bitmap   = 0;
    int contact_bitmap = 0;

    if (settings.use_mini_flist) {
        default_w      = BM_CONTACT_WIDTH / 2;
        ava_top        = ROSTER_AVATAR_TOP / 2;
        group_bitmap   = BM_GROUP_MINI;
        contact_bitmap = BM_CONTACT_MINI;
    } else {
        default_w      = BM_CONTACT_WIDTH;
        ava_top        = ROSTER_AVATAR_TOP;
        group_bitmap   = BM_GROUP;
        contact_bitmap = BM_CONTACT;
    }

    switch (i->item) {
        case ITEM_FRIEND: {
            FRIEND *f      = i->data;
            uint8_t status = f->online ? f->status : 3;

            // draw avatar or default image
            if (friend_has_avatar(f)) {
                draw_avatar_image(f->avatar.img, ROSTER_AVATAR_LEFT, y + ava_top, f->avatar.width, f->avatar.height,
                                  default_w, default_w);
            } else {
                drawalpha(contact_bitmap, ROSTER_AVATAR_LEFT, y + ava_top, default_w, default_w,
                          (selected_item == i) ? COLOR_MAIN_TEXT : COLOR_LIST_TEXT);
            }

            flist_draw_name(i, y, UTOX_FRIEND_NAME(f), f->status_message, UTOX_FRIEND_NAME_LENGTH(f), f->status_length,
                            0, 0);

            flist_draw_status_icon(status, y, f->unread_msg);
            break;
        }

        case ITEM_GROUP: {
            GROUPCHAT *g = i->data;
            drawalpha(group_bitmap, ROSTER_AVATAR_LEFT, y + ava_top, default_w, default_w,
                      (selected_item == i) ? COLOR_MAIN_TEXT : COLOR_LIST_TEXT);
            bool     color_overide = 0;
            uint32_t color         = 0;

            if (g->muted) {
                color_overide = 1;
                color         = COLOR_GROUP_MUTED;
            } else {
                uint64_t     time = get_time();
                unsigned int j;
                for (j = 0; j < g->peer_count; ++j) {
                    if (time - g->last_recv_audio[j] <= (uint64_t)1 * 1000 * 1000 * 1000) {
                        color_overide = 1;
                        color         = COLOR_GROUP_AUDIO;
                        break;
                    }
                }
            }

            flist_draw_name(i, y, g->name, g->topic, g->name_length, g->topic_length, color_overide, color);

            flist_draw_status_icon(0, y, g->unread_msg);
            break;
        }

        case ITEM_FRIEND_ADD: {
            FRIENDREQ *f = i->data;

            char name[TOX_ADDRESS_SIZE * 2];
            id_to_string(name, f->id);

            drawalpha(contact_bitmap, ROSTER_AVATAR_LEFT, y + ROSTER_AVATAR_TOP, default_w, default_w,
                      (selected_item == i) ? COLOR_MAIN_TEXT : COLOR_LIST_TEXT);
            flist_draw_name(i, y, name, f->msg, sizeof(name), f->length, 0, 0);
            break;
        }

        case ITEM_GROUP_CREATE: {
            drawalpha(group_bitmap, ROSTER_AVATAR_LEFT, y + ROSTER_AVATAR_TOP, default_w, default_w,
                      (selected_item == i) ? COLOR_MAIN_TEXT : COLOR_LIST_TEXT);
            flist_draw_name(i, y, S(CREATEGROUPCHAT), S(CURSOR_CLICK_RIGHT), SLEN(CREATEGROUPCHAT),
                            SLEN(CURSOR_CLICK_RIGHT), 1, (selected_item == i) ? COLOR_MAIN_TEXT : COLOR_LIST_TEXT);
            break;
        }

        default: {
            debug_error("Trying to draw an item that we shouldn't be drawing!\n");
            break;
        }
    }
}

// find index of given item in shown_list, or INT_MAX if it can't be found
static int find_item_shown_index(ITEM *it) {
    unsigned int i = 0;
    while (i < showncount) {
        if (shown_list[i] == it - item) { // (it - item) returns the index of the item in the full items list
            return i;
        }
        i++;
    }
    return INT_MAX; // can't be found!
}

void flist_re_scale(void) {
    scrollbar_flist.content_height = showncount * (ROSTER_BOX_HEIGHT / (!!settings.use_mini_flist + 1));
}

bool friend_matches_search_string(FRIEND *f, char *str) {
    return !str || strstr_case((char *)f->name, (char *)str) || (f->alias && strstr_case((char *)f->alias, (char *)str));
}

void flist_update_shown_list(void) {
    uint32_t i; // index in item array
    uint32_t j; // index in shown_list array
    for (i = j = 0; i < itemcount; i++) {
        ITEM  *it = &item[i];
        FRIEND *f = it->data;
        if (it->item != ITEM_FRIEND || ((!filter || f->online) && friend_matches_search_string(f, search_string))) {
            shown_list[j++] = i;
        }
    }

    showncount = j;
    flist_re_scale();
}

static ITEM *newitem(void) {
    ITEM *i              = &item[itemcount - 1];
    item[itemcount].item = ITEM_GROUP_CREATE;
    item[itemcount].data = NULL;
    itemcount++;
    flist_update_shown_list();
    return i;
}

// return item that the user is mousing over
static ITEM *item_hit(int mx, int my, int UNUSED(height)) {
    int real_height = 0;
    if (settings.use_mini_flist) {
        real_height = ROSTER_BOX_HEIGHT / 2;
    } else {
        real_height = ROSTER_BOX_HEIGHT;
    }

    /* Mouse is outsite the list */
    if (mx < ROSTER_BOX_LEFT || mx >= SIDEBAR_WIDTH || my < 0
        || my >= (int)(showncount * real_height)) { /* TODO: Height is a bit buggy, Height needs /2
                                                     * figure out why!  */
        mouse_in_list = 0;
        return NULL;
    }

    uint32_t item_idx = my;
    item_idx /= real_height;

    /* mouse is below the last item */
    if (item_idx >= showncount) {
        mouse_in_list = 1;
        return NULL;
    }

    ITEM *i;
    i             = &item[shown_list[item_idx]];
    mouse_in_list = 1;
    return i;
}

uint8_t flist_get_filter(void) {
    return filter;
}

void flist_set_filter(uint8_t new_filter) {
    filter = new_filter;
    flist_update_shown_list();
}

void flist_search(char *str) {
    search_string = str;
    flist_update_shown_list();
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
        // flist_selectchat will check if out of bounds
        flist_selectchat((index + offset + showncount) % showncount);
    }
    // }
}

void flist_previous_tab(void) {
    change_tab(-1);
}

void flist_next_tab(void) {
    change_tab(1);
}

/* TODO: move this out of here!
 * maybe to ui.c ? */
static int  current_width; // I know, but I'm in a hurry, so I'll fix this later
static void page_close(ITEM *i) {
    switch (i->item) {
        case ITEM_FRIEND: {
            FRIEND *f = i->data;

            current_width = f->msg.width;

            free(f->typed);
            f->typed_length = edit_msg.length;
            f->typed        = calloc(1, edit_msg.length);
            memcpy(f->typed, edit_msg.data, edit_msg.length);

            f->msg.scroll = messages_friend.content_scroll->d;

            f->edit_history        = edit_msg.history;
            f->edit_history_cur    = edit_msg.history_cur;
            f->edit_history_length = edit_msg.history_length;

            panel_chat.disabled            = 1;
            panel_friend.disabled          = 1;
            panel_friend_chat.disabled     = 1;
            panel_friend_video.disabled    = 1;
            panel_friend_settings.disabled = 1;
            settings.inline_video          = 1;
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
            g->typed        = calloc(1, edit_msg_group.length);
            memcpy(g->typed, edit_msg_group.data, edit_msg_group.length);

            g->msg.scroll = messages_group.content_scroll->d;

            g->edit_history        = edit_msg_group.history;
            g->edit_history_cur    = edit_msg_group.history_cur;
            g->edit_history_length = edit_msg_group.history_length;

            panel_chat.disabled  = 1;
            panel_group.disabled = 1;
            break;
        }

        case ITEM_SETTINGS: {
            if (panel_profile_password.disabled) {
                panel_splash_page.disabled = 1;
                settings.show_splash       = 0;

                panel_settings_master.disabled = 1;
                panel_overhead.disabled        = 1;

                panel_profile_password_settings.disabled = true;
                panel_nospam_settings.disabled = true;

                button_settings.disabled = 0;
            }
            break;
        }

        case ITEM_ADD: {
            button_add_new_contact.disabled = 0;
            panel_add_friend.disabled       = 1;
            break;
        }

        default: {
            debug_error("Trying to switch to an item that we shouldn't be selecting\n");
            break;
        }
    }
}

static void page_open(ITEM *i) {
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
            messages_friend.object = ((void **)&f->msg);
            messages_updateheight((MESSAGES *)messages_friend.object, current_width);

            ((MESSAGES *)messages_friend.object)->cursor_over_msg      = UINT32_MAX;
            ((MESSAGES *)messages_friend.object)->cursor_over_position = UINT32_MAX;
            ((MESSAGES *)messages_friend.object)->cursor_down_msg      = UINT32_MAX;
            ((MESSAGES *)messages_friend.object)->cursor_down_position = UINT32_MAX;
            ((MESSAGES *)messages_friend.object)->cursor_over_uri      = UINT32_MAX;

            scrollbar_friend.content_height   = f->msg.height;
            messages_friend.content_scroll->d = f->msg.scroll;

            edit_msg.history        = f->edit_history;
            edit_msg.history_cur    = f->edit_history_cur;
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

            g->msg.width  = current_width;
            g->msg.id     = g - group;
            g->unread_msg = 0;
            /* We use the MESSAGES struct from the group, but we need the info from the panel. */
            messages_group.object = &g->msg;
            messages_updateheight((MESSAGES *)messages_group.object, current_width);

            ((MESSAGES *)messages_group.object)->cursor_over_msg      = UINT32_MAX;
            ((MESSAGES *)messages_group.object)->cursor_over_position = UINT32_MAX;
            ((MESSAGES *)messages_group.object)->cursor_down_msg      = UINT32_MAX;
            ((MESSAGES *)messages_group.object)->cursor_down_position = UINT32_MAX;
            ((MESSAGES *)messages_group.object)->cursor_over_uri      = UINT32_MAX;

            messages_group.content_scroll->content_height = g->msg.height;
            messages_group.content_scroll->d              = g->msg.scroll;
            edit_setfocus(&edit_msg_group);

            edit_msg_group.history        = g->edit_history;
            edit_msg_group.history_cur    = g->edit_history_cur;
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
                button_settings.disabled       = 1;
                panel_overhead.disabled        = 0;
                panel_settings_master.disabled = 0;
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

        case ITEM_GROUP_CREATE: {
            // postmessage_toxcore(TOX_GROUP_CREATE, 0, 0, NULL);
            break;
        }

        default: {
            debug_error("Trying to switch to an item that we shouldn't be selecting\n");
            break;
        }
    }
}

static void show_page(ITEM *i) {
    // TODO!!
    // panel_item[selected_item->item - 1].disabled = 1;
    // panel_item[i->item - 1].disabled = 0;
    edit_resetfocus();

    /* First things first, we need to deselect and store the old data. */
    page_close(selected_item);

    /* Now we activate/select the new page, and load stored data */
    page_open(i);

    selected_item = i;

    addfriend_status = 0;
}

void flist_start(void) {
    selected_item            = &item_settings;
    button_settings.disabled = 1;

    item_add.item      = ITEM_ADD;
    item_settings.item = ITEM_SETTINGS;

    ITEM *i = item;
    for (uint32_t num = 0; num < self.friend_list_count; ++num) {
        FRIEND *f = &friend[num];
        i->item   = ITEM_FRIEND;
        i->data   = f;
        i++;
    }

    itemcount = i - item;

    newitem(); /* Called alone will create the group bar */

    search_string = NULL;
    flist_update_shown_list();
}

void flist_addfriend(FRIEND *f) {
    ITEM *i = newitem();
    i->item = ITEM_FRIEND;
    i->data = f;
}

void flist_addfriend2(FRIEND *f, FRIENDREQ *req) {
    uint32_t i = 0;
    while (i < itemcount) {
        if (item[i].data == req) {
            if (&item[i] == selected_item) {
                // panel_item[selected_item->item - 1].disabled = 1;
                // panel_item[ITEM_FRIEND - 1].disabled = 0;

                messages_friend.object                                = &f->msg;
                ((MESSAGES *)messages_friend.object)->cursor_over_msg = UINT32_MAX;
                messages_friend.content_scroll->content_height        = f->msg.height;
                messages_friend.content_scroll->d                     = f->msg.scroll;

                f->msg.id = f - friend;
            }

            item[i].item = ITEM_FRIEND;
            item[i].data = f;
            return;
        }
        i++;
    }
}

void flist_addgroup(GROUPCHAT *g) {
    ITEM *i = newitem();
    i->item = ITEM_GROUP;
    i->data = g;
}

void flist_addfriendreq(FRIENDREQ *f) {
    ITEM *i = newitem();
    i->item = ITEM_FRIEND_ADD;
    i->data = f;
}

void group_av_peer_remove(GROUPCHAT *g, int peernumber);

// FIXME removing multiple items without moving the mouse causes asan neg-size-param error on memmove!
static void deleteitem(ITEM *i) {
    right_mouse_item = NULL;

    if (i == selected_item) {
        if (i == &item[itemcount] - 1) {
            if (i == item) {
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

        default: { return; }
    }

    itemcount--;

    int size = (&item[itemcount] - i) * sizeof(ITEM);
    memmove(i, i + 1, size);

    if (i != selected_item && selected_item > i && selected_item >= item && selected_item < item + COUNTOF(item)) {
        selected_item--;
    }

    flist_update_shown_list();
    redraw(); // flist_draw();
}

void flist_deletesitem(void) {
    if (selected_item >= item && selected_item < item + COUNTOF(item)) {
        deleteitem(selected_item);
    }
}

void flist_delete_rmouse_item(void) {
    if (right_mouse_item >= item && right_mouse_item < item + COUNTOF(item)) {
        deleteitem(right_mouse_item);
    }
}

void flist_freeall(void) {
    ITEM *i;
    for (i = item; i != item + itemcount; i++) {
        switch (i->item) {
            case ITEM_FRIEND: {
                friend_free(i->data);
                break;
            }

            case ITEM_GROUP: {
                group_free(i->data);
                break;
            }

            case ITEM_FRIEND_ADD: {
                free(i->data);
                break;
            }

            default: { break; }
        }
        i->item = ITEM_NONE;
    }
    itemcount  = 0;
    showncount = 0;
}

void flist_selectchat(int index) {
    if (index >= 0 && (unsigned)index < showncount) {
        show_page(&item[shown_list[index]]);
    }
}

void flist_reselect_current(void) {
    show_page(selected_item);
}

void flist_selectsettings(void) {
    show_page(&item_settings);
}

void flist_selectaddfriend(void) {
    show_page(&item_add);
}

void flist_selectswap(void) {
    show_page(&item_transfer);
}

/******************************************************************************
 ****** Updated functions                                                ******
 ******************************************************************************/

static struct {
    ITEM_TYPE type;
    void *    data;
} push_pop;

static void push_selected(void) {
    push_pop.type = selected_item->item;

    switch (push_pop.type) {
        case ITEM_NONE:
        case ITEM_SETTINGS:
        case ITEM_ADD: {
            return;
        }
        case ITEM_FRIEND: {
            push_pop.data = calloc(1, TOX_PUBLIC_KEY_SIZE);
            FRIEND *f     = selected_item->data;
            memcpy(push_pop.data, &f->cid, TOX_PUBLIC_KEY_SIZE);
            break;
        }
        case ITEM_FRIEND_ADD:
        case ITEM_GROUP:
        case ITEM_GROUP_CREATE: {
            return;
        }
    }
}

static void pop_selected(void) {
    switch (push_pop.type) {
        case ITEM_NONE:
        case ITEM_SETTINGS: {
            show_page(&item_settings);
            return;
        }

        case ITEM_ADD: {
            show_page(&item_add);
            return;
        }

        case ITEM_FRIEND: {
            uint16_t i;
            for (i = 0; i < itemcount; ++i) {
                if (item[i].item == ITEM_FRIEND) {
                    FRIEND *f = item[i].data;
                    if (memcmp(push_pop.data, &f->cid, TOX_PUBLIC_KEY_SIZE) == 0) {
                        show_page(&item[i]);
                        return;
                    }
                }
            }
            show_page(&item_settings);
            break;
        }

        case ITEM_FRIEND_ADD:
        case ITEM_GROUP:
        case ITEM_GROUP_CREATE: {
            show_page(&item_settings);
            return;
        }
    }
}

void flist_select_last(void) {
    /* -2 should be the last, -1 is the create group */
    show_page(&item[itemcount - 2]);
}

void flist_dump_contacts(void) {
    push_selected();
    flist_freeall();
}

void flist_reload_contacts(void) {
    flist_start();
    pop_selected();
}

ITEM *flist_get_selected(void) {
    return selected_item;
}

/******************************************************************************
 ****** UI functions                                                     ******
 ******************************************************************************/

void flist_draw(void *UNUSED(n), int UNUSED(x), int y, int UNUSED(width), int UNUSED(height)) {
    int real_height = 0;
    if (settings.use_mini_flist) {
        real_height = ROSTER_BOX_HEIGHT / 2;
    } else {
        real_height = ROSTER_BOX_HEIGHT;
    }

    ITEM *mi = NULL; // item being dragged
    int   my;        // y of item being dragged

    for (unsigned int i = 0; i < showncount; i++) {
        ITEM *it = &item[shown_list[i]];
        if (it == selected_item && (selected_item_dy >= 5 || selected_item_dy <= -5)) {
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

bool flist_mmove(void *UNUSED(n), int UNUSED(x), int UNUSED(y), int UNUSED(width), int height, int mx, int my,
                 int UNUSED(dx), int dy) {
    int real_height = 0;

    if (settings.use_mini_flist) {
        real_height = ROSTER_BOX_HEIGHT / 2;
    } else {
        real_height = ROSTER_BOX_HEIGHT;
    }

    ITEM *i = item_hit(mx, my, height);

    bool draw = 0;

    if (i != mouseover_item) {
        mouseover_item = i;
        draw           = 1;
    }

    if (selected_item_mousedown) {
        // drag item
        selected_item_dy += dy;
        nitem = NULL;

        if (selected_item_mousedown_move_pend == true && (selected_item_dy >= 5 || selected_item_dy <= -5)) {
            selected_item_mousedown_move_pend = false;
            show_page(i);
        }

        if (abs(selected_item_dy) >= real_height / 2) {

            int d; // offset, in number of items, of where the dragged item is compared to where it started
            if (selected_item_dy > 0) {
                d = (selected_item_dy + real_height / 2) / real_height;
            } else {
                d = (selected_item_dy - real_height / 2) / real_height;
            }
            int index = find_item_shown_index(selected_item);
            if (index != INT_MAX) { // selected_item was found in shown list
                index += d;         // get item being dragged over

                // set item being dragged over
                if (index >= 0 && ((uint32_t)index) < itemcount) {
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

bool flist_mdown(void *UNUSED(n)) {
    tooltip_mdown(); /* may need to return on true */
    if (mouseover_item) {
        // show_page(mouseover_item);
        selected_item_mousedown           = true;
        selected_item_mousedown_move_pend = true;
        return true;
    }
    return false;
}

static void flist_init_friend_settings_page(void) {
    FRIEND *f = right_mouse_item->data;

    panel_friend_chat.disabled     = 1;
    panel_friend_video.disabled    = 1;
    panel_friend_settings.disabled = 0;

    edit_setstr(&edit_friend_pubkey, (char *)&f->id_str, TOX_PUBLIC_KEY_SIZE * 2);

    maybe_i18nal_string_set_plain(&edit_friend_alias.empty_str, f->name, f->name_length);
    edit_setstr(&edit_friend_alias, f->alias, f->alias_length);

    dropdown_friend_autoaccept_ft.over = dropdown_friend_autoaccept_ft.selected = f->ft_autoaccept;
}

static void flist_init_group_settings_page(void) {
    GROUPCHAT *g = right_mouse_item->data;

    panel_group_chat.disabled     = 1;
    panel_group_video.disabled    = 1;
    panel_group_settings.disabled = 0;

    edit_setstr(&edit_group_topic, g->name, g->name_length);

    dropdown_notify_groupchats.over = dropdown_notify_groupchats.selected = g->notify;
}

static void contextmenu_friend(uint8_t rcase) {
    FRIEND *f = right_mouse_item->data;

    panel_friend_chat.disabled     = 0;
    panel_friend_video.disabled    = 1;
    panel_friend_settings.disabled = 1;
    switch (rcase) {
        case 0: {
            /* should be settings page */
            flist_init_friend_settings_page();
            break;
        }
        case 1: {
            /* Should be show inline video */
            panel_friend_chat.disabled     = 1;
            panel_friend_video.disabled    = 0;
            panel_friend_settings.disabled = 1;
            settings.inline_video          = 1;
            f->video_inline                = 1;
            postmessage_utox(AV_CLOSE_WINDOW, f->number + 1, 0, NULL);
            break;
        }
        case 2: {
            /* should be clean history */
            friend_history_clear((FRIEND *)right_mouse_item->data);
            break;
        }
        case 3: {
            /* Should be: delete friend */
            flist_delete_rmouse_item();
        }
    }
}

static void contextmenu_list_onselect(uint8_t i) {
    if (right_mouse_item) {
        switch (right_mouse_item->item) {
            case ITEM_FRIEND: {
                contextmenu_friend(i);
                return;
            }
            case ITEM_GROUP: {
                panel_group_chat.disabled = 0;
                GROUPCHAT *g              = right_mouse_item->data;
                if (i == 0) {
                    flist_init_group_settings_page();
                } else if (i == 1) {
                    if (right_mouse_item != selected_item) {
                        show_page(right_mouse_item);
                    }

                    char str[g->name_length + 7];
                    strcpy(str, "/topic ");
                    memcpy(str + 7, g->name, g->name_length);
                    edit_setfocus(&edit_msg_group);
                    edit_paste((char *)str, sizeof(str), 0);
                } else if (i == 2 && g->av_group) {
                    g->muted = !g->muted;
                } else {
                    flist_delete_rmouse_item();
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

            case ITEM_GROUP_CREATE: {
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

bool flist_mright(void *UNUSED(n)) {
    static UTOX_I18N_STR menu_friend[] = { STR_FRIEND_SETTINGS, STR_CALL_VIDEO_SHOW_INLINE, STR_CLEAR_HISTORY,
                                           STR_REMOVE_FRIEND };

    static UTOX_I18N_STR menu_group_unmuted[] = { STR_GROUPCHAT_SETTINGS, STR_CHANGE_GROUP_TOPIC, STR_MUTE,
                                                  STR_REMOVE_GROUP };
    static UTOX_I18N_STR menu_group_muted[] = { STR_GROUPCHAT_SETTINGS, STR_CHANGE_GROUP_TOPIC, STR_UNMUTE,
                                                STR_REMOVE_GROUP };

    static UTOX_I18N_STR menu_group[]        = { STR_GROUPCHAT_SETTINGS, STR_CHANGE_GROUP_TOPIC, STR_REMOVE_GROUP };
    static UTOX_I18N_STR menu_create_group[] = { STR_GROUP_CREATE_TEXT, STR_GROUP_CREATE_VOICE };
    static UTOX_I18N_STR menu_request[]      = { STR_REQ_ACCEPT, STR_REQ_DECLINE };

    if (mouseover_item) {
        right_mouse_item = mouseover_item;
        switch (mouseover_item->item) {
            case ITEM_FRIEND: {
                contextmenu_new(COUNTOF(menu_friend), menu_friend, contextmenu_list_onselect);
                show_page(mouseover_item);
                break;
            }

            case ITEM_GROUP: {
                GROUPCHAT *g = mouseover_item->data;
                if (g->av_group) {
                    if (g->muted) {
                        contextmenu_new(COUNTOF(menu_group_muted), menu_group_muted, contextmenu_list_onselect);
                    } else {
                        contextmenu_new(COUNTOF(menu_group_unmuted), menu_group_unmuted, contextmenu_list_onselect);
                    }
                } else {
                    contextmenu_new(COUNTOF(menu_group), menu_group, contextmenu_list_onselect);
                }
                show_page(mouseover_item);
                break;
            }

            case ITEM_GROUP_CREATE: {
                contextmenu_new(COUNTOF(menu_create_group), menu_create_group, contextmenu_list_onselect);
                break;
            }

            case ITEM_FRIEND_ADD: {
                contextmenu_new(COUNTOF(menu_request), menu_request, contextmenu_list_onselect);
                break;
            }

            default: {
                debug_error("MRIGHT on a flist entry that shouldn't exist!\n");
                break;
            }
        }

        return 1;
    } else if (mouse_in_list) {
        right_mouse_item = NULL; /* Unset right_mouse_item so that we don't interact with the incorrect context menu
                                  * I'm not sure if this belongs here or in flist_mmove, or maybe item_hit. */
    }
    return 0;
}

bool flist_mwheel(void *UNUSED(n), int UNUSED(height), double UNUSED(d), bool UNUSED(smooth)) {
    return 0;
}

bool flist_mup(void *UNUSED(n)) {
    bool draw = false;
    tooltip_mup(); /* may need to return one true */

    if (mouseover_item && selected_item_mousedown_move_pend == true) {
        show_page(mouseover_item);
        draw = true;
    }

    if (selected_item_mousedown && abs(selected_item_dy) >= 5) {
        if (nitem && find_item_shown_index(nitem) != INT_MAX) {
            if (selected_item->item == ITEM_FRIEND) {
                if (nitem->item == ITEM_FRIEND) {
                    ITEM temp;

                    temp           = *selected_item;
                    *selected_item = *nitem;
                    *nitem         = temp;

                    selected_item = nitem;
                }

                if (nitem->item == ITEM_GROUP) {
                    FRIEND *   f = selected_item->data;
                    GROUPCHAT *g = nitem->data;

                    if (f->online) {
                        postmessage_toxcore(TOX_GROUP_SEND_INVITE, (g - group), (f - friend), NULL);
                    }
                }
            }

            if (selected_item->item == ITEM_GROUP) {
                if (nitem->item == ITEM_FRIEND || nitem->item == ITEM_GROUP) {
                    ITEM temp;

                    temp           = *selected_item;
                    *selected_item = *nitem;
                    *nitem         = temp;

                    selected_item = nitem;
                }
            }

            nitem = NULL;
        }

        draw = true;
    }

    selected_item_mousedown           = 0;
    selected_item_mousedown_move_pend = 0;
    selected_item_dy                  = 0;

    return draw;
}

bool flist_mleave(void *UNUSED(n)) {
    if (mouseover_item) {
        mouseover_item = NULL;
        return 1;
    }

    return 0;
}
