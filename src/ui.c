#include "main.h"

// Application-wide language setting
UI_LANG_ID LANG;

/***** MAYBE_I18NAL_STRING helpers start *****/

void maybe_i18nal_string_set_plain(MAYBE_I18NAL_STRING *mis, char_t *str, uint16_t length) {
    mis->i18nal       = UI_STRING_ID_INVALID;
    mis->plain.length = length;
    mis->plain.str    = str;
}

void maybe_i18nal_string_set_i18nal(MAYBE_I18NAL_STRING *mis, UI_STRING_ID string_id) {
    mis->plain.str    = NULL;
    mis->plain.length = 0;
    mis->i18nal       = string_id;
}

STRING *maybe_i18nal_string_get(MAYBE_I18NAL_STRING *mis) {
    if (mis->plain.str) {
        return &mis->plain;
    } else {
        return SPTRFORLANG(LANG, mis->i18nal);
    }
}

_Bool maybe_i18nal_string_is_valid(MAYBE_I18NAL_STRING *mis) {
    return (mis->plain.str || ((UI_STRING_ID_INVALID != mis->i18nal) && (mis->i18nal < NUM_STRS)));
}

/***** MAYBE_I18NAL_STRING helpers end *****/

void draw_avatar_image(UTOX_NATIVE_IMAGE *image, int x, int y, uint32_t width, uint32_t height, uint32_t targetwidth,
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
static void draw_user_badge(int UNUSED(x), int UNUSED(y), int UNUSED(width), int UNUSED(height)) {
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
                      list_get_filter() ? S(FILTER_ONLINE) : S(FILTER_ALL),
                      list_get_filter() ? SLEN(FILTER_ONLINE) : SLEN(FILTER_ALL));
    } else {
        drawalpha(BM_CONTACT, SIDEBAR_AVATAR_LEFT, SIDEBAR_AVATAR_TOP, BM_CONTACT_WIDTH, BM_CONTACT_WIDTH,
                  COLOR_MENU_TEXT);

        setcolor(!button_name.mouseover ? COLOR_MENU_TEXT : COLOR_MENU_TEXT_SUBTEXT);
        setfont(FONT_SELF_NAME);
        drawtextrange(SIDEBAR_NAME_LEFT, SIDEBAR_NAME_WIDTH, SIDEBAR_NAME_TOP, S(NOT_CONNECTED), SLEN(NOT_CONNECTED));
    }
}

static void draw_splash_page(int x, int y, int w, int h) {
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
static void draw_friend(int x, int y, int w, int height) {
    FRIEND *f = selected_item->data;

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

static void draw_group(int UNUSED(x), int UNUSED(y), int UNUSED(w), int UNUSED(height)) {
    GROUPCHAT *g = selected_item->data;

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
            uint8_t buf[TOX_MAX_NAME_LENGTH];
            int text_length = snprintf((char *)buf, TOX_MAX_NAME_LENGTH, "%.*s, ", (int)peer->name_length, peer->name);

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
                    drawtext(k, SCALE(pos_y * 2), (uint8_t *)"...", 3);
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
static void draw_friend_request(int UNUSED(x), int UNUSED(y), int UNUSED(w), int UNUSED(height)) {
    FRIENDREQ *req = selected_item->data;

    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(MAIN_LEFT + SCALE(10), UTOX_SCALE(10), FRIENDREQUEST);

    setcolor(COLOR_MAIN_TEXT_SUBTEXT);
    setfont(FONT_STATUS);
    drawtextrange(MAIN_LEFT + SCALE(10), settings.window_width, UTOX_SCALE(20), req->msg, req->length);
}

/* Draw add a friend window */
static void draw_add_friend(int UNUSED(x), int UNUSED(y), int UNUSED(w), int height) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(MAIN_LEFT + SCALE(10), UTOX_SCALE(10), ADDFRIENDS);

    setcolor(COLOR_MAIN_TEXT_SUBTEXT);
    setfont(FONT_TEXT);
    drawstr(MAIN_LEFT + SCALE(10), MAIN_TOP + SCALE(10), TOXID);

    drawstr(MAIN_LEFT + SCALE(10), MAIN_TOP + UTOX_SCALE(29), MESSAGE);

    if (settings.force_proxy) {
        int push = UTOX_STR_WIDTH(TOXID);
        setfont(FONT_MISC);
        setcolor(C_RED);
        drawstr(MAIN_LEFT + UTOX_SCALE(10) + push, MAIN_TOP + UTOX_SCALE(6), DNS_DISABLED);
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
static void draw_profile_password(int UNUSED(x), int UNUSED(y), int UNUSED(w), int height) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(MAIN_LEFT + SCALE(10), UTOX_SCALE(10), PROFILE_PASSWORD);

    setcolor(COLOR_MAIN_TEXT_SUBTEXT);
    setfont(FONT_TEXT);
    drawstr(MAIN_LEFT + SCALE(10), MAIN_TOP + SCALE(10), PROFILE_PASSWORD);
}

/* Top bar for user settings */
static void draw_settings_header(int UNUSED(x), int UNUSED(y), int w, int UNUSED(height)) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(MAIN_LEFT + SCALE(10), SCALE(10), UTOX_SETTINGS);
#ifdef GIT_VERSION
    int x = MAIN_LEFT + SCALE(10) + UTOX_STR_WIDTH(UTOX_SETTINGS) + SCALE(10);
    setfont(FONT_TEXT);
    drawtext(x, SCALE(10), (uint8_t *)GIT_VERSION, strlen(GIT_VERSION));
    char version_string[64];
    int  count;
    count = snprintf(version_string, 64, "Core v%u.%u.%u ToxAV v%u.%u.%u ToxES v%u.%u.%u", tox_version_major(),
                     tox_version_minor(), tox_version_patch(), toxav_version_major(), toxav_version_minor(),
                     toxav_version_patch(), toxes_version_major(), toxes_version_minor(), toxes_version_patch());
    drawtextwidth_right(w, textwidth((char_t *)version_string, count), SCALE(10), (uint8_t *)version_string,
                        strlen(version_string));
#endif
}

/* TODO make this fxn readable */
static void draw_settings_sub_header(int x, int y, int w, int UNUSED(height)) {
    setfont(FONT_SELF_NAME);

#define DRAW_UNDERLINE() drawhline(x, y + SCALE(30), x_right_edge, COLOR_EDGE_NORMAL)
#define DRAW_OVERLINE()                                                                                                \
    drawhline(x, y + 0, x_right_edge, COLOR_EDGE_ACTIVE);                                                              \
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
static void draw_settings_text_profile(int x, int y, int w, int h) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(10), NAME);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(60), STATUSMESSAGE);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(110), TOXID);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(160), LANGUAGE);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(210), PROFILE_PASSWORD);
}

static void draw_settings_text_devices(int x, int y, int w, int h) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(10), DEVICES_ADD_NEW);

    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(60), DEVICES_NUMBER);

    uint8_t str[10];
    size_t  strlen = snprintf(str, 10, "%u", self.device_list_count);

    drawtext(MAIN_LEFT + SCALE(10), y + SCALE(75), str, strlen);
}

static void draw_settings_text_password(int x, int y, int w, int h) {
    setfont(FONT_MISC);
    setcolor(C_RED);
    drawstr(MAIN_LEFT + SCALE(80), y + SCALE(256), PROFILE_PW_WARNING);
    drawstr(MAIN_LEFT + SCALE(80), y + SCALE(270), PROFILE_PW_NO_RECOVER);
}

static void draw_settings_text_network(int x, int y, int w, int UNUSED(height)) {
    setfont(FONT_MISC);
    setcolor(C_RED);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(10), WARNING);

    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(30), IPV6);

    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(60), UDP);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(90), PROXY);
    drawtext(MAIN_LEFT + SCALE(264), y + SCALE(114), (uint8_t *)":", 1);
}

static void draw_settings_text_ui(int x, int y, int w, int UNUSED(height)) {
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

static void draw_settings_text_av(int x, int y, int w, int UNUSED(height)) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(10), RINGTONE);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(40), PUSH_TO_TALK);
    drawstr(MAIN_LEFT + SCALE(240), y + SCALE(10), GROUP_NOTIFICATIONS);
#ifdef AUDIO_FILTERING
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(70), AUDIOFILTERING);
#endif
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(100), AUDIOINPUTDEVICE);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(160), AUDIOOUTPUTDEVICE);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(220), VIDEOINPUTDEVICE);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(280), PREVIEW);
}

static void draw_friend_settings(int UNUSED(x), int y, int width, int height) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);

    drawstr(MAIN_LEFT + SCALE(10), y + MAIN_TOP + SCALE(10), FRIEND_PUBLIC_KEY);
    drawstr(MAIN_LEFT + SCALE(10), y + MAIN_TOP + SCALE(60), FRIEND_ALIAS);
    drawstr(MAIN_LEFT + SCALE(10), y + MAIN_TOP + SCALE(110), FRIEND_AUTOACCEPT);
}

static void draw_group_settings(int UNUSED(x), int y, int width, int height) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);

    drawstr(MAIN_LEFT + SCALE(10), y + MAIN_TOP + SCALE(10), GROUP_TOPIC);
    drawstr(MAIN_LEFT + SCALE(10), y + MAIN_TOP + SCALE(70), GROUP_NOTIFICATIONS);
}

static void draw_background(int UNUSED(x), int UNUSED(y), int width, int height) {
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
static void background_draw(PANEL *UNUSED(p), int UNUSED(x), int UNUSED(y), int width, int height) {
    return;
}
static _Bool background_mmove(PANEL *UNUSED(p), int UNUSED(x), int UNUSED(y), int UNUSED(width), int UNUSED(height),
                              int UNUSED(mx), int UNUSED(my), int UNUSED(dx), int UNUSED(dy)) {
    return 0;
}
static _Bool background_mdown(PANEL *UNUSED(p)) {
    return 0;
}
static _Bool background_mright(PANEL *UNUSED(p)) {
    return 0;
}
static _Bool background_mwheel(PANEL *UNUSED(p), int UNUSED(height), double UNUSED(d), _Bool UNUSED(smooth)) {
    return 0;
}
static _Bool background_mup(PANEL *UNUSED(p)) {
    return 0;
}
static _Bool background_mleave(PANEL *UNUSED(p)) {
    return 0;
}

// Scrollbar or friend list
SCROLLABLE scrollbar_roster =
               {
                   .panel =
                       {
                           .type = PANEL_SCROLLABLE,
                       },
                   .color = C_SCROLL,
                   .x     = 2,
                   .left  = 1,
                   .small = 1,
},

           // Scrollbar in chat window
    scrollbar_friend =
        {
            .panel =
                {
                    .type = PANEL_SCROLLABLE,
                },
            .color = C_SCROLL,
},

           scrollbar_group =
               {
                   .panel =
                       {
                           .type = PANEL_SCROLLABLE,
                       },
                   .color = C_SCROLL,
},

           // Color is not used for settings
    // @TODO
    scrollbar_settings = {
        .panel =
            {
                .type = PANEL_SCROLLABLE,
            },
        .color = C_SCROLL,
};

/* */
PANEL messages_friend =
          {
              .type = PANEL_MESSAGES, .content_scroll = &scrollbar_friend,
},

      messages_group = {
          .type = PANEL_MESSAGES, .content_scroll = &scrollbar_group,
};

/* Root panel, hold all the other panels */
PANEL panel_root = {
    .type = PANEL_NONE,
    .drawfunc = draw_background,
    .disabled = 0,
    .child = (PANEL*[]) {
        &panel_side_bar, &panel_main,
        NULL
    }
},

/* Left side bar, holds the user, the roster, and the setting buttons */
panel_side_bar = {
    .type = PANEL_NONE,
    .disabled = 0,
    .child = (PANEL*[]) {
        &panel_self,
        &panel_quick_buttons,
        &panel_roster,
        NULL
    }
},
    /* The user badge and buttons */
    panel_self = {
        .type     = PANEL_NONE,
        .disabled = 0,
        .drawfunc = draw_user_badge,
        .child    = (PANEL*[]) {
            (void*)&button_avatar, (void*)&button_name,       (void*)&button_usr_state,
                                   (void*)&button_status_msg,
            NULL
        }
    },
    /* Left sided toggles */
    panel_quick_buttons = {
        .type     = PANEL_NONE,
        .disabled = 0,
        .child    = (PANEL*[]) {
            (void*)&button_filter_friends, /* Top of roster */

            (void*)&edit_search,           /* Bottom of roster*/
            (void*)&button_settings,
            (void*)&button_add_new_contact,
            NULL
        }
    },
    /* The friends and group was called list */
    panel_roster = {
        .type     = PANEL_NONE,
        .disabled = 0,
        .child    = (PANEL*[]) {
            // TODO rename these
            (void*)&panel_roster_list,
            (void*)&scrollbar_roster,
            NULL
        }
    },
        panel_roster_list = {
            .type           = PANEL_LIST,
            .content_scroll = &scrollbar_roster,
        },

/* Main panel, holds the overhead/settings, or the friend/group containers */
panel_main = {
    .type = PANEL_NONE,
    .disabled = 0,
    .child = (PANEL*[]) {
        (void*)&panel_chat,
        (void*)&panel_overhead,
        NULL
    }
},
    /* Chat panel, friend or group, depending on what's selected */
    panel_chat = {
        .type = PANEL_NONE,
        .disabled = 1,
        .child = (PANEL*[]) {
            (void*)&panel_group,
            (void*)&panel_friend,
            (void*)&panel_friend_request,
            NULL
        }
    },
        panel_group = {
            .type = PANEL_NONE,
            .disabled = 1,
            .child = (PANEL*[]) {
                (void*)&panel_group_chat,
                (void*)&panel_group_video,
                (void*)&panel_group_settings,
                NULL
            }
        },
            panel_group_chat = {
                .type = PANEL_NONE,
                .disabled = 0,
                .drawfunc = draw_group,
                .child = (PANEL*[]) {
                    (void*)&scrollbar_group,
                    (void*)&edit_msg_group, // this needs to be one of the first, to get events before the others
                    (void*)&messages_group,
                    (void*)&button_group_audio,
                    (void*)&button_chat_send,
                    NULL
                }
            },
            panel_group_video = {
                .type = PANEL_NONE,
                .disabled = 1,
                .child = (PANEL*[]) {
                    NULL
                }
            },
            panel_group_settings = {
                .type = PANEL_NONE,
                .disabled = 1,
                .drawfunc = draw_group_settings,
                .child = (PANEL*[]) {
                    (void *)&edit_group_topic,
                    (void *)&dropdown_notify_groupchats,
                    NULL
                }
            },
        panel_friend = {
            .type = PANEL_NONE,
            .disabled = 1,
            .child = (PANEL*[]) {
                (void*)&panel_friend_chat,
                (void*)&panel_friend_video,
                (void*)&panel_friend_settings,
                NULL
            }
        },
            panel_friend_chat = {
                .type = PANEL_NONE,
                .disabled = 0,
                .drawfunc = draw_friend,
                .child = (PANEL*[]) {
                    (void*)&scrollbar_friend,
                    (void*)&edit_msg, // this needs to be one of the first, to get events before the others
                    (void*)&messages_friend,
                    (void*)&button_call_decline, (void*)&button_call_audio, (void*)&button_call_video,
                    (void*)&button_send_file, (void*)&button_send_screenshot, (void*)&button_chat_send,
                    NULL
                }
            },
            panel_friend_video = {
                .type = PANEL_INLINE_VIDEO,
                .disabled = 1,
                .child = (PANEL*[]) {
                    NULL
                }
            },
            panel_friend_settings = {
                .type = PANEL_NONE,
                .disabled = 1,
                .drawfunc = draw_friend_settings,
                .child = (PANEL*[]) {
                    (void*)&edit_friend_pubkey,
                    (void*)&edit_friend_alias,
                    (void*)&dropdown_friend_autoaccept_ft,
                    (void*)&button_export_chatlog,
                    NULL
                }
            },
        panel_friend_request = {
            .type = PANEL_NONE,
            .disabled = 1,
            .drawfunc = draw_friend_request,
            .child = (PANEL*[]) {
                (void*)&button_accept_friend,
                NULL
            }
        },
    /* Settings master panel, holds the lower level settings */
    panel_overhead = {
        .type = PANEL_NONE,
        .disabled = 0,
        .child = (PANEL*[]) {
            (void*)&panel_splash_page,
            (void*)&panel_profile_password,
            (void*)&panel_add_friend,
            (void*)&panel_settings_master,
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
        },

        panel_profile_password = {
            .type = PANEL_NONE,
            .disabled = 0,
            .drawfunc = draw_profile_password,
            .child = (PANEL*[]) {
                (void*)&edit_profile_password,
                NULL
            }
        },
        panel_add_friend = {
            .type = PANEL_NONE,
            .disabled = 1,
            .drawfunc = draw_add_friend,
            .child = (PANEL*[]) {
                (void*)&button_send_friend_request,
                (void*)&edit_add_id, (void*)&edit_add_msg,
                NULL
            }
        },
        panel_settings_master = {
            .type = PANEL_NONE,
            .disabled = 1,
            .drawfunc = draw_settings_header,
            .child = (PANEL*[]) {
                (void*)&panel_settings_subheader,
                NULL
            }
        },
            panel_settings_subheader = {
                .type = PANEL_NONE,
                .disabled = 0,
                .drawfunc = draw_settings_sub_header,
                .child = (PANEL*[]) {
                    (void*)&button_settings_sub_profile,
                    (void*)&button_settings_sub_devices,
                    (void*)&button_settings_sub_net,
                    (void*)&button_settings_sub_ui,
                    (void*)&button_settings_sub_av,
                    (void*)&scrollbar_settings,
                    (void*)&panel_settings_profile,
                    (void*)&panel_settings_devices,
                    (void*)&panel_settings_net,
                    (void*)&panel_settings_ui,
                    (void*)&panel_settings_av,
                    NULL
                }
            },

            /* Panel to draw settings page */
            panel_settings_profile = {
                .type = PANEL_NONE,
                .disabled = 0,
                .drawfunc = draw_settings_text_profile,
                .content_scroll = &scrollbar_settings,
                .child = (PANEL*[]) {
                    (void*)&edit_name,
                    (void*)&edit_status,
                    // Text: Tox ID
                    (void*)&edit_toxid,
                    (void*)&button_copyid,
// User's tox id
#ifdef EMOJI_IDS
                    (void*)&button_change_id_type,
#endif
                    (void*)&dropdown_language,
                    (void*)&button_show_password_settings,
                    (void*)&panel_profile_password_settings,
                    NULL
                }
            },

                panel_profile_password_settings = {
                    .type     = PANEL_NONE,
                    .disabled = 1,
                    .drawfunc = draw_settings_text_password,
                    .child = (PANEL*[]) {
                        (void*)&edit_profile_password,
                        (void*)&button_lock_uTox,
                        NULL
                    }
                },

            /* Panel to draw settings page */
            panel_settings_devices = {
                .type = PANEL_NONE,
                .disabled = 1,
                .drawfunc = draw_settings_text_devices,
                .content_scroll = &scrollbar_settings,
                .child = NULL,
            },

            panel_settings_net = {
                .type = PANEL_NONE,
                /* Disabled by default, enabled by network button */
                .disabled = 1,
                .drawfunc = draw_settings_text_network,
                .content_scroll = &scrollbar_settings,
                .child = (PANEL*[]) {
                    (void*)&edit_proxy_ip,
                    (void*)&edit_proxy_port,
                    (void*)&dropdown_proxy,
                    (void*)&switch_ipv6,
                    (void*)&switch_udp,
                    NULL
                }
            },

            panel_settings_ui = {
                .type = PANEL_NONE,
                .drawfunc = draw_settings_text_ui,
                .disabled = 1,
                .content_scroll = &scrollbar_settings,
                .child = (PANEL*[]) {
                    (void*)&dropdown_dpi,
                    (void*)&dropdown_theme,
                    (void*)&switch_logging,
                    (void*)&switch_close_to_tray,
                    (void*)&switch_start_in_tray,
                    (void*)&switch_auto_startup,
                    (void*)&switch_typing_notes,
                    (void*)&switch_mini_contacts,
                    NULL
                }
            },

            panel_settings_av = {
                .type = PANEL_NONE,
                .disabled = 1,
                .drawfunc = draw_settings_text_av,
                .content_scroll = &scrollbar_settings,
                .child = (PANEL*[]) {
                    (void*)&button_callpreview,
                    (void*)&switch_push_to_talk,
                    (void*)&button_videopreview,
                    (void*)&dropdown_audio_in,
                    (void*)&dropdown_audio_out,
                    (void*)&dropdown_video,
                    (void*)&switch_audible_notifications,
                    (void*)&switch_audio_filtering,
                    (void*)&dropdown_global_group_notifications,
                    NULL
                }
            };

#define CREATE_BUTTON(n, a, b, w, h)                                                                                   \
    PANEL b_##n = {                                                                                                    \
        .type = PANEL_BUTTON, .x = a, .y = b, .width = w, .height = h,                                                 \
    };                                                                                                                 \
    button_##n.panel = b_##n

#define CREATE_EDIT(n, a, b, w, h)                                                                                     \
    PANEL e_##n = {                                                                                                    \
        .type = PANEL_EDIT, .x = a, .y = b, .width = w, .height = h,                                                   \
    };                                                                                                                 \
    edit_##n.panel = e_##n

void ui_set_scale(uint8_t scale) {
    if (ui_scale == scale) {
        return;
    }

    ui_scale = scale;
    roster_re_scale();
    setscale_fonts();
    setfont(FONT_SELF_NAME);

    /* DEFAULT positions */
    panel_side_bar.x     = 0;
    panel_side_bar.y     = 0;
    panel_side_bar.width = SIDEBAR_WIDTH;

    scrollbar_roster.panel.y      = ROSTER_TOP;
    scrollbar_roster.panel.width  = MAIN_LEFT;
    scrollbar_roster.panel.height = ROSTER_BOTTOM;

    panel_roster_list.x      = 0;
    panel_roster_list.y      = ROSTER_TOP;
    panel_roster_list.width  = MAIN_LEFT;
    panel_roster_list.height = ROSTER_BOTTOM;

    panel_main.x = MAIN_LEFT;
    panel_main.y = 0;

    scrollbar_settings.panel.y        = SCALE(32);  /* TODO magic numbers are bad */
    scrollbar_settings.content_height = SCALE(300); /* TODO magic numbers are bad */

    panel_settings_master.y  = MAIN_TOP_FRAME_THIN;
    panel_settings_profile.y = SCALE(32);
    panel_settings_devices.y = SCALE(32);
    panel_settings_net.y     = SCALE(32);
    panel_settings_ui.y      = SCALE(32);
    panel_settings_av.y      = SCALE(32);

    scrollbar_friend.panel.y      = MAIN_TOP;
    scrollbar_friend.panel.height = CHAT_BOX_TOP;
    messages_friend.y             = MAIN_TOP;
    messages_friend.height        = CHAT_BOX_TOP - SCALE(10);
    messages_friend.width         = -SCROLL_WIDTH;

    scrollbar_group.panel.y      = MAIN_TOP;
    scrollbar_group.panel.height = CHAT_BOX_TOP;
    messages_group.y             = MAIN_TOP;
    messages_group.height        = CHAT_BOX_TOP;
    messages_group.width         = -SCROLL_WIDTH;

    setfont(FONT_SELF_NAME);

    /* TODO MOVE THIS */
    PANEL panel_switch_logging = {
        .type   = PANEL_SWITCH,
        .x      = SCALE(-10) - BM_SWITCH_WIDTH,
        .y      = SCALE(60),
        .width  = BM_SWITCH_WIDTH,
        .height = BM_SWITCH_HEIGHT,
    };

    PANEL panel_switch_close_to_tray = {
        .type   = PANEL_SWITCH,
        .x      = SCALE(-10) - BM_SWITCH_WIDTH,
        .y      = SCALE(90),
        .width  = BM_SWITCH_WIDTH,
        .height = BM_SWITCH_HEIGHT,
    };

    PANEL panel_switch_start_in_tray = {
        .type   = PANEL_SWITCH,
        .x      = SCALE(-10) - BM_SWITCH_WIDTH,
        .y      = SCALE(120),
        .width  = BM_SWITCH_WIDTH,
        .height = BM_SWITCH_HEIGHT,
    };

    PANEL panel_switch_auto_startup = {
        .type   = PANEL_SWITCH,
        .x      = SCALE(-10) - BM_SWITCH_WIDTH,
        .y      = SCALE(150),
        .width  = BM_SWITCH_WIDTH,
        .height = BM_SWITCH_HEIGHT,
    };

    PANEL panel_switch_typing_notes = {
        .type   = PANEL_SWITCH,
        .x      = SCALE(-10) - BM_SWITCH_WIDTH,
        .y      = SCALE(180),
        .width  = BM_SWITCH_WIDTH,
        .height = BM_SWITCH_HEIGHT,
    };

    PANEL panel_switch_mini_contacts = {
        .type   = PANEL_SWITCH,
        .x      = SCALE(-10) - BM_SWITCH_WIDTH,
        .y      = SCALE(210),
        .width  = BM_SWITCH_WIDTH,
        .height = BM_SWITCH_HEIGHT,
    };

    PANEL panel_switch_ipv6 = {
        .type   = PANEL_SWITCH,
        .x      = SCALE(-10) - BM_SWITCH_WIDTH,
        .y      = SCALE(30),
        .width  = BM_SWITCH_WIDTH,
        .height = BM_SWITCH_HEIGHT,
    };

    PANEL panel_switch_udp = {
        .type   = PANEL_SWITCH,
        .x      = SCALE(-10) - BM_SWITCH_WIDTH,
        .y      = SCALE(60),
        .width  = BM_SWITCH_WIDTH,
        .height = BM_SWITCH_HEIGHT,
    };

    PANEL panel_switch_audible_notifications = {
        .type   = PANEL_SWITCH,
        .x      = SCALE(-10) - BM_SWITCH_WIDTH,
        .y      = SCALE(10),
        .width  = BM_SWITCH_WIDTH,
        .height = BM_SWITCH_HEIGHT,
    };

    PANEL panel_switch_push_to_talk = {
        .type   = PANEL_SWITCH,
        .x      = SCALE(-10) - BM_SWITCH_WIDTH,
        .y      = SCALE(40),
        .width  = BM_SWITCH_WIDTH,
        .height = BM_SWITCH_HEIGHT,
    };

#ifdef AUDIO_FILTERING
    PANEL panel_switch_audio_filtering = {
        .type   = PANEL_SWITCH,
        .x      = SCALE(-10) - BM_SWITCH_WIDTH,
        .y      = SCALE(70),
        .width  = BM_SWITCH_WIDTH,
        .height = BM_SWITCH_HEIGHT,
    };
#endif

    switch_logging.panel               = panel_switch_logging;
    switch_mini_contacts.panel         = panel_switch_mini_contacts;
    switch_ipv6.panel                  = panel_switch_ipv6;
    switch_udp.panel                   = panel_switch_udp;
    switch_close_to_tray.panel         = panel_switch_close_to_tray;
    switch_start_in_tray.panel         = panel_switch_start_in_tray;
    switch_auto_startup.panel          = panel_switch_auto_startup;
    switch_typing_notes.panel          = panel_switch_typing_notes;
    switch_audible_notifications.panel = panel_switch_audible_notifications;
    switch_push_to_talk.panel          = panel_switch_push_to_talk;

#ifdef AUDIO_FILTERING
    switch_audio_filtering.panel = panel_switch_audio_filtering;
#endif

    /* User Badge & Roster  */
    CREATE_BUTTON(avatar, SIDEBAR_AVATAR_LEFT, SIDEBAR_AVATAR_TOP, BM_CONTACT_WIDTH, BM_CONTACT_WIDTH);
    CREATE_BUTTON(name, SIDEBAR_NAME_LEFT, SIDEBAR_NAME_TOP, SIDEBAR_NAME_WIDTH, SIDEBAR_NAME_HEIGHT - SCALE(2));
    CREATE_BUTTON(status_msg, SIDEBAR_STATUSMSG_LEFT, SIDEBAR_STATUSMSG_TOP,
                  (SELF_STATUS_ICON_LEFT - SIDEBAR_STATUSMSG_LEFT - SCALE(2)), SIDEBAR_STATUSMSG_HEIGHT - SCALE(2));
    CREATE_BUTTON(usr_state, SELF_STATUS_ICON_LEFT, SELF_STATUS_ICON_TOP, BM_STATUSAREA_WIDTH, BM_STATUSAREA_HEIGHT);
    CREATE_BUTTON(filter_friends, SIDEBAR_FILTER_FRIENDS_LEFT, SIDEBAR_FILTER_FRIENDS_TOP, SIDEBAR_FILTER_FRIENDS_WIDTH,
                  SIDEBAR_FILTER_FRIENDS_HEIGHT);
    CREATE_BUTTON(add_new_contact, SIDEBAR_BUTTON_LEFT, ROSTER_BOTTOM, SIDEBAR_BUTTON_WIDTH, SIDEBAR_BUTTON_HEIGHT);
    b_add_new_contact.disabled = 1;
    CREATE_BUTTON(settings, SIDEBAR_BUTTON_LEFT, ROSTER_BOTTOM, SIDEBAR_BUTTON_WIDTH, SIDEBAR_BUTTON_HEIGHT);

    /* Setting pages        */
    CREATE_BUTTON(settings_sub_profile, 1, 1, SCALE(18) + UTOX_STR_WIDTH(PROFILE_BUTTON), SCALE(28));
    uint32_t settings_tab_x = 1 + UTOX_STR_WIDTH(PROFILE_BUTTON);

#ifdef ENABLE_MULTIDEVICE
    CREATE_BUTTON(settings_sub_devices, settings_tab_x, 1, SCALE(22) + UTOX_STR_WIDTH(DEVICES_BUTTON), SCALE(28));
    settings_tab_x += SCALE(22) + UTOX_STR_WIDTH(DEVICES_BUTTON);
#endif

    CREATE_BUTTON(settings_sub_net, settings_tab_x, 1, SCALE(18) + UTOX_STR_WIDTH(NETWORK_BUTTON), SCALE(28));
    settings_tab_x += SCALE(20) + UTOX_STR_WIDTH(NETWORK_BUTTON);

    CREATE_BUTTON(settings_sub_ui, settings_tab_x, 1, SCALE(18) + UTOX_STR_WIDTH(USER_INTERFACE_BUTTON), SCALE(28));
    settings_tab_x += SCALE(20) + UTOX_STR_WIDTH(USER_INTERFACE_BUTTON);

    CREATE_BUTTON(settings_sub_av, settings_tab_x, 1, -1, SCALE(28));

    /* Profile              */
    CREATE_BUTTON(copyid, SCALE(66), SCALE(106), BM_SBUTTON_WIDTH, BM_SBUTTON_HEIGHT);
    CREATE_BUTTON(show_password_settings, SCALE(130), SCALE(206), BM_SBUTTON_WIDTH, BM_SBUTTON_HEIGHT);
    CREATE_BUTTON(lock_uTox, SCALE(10), SCALE(260), BM_SBUTTON_WIDTH, BM_SBUTTON_HEIGHT);

#ifdef EMOJI_IDS
    CREATE_BUTTON(change_id_type, SCALE(160), SCALE(106), BM_SBUTTON_WIDTH, BM_SBUTTON_HEIGHT);
#endif

    PANEL e_name = {.type = PANEL_EDIT, .x = SCALE(10), .y = SCALE(27), .height = SCALE(24), .width = -SCALE(10)},
          e_status = {.type = PANEL_EDIT, .x = SCALE(10), .y = SCALE(76), .height = SCALE(24), .width = -SCALE(10)},
          e_toxid = {.type = PANEL_EDIT, .x = SCALE(10), .y = SCALE(126), .height = SCALE(24), .width = -SCALE(10)};

    edit_name.panel   = e_name;
    edit_status.panel = e_status;
    edit_toxid.panel  = e_toxid;

    /* Devices              */
    CREATE_BUTTON(add_new_device_to_self, SCALE(-10) - BM_SBUTTON_WIDTH, SCALE(28), BM_SBUTTON_WIDTH,
                  BM_SBUTTON_HEIGHT);

    CREATE_EDIT(add_new_device_to_self, SCALE(10), SCALE(27), SCALE(0) - UTOX_STR_WIDTH(ADD) - BM_SBUTTON_WIDTH,
                SCALE(24));

    /* Network              */

    /* User Interface       */

    /* Audio/Video          */
    CREATE_BUTTON(callpreview, SCALE(10), SCALE(280), BM_LBUTTON_WIDTH, BM_LBUTTON_HEIGHT);
    CREATE_BUTTON(videopreview, SCALE(70), SCALE(280), BM_LBUTTON_WIDTH, BM_LBUTTON_HEIGHT);

    /* Friend Add Page      */
    CREATE_BUTTON(send_friend_request, SCALE(-10) - BM_SBUTTON_WIDTH, MAIN_TOP + UTOX_SCALE(84), BM_SBUTTON_WIDTH,
                  BM_SBUTTON_HEIGHT);
    CREATE_BUTTON(accept_friend, SCALE(10), MAIN_TOP + SCALE(10), BM_SBUTTON_WIDTH, BM_SBUTTON_HEIGHT);

    /* Friend Settings Page */
    CREATE_BUTTON(export_chatlog, SCALE(10), SCALE(220), BM_SBUTTON_WIDTH, BM_SBUTTON_HEIGHT);

    PANEL e_friend_pubkey = {
        .type = PANEL_EDIT, .x = SCALE(10), .y = SCALE(88), .height = SCALE(24), .width = -SCALE(10)};

    edit_friend_pubkey.panel = e_friend_pubkey;

    PANEL e_friend_alias = {
        .type = PANEL_EDIT, .x = SCALE(10), .y = SCALE(138), .height = SCALE(24), .width = SCALE(-10)};

    edit_friend_alias.panel = e_friend_alias;

    PANEL d_friend_autoaccept = {
        .type = PANEL_DROPDOWN, .x = SCALE(10), .y = SCALE(188), .height = SCALE(24), .width = SCALE(40)};

    dropdown_friend_autoaccept_ft.panel = d_friend_autoaccept;

    /* Group Settings */
    PANEL e_group_topic = {
        .type = PANEL_EDIT, .x = SCALE(10), .y = SCALE(95), .height = SCALE(24), .width = SCALE(-10)};

    edit_group_topic.panel = e_group_topic;

    PANEL d_group_notifications = {
        .type = PANEL_DROPDOWN, .x = SCALE(10), .y = SCALE(155), .height = SCALE(24), .width = SCALE(85)};

    dropdown_notify_groupchats.panel = d_group_notifications;

    /* Friend / Group Page  */
    CREATE_BUTTON(call_decline, SCALE(-186), SCALE(10), BM_LBUTTON_WIDTH, BM_LBUTTON_HEIGHT);
    CREATE_BUTTON(call_audio, SCALE(-124), SCALE(10), BM_LBUTTON_WIDTH, BM_LBUTTON_HEIGHT);
    CREATE_BUTTON(call_video, SCALE(-62), SCALE(10), BM_LBUTTON_WIDTH, BM_LBUTTON_HEIGHT);
    CREATE_BUTTON(group_audio, SCALE(-62), SCALE(10), BM_LBUTTON_WIDTH, BM_LBUTTON_HEIGHT);
    /* bottom left button in chat */
    CREATE_BUTTON(send_file, SCALE(6), SCALE(-46), BM_CHAT_BUTTON_WIDTH, BM_CHAT_BUTTON_HEIGHT);
    /* button to the right of b_chat_left */
    CREATE_BUTTON(send_screenshot, SCALE(8) + BM_CHAT_BUTTON_WIDTH, SCALE(-46), BM_CHAT_BUTTON_WIDTH,
                  BM_CHAT_BUTTON_HEIGHT);
    CREATE_BUTTON(chat_send, SCALE(-6) - BM_CHAT_SEND_WIDTH, SCALE(-46), BM_CHAT_SEND_WIDTH, BM_CHAT_SEND_HEIGHT);

    /* Drop down structs    */
    setfont(FONT_TEXT);
    PANEL d_theme = {.type = PANEL_DROPDOWN, .x = SCALE(10), .y = SCALE(30), .height = SCALE(24), .width = SCALE(120)},
          d_dpi = {.type = PANEL_DROPDOWN, .x = SCALE(150), .y = SCALE(30), .height = SCALE(24), .width = SCALE(200)};

    /* Unsorted */
    PANEL d_audio_in = {.type   = PANEL_DROPDOWN,
                        .x      = SCALE(10),
                        .y      = SCALE(120),
                        .height = SCALE(24),
                        .width  = SCALE(360)},

          d_audio_out = {.type   = PANEL_DROPDOWN,
                         .x      = SCALE(10),
                         .y      = SCALE(180),
                         .height = SCALE(24),
                         .width  = SCALE(360)},

          d_video = {.type = PANEL_DROPDOWN, .x = SCALE(10), .y = SCALE(240), .height = SCALE(24), .width = SCALE(360)},

          d_language = {.type   = PANEL_DROPDOWN,
                        .x      = SCALE(10),
                        .y      = SCALE(177),
                        .height = SCALE(24),
                        .width  = -SCALE(10)},

          d_proxy = {.type = PANEL_DROPDOWN, .x = SCALE(10), .y = SCALE(110), .height = SCALE(24), .width = SCALE(120)},

          d_global_group_notifications = {.type   = PANEL_DROPDOWN,
                                          .x      = UTOX_SCALE(120),
                                          .y      = UTOX_SCALE(15),
                                          .height = UTOX_SCALE(12),
                                          .width  = UTOX_SCALE(50)};

    /* Drop down panels */
    dropdown_audio_in.panel  = d_audio_in;
    dropdown_audio_out.panel = d_audio_out;
    dropdown_video.panel     = d_video;
    dropdown_dpi.panel       = d_dpi;
    dropdown_language.panel  = d_language;
    dropdown_proxy.panel     = d_proxy;
    dropdown_theme.panel     = d_theme;

    dropdown_global_group_notifications.panel = d_global_group_notifications;

    /* Text entry boxes */
    PANEL e_add_id =
              {
                  .type   = PANEL_EDIT,
                  .x      = UTOX_SCALE(5),
                  .y      = UTOX_SCALE(14) + MAIN_TOP,
                  .height = UTOX_SCALE(12),
                  .width  = -SCALE(10),
              },

          e_add_msg =
              {
                  .type   = PANEL_EDIT,
                  .x      = UTOX_SCALE(5),
                  .y      = UTOX_SCALE(38) + MAIN_TOP,
                  .height = UTOX_SCALE(42),
                  .width  = -SCALE(10),
              },

          e_profile_password =
              {
                  .type   = PANEL_EDIT,
                  .x      = UTOX_SCALE(5), /* move the edit depending on what page! */
                  .y      = UTOX_SCALE(44) + (UTOX_SCALE(70) * panel_profile_password.disabled),
                  .height = UTOX_SCALE(12),
                  .width  = -UTOX_SCALE(5),
              },

          /* Message entry box for friends and groups */
        e_msg =
            {
                .type   = PANEL_EDIT,
                .x      = UTOX_SCALE(5) + BM_CHAT_BUTTON_WIDTH * 2, /* Make space for the left button  */
                .y      = -UTOX_SCALE(23),
                .width  = -UTOX_SCALE(32),
                .height = UTOX_SCALE(20),
                /* text is 8 high. 8 * 2.5 = 20. */
            },

          e_msg_group =
              {
                  .type   = PANEL_EDIT,
                  .x      = SCALE(6),
                  .y      = SCALE(-46),
                  .width  = SCALE(-10) - BM_CHAT_SEND_WIDTH,
                  .height = SCALE(40),
              },

          e_search =
              {
                  .type   = PANEL_EDIT,
                  .y      = SIDEBAR_SEARCH_TOP,
                  .x      = SIDEBAR_SEARCH_LEFT,
                  .width  = SIDEBAR_SEARCH_WIDTH,
                  .height = SIDEBAR_SEARCH_HEIGHT,
              },

          e_proxy_ip =
              {
                  .type = PANEL_EDIT, .x = SCALE(140), .y = SCALE(110), .width = SCALE(120), .height = SCALE(24),
              },

          e_proxy_port = {
              .type = PANEL_EDIT, .x = SCALE(270), .y = SCALE(110), .width = SCALE(60), .height = SCALE(24),
          };

    /* Text entry panels */
    edit_name.panel             = e_name;
    edit_status.panel           = e_status;
    edit_toxid.panel            = e_toxid;
    edit_add_id.panel           = e_add_id;
    edit_add_msg.panel          = e_add_msg;
    edit_profile_password.panel = e_profile_password;
    edit_msg.panel              = e_msg;
    edit_msg_group.panel        = e_msg_group;
    edit_search.panel           = e_search;
    edit_proxy_ip.panel         = e_proxy_ip;
    edit_proxy_port.panel       = e_proxy_port;

    setscale();
}

/* Use the preprocessor to build function prototypes for all user interactions
 * These are functions that are (must be) defined elsewehere. The preprocessor in this case creates the prototypes that
 * will then be used by panel_draw_sub to call the correct function
*/
#define FUNC(x, ret, ...)                                                                                              \
    static ret (*x##func[])(void *p, ##__VA_ARGS__) =                                                                  \
        {                                                                                                              \
            (void *)background_##x, (void *)messages_##x, (void *)inline_video_##x,                                    \
            (void *)list_##x,       (void *)button_##x,   (void *)switch_##x,                                          \
            (void *)dropdown_##x,   (void *)edit_##x,     (void *)scroll_##x,                                          \
    };

FUNC(draw, void, int x, int y, int width, int height);
FUNC(mmove, _Bool, int x, int y, int width, int height, int mx, int my, int dx, int dy);
FUNC(mdown, _Bool);
FUNC(mright, _Bool);
FUNC(mwheel, _Bool, int height, double d);
FUNC(mup, _Bool);
FUNC(mleave, _Bool);

#undef FUNC

/* Use the preprocessor to add code to adjust the x,y cords for panels or sub panels.
 * If neg value place x/y from the right/bottom of panel.
 *
 * change the relative
 *
 * if w/h <0 use parent panel width (maybe?)    */
#define FIX_XY_CORDS_FOR_SUBPANELS()                                                                                   \
    {                                                                                                                  \
        int relx = (p->x < 0) ? width + p->x : p->x;                                                                   \
        int rely = (p->y < 0) ? height + p->y : p->y;                                                                  \
        x += relx;                                                                                                     \
        y += rely;                                                                                                     \
        width  = (p->width <= 0) ? width + p->width - relx : p->width;                                                 \
        height = (p->height <= 0) ? height + p->height - rely : p->height;                                             \
    }

static void panel_update(PANEL *p, int x, int y, int width, int height) {
    FIX_XY_CORDS_FOR_SUBPANELS();

    switch (p->type) {
        case PANEL_NONE: {
            if (p == &panel_settings_devices) {
#ifdef ENABLE_MULTIDEVICE
                devices_update_ui();
#endif
            }
            break;
        }

        case PANEL_MESSAGES: {
            if (p->object) {
                MESSAGES *m = p->object;
                m->width    = width;
                messages_updateheight(m, width);
            }
            break;
        }

        default: { break; }
    }

    PANEL **pp = p->child, *subp;
    if (pp) {
        while ((subp = *pp++)) {
            panel_update(subp, x, y, width, height);
        }
    }
}

void ui_size(int width, int height) {
    panel_update(&panel_root, 0, 0, width, height);
    tooltip_reset();
}

void ui_mouseleave(void) {
    panel_mleave(&panel_root);
    tooltip_reset();
    redraw();
}

static void panel_draw_sub(PANEL *p, int x, int y, int width, int height) {
    FIX_XY_CORDS_FOR_SUBPANELS();

    if (p->content_scroll) {
        pushclip(x, y, width, height);
        y -= scroll_gety(p->content_scroll, height);
    }

    if (p->type) {
        drawfunc[p->type - 1](p, x, y, width, height);
    } else {
        if (p->drawfunc) {
            p->drawfunc(x, y, width, height);
        }
    }

    PANEL **pp = p->child, *subp;
    if (pp) {
        while ((subp = *pp++)) {
            if (!subp->disabled) {
                panel_draw_sub(subp, x, y, width, height);
            }
        }
    }

    if (p->content_scroll) {
        popclip();
    }
}

void panel_draw(PANEL *p, int x, int y, int width, int height) {
    FIX_XY_CORDS_FOR_SUBPANELS();

    // pushclip(x, y, width, height);

    if (p->type) {
        drawfunc[p->type - 1](p, x, y, width, height);
    } else {
        if (p->drawfunc) {
            p->drawfunc(x, y, width, height);
        }
    }

    PANEL **pp = p->child, *subp;
    if (pp) {
        while ((subp = *pp++)) {
            if (!subp->disabled) {
                panel_draw_sub(subp, x, y, width, height);
            }
        }
    }

    // popclip();

    dropdown_drawactive();
    contextmenu_draw();
    tooltip_draw();

    enddraw(x, y, width, height);
}

_Bool panel_mmove(PANEL *p, int x, int y, int width, int height, int mx, int my, int dx, int dy) {
    if (p == &panel_root) {
        mouse.x = mx;
        mouse.y = my;
    }

    mx -= (p->x < 0) ? width + p->x : p->x;
    my -= (p->y < 0) ? height + p->y : p->y;
    FIX_XY_CORDS_FOR_SUBPANELS();

    int mmy = my;

    if (p->content_scroll) {
        int scroll_y = scroll_gety(p->content_scroll, height);
        if (my < 0) {
            mmy = -1;
        } else if (my >= height) {
            mmy = 1024 * 1024 * 1024; // large value
        } else {
            mmy = my + scroll_y;
        }
        y -= scroll_y;
        my += scroll_y;
    }

    _Bool draw = p->type ? mmovefunc[p->type - 1](p, x, y, width, height, mx, mmy, dx, dy) : 0;
    // Has to be called before children mmove
    if (p == &panel_root) {
        draw |= tooltip_mmove();
    }
    PANEL **pp = p->child, *subp;
    if (pp) {
        while ((subp = *pp++)) {
            if (!subp->disabled) {
                draw |= panel_mmove(subp, x, y, width, height, mx, my, dx, dy);
            }
        }
    }

    if (p == &panel_root) {
        draw |= contextmenu_mmove(mx, my, dx, dy);
        if (draw) {
            redraw();
        }
    }

    return draw;
}

static _Bool panel_mdown_sub(PANEL *p) {
    if (p->type && mdownfunc[p->type - 1](p)) {
        return 1;
    }

    PANEL **pp = p->child, *subp;
    if (pp) {
        while ((subp = *pp++)) {
            if (!subp->disabled) {
                if (panel_mdown_sub(subp)) {
                    return 1;
                }
            }
        }
    }

    return 0;
}

void panel_mdown(PANEL *p) {
    if (contextmenu_mdown() || tooltip_mdown()) {
        redraw();
        return;
    }

    _Bool   draw = edit_active();
    PANEL **pp   = p->child, *subp;
    if (pp) {
        while ((subp = *pp++)) {
            if (!subp->disabled) {
                if (panel_mdown_sub(subp)) {
                    draw = 1;
                    break;
                }
            }
        }
    }

    if (draw) {
        redraw();
    }
}

_Bool panel_dclick(PANEL *p, _Bool triclick) {
    _Bool draw = 0;
    if (p->type == PANEL_EDIT) {
        draw = edit_dclick((EDIT *)p, triclick);
    } else if (p->type == PANEL_MESSAGES) {
        draw = messages_dclick(p, triclick);
    }

    PANEL **pp = p->child, *subp;
    if (pp) {
        while ((subp = *pp++)) {
            if (!subp->disabled) {
                draw = panel_dclick(subp, triclick);
                if (draw) {
                    break;
                }
            }
        }
    }

    if (draw && p == &panel_root) {
        redraw();
    }

    return draw;
}

_Bool panel_mright(PANEL *p) {
    _Bool   draw = p->type ? mrightfunc[p->type - 1](p) : 0;
    PANEL **pp   = p->child, *subp;
    if (pp) {
        while ((subp = *pp++)) {
            if (!subp->disabled) {
                draw |= panel_mright(subp);
            }
        }
    }

    if (draw && p == &panel_root) {
        redraw();
    }

    return draw;
}

_Bool panel_mwheel(PANEL *p, int x, int y, int width, int height, double d, _Bool smooth) {
    FIX_XY_CORDS_FOR_SUBPANELS();

    _Bool   draw = p->type ? mwheelfunc[p->type - 1](p, height, d) : 0;
    PANEL **pp   = p->child, *subp;
    if (pp) {
        while ((subp = *pp++)) {
            if (!subp->disabled) {
                draw |= panel_mwheel(subp, x, y, width, height, d, smooth);
            }
        }
    }

    if (draw && p == &panel_root) {
        redraw();
    }

    return draw;
}

_Bool panel_mup(PANEL *p) {
    _Bool   draw = p->type ? mupfunc[p->type - 1](p) : 0;
    PANEL **pp   = p->child, *subp;
    if (pp) {
        while ((subp = *pp++)) {
            if (!subp->disabled) {
                draw |= panel_mup(subp);
            }
        }
    }

    if (p == &panel_root) {
        draw |= contextmenu_mup();
        tooltip_mup();
        if (draw) {
            redraw();
        }
    }

    return draw;
}

_Bool panel_mleave(PANEL *p) {
    _Bool   draw = p->type ? mleavefunc[p->type - 1](p) : 0;
    PANEL **pp   = p->child, *subp;
    if (pp) {
        while ((subp = *pp++)) {
            if (!subp->disabled) {
                draw |= panel_mleave(subp);
            }
        }
    }

    if (p == &panel_root) {
        draw |= contextmenu_mleave();
        if (draw) {
            redraw();
        }
    }

    return draw;
}
