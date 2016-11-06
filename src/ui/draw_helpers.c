// draw_helpers.c

#include "draw_helpers.h"

#include "buttons.h"
#include "text.h"

#include "../flist.h"
#include "../friend.h"
#include "../theme.h"

void draw_avatar_image(NATIVE_IMAGE *image, int x, int y, uint32_t width, uint32_t height, uint32_t targetwidth,
                       uint32_t targetheight) {
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
    if (tox_thread_init) {
        /* Only draw the user badge if toxcore is running */
        /*draw avatar or default image */
        if (self_has_avatar()) {
            draw_avatar_image(self.avatar.image, SIDEBAR_AVATAR_LEFT, SIDEBAR_AVATAR_TOP, self.avatar.width,
                              self.avatar.height, BM_CONTACT_WIDTH, BM_CONTACT_WIDTH);
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
        drawtextrange(SIDEBAR_NAME_LEFT, SIDEBAR_NAME_WIDTH, SIDEBAR_NAME_TOP, S(NOT_CONNECTED), SLEN(NOT_CONNECTED));
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

/* Header for friend chat window */
void draw_friend(int x, int y, int w, int height) {
    FRIEND *f = (flist_get_selected()->data);

    // draw avatar or default image
    if (friend_has_avatar(f)) {
        draw_avatar_image(f->avatar.image, MAIN_LEFT + SCALE(10), SCALE(10), f->avatar.width, f->avatar.height,
                          BM_CONTACT_WIDTH, BM_CONTACT_WIDTH);
    } else {
        drawalpha(BM_CONTACT, MAIN_LEFT + SCALE(10), SCALE(10), BM_CONTACT_WIDTH, BM_CONTACT_WIDTH, COLOR_MAIN_TEXT);
    }

    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_TITLE);

    if (f->alias) {
        drawtextrange(MAIN_LEFT + SCALE(60), settings.window_width - SCALE(128), SCALE(18), f->alias, f->alias_length);
    } else {
        drawtextrange(MAIN_LEFT + SCALE(60), settings.window_width - SCALE(128), SCALE(18), f->name, f->name_length);
    }

    setcolor(COLOR_MAIN_TEXT_SUBTEXT);
    setfont(FONT_STATUS);
    drawtextrange(MAIN_LEFT + SCALE(60), settings.window_width - SCALE(128), SCALE(32), f->status_message,
                  f->status_length);

    if (f->typing) {
        int typing_y = ((y + height) + CHAT_BOX_TOP - SCALE(14));
        setfont(FONT_MISC);
        // @TODO: separate these colors if needed
        setcolor(COLOR_MAIN_TEXT_HINT);
        drawtextwidth_right(x, MESSAGES_X - NAME_OFFSET, typing_y, f->name, f->name_length);
        drawtextwidth(x + MESSAGES_X, x + w, typing_y, S(IS_TYPING), SLEN(IS_TYPING));
    }
}

void draw_group(int UNUSED(x), int UNUSED(y), int UNUSED(w), int UNUSED(height)) {
    GROUPCHAT *g = flist_get_selected()->data;

    drawalpha(BM_GROUP, MAIN_LEFT + SCALE(10), SCALE(10), BM_CONTACT_WIDTH, BM_CONTACT_WIDTH, COLOR_MAIN_TEXT);

    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_TITLE);
    drawtextrange(MAIN_LEFT + SCALE(60), settings.window_width - SCALE(64), SCALE(2), g->name, g->name_length);

    setcolor(COLOR_MAIN_TEXT_SUBTEXT);
    setfont(FONT_STATUS);
    drawtextrange(MAIN_LEFT + SCALE(60), settings.window_width - SCALE(64), SCALE(16), g->topic, g->topic_length);

    uint32_t i = 0;
    int      k = MAIN_LEFT + SCALE(60);

    unsigned int pos_y = 15;
    while (i < g->peer_count) {
        GROUP_PEER *peer = g->peer[i];

        if (peer && peer->name_length) {
            char buf[TOX_MAX_NAME_LENGTH];
            int  text_length = snprintf((char *)buf, TOX_MAX_NAME_LENGTH, "%.*s, ", (int)peer->name_length, peer->name);

            int w = textwidth(buf, text_length);
            if (peer->name_color) {
                setcolor(peer->name_color);
            } else {
                setcolor(COLOR_GROUP_PEER);
            }

            if (k + w >= (settings.window_width - SCALE(64))) {
                if (pos_y == 15) {
                    pos_y += 6;
                    k = MAIN_LEFT + SCALE(60);
                } else {
                    drawtext(k, SCALE(pos_y * 2), "...", 3);
                    break;
                }
            }

            drawtext(k, SCALE(pos_y * 2), buf, text_length);

            k += w;
        }
        i++;
    }
}

/* Draw an invite to be a friend window */
void draw_friend_request(int UNUSED(x), int UNUSED(y), int UNUSED(w), int UNUSED(height)) {
    FRIENDREQ *req = (flist_get_selected()->data);

    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(MAIN_LEFT + SCALE(10), SCALE(20), FRIENDREQUEST);

    setcolor(COLOR_MAIN_TEXT_SUBTEXT);
    setfont(FONT_STATUS);
    drawtextrange(MAIN_LEFT + SCALE(10), settings.window_width, SCALE(40), req->msg, req->length);
}

/* Draw add a friend window */
void draw_add_friend(int UNUSED(x), int UNUSED(y), int UNUSED(w), int height) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(MAIN_LEFT + SCALE(10), SCALE(20), ADDFRIENDS);

    setcolor(COLOR_MAIN_TEXT_SUBTEXT);
    setfont(FONT_TEXT);
    drawstr(MAIN_LEFT + SCALE(10), MAIN_TOP + SCALE(10), TOXID);

    drawstr(MAIN_LEFT + SCALE(10), MAIN_TOP + UTOX_SCALE(29), MESSAGE);

    if (settings.force_proxy) {
        int push = UTOX_STR_WIDTH(TOXID);
        setfont(FONT_MISC);
        setcolor(C_RED);
        drawstr(MAIN_LEFT + SCALE(20) + push, MAIN_TOP + UTOX_SCALE(6), DNS_DISABLED);
    }

    if (addfriend_status) {
        setfont(FONT_MISC);
        setcolor(C_RED);

        STRING *str;
        switch (addfriend_status) {
            case ADDF_SENT: str     = SPTR(REQ_SENT); break;
            case ADDF_DISCOVER: str = SPTR(REQ_RESOLVE); break;
            case ADDF_BADNAME: str  = SPTR(REQ_INVALID_ID); break;
            case ADDF_NONAME: str   = SPTR(REQ_EMPTY_ID); break;
            case ADDF_TOOLONG: // if message length is too long.
                str = SPTR(REQ_LONG_MSG);
                break;
            case ADDF_NOMESSAGE: // if no message (message length must be >= 1 byte).
                str = SPTR(REQ_NO_MSG);
                break;
            case ADDF_OWNKEY: // if user's own key.
                str = SPTR(REQ_SELF_ID);
                break;
            case ADDF_ALREADYSENT: // if friend request already sent or already a friend.
                str = SPTR(REQ_ALREADY_FRIENDS);
                break;
            case ADDF_BADCHECKSUM: // if bad checksum in address.
                str = SPTR(REQ_BAD_CHECKSUM);
                break;
            case ADDF_SETNEWNOSPAM: // if the friend was already there but the nospam was different.
                str = SPTR(REQ_BAD_NOSPAM);
                break;
            case ADDF_NOMEM: // if increasing the friend list size fails.
                str = SPTR(REQ_NO_MEMORY);
                break;
            case ADDF_UNKNOWN: // for unknown error.
            case ADDF_NONE:    // this case must never be rendered, but if it does, assume it's an error
            default: str = SPTR(REQ_UNKNOWN); break;
        }

        utox_draw_text_multiline_compat(MAIN_LEFT + SCALE(10), settings.window_width - BM_SBUTTON_WIDTH - SCALE(10),
                                        MAIN_TOP + SCALE(166), 0, height, font_small_lineheight, str->str, str->length,
                                        0xFFFF, 0, 0, 0, 1);
    }
}

/* Draw the text for profile password window */
void draw_profile_password(int UNUSED(x), int UNUSED(y), int UNUSED(w), int height) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(MAIN_LEFT + SCALE(10), SCALE(20), PROFILE_PASSWORD);

    setcolor(COLOR_MAIN_TEXT_SUBTEXT);
    setfont(FONT_TEXT);
    drawstr(MAIN_LEFT + SCALE(10), MAIN_TOP + SCALE(10), PROFILE_PASSWORD);
}

/* Top bar for user settings */
void draw_settings_header(int UNUSED(x), int UNUSED(y), int w, int UNUSED(height)) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(MAIN_LEFT + SCALE(10), SCALE(10), UTOX_SETTINGS);
#ifdef GIT_VERSION
    int x = MAIN_LEFT + SCALE(10) + UTOX_STR_WIDTH(UTOX_SETTINGS) + SCALE(10);
    setfont(FONT_TEXT);
    drawtext(x, SCALE(10), GIT_VERSION, strlen(GIT_VERSION));
    char ver_string[64];
    int  count;
    count = snprintf(ver_string, 64, "Toxcore v%u.%u.%u", tox_version_major(), tox_version_minor(), tox_version_patch());
    drawtextwidth_right(w + SIDEBAR_WIDTH - textwidth(ver_string, count), textwidth(ver_string, count), SCALE(10),
                        ver_string, count);
#endif
}

/* TODO make this fxn readable */
void draw_settings_sub_header(int x, int y, int w, int UNUSED(height)) {
    setfont(FONT_SELF_NAME);

#define DRAW_UNDERLINE() drawhline(x, y + SCALE(30), x_right_edge, COLOR_EDGE_NORMAL)
#define DRAW_OVERLINE()                                   \
    drawhline(x, y + 0, x_right_edge, COLOR_EDGE_ACTIVE); \
    drawhline(x, y + 1, x_right_edge, COLOR_EDGE_ACTIVE)

    /* Draw the text and bars for general settings */
    setcolor(!button_settings_sub_profile.mouseover ? COLOR_MAIN_TEXT : COLOR_MAIN_TEXT_SUBTEXT);
    int x_right_edge = x + SCALE(10) + UTOX_STR_WIDTH(PROFILE_BUTTON) + SCALE(10);
    drawstr(x + SCALE(10), y + SCALE(10), PROFILE_BUTTON);

    if (panel_settings_profile.disabled) {
        DRAW_UNDERLINE();
    } else {
        DRAW_OVERLINE();
    }
    drawvline(x_right_edge, y + SCALE(0), y + SCALE(30), COLOR_EDGE_NORMAL);

#ifdef ENABLE_MULTIDEVICE
    /* Draw the text and bars for device settings */
    setcolor(!button_settings_sub_devices.mouseover ? COLOR_MAIN_TEXT : COLOR_MAIN_TEXT_SUBTEXT);
    x            = x_right_edge;
    x_right_edge = x_right_edge + SCALE(10) + UTOX_STR_WIDTH(DEVICES_BUTTON) + SCALE(10);
    drawstr(x + SCALE(10), y + SCALE(10), DEVICES_BUTTON);

    if (panel_settings_devices.disabled) {
        DRAW_UNDERLINE();
    } else {
        DRAW_OVERLINE();
    }
    drawvline(x_right_edge, y + SCALE(0), y + UTOX_SCALE(15), COLOR_EDGE_NORMAL);
#endif

    /* Draw the text and bars for network settings */
    setcolor(!button_settings_sub_net.mouseover ? COLOR_MAIN_TEXT : COLOR_MAIN_TEXT_SUBTEXT);
    x            = x_right_edge;
    x_right_edge = x_right_edge + SCALE(10) + UTOX_STR_WIDTH(NETWORK_BUTTON) + SCALE(10);
    drawstr(x + SCALE(10), y + SCALE(10), NETWORK_BUTTON);

    if (panel_settings_net.disabled) {
        DRAW_UNDERLINE();
    } else {
        DRAW_OVERLINE();
    }
    drawvline(x_right_edge, y + SCALE(0), y + SCALE(30), COLOR_EDGE_NORMAL);

    /* Draw the text and bars for User interface settings */
    setcolor(!button_settings_sub_ui.mouseover ? COLOR_MAIN_TEXT : COLOR_MAIN_TEXT_SUBTEXT);
    x            = x_right_edge;
    x_right_edge = x_right_edge + SCALE(10) + UTOX_STR_WIDTH(USER_INTERFACE_BUTTON) + SCALE(10);
    drawstr(x + SCALE(10), y + SCALE(10), USER_INTERFACE_BUTTON);

    if (panel_settings_ui.disabled) {
        DRAW_UNDERLINE();
    } else {
        DRAW_OVERLINE();
    }
    drawvline(x_right_edge, y, y + SCALE(30), COLOR_EDGE_NORMAL);

    /* Draw the text and bars for A/V settings */
    setcolor(!button_settings_sub_av.mouseover ? COLOR_MAIN_TEXT : COLOR_MAIN_TEXT_SUBTEXT);
    x            = x_right_edge;
    x_right_edge = x_right_edge + SCALE(10) + UTOX_STR_WIDTH(AUDIO_VIDEO_BUTTON) + SCALE(1000);
    drawstr(x + SCALE(10), y + SCALE(10), AUDIO_VIDEO_BUTTON);

    if (panel_settings_av.disabled) {
        DRAW_UNDERLINE();
    } else {
        DRAW_OVERLINE();
    }
}

/* draw switch profile top bar */
/* Text content for settings page */
void draw_settings_text_profile(int x, int y, int w, int h) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(10), NAME);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(60), STATUSMESSAGE);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(110), TOXID);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(160), LANGUAGE);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(210), PROFILE_PASSWORD);
}

void draw_settings_text_devices(int x, int y, int w, int h) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(10), DEVICES_ADD_NEW);

    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(60), DEVICES_NUMBER);

    char   str[10];
    size_t strlen = snprintf(str, 10, "%zu", self.device_list_count);

    drawtext(MAIN_LEFT + SCALE(10), y + SCALE(75), str, strlen);
}

void draw_settings_text_password(int x, int y, int w, int h) {
    setfont(FONT_MISC);
    setcolor(C_RED);
    drawstr(MAIN_LEFT + SCALE(80), y + SCALE(256), PROFILE_PW_WARNING);
    drawstr(MAIN_LEFT + SCALE(80), y + SCALE(270), PROFILE_PW_NO_RECOVER);
}

void draw_settings_text_network(int x, int y, int w, int UNUSED(height)) {
    setfont(FONT_MISC);
    setcolor(C_RED);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(10), WARNING);

    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(30), IPV6);

    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(60), UDP);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(90), PROXY);
    drawtext(MAIN_LEFT + SCALE(264), y + SCALE(114), ":", 1);
}

void draw_settings_text_ui(int x, int y, int w, int UNUSED(height)) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(MAIN_LEFT + SCALE(150), y + SCALE(10), DPI);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(10), THEME);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(65), LOGGING);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(95), CLOSE_TO_TRAY);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(125), START_IN_TRAY);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(155), AUTO_STARTUP);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(185), SEND_TYPING_NOTIFICATIONS);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(215), SETTINGS_UI_MINI_ROSTER);
}

void draw_settings_text_av(int x, int y, int w, int UNUSED(height)) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(10), RINGTONE);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(40), PUSH_TO_TALK);
    drawstr(MAIN_LEFT + SCALE(240), y + SCALE(10), GROUP_NOTIFICATIONS);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(60), STATUS_NOTIFICATIONS);
#ifdef AUDIO_FILTERING
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(80), AUDIOFILTERING);
#endif
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(100), AUDIOINPUTDEVICE);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(160), AUDIOOUTPUTDEVICE);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(220), VIDEOINPUTDEVICE);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(280), PREVIEW);
}

void draw_friend_settings(int UNUSED(x), int y, int width, int height) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);

    drawstr(MAIN_LEFT + SCALE(10), y + MAIN_TOP + SCALE(10), FRIEND_PUBLIC_KEY);
    drawstr(MAIN_LEFT + SCALE(10), y + MAIN_TOP + SCALE(60), FRIEND_ALIAS);
    drawstr(MAIN_LEFT + SCALE(10), y + MAIN_TOP + SCALE(110), FRIEND_AUTOACCEPT);
}

void draw_group_settings(int UNUSED(x), int y, int width, int height) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);

    drawstr(MAIN_LEFT + SCALE(10), y + MAIN_TOP + SCALE(10), GROUP_TOPIC);
    drawstr(MAIN_LEFT + SCALE(10), y + MAIN_TOP + SCALE(70), GROUP_NOTIFICATIONS);
}

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
void background_draw(PANEL *UNUSED(p), int UNUSED(x), int UNUSED(y), int width, int height) {
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
