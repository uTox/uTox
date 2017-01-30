// TODO, whole file should go away
#include "draw_helpers.h"

#include "buttons.h"
#include "draw.h"
#include "svg.h"
#include "text.h"

#include "../dns.h"
#include "../flist.h"
#include "../friend.h"
#include "../groups.h"
#include "../macros.h"
#include "../main_native.h"
#include "../self.h"
#include "../settings.h"
#include "../theme.h"

#include "../layout/tree.h"

#include "../main.h"


void draw_notification(int x, int y, int w, int h) {
    if (!tox_thread_init) {
        return;
    }

    drawrect(x, y, w, h, COLOR_BKGRND_MAIN);

    if (self_has_avatar()) {
        draw_avatar_image(self.avatar->img, SIDEBAR_AVATAR_LEFT, SIDEBAR_AVATAR_TOP, self.avatar->width,
                          self.avatar->height, BM_CONTACT_WIDTH, BM_CONTACT_WIDTH);
    } else {
        drawalpha(BM_CONTACT, SIDEBAR_AVATAR_LEFT, SIDEBAR_AVATAR_TOP, BM_CONTACT_WIDTH, BM_CONTACT_WIDTH,
                  COLOR_MENU_TEXT);
    }

    setcolor(!button_name.mouseover ? COLOR_MENU_TEXT : COLOR_MENU_TEXT_SUBTEXT);
    setfont(FONT_SELF_NAME);
    drawtext(SIDEBAR_NAME_LEFT, SIDEBAR_NAME_TOP, "This is a test of the new uTox popup", 36);

}

void draw_avatar_image(NATIVE_IMAGE *image, int x, int y, uint32_t width, uint32_t height, uint32_t targetwidth,
                       uint32_t targetheight)
{
    /* get smallest of width or height */
    double scale = (width > height) ? (double)targetheight / height : (double)targetwidth / width;

    image_set_scale(image, scale);
    image_set_filter(image, FILTER_BILINEAR);

    /* set position to show the middle of the image in the center  */
    int xpos = (int)((double)width * scale / 2 - (double)targetwidth / 2);
    int ypos = (int)((double)height * scale / 2 - (double)targetheight / 2);

    draw_image(image, x, y, targetwidth, targetheight, xpos, ypos);

    image_set_scale(image, 1.0);
    image_set_filter(image, FILTER_NEAREST);
}

/* Top left self interface Avatar, name, statusmsg, status icon */
void draw_user_badge(int UNUSED(x), int UNUSED(y), int UNUSED(width), int UNUSED(height)) {
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

void draw_splash_page(int x, int y, int w, int h) {
    setcolor(COLOR_MAIN_TEXT);

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
    ny = utox_draw_text_multiline_within_box(x, ny, w + x, ny, ny + h, font_small_lineheight, S(CHANGE_LOG_TEXT),
                                             SLEN(CHANGE_LOG_TEXT), ~0, ~0, 0, 0, 1);
}

#include "../layout/settings.h" // TODO move this
void draw_background(int UNUSED(x), int UNUSED(y), int width, int height) {
    /* Default background                */
    drawrect(0, 0, width, height, COLOR_BKGRND_MAIN);
    /* Friend list (roster) background   */
    drawrect(0, 0, SIDEBAR_WIDTH, height, COLOR_BKGRND_LIST);
    /* Current user badge background     */
    drawrect(0, 0, SIDEBAR_WIDTH, ROSTER_TOP, COLOR_BKGRND_MENU);

    if (!panel_chat.disabled) {
        /* Top frame for main chat panel */
        drawrect(MAIN_LEFT, 0, width, MAIN_TOP_FRAME_THICK, COLOR_BKGRND_ALT);
        drawhline(MAIN_LEFT, MAIN_TOP_FRAME_THICK - 1, width, COLOR_EDGE_NORMAL);
        /* Frame for the bottom chat text entry box */
        drawrect(MAIN_LEFT, height + CHAT_BOX_TOP, width, height, COLOR_BKGRND_ALT);
        drawhline(MAIN_LEFT, height + CHAT_BOX_TOP, width, COLOR_EDGE_NORMAL);
    }
    // Chat and chat header separation
    if (panel_settings_master.disabled) {
        drawhline(MAIN_LEFT, MAIN_TOP_FRAME_THICK - 1, width, COLOR_EDGE_NORMAL);
    } else {
        drawhline(MAIN_LEFT, MAIN_TOP_FRAME_THIN - 1, width, COLOR_EDGE_NORMAL);
    }
}

/* These remain for legacy reasons, PANEL_MAIN calls these by default when not given it's own function to call */
void background_draw(PANEL *UNUSED(p), int UNUSED(x), int UNUSED(y), int UNUSED(width), int UNUSED(height)) {
    return;
}
bool background_mmove(PANEL *UNUSED(p), int UNUSED(x), int UNUSED(y), int UNUSED(width), int UNUSED(height),
                      int UNUSED(mx), int UNUSED(my), int UNUSED(dx), int UNUSED(dy)) {
    return 0;
}
bool background_mdown(PANEL *UNUSED(p)) {
    return 0;
}
bool background_mright(PANEL *UNUSED(p)) {
    return 0;
}
bool background_mwheel(PANEL *UNUSED(p), int UNUSED(height), double UNUSED(d), bool UNUSED(smooth)) {
    return 0;
}
bool background_mup(PANEL *UNUSED(p)) {
    return 0;
}
bool background_mleave(PANEL *UNUSED(p)) {
    return 0;
}
