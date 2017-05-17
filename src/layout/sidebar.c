#include "sidebar.h"

#include "settings.h"
#include "friend.h"

#include "../avatar.h"
#include "../flist.h"
#include "../macros.h"
#include "../self.h"
#include "../theme.h"
#include "../tox.h"

#include "../native/ui.h"

#include "../ui.h"
#include "../ui/draw.h"
#include "../ui/scrollable.h"
#include "../ui/edit.h"
#include "../ui/button.h"
#include "../ui/svg.h"

#include <string.h>

// Scrollbar or friend list
SCROLLABLE scrollbar_flist = {
    .panel = { .type = PANEL_SCROLLABLE, },
    .color = C_SCROLL,
    .x     = 2,
    .left  = 1,
    .small = 1,
};

static void draw_background_sidebar(int x, int y, int width, int height) {
    /* Friend list (roster) background   */
    drawrect(x, y, width, height, COLOR_BKGRND_LIST);
    /* Current user badge background     */
    drawrect(x, y, width, SCALE(70), COLOR_BKGRND_MENU); // TODO magic numbers are bad
}

/* Top left self interface Avatar, name, statusmsg, status icon */
static void draw_user_badge(int x, int y, int width, int UNUSED(height)) {
    if (tox_thread_init == UTOX_TOX_THREAD_INIT_SUCCESS) {
        /* Only draw the user badge if toxcore is running */
        /*draw avatar or default image */
        x += SCALE(SIDEBAR_PADDING);
        y += SCALE(SIDEBAR_PADDING);
        if (self_has_avatar()) {
            draw_avatar_image(self.avatar->img, x, y, self.avatar->width,
                              self.avatar->height, BM_CONTACT_WIDTH, BM_CONTACT_WIDTH);
        } else {
            drawalpha(BM_CONTACT, x, y, BM_CONTACT_WIDTH, BM_CONTACT_WIDTH,
                      COLOR_MENU_TEXT);
        }

        /* Draw online/all friends filter text. */
        setcolor(!button_filter_friends.mouseover ? COLOR_MENU_TEXT_SUBTEXT : COLOR_MAIN_TEXT_HINT);
        setfont(FONT_STATUS);
        drawtextrange(x, width - SCALE(SIDEBAR_PADDING),
                      y + SCALE(SIDEBAR_PADDING) + BM_CONTACT_WIDTH,
                      flist_get_filter() ? S(FILTER_ONLINE) : S(FILTER_ALL),
                      flist_get_filter() ? SLEN(FILTER_ONLINE) : SLEN(FILTER_ALL));

        x += SCALE(SIDEBAR_PADDING) + BM_CONTACT_WIDTH;

        /* Draw name */
        setcolor(!button_name.mouseover ? COLOR_MENU_TEXT : COLOR_MENU_TEXT_SUBTEXT);
        setfont(FONT_SELF_NAME);
        drawtextrange(x, width - SCALE(SIDEBAR_PADDING * 5), y + SCALE(SIDEBAR_PADDING), self.name, self.name_length);

        /*&Draw current status message
        @TODO: separate these colors if needed (COLOR_MAIN_TEXT_HINT) */
        setcolor(!button_status_msg.mouseover ? COLOR_MENU_TEXT_SUBTEXT : COLOR_MAIN_TEXT_HINT);
        setfont(FONT_STATUS);
        drawtextrange(x, width - SCALE(SIDEBAR_PADDING * 5), y + SCALE(SIDEBAR_PADDING * 4), self.statusmsg,
                      self.statusmsg_length);

        /* Draw status button icon */
        drawalpha(BM_STATUSAREA,
                  width - BM_STATUSAREA_WIDTH - SCALE(SIDEBAR_PADDING),
                  y + SCALE(SIDEBAR_PADDING),
                  BM_STATUSAREA_WIDTH, BM_STATUSAREA_HEIGHT,
                  button_usr_state.mouseover ? COLOR_BKGRND_LIST_HOVER : COLOR_BKGRND_LIST);
        uint8_t status = tox_connected ? self.status : 3;
        drawalpha(BM_ONLINE + status,
                  width - BM_STATUS_WIDTH - BM_STATUSAREA_WIDTH / 2 - SCALE(1),
                  y + SCALE(SIDEBAR_PADDING) + BM_STATUSAREA_HEIGHT / 2 - BM_STATUS_WIDTH / 2,
                  BM_STATUS_WIDTH, BM_STATUS_WIDTH, status_color[status]);

    } else {
        drawalpha(BM_CONTACT, SCALE(SIDEBAR_PADDING), SIDEBAR_AVATAR_TOP, BM_CONTACT_WIDTH, BM_CONTACT_WIDTH,
                  COLOR_MENU_TEXT);

        setcolor(!button_name.mouseover ? COLOR_MENU_TEXT : COLOR_MENU_TEXT_SUBTEXT);
        setfont(FONT_SELF_NAME);
        drawtextrange(SCALE(SIDEBAR_NAME_LEFT), SCALE(230) - SCALE(SIDEBAR_PADDING), SIDEBAR_NAME_TOP, S(NOT_CONNECTED), SLEN(NOT_CONNECTED)); // TODO rm magic number

        if (tox_thread_init == UTOX_TOX_THREAD_INIT_ERROR) {
            setcolor(!button_status_msg.mouseover ? COLOR_MENU_TEXT_SUBTEXT : COLOR_MAIN_TEXT_HINT);
            setfont(FONT_STATUS);
            drawtextrange(SIDEBAR_STATUSMSG_LEFT, SCALE(230), SIDEBAR_STATUSMSG_TOP, S(NOT_CONNECTED_SETTINGS), SLEN(NOT_CONNECTED_SETTINGS)); // TODO rm magic number
        }
    }
}

/* Left side bar, holds the user, the roster, and the setting buttons */
PANEL

panel_side_bar = {
    .type = PANEL_NONE,
    .disabled = 0,
    .drawfunc = draw_background_sidebar,
    .width = 230,
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
    .x      = 0,
    .y      = 70,
    .width  = 230,
    .height = ROSTER_BOTTOM,

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


#include "../friend.h"
static void e_search_onchange(EDIT *edit) {
    char *data = edit->data;
    uint16_t length = edit->length > sizeof search_data ? sizeof search_data - 1 : edit->length;

    if (length) {
        button_add_new_contact.panel.disabled = 0;
        button_add_new_contact.nodraw         = 0;
        button_settings.panel.disabled        = 1;
        button_settings.nodraw                = 1;
        memcpy(search_data, data, length);
        search_data[length] = 0;
        flist_search((char *)search_data);
    } else {
        button_add_new_contact.panel.disabled = 1;
        button_add_new_contact.nodraw         = 1;
        button_settings.panel.disabled        = 0;
        button_settings.nodraw                = 0;
        flist_search(NULL);
    }

    redraw();
}

static void e_search_onenter(EDIT *edit) {
    char *   data   = edit->data;
    uint16_t length = edit->length;

    if (length == 76) { // FIXME, this should be error checked!
                        // No, srsly... this is lucky, not right.
        friend_add(data, length, (char *)"", 0);
        edit_setstr(&edit_search, (char *)"", 0);
    } else {
        if (tox_thread_init == UTOX_TOX_THREAD_INIT_SUCCESS) {
            /* Only change if we're logged in! */
            edit_setstr(&edit_add_new_friend_id, data, length);
            edit_setstr(&edit_search, (char *)"", 0);
            flist_selectaddfriend();
            edit_setfocus(&edit_add_new_friend_msg);
        }
    }
    return;
}

static char e_search_data[1024];
EDIT edit_search = {
    .data      = e_search_data,
    .maxlength = sizeof e_search_data - 1,
    .onchange  = e_search_onchange,
    .onenter   = e_search_onenter,
    .style     = AUXILIARY_STYLE,
    .vcentered = true,
    .empty_str = { .i18nal = STR_CONTACT_SEARCH_ADD_HINT },
};
