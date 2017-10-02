#include "flist.h"

// TODO: Separate from UI or include in UI.

#include "avatar.h"
#include "friend.h"
#include "group_invite.h"
#include "groups.h"
#include "debug.h"
#include "macros.h"
#include "self.h"
#include "settings.h"
#include "text.h"
#include "theme.h"
#include "tox.h"
#include "utox.h"

#include "ui/contextmenu.h"
#include "ui/draw.h"
#include "ui/dropdown.h"
#include "ui/edit.h"
#include "ui/button.h"
#include "ui/scrollable.h"
#include "ui/switch.h"
#include "ui/tooltip.h"

#include "layout/background.h"
#include "layout/friend.h"
#include "layout/group.h"
#include "layout/group_invite.h"
#include "layout/settings.h"
#include "layout/sidebar.h"

#include "native/time.h"
#include "native/ui.h"

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

static uint32_t shown_list[1024]; // list of chats actually shown in the GUI after filtering
                                  // (actually indices pointing to chats in the chats array)
static uint32_t showncount;

// search and filter stuff
static char *  search_string;
static uint8_t filter;

static ITEM *mouseover_item;
static ITEM *nitem; // item that selected_item is being dragged over
static ITEM *selected_item = &item_add;

static ITEM *right_mouse_item;


static bool mouse_in_list;
static bool selected_item_mousedown;
static bool selected_item_mousedown_move_pend;

static int selected_item_dy; // y offset of selected item being dragged from its original position

static void flist_draw_itembox(ITEM *i, int x, int y, int width) {
    int height;

    if (settings.use_mini_flist) {
        height = SCALE(ROSTER_BOX_HEIGHT / 2);
    } else {
        height = SCALE(ROSTER_BOX_HEIGHT);
    }

    if (selected_item == i) {
        drawrect(x, y + 1, width, y + height, COLOR_BKGRND_MAIN);
    } else if (mouseover_item == i) {
        drawrect(x, y + 1, width, y + height, COLOR_BKGRND_LIST_HOVER);
    }
}

static void flist_draw_name(ITEM *i, int x, int y, int width, char *name, char *msg, uint16_t name_length, uint16_t msg_length,
                            bool color_overide, uint32_t color)
{
    if (!color_overide) {
        color = (selected_item == i) ? COLOR_MAIN_TEXT : COLOR_LIST_TEXT;
    }
    setcolor(color);
    setfont(FONT_LIST_NAME);
    /* Always draw name*/
    drawtextrange(x, width - SCALE(SIDEBAR_PADDING * 5), y, name, name_length);

    if (!settings.use_mini_flist) {
        /* Name + user status msg*/
        if (!color_overide) {
            color = (selected_item == i) ? COLOR_MAIN_TEXT_SUBTEXT : COLOR_LIST_TEXT_SUBTEXT;
        }
        setcolor(color);
        setfont(FONT_STATUS);
        drawtextrange(x, width - SCALE(SIDEBAR_PADDING * 5), y + SCALE(16), msg, msg_length);
    }
}

static void flist_draw_status_icon(uint8_t status, int x, int y, bool notify) {
    y -= BM_STATUS_WIDTH / 2;
    x -= BM_STATUS_WIDTH / 2;
    drawalpha(BM_ONLINE + status, x, y, BM_STATUS_WIDTH, BM_STATUS_WIDTH, status_color[status]);

    if (notify) {
        y += BM_STATUS_WIDTH / 2;
        y -= BM_STATUS_NOTIFY_WIDTH / 2;
        x += BM_STATUS_WIDTH / 2;
        x -= BM_STATUS_NOTIFY_WIDTH / 2;
        drawalpha(BM_STATUS_NOTIFY, x, y, BM_STATUS_NOTIFY_WIDTH, BM_STATUS_NOTIFY_WIDTH, status_color[status]);
    }
}

static void drawitem(ITEM *i, int x, int y, int width) {
    flist_draw_itembox(i, x + SCALE(SCROLL_WIDTH), y, width);

    int box_height;
    int avatar_x;
    int avatar_y;
    int name_x;
    int name_y;
    int default_w;
    int group_bitmap;
    int contact_bitmap;


    if (settings.use_mini_flist) {
        box_height      = SCALE(ROSTER_BOX_HEIGHT / 2);
        avatar_x        = x + SCALE(SCROLL_WIDTH);
        avatar_y        = y + SCALE(ROSTER_AVATAR_TOP / 2);
        name_x          = avatar_x + BM_CONTACT_WIDTH / 2 + SCALE(5);
        name_y          = y + SCALE(ROSTER_NAME_TOP / 2);
        default_w       = BM_CONTACT_WIDTH / 2;
        group_bitmap    = BM_GROUP_MINI;
        contact_bitmap  = BM_CONTACT_MINI;
    } else {
        box_height      = SCALE(ROSTER_BOX_HEIGHT);
        avatar_x        = x + SCALE(SCROLL_WIDTH);
        avatar_y        = y + SCALE(ROSTER_AVATAR_TOP);
        name_x          = avatar_x + BM_CONTACT_WIDTH + SCALE(5);
        name_y          = y + SCALE(ROSTER_NAME_TOP);
        default_w       = BM_CONTACT_WIDTH;
        group_bitmap    = BM_GROUP;
        contact_bitmap  = BM_CONTACT;
    }

    switch (i->item) {
        case ITEM_FRIEND: {
            FRIEND *f = get_friend(i->id_number);
            uint8_t status = f->online ? f->status : 3;

            // draw avatar or default image
            if (friend_has_avatar(f)) {
                draw_avatar_image(f->avatar->img, avatar_x, avatar_y, f->avatar->width,
                                  f->avatar->height, default_w, default_w);
            } else {
                drawalpha(contact_bitmap, avatar_x, avatar_y, default_w, default_w,
                          (selected_item == i) ? COLOR_MAIN_TEXT : COLOR_LIST_TEXT);
            }

            flist_draw_name(i, name_x, name_y, width, UTOX_FRIEND_NAME(f), f->status_message, UTOX_FRIEND_NAME_LENGTH(f), f->status_length,
                            0, 0);

            flist_draw_status_icon(status, width - SCALE(15), y + box_height / 2, f->unread_msg);
            break;
        }

        case ITEM_GROUP: {
            GROUPCHAT *g = get_group(i->id_number);
            drawalpha(group_bitmap, avatar_x, avatar_y, default_w, default_w,
                      selected_item == i ? COLOR_MAIN_TEXT : COLOR_LIST_TEXT);

            bool color_overide = false;
            uint32_t color = 0;
            if (g->muted) {
                color_overide = true;
                color = COLOR_GROUP_MUTED;
            } else {
                uint64_t time = get_time();
                for (unsigned int j = 0; j < g->peer_count; ++j) {
                    if (time - g->last_recv_audio[j] <= (uint64_t)1 * 1000 * 1000 * 1000) {
                        color_overide = true;
                        color = COLOR_GROUP_AUDIO;
                        break;
                    }
                }
            }

            flist_draw_name(i, name_x, name_y, width, g->name, g->topic, g->name_length, g->topic_length, color_overide, color);

            flist_draw_status_icon(0, SCALE(width - 15), y + box_height / 2, g->unread_msg);
            break;
        }

        case ITEM_FREQUEST: {
            FREQUEST *r = get_frequest(i->id_number);
            if (!r) {
                LOG_WARN("FList", "Can't get the request at this number.");
                break;
            }

            char name[TOX_ADDRESS_SIZE * 2];
            id_to_string(name, r->bin_id);

            drawalpha(contact_bitmap, avatar_x, y + ROSTER_AVATAR_TOP, default_w, default_w,
                      (selected_item == i) ? COLOR_MAIN_TEXT : COLOR_LIST_TEXT);
            flist_draw_name(i, name_x, name_y, width, name, r->msg, sizeof(name), r->length, 0, 0);
            break;
        }

        case ITEM_GROUP_CREATE: {
            drawalpha(group_bitmap, avatar_x, y + ROSTER_AVATAR_TOP, default_w, default_w,
                      (selected_item == i) ? COLOR_MAIN_TEXT : COLOR_LIST_TEXT);
            flist_draw_name(i, name_x, name_y, width, S(CREATEGROUPCHAT), S(CURSOR_CLICK_RIGHT), SLEN(CREATEGROUPCHAT),
                            SLEN(CURSOR_CLICK_RIGHT), 1, (selected_item == i) ? COLOR_MAIN_TEXT : COLOR_LIST_TEXT);
            break;
        }

        case ITEM_GROUP_INVITE: {
            drawalpha(group_bitmap, avatar_x, y + ROSTER_AVATAR_TOP, default_w, default_w,
                      selected_item == i ? COLOR_MAIN_TEXT : COLOR_LIST_TEXT);

            char *title = "Groupchat invite";

            flist_draw_name(i, name_x, name_y, width, title, NULL,
                            strlen(title), 0, 0, 0);
            break;
        }

        default: {
            LOG_ERR("F-List", "Trying to draw an item that we shouldn't be drawing!");
            break;
        }
    }
}

// find index of given item in shown_list, or INT_MAX if it can't be found
static unsigned int find_item_shown_index(ITEM *it) {
    for (unsigned int i = 0; i < showncount; ++i) {
        if (shown_list[i] == it - item) { // (it - item) returns the index of the item in the full items list
            return i;
        }
    }
    return INT_MAX; // can't be found!
}

void flist_re_scale(void) {
    if (settings.use_mini_flist) {
        scrollbar_flist.content_height = SCALE(ROSTER_BOX_HEIGHT / 2) * showncount;
    } else {
        scrollbar_flist.content_height = SCALE(ROSTER_BOX_HEIGHT) * showncount;
    }
}

bool friend_matches_search_string(FRIEND *f, char *str) {
    return !str || strstr_case(f->name, str) || (f->alias && strstr_case(f->alias, str));
}

void flist_update_shown_list(void) {
    uint32_t j; // index in shown_list array
    for (uint32_t i = j = 0; i < itemcount; i++) {
        ITEM  *it = &item[i];
        if (it->item == ITEM_FRIEND) {
            FRIEND *f = get_friend(it->id_number);
            if ((!filter || f->online) && friend_matches_search_string(f, search_string)) {
                shown_list[j++] = i;
            }
        } else {
            shown_list[j++] = i;
        }
    }

    showncount = j;
    flist_re_scale();
}

static ITEM *newitem(void) {
    ITEM *i = &item[itemcount - 1];
    item[itemcount].item = ITEM_GROUP_CREATE;
    item[itemcount].id_number = UINT32_MAX;
    itemcount++;
    flist_update_shown_list();
    return i;
}

// return item that the user is mousing over
static ITEM *item_hit(int mx, int my, int UNUSED(height)) {
    int real_height = SCALE(ROSTER_BOX_HEIGHT);
    if (settings.use_mini_flist) {
        real_height /= 2;
    }

    /* Mouse is outside the list */
    if (mx < SCROLL_WIDTH || mx >= SCALE(230) || my < 0 // TODO magic numbers are bad 230 should be width
        || my >= (int)(showncount * real_height)) { /* TODO: Height is a bit buggy, Height needs /2
                                                     * figure out why!  */
        mouse_in_list = false;
        return NULL;
    } else {
        mouse_in_list = 1;
    }

    uint32_t item_idx = my / real_height;
    mouse_in_list = true;

    /* mouse is below the last item */
    if (item_idx >= showncount) {
        return NULL;
    }

    return &item[shown_list[item_idx]];
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
    unsigned int index = find_item_shown_index(selected_item);
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
            FRIEND *f = get_friend(i->id_number);

            current_width = f->msg.width;

            free(f->typed);
            f->typed_length = edit_chat_msg_friend.length;
            f->typed = calloc(1, f->typed_length);
            if (!f->typed) {
                LOG_ERR("flist", "Unable to calloc for f->typed.");
                return;
            }

            memcpy(f->typed, edit_chat_msg_friend.data, f->typed_length);

            f->msg.scroll = messages_friend.content_scroll->d;

            f->edit_history        = edit_chat_msg_friend.history;
            f->edit_history_cur    = edit_chat_msg_friend.history_cur;
            f->edit_history_length = edit_chat_msg_friend.history_length;

            panel_chat.disabled                    = true;
            panel_friend.disabled                  = true;
            panel_friend_chat.disabled             = true;
            panel_friend_video.disabled            = true;
            panel_friend_settings.disabled         = true;
            panel_friend_confirm_deletion.disabled = true;
            settings.inline_video                  = true;

            panel_friend_request.disabled          = true;
            break;
        }

        case ITEM_FREQUEST: {
            panel_chat.disabled           = true;
            panel_friend_request.disabled = true;
            break;
        }

        case ITEM_GROUP: {
            GROUPCHAT *g = get_group(selected_item->id_number);
            if (g) {
                current_width = g->msg.width;

                free(g->typed);
                g->typed_length = edit_chat_msg_group.length;
                g->typed = calloc(1, g->typed_length);
                if (!g->typed) {
                    LOG_ERR("F-List", "Unable to calloc for g->typed.");
                    return;
                }

                memcpy(g->typed, edit_chat_msg_group.data, g->typed_length);

                g->msg.scroll = messages_group.content_scroll->d;

                g->edit_history        = edit_chat_msg_group.history;
                g->edit_history_cur    = edit_chat_msg_group.history_cur;
                g->edit_history_length = edit_chat_msg_group.history_length;
            }

            panel_chat.disabled  = true;
            panel_group.disabled = true;

            break;
        }

        case ITEM_GROUP_INVITE: {
            panel_chat.disabled         = true;
            panel_group_invite.disabled = true;
            break;
        }

        case ITEM_SETTINGS: {
            if (panel_profile_password.disabled) {
                panel_splash_page.disabled = true;
                settings.show_splash = false;

                panel_settings_master.disabled = true;
                panel_overhead.disabled = true;

                panel_profile_password_settings.disabled = true;
                panel_nospam_settings.disabled = true;

                button_settings.disabled = false;
            }
            break;
        }

        case ITEM_ADD: {
            button_add_new_contact.disabled = false;
            panel_add_friend.disabled       = true;
            break;
        }

        case ITEM_GROUP_CREATE: {
            break;
        }

        case ITEM_NONE: {
            break;
        }
    }
}

static void page_open(ITEM *i) {
    switch (i->item) {
        case ITEM_FREQUEST: {
            panel_chat.disabled           = false;
            panel_friend_request.disabled = false;
            break;
        }

        case ITEM_FRIEND: {
            FRIEND *f = get_friend(i->id_number);
            if (!f) {
                LOG_ERR("Flist", "Could not get friend data from item");
                return;
            }

            #ifdef UNITY
            if (unity_running) {
                mm_rm_entry(f->cid);
            }
            #endif

            memcpy(edit_chat_msg_friend.data, f->typed, f->typed_length);
            edit_chat_msg_friend.length = f->typed_length;

            f->msg.width  = current_width;
            f->msg.id     = f->number;
            f->unread_msg = false;
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

            edit_chat_msg_friend.history        = f->edit_history;
            edit_chat_msg_friend.history_cur    = f->edit_history_cur;
            edit_chat_msg_friend.history_length = f->edit_history_length;
            edit_setfocus(&edit_chat_msg_friend);

            panel_chat.disabled            = 0;
            panel_friend.disabled          = 0;
            panel_friend_chat.disabled     = 0;
            panel_friend_video.disabled    = 1;
            panel_friend_settings.disabled = 1;
            break;
        }

        case ITEM_GROUP: {
            GROUPCHAT *g = get_group(i->id_number);
            if (!g) {
                LOG_FATAL_ERR(EXIT_FAILURE, "F-List", "Selected group no longer exists. Group number: %u", i->id_number);
            }

            memcpy(edit_chat_msg_group.data, g->typed, g->typed_length);
            edit_chat_msg_group.length = g->typed_length;

            g->msg.width  = current_width;
            g->msg.id     = g->number;
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
            edit_setfocus(&edit_chat_msg_group);

            edit_chat_msg_group.history        = g->edit_history;
            edit_chat_msg_group.history_cur    = g->edit_history_cur;
            edit_chat_msg_group.history_length = g->edit_history_length;

            panel_chat.disabled           = 0;
            panel_group.disabled          = 0;
            panel_group_chat.disabled     = 0;
            panel_group_video.disabled    = 1;
            panel_group_settings.disabled = 1;
            break;
        }

        case ITEM_GROUP_INVITE: {
            panel_chat.disabled         = false;
            panel_group_invite.disabled = false;
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
            edit_setfocus(&edit_add_new_friend_id);
            break;
        }

        case ITEM_GROUP_CREATE: {
            // postmessage_toxcore(TOX_GROUP_CREATE, 0, 0, NULL);
            break;
        }

        case ITEM_NONE: {
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
    button_settings.disabled = true;

    item_add.item      = ITEM_ADD;
    item_settings.item = ITEM_SETTINGS;

    ITEM *i = item;
    for (uint32_t num = 0; num < self.friend_list_count; ++num) {
        FRIEND *f    = get_friend(num);
        i->item      = ITEM_FRIEND;
        i->id_number = f->number;
        i++;
    }

    itemcount = i - item;

    newitem(); /* Called alone will create the group bar */

    search_string = NULL;
    flist_update_shown_list();
}

void flist_add_friend(FRIEND *f) {
    ITEM *i = newitem();
    i->item = ITEM_FRIEND;
    i->id_number = f->number;
}

void flist_add_friend_accepted(FRIEND *f, FREQUEST *req) {
    for (uint32_t i = 0; i < itemcount; ++i) {
        if (item[i].item == ITEM_FREQUEST && item[i].id_number == req->number) {
            LOG_INFO("FList", "Friend found and accepted.");
            item[i].item = ITEM_FRIEND;
            item[i].id_number = f->number;

            if (&item[i] == selected_item) {
                // panel_item[selected_item->item - 1].disabled = 1;
                // panel_item[ITEM_FRIEND - 1].disabled = 0;

                messages_friend.object                                = &f->msg;
                ((MESSAGES *)messages_friend.object)->cursor_over_msg = UINT32_MAX;
                messages_friend.content_scroll->content_height        = f->msg.height;
                messages_friend.content_scroll->d                     = f->msg.scroll;

                f->msg.id = f->number;
            }

            return;
        }
    }
}

void flist_add_group_request(uint8_t request_id) {
    ITEM *i = newitem();
    i->item = ITEM_GROUP_INVITE;
    i->id_number = request_id;
}

void flist_add_group(GROUPCHAT *g) {
    ITEM *i = newitem();
    i->item = ITEM_GROUP;
    i->id_number = g->number;
}

void flist_add_frequest(FREQUEST *r) {
    ITEM *i = newitem();
    i->item = ITEM_FREQUEST;
    i->id_number = r->number;
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
            FRIEND *f = get_friend(i->id_number);
            postmessage_toxcore(TOX_FRIEND_DELETE, f->number, 0, f);
            break;
        }

        case ITEM_GROUP: {
            GROUPCHAT *g = get_group(i->id_number);
            postmessage_toxcore(TOX_GROUP_PART, g->number, 0, NULL);
            group_free(g);
            break;
        }

        case ITEM_GROUP_INVITE: {
            break;
        }

        case ITEM_FREQUEST: {
            friend_request_free(i->id_number);
            break;
        }

        default: {
            return;
        }
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

void flist_delete_sitem(void) {
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
    for (ITEM *i = item; i != item + itemcount; i++) {
        switch (i->item) {
            case ITEM_FRIEND: {
                friend_free(get_friend(i->id_number));
                break;
            }

            case ITEM_GROUP: {
                group_free(get_group(i->id_number));
                break;
            }

            case ITEM_FREQUEST: {
                friend_request_free(i->id_number);
                break;
            }

            default: {
                break;
            }
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
            FRIEND *f     = get_friend(selected_item->id_number);
            if (!f) {
                LOG_ERR("Flist", "id_number is out of sync with friend_list"); // TODO should this be an exit code?
                                                                               // It's a critical error that could do
                                                                               // a lot of damage
                return;
            }
            memcpy(push_pop.data, &f->cid, TOX_PUBLIC_KEY_SIZE);
            break;
        }
        case ITEM_FREQUEST:
        case ITEM_GROUP:
        case ITEM_GROUP_INVITE:
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
            for (uint16_t i = 0; i < itemcount; ++i) {
                if (item[i].item == ITEM_FRIEND) {
                    FRIEND *f = get_friend(item[i].id_number);
                    if (memcmp(push_pop.data, &f->cid, TOX_PUBLIC_KEY_SIZE) == 0) {
                        show_page(&item[i]);
                        return;
                    }
                }
            }
            show_page(&item_settings);
            break;
        }

        case ITEM_FREQUEST:
        case ITEM_GROUP:
        case ITEM_GROUP_INVITE:
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

FRIEND *flist_get_friend(void) {
    if (flist_get_type() == ITEM_FRIEND) {
        return get_friend(selected_item->id_number);
    }
    return NULL;
}

FREQUEST *flist_get_frequest(void) {
    if (flist_get_type() == ITEM_FREQUEST) {
        return get_frequest(selected_item->id_number);
    }

    return NULL;
}

GROUPCHAT *flist_get_groupchat(void) {
    if (flist_get_type() == ITEM_GROUP) {
        return get_group(selected_item->id_number);
    }

    return NULL;
}

uint8_t flist_get_group_invite_id(void) {
    if (flist_get_type() == ITEM_GROUP_INVITE) {
        return selected_item->id_number;
    }

    return UINT8_MAX;
}

ITEM_TYPE flist_get_type(void) {
    return selected_item->item;
}

/**
 * @brief Extract string ToxId from Tox URI.
 *
 * @param str Null-terminated Tox URI.
 * @param tox_id Extracted ToxId; it has to be at least TOX_ADDRESS_SIZE * 2 + 1.
 *
 * @return True if success false otherwise.
 */
static bool get_tox_id_from_uri(const char *str, char *tox_id) {
    const char *tox_uri_scheme = "tox:";
    const int tox_uri_scheme_length = 4;

    if (strncmp(str, tox_uri_scheme, tox_uri_scheme_length) == 0 &&
        strlen(str) - tox_uri_scheme_length == TOX_ADDRESS_SIZE * 2) {
        memcpy(tox_id, &str[tox_uri_scheme_length], TOX_ADDRESS_SIZE * 2);
        tox_id[TOX_ADDRESS_SIZE * 2] = '\0';
        return true;
    }

    return false;
}

bool try_open_tox_uri(const char *str) {
    char tox_id[TOX_ADDRESS_SIZE * 2 + 1];

    if (!get_tox_id_from_uri(str, tox_id)) {
        return false;
    }

    FRIEND *friend = get_friend_by_id(tox_id);

    if (friend) {
        flist_selectchat(friend->number);
    } else if (tox_thread_init == UTOX_TOX_THREAD_INIT_SUCCESS) {
        edit_setstr(&edit_add_new_friend_id, tox_id, TOX_ADDRESS_SIZE * 2);
        edit_setstr(&edit_search, (char *)"", 0);
        flist_selectaddfriend();
        edit_setfocus(&edit_add_new_friend_msg);
    }

    return true;
}

/******************************************************************************
 ****** UI functions                                                     ******
 ******************************************************************************/

void flist_draw(void *UNUSED(n), int x, int y, int width, int UNUSED(height)) {
    int real_height = 0;
    if (settings.use_mini_flist) {
        real_height = SCALE(ROSTER_BOX_HEIGHT / 2);
    } else {
        real_height = SCALE(ROSTER_BOX_HEIGHT);
    }

    ITEM *mi = NULL; // item being dragged
    int   my;        // y of item being dragged

    for (unsigned int i = 0; i < showncount; i++) {
        ITEM *it = &item[shown_list[i]];
        if (it == selected_item && (selected_item_dy >= 5 || selected_item_dy <= -5)) {
            mi = it;
            my = y + selected_item_dy;
        } else {
            drawitem(it, x, y, width);
        }
        y += real_height;
    }

    if (mi) {
        drawitem(mi, x, my, width);
    }
}

bool flist_mmove(void *UNUSED(n), int UNUSED(x), int UNUSED(y), int UNUSED(width), int height, int mx, int my,
                 int UNUSED(dx), int dy)
{
    int real_height = 0;

    if (settings.use_mini_flist) {
        real_height = ROSTER_BOX_HEIGHT / 2;
    } else {
        real_height = ROSTER_BOX_HEIGHT;
    }

    ITEM *i = item_hit(mx, my, height);

    bool draw = false;

    if (i != mouseover_item) {
        mouseover_item = i;
        draw = true;
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
            unsigned int index = find_item_shown_index(selected_item);
            if (index != INT_MAX) { // selected_item was found in shown list
                index += d;         // get item being dragged over

                // set item being dragged over
                if (index < itemcount) {
                    nitem = &item[shown_list[index]];
                }
            }
        }

        draw = true;
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
    FRIEND *f = get_friend(right_mouse_item->id_number);

    panel_friend_chat.disabled     = true;
    panel_friend_video.disabled    = true;
    panel_friend_settings.disabled = false;

    edit_setstr(&edit_friend_pubkey, (char *)&f->id_str, TOX_PUBLIC_KEY_SIZE * 2);

    maybe_i18nal_string_set_plain(&edit_friend_alias.empty_str, f->name, f->name_length);
    edit_setstr(&edit_friend_alias, f->alias, f->alias_length);

    switch_friend_autoaccept_ft.switch_on = f->ft_autoaccept;
}

static void flist_init_group_settings_page(void) {
    GROUPCHAT *g = get_group(right_mouse_item->id_number);

    panel_group_chat.disabled     = true;
    panel_group_video.disabled    = true;
    panel_group_settings.disabled = false;

    edit_setstr(&edit_group_topic, g->name, g->name_length);

    dropdown_notify_groupchats.over = dropdown_notify_groupchats.selected = g->notify;
}

typedef enum {
    SHOW_SETTINGS,
    SHOW_INLINE_VID,
    CLEAR_HISTOR,
    DELETE_FRIEND,
} FLIST_CONTEXT_MENU;

static void contextmenu_friend(FLIST_CONTEXT_MENU rcase) {
    FRIEND *f = get_friend(right_mouse_item->id_number);

    panel_friend_chat.disabled     = false;
    panel_friend_video.disabled    = true;
    panel_friend_settings.disabled = true;
    switch (rcase) {
        case SHOW_SETTINGS: {
            /* should be settings page */
            flist_init_friend_settings_page();
            break;
        }
        case SHOW_INLINE_VID: {
            /* Should be show inline video */
            panel_friend_chat.disabled     = true;
            panel_friend_video.disabled    = false;
            panel_friend_settings.disabled = true;
            settings.inline_video          = true;
            f->video_inline                = true;
            postmessage_utox(AV_CLOSE_WINDOW, f->number + 1, 0, NULL);
            break;
        }
        case CLEAR_HISTOR: {
            /* should be clean history */
            friend_history_clear(get_friend(right_mouse_item->id_number));
            break;
        }
        case DELETE_FRIEND: {
            /* Should be: delete friend */
            panel_friend_chat.disabled             = true;
            panel_friend_confirm_deletion.disabled = false;
            break;
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
                panel_group_chat.disabled = false;
                GROUPCHAT *g = get_group(right_mouse_item->id_number);
                if (i == 0) {
                    flist_init_group_settings_page();
                } else if (i == 1) {
                    if (right_mouse_item != selected_item) {
                        show_page(right_mouse_item);
                    }

                    char str[g->name_length + 7];
                    strcpy(str, "/topic ");
                    memcpy(str + 7, g->name, g->name_length);
                    edit_setfocus(&edit_chat_msg_group);
                    edit_paste(str, sizeof(str), 0);
                } else if (i == 2 && g->av_group) {
                    g->muted = !g->muted;
                } else {
                    flist_delete_rmouse_item();
                }

                return;
            }
            case ITEM_FREQUEST: {
                if (i == 0) {
                    FREQUEST *req = get_frequest(right_mouse_item->id_number);
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
                return;
            }

            default: {
                LOG_TRACE("F-List", "blerg" );
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
    static UTOX_I18N_STR menu_friend[] = {
                STR_FRIEND_SETTINGS,
                STR_CALL_VIDEO_SHOW_INLINE,
                STR_CLEAR_HISTORY,
                STR_REMOVE_FRIEND
            };

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
                GROUPCHAT *g = get_group(mouseover_item->id_number);
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

            case ITEM_FREQUEST: {
                contextmenu_new(COUNTOF(menu_request), menu_request, contextmenu_list_onselect);
                break;
            }

            default: {
                LOG_ERR("F-List", "MRIGHT on a flist entry that shouldn't exist!");
                break;
            }
        }

        return true;
    } else if (mouse_in_list) {
        right_mouse_item = NULL; /* Unset right_mouse_item so that we don't interact with the incorrect context menu
                                  * I'm not sure if this belongs here or in flist_mmove, or maybe item_hit. */
    }

    return false;
}

bool flist_mwheel(void *UNUSED(n), int UNUSED(height), double UNUSED(d), bool UNUSED(smooth)) {
    return false;
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
                    ITEM temp      = *selected_item;
                    *selected_item = *nitem;
                    *nitem         = temp;

                    selected_item = nitem;
                }

                if (nitem->item == ITEM_GROUP) {
                    FRIEND *f = get_friend(selected_item->id_number);
                    GROUPCHAT *g = get_group(nitem->id_number);

                    if (f->online) {
                        size_t msg_length = UTOX_FRIEND_NAME_LENGTH(f) + SLEN(GROUP_MESSAGE_INVITE);

                        uint8_t *msg = calloc(msg_length, sizeof(uint8_t));
                        msg_length = snprintf((char *)msg, msg_length, S(GROUP_MESSAGE_INVITE), UTOX_FRIEND_NAME(f));

                        group_add_message(g, 0, msg, msg_length, MSG_TYPE_NOTICE);
                        postmessage_toxcore(TOX_GROUP_SEND_INVITE, g->number, f->number, NULL);
                    }
                }
            }

            if (selected_item->item == ITEM_GROUP) {
                if (nitem->item == ITEM_FRIEND || nitem->item == ITEM_GROUP) {
                    ITEM temp      = *selected_item;
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
        return true;
    }

    return false;
}
