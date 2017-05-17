#include "background.h"

#include "friend.h"
#include "group.h"
#include "notify.h"
#include "settings.h"
#include "sidebar.h"

#include "../macros.h"
#include "../theme.h"

#include "../ui.h"

#include "../ui/draw.h"
#include "../ui/panel.h"
#include "../ui/text.h"

#include <stddef.h>

static void draw_background(int x, int y, int width, int height) {
    /* Default background                */
    drawrect(x, y, width, height, COLOR_BKGRND_MAIN);

    if (!panel_chat.disabled) {
        /* Top frame for main chat panel */
        drawrect(x, 0, width, SCALE(MAIN_TOP_FRAME_THICK), COLOR_BKGRND_ALT);
        drawhline(x, SCALE(MAIN_TOP_FRAME_THICK), width, COLOR_EDGE_NORMAL);
        /* Frame for the bottom chat text entry box */
        drawrect(x, height + SCALE(CHAT_BOX_TOP), width, height, COLOR_BKGRND_ALT);
        drawhline(x, height + SCALE(CHAT_BOX_TOP), width, COLOR_EDGE_NORMAL);
    }
    // Chat and chat header separation
    if (panel_settings_master.disabled) {
        drawhline(x, SCALE(MAIN_TOP_FRAME_THICK), width, COLOR_EDGE_NORMAL);
    }
}

static void draw_splash_page(int x, int y, int w, int h) {
    setcolor(COLOR_MAIN_TEXT);

    y += SCALE(10);

    x += SCALE(10);

    /* Generic Splash */
    setfont(FONT_SELF_NAME);
    int ny = utox_draw_text_multiline_within_box(x, y, w + x, y, y + h, font_small_lineheight, S(SPLASH_TITLE),
                                                 SLEN(SPLASH_TITLE), ~0, ~0, 0, 0, 1);
    setfont(FONT_TEXT);
    ny = utox_draw_text_multiline_within_box(x, ny, w + x, ny, ny + h, font_small_lineheight, S(SPLASH_TEXT),
                                             SLEN(SPLASH_TEXT), ~0, ~0, 0, 0, 1);

    ny += SCALE(30);
    /* Change log */
    setfont(FONT_SELF_NAME);
    ny = utox_draw_text_multiline_within_box(x, ny, w + x, y, ny + h, font_small_lineheight, S(CHANGE_LOG_TITLE),
                                             SLEN(CHANGE_LOG_TITLE), ~0, ~0, 0, 0, 1);
    setfont(FONT_TEXT);
    /* ny =*/ utox_draw_text_multiline_within_box(x, ny, w + x, ny, ny + h, font_small_lineheight, S(CHANGE_LOG_TEXT),
                                                  SLEN(CHANGE_LOG_TEXT), ~0, ~0, 0, 0, 1);
}

PANEL
panel_root = {
    .type = PANEL_NONE,
    .drawfunc = draw_background,
    .disabled = 0,
    .child = (PANEL*[]) {
        &panel_main,
        &panel_side_bar,
        NULL
    }
},

/* Main panel, holds the overhead/settings, or the friend/group containers */
panel_main = {
    .type = PANEL_NONE,
    .disabled = 0,
    .child = (PANEL*[]) {
        &panel_chat,
        &panel_overhead,
        NULL
    }
},

/* Chat panel, friend or group, depending on what's selected */
panel_chat = {
    .type = PANEL_NONE,
    .disabled = 1,
    .child = (PANEL*[]) {
        &panel_group,
        &panel_friend,
        &panel_friend_request,
        NULL
    }
},

/* Settings master panel, holds the lower level settings */
panel_overhead = {
    .type = PANEL_NONE,
    .disabled = 0,
    .child = (PANEL*[]) {
        &panel_splash_page,
        &panel_profile_password,
        &panel_add_friend,
        &panel_settings_master,
        // (PANEL*)&button_notify_create, // FIXME, left as a comment for later work on popup notifications
        NULL
    }
},

panel_splash_page = {
    .type = PANEL_NONE,
    .disabled = 1,
    .drawfunc = draw_splash_page,
    .content_scroll = &scrollbar_settings,
    .child = (PANEL*[]) {
        NULL,
    }
};
