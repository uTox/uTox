#include "side_bar.h"

#include "settings.h"

#include "../avatar.h"
#include "../flist.h"
#include "../macros.h"
#include "../main_native.h"
#include "../self.h"
#include "../theme.h"

#include "../ui.h"
#include "../ui/draw.h"
#include "../ui/svg.h"
#include "../ui/scrollable.h"

#include "../main.h" // tox_thread global

// Scrollbar or friend list
SCROLLABLE scrollbar_flist = {
    .panel = { .type = PANEL_SCROLLABLE, },
    .color = C_SCROLL,
    .x     = 2,
    .left  = 1,
    .small = 1,
};

/* Top left self interface Avatar, name, statusmsg, status icon */
static void draw_user_badge(int UNUSED(x), int UNUSED(y), int UNUSED(width), int UNUSED(height)) {
    if (tox_thread_init == UTOX_TOX_THREAD_INIT_SUCCESS) {
        /* Only draw the user badge if toxcore is running */
        /*draw avatar or default image */
        if (self_has_avatar()) {
            draw_avatar_image(self.avatar->img, SIDEBAR_AVATAR_LEFT, SIDEBAR_AVATAR_TOP, self.avatar->width,
                              self.avatar->height, BM_CONTACT_WIDTH, BM_CONTACT_WIDTH);
        } else {
            drawalpha(BM_CONTACT, SIDEBAR_AVATAR_LEFT, SIDEBAR_AVATAR_TOP, BM_CONTACT_WIDTH, BM_CONTACT_WIDTH,
                      COLOR_MENU_TEXT);
        }
        /* Draw name */
        setcolor(!button_name.mouseover ? COLOR_MENU_TEXT : COLOR_MENU_TEXT_SUBTEXT);
        setfont(FONT_SELF_NAME);
        drawtextrange(SIDEBAR_NAME_LEFT, SIDEBAR_NAME_WIDTH * 1.5, SIDEBAR_NAME_TOP, self.name, self.name_length);

        /*&Draw current status message
        @TODO: separate these colors if needed (COLOR_MAIN_TEXT_HINT) */
        setcolor(!button_status_msg.mouseover ? COLOR_MENU_TEXT_SUBTEXT : COLOR_MAIN_TEXT_HINT);
        setfont(FONT_STATUS);
        drawtextrange(SIDEBAR_STATUSMSG_LEFT, SIDEBAR_STATUSMSG_WIDTH * 1.5, SIDEBAR_STATUSMSG_TOP, self.statusmsg,
                      self.statusmsg_length);

        /* Draw status button icon */
        drawalpha(BM_STATUSAREA, SELF_STATUS_ICON_LEFT, SELF_STATUS_ICON_TOP, BM_STATUSAREA_WIDTH, BM_STATUSAREA_HEIGHT,
                  button_usr_state.mouseover ? COLOR_BKGRND_LIST_HOVER : COLOR_BKGRND_LIST);
        uint8_t status = tox_connected ? self.status : 3;
        drawalpha(BM_ONLINE + status, SELF_STATUS_ICON_LEFT + BM_STATUSAREA_WIDTH / 2 - BM_STATUS_WIDTH / 2,
                  SELF_STATUS_ICON_TOP + BM_STATUSAREA_HEIGHT / 2 - BM_STATUS_WIDTH / 2, BM_STATUS_WIDTH,
                  BM_STATUS_WIDTH, status_color[status]);

        /* Draw online/all friends filter text. */
        setcolor(!button_filter_friends.mouseover ? COLOR_MENU_TEXT_SUBTEXT : COLOR_MAIN_TEXT_HINT);
        setfont(FONT_STATUS);
        drawtextrange(SIDEBAR_FILTER_FRIENDS_LEFT, SIDEBAR_FILTER_FRIENDS_WIDTH, SIDEBAR_FILTER_FRIENDS_TOP,
                      flist_get_filter() ? S(FILTER_ONLINE) : S(FILTER_ALL),
                      flist_get_filter() ? SLEN(FILTER_ONLINE) : SLEN(FILTER_ALL));
    } else {
        drawalpha(BM_CONTACT, SIDEBAR_AVATAR_LEFT, SIDEBAR_AVATAR_TOP, BM_CONTACT_WIDTH, BM_CONTACT_WIDTH,
                  COLOR_MENU_TEXT);

        setcolor(!button_name.mouseover ? COLOR_MENU_TEXT : COLOR_MENU_TEXT_SUBTEXT);
        setfont(FONT_SELF_NAME);
        drawtextrange(SIDEBAR_NAME_LEFT, SIDEBAR_WIDTH - SIDEBAR_AVATAR_LEFT, SIDEBAR_NAME_TOP, S(NOT_CONNECTED), SLEN(NOT_CONNECTED));

        if (tox_thread_init == UTOX_TOX_THREAD_INIT_ERROR) {
            setcolor(!button_status_msg.mouseover ? COLOR_MENU_TEXT_SUBTEXT : COLOR_MAIN_TEXT_HINT);
            setfont(FONT_STATUS);
            drawtextrange(SIDEBAR_STATUSMSG_LEFT, SIDEBAR_WIDTH, SIDEBAR_STATUSMSG_TOP, S(NOT_CONNECTED_SETTINGS), SLEN(NOT_CONNECTED_SETTINGS));
        }
    }
}

#include "../ui/edits.h"
/* Left side bar, holds the user, the roster, and the setting buttons */
PANEL panel_side_bar = {
    .type = PANEL_NONE,
    .disabled = 0,
    .child = (PANEL*[]) {
        &panel_self,
        &panel_quick_buttons,
        &panel_flist,
        NULL
    }
},
    /* The user badge and buttons */
    panel_self = {
        .type     = PANEL_NONE,
        .disabled = 0,
        .drawfunc = draw_user_badge,
        .child    = (PANEL*[]) {
            (PANEL*)&button_avatar, (PANEL*)&button_name,       (PANEL*)&button_usr_state,
                                    (PANEL*)&button_status_msg,
            NULL
        }
    },
    /* Left sided toggles */
    panel_quick_buttons = {
        .type     = PANEL_NONE,
        .disabled = 0,
        .child    = (PANEL*[]) {
            (PANEL*)&button_filter_friends, /* Top of roster */

            (PANEL*)&edit_search,           /* Bottom of roster*/
            (PANEL*)&button_settings,
            (PANEL*)&button_add_new_contact,
            NULL
        }
    },
    /* The friends and group was called list */
    panel_flist = {
        .type     = PANEL_NONE,
        .disabled = 0,
        .child    = (PANEL*[]) {
            // TODO rename these
            &panel_flist_list,
            (PANEL*)&scrollbar_flist,
            NULL
        }
    },
        panel_flist_list = {
            .type           = PANEL_LIST,
            .content_scroll = &scrollbar_flist,
        };


