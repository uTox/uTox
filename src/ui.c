// ui.c
#include "ui.h"

#include "flist.h"
#include "friend.h"
#include "inline_video.h"
#include "main.h"
#include "theme.h"

#include "ui/buttons.h"
#include "ui/contextmenu.h"
#include "ui/draw_helpers.h"
#include "ui/dropdowns.h"
#include "ui/layout_tree.h"
#include "ui/switches.h"
#include "ui/text.h"
#include "ui/tooltip.h"

// Application-wide language setting
UTOX_LANG LANG;

/***** MAYBE_I18NAL_STRING helpers start *****/

void maybe_i18nal_string_set_plain(MAYBE_I18NAL_STRING *mis, char *str, uint16_t length) {
    mis->i18nal       = UI_STRING_ID_INVALID;
    mis->plain.length = length;
    mis->plain.str    = str;
}

void maybe_i18nal_string_set_i18nal(MAYBE_I18NAL_STRING *mis, UTOX_I18N_STR string_id) {
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

bool maybe_i18nal_string_is_valid(MAYBE_I18NAL_STRING *mis) {
    return (mis->plain.str || ((UI_STRING_ID_INVALID != mis->i18nal) && (mis->i18nal < NUM_STRS)));
}

/***** MAYBE_I18NAL_STRING helpers end *****/

#define CREATE_BUTTON(n, a, b, w, h)                                   \
    PANEL b_##n = {                                                    \
        .type = PANEL_BUTTON, .x = a, .y = b, .width = w, .height = h, \
    };                                                                 \
    button_##n.panel = b_##n

#define CREATE_EDIT(n, a, b, w, h)                                   \
    PANEL e_##n = {                                                  \
        .type = PANEL_EDIT, .x = a, .y = b, .width = w, .height = h, \
    };                                                               \
    edit_##n.panel = e_##n

void ui_set_scale(uint8_t scale) {
    if (ui_scale == scale) {
        return;
    }

    ui_scale = scale;
    flist_re_scale();
    setscale_fonts();
    setfont(FONT_SELF_NAME);

    /* DEFAULT positions */
    panel_side_bar.x     = 0;
    panel_side_bar.y     = 0;
    panel_side_bar.width = SIDEBAR_WIDTH;

    scrollbar_flist.panel.y      = ROSTER_TOP;
    scrollbar_flist.panel.width  = MAIN_LEFT;
    scrollbar_flist.panel.height = ROSTER_BOTTOM;

    panel_flist.x      = 0;
    panel_flist.y      = ROSTER_TOP;
    panel_flist.width  = MAIN_LEFT;
    panel_flist.height = ROSTER_BOTTOM;

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


    // User Interface tab

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


    // Network tab

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


    // Audio & Video tab

    PANEL panel_switch_audible_notifications = {
        .type   = PANEL_SWITCH,
        .x      = SCALE(-10) - BM_SWITCH_WIDTH,
        .y      = SCALE(0),
        .width  = BM_SWITCH_WIDTH,
        .height = BM_SWITCH_HEIGHT,
    };

    PANEL panel_switch_push_to_talk = {
        .type   = PANEL_SWITCH,
        .x      = SCALE(-10) - BM_SWITCH_WIDTH,
        .y      = SCALE(30),
        .width  = BM_SWITCH_WIDTH,
        .height = BM_SWITCH_HEIGHT,
    };

    PANEL panel_switch_status_notifications = {
        .type   = PANEL_SWITCH,
        .x      = SCALE(-10) - BM_SWITCH_WIDTH,
        .y      = SCALE(60),
        .width  = BM_SWITCH_WIDTH,
        .height = BM_SWITCH_HEIGHT 
    };

#ifdef AUDIO_FILTERING
    PANEL panel_switch_audio_filtering = {
        .type   = PANEL_SWITCH,
        .x      = SCALE(-10) - BM_SWITCH_WIDTH,
        .y      = SCALE(90),
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
    switch_status_notifications.panel  = panel_switch_status_notifications;

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
    b_add_new_contact.disabled = true;
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

    CREATE_BUTTON(settings_sub_av, settings_tab_x, 1, SCALE(18) + UTOX_STR_WIDTH(AUDIO_VIDEO_BUTTON), SCALE(28));

    /* Profile              */
    CREATE_BUTTON(copyid, SCALE(66), SCALE(106), BM_SBUTTON_WIDTH, BM_SBUTTON_HEIGHT);
    CREATE_BUTTON(show_password_settings, SCALE(145), SCALE(206), BM_SBUTTON_WIDTH, BM_SBUTTON_HEIGHT);
    CREATE_BUTTON(lock_uTox, SCALE(10), SCALE(260), BM_SBUTTON_WIDTH, BM_SBUTTON_HEIGHT);

    PANEL e_name = {.type = PANEL_EDIT, .x = SCALE(10), .y = SCALE(27), .height = SCALE(24), .width = -SCALE(10) },
          e_status = {.type = PANEL_EDIT, .x = SCALE(10), .y = SCALE(76), .height = SCALE(24), .width = -SCALE(10) },
          e_toxid = {.type = PANEL_EDIT, .x = SCALE(10), .y = SCALE(126), .height = SCALE(24), .width = -SCALE(10) };

    edit_name.panel   = e_name;
    edit_status.panel = e_status;
    edit_toxid.panel  = e_toxid;

    /* Devices              */
    CREATE_BUTTON(add_new_device_to_self, SCALE(-10) - BM_SBUTTON_WIDTH, SCALE(28), BM_SBUTTON_WIDTH, BM_SBUTTON_HEIGHT);

    CREATE_EDIT(add_new_device_to_self, SCALE(10), SCALE(27), SCALE(0) - UTOX_STR_WIDTH(ADD) - BM_SBUTTON_WIDTH,
                SCALE(24));

    /* Network              */

    /* User Interface       */

    /* Audio/Video          */
    CREATE_BUTTON(callpreview, SCALE(10), SCALE(360), BM_LBUTTON_WIDTH, BM_LBUTTON_HEIGHT);
    CREATE_BUTTON(videopreview, SCALE(70), SCALE(360), BM_LBUTTON_WIDTH, BM_LBUTTON_HEIGHT);

    /* Friend Add Page      */
    CREATE_BUTTON(send_friend_request, SCALE(-10) - BM_SBUTTON_WIDTH, MAIN_TOP + UTOX_SCALE(84), BM_SBUTTON_WIDTH,
                  BM_SBUTTON_HEIGHT);
    CREATE_BUTTON(accept_friend, SCALE(10), MAIN_TOP + SCALE(10), BM_SBUTTON_WIDTH, BM_SBUTTON_HEIGHT);

    /* Friend Settings Page */
    CREATE_BUTTON(export_chatlog, SCALE(10), SCALE(220), BM_SBUTTON_WIDTH, BM_SBUTTON_HEIGHT);

    PANEL e_friend_pubkey = {.type = PANEL_EDIT, .x = SCALE(10), .y = SCALE(88), .height = SCALE(24), .width = -SCALE(10) };

    edit_friend_pubkey.panel = e_friend_pubkey;

    PANEL e_friend_alias = {.type = PANEL_EDIT, .x = SCALE(10), .y = SCALE(138), .height = SCALE(24), .width = SCALE(-10) };

    edit_friend_alias.panel = e_friend_alias;

    PANEL d_friend_autoaccept = {
        .type = PANEL_DROPDOWN, .x = SCALE(10), .y = SCALE(188), .height = SCALE(24), .width = SCALE(40)
    };

    dropdown_friend_autoaccept_ft.panel = d_friend_autoaccept;

    /* Group Settings */
    PANEL e_group_topic = {.type = PANEL_EDIT, .x = SCALE(10), .y = SCALE(95), .height = SCALE(24), .width = SCALE(-10) };

    edit_group_topic.panel = e_group_topic;

    PANEL d_group_notifications = {
        .type = PANEL_DROPDOWN, .x = SCALE(10), .y = SCALE(155), .height = SCALE(24), .width = SCALE(85)
    };

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

    /* Drop down structs */

    setfont(FONT_TEXT);

    // Profile tab

    PANEL d_language = {
        .type = PANEL_DROPDOWN,
        .x = SCALE(10),
        .y = SCALE(177),
        .height = SCALE(24),
        .width = -SCALE(10)
    };


    // Network tab

    PANEL d_proxy = {
        .type = PANEL_DROPDOWN,
        .x = SCALE(10),
        .y = SCALE(110),
        .height = SCALE(24),
        .width = SCALE(120) 
    };


    // User Interface tab

    PANEL d_theme = {
        .type = PANEL_DROPDOWN,
        .x = SCALE(10),
        .y = SCALE(30),
        .height = SCALE(24),
        .width = SCALE(120)
    };

    PANEL d_dpi = {
        .type = PANEL_DROPDOWN,
        .x = SCALE(150),
        .y = SCALE(30),
        .height = SCALE(24),
        .width = SCALE(200)
    };


    // Audio & Video tab

    // Each element is draw_pos_y_inc units apart and they start draw_pos_y down.
    uint16_t draw_pos_y = 120;
    const uint16_t draw_pos_y_inc = 60;

    // No idea why, but this dropdown obeys different rules than the rest.
    PANEL d_global_group_notifications = {
        .type   = PANEL_DROPDOWN,
        .x      = UTOX_SCALE(5),
        .y      = UTOX_SCALE(60),
        .height = UTOX_SCALE(12),
        .width  = UTOX_SCALE(50) 
    };
    draw_pos_y += draw_pos_y_inc;

    PANEL d_audio_in = {
        .type = PANEL_DROPDOWN,
        .x = SCALE(10),
        .y = SCALE(draw_pos_y),
        .height = SCALE(24),
        .width = SCALE(360) 
    };
    draw_pos_y += draw_pos_y_inc;

    PANEL d_audio_out = {
        .type = PANEL_DROPDOWN,
        .x = SCALE(10),
        .y = SCALE(draw_pos_y),
        .height = SCALE(24),
        .width = SCALE(360) 
    };
    draw_pos_y += draw_pos_y_inc;

    PANEL d_video = {
        .type = PANEL_DROPDOWN,
        .x = SCALE(10),
        .y = SCALE(draw_pos_y),
        .height = SCALE(24),
        .width = SCALE(360) 
    };

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
#define MAKE_FUNC(ret, x, ...)                                                                                          \
    static ret (*x##func[])(void *p, ##__VA_ARGS__) = {                                                                 \
        (void *)background_##x, (void *)messages_##x, (void *)inline_video_##x, (void *)flist_##x,  (void *)button_##x, \
        (void *)switch_##x,     (void *)dropdown_##x, (void *)edit_##x,         (void *)scroll_##x,                     \
    };

MAKE_FUNC(void, draw, int x, int y, int width, int height);
MAKE_FUNC(bool, mmove, int x, int y, int width, int height, int mx, int my, int dx, int dy);
MAKE_FUNC(bool, mdown);
MAKE_FUNC(bool, mright);
MAKE_FUNC(bool, mwheel, int height, double d);
MAKE_FUNC(bool, mup);
MAKE_FUNC(bool, mleave);

#undef MAKE_FUNC

/* Use the preprocessor to add code to adjust the x,y cords for panels or sub panels.
 * If neg value place x/y from the right/bottom of panel.
 *
 * change the relative
 *
 * if w/h <0 use parent panel width (maybe?)    */
#define FIX_XY_CORDS_FOR_SUBPANELS()                                       \
    {                                                                      \
        int relx = (p->x < 0) ? width + p->x : p->x;                       \
        int rely = (p->y < 0) ? height + p->y : p->y;                      \
        x += relx;                                                         \
        y += rely;                                                         \
        width  = (p->width <= 0) ? width + p->width - relx : p->width;     \
        height = (p->height <= 0) ? height + p->height - rely : p->height; \
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

bool panel_mmove(PANEL *p, int x, int y, int width, int height, int mx, int my, int dx, int dy) {
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

    bool draw = p->type ? mmovefunc[p->type - 1](p, x, y, width, height, mx, mmy, dx, dy) : 0;
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

static bool panel_mdown_sub(PANEL *p) {
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

    bool    draw = edit_active();
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

bool panel_dclick(PANEL *p, bool triclick) {
    bool draw = 0;
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

bool panel_mright(PANEL *p) {
    bool    draw = p->type ? mrightfunc[p->type - 1](p) : 0;
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

bool panel_mwheel(PANEL *p, int x, int y, int width, int height, double d, bool smooth) {
    FIX_XY_CORDS_FOR_SUBPANELS();

    bool    draw = p->type ? mwheelfunc[p->type - 1](p, height, d) : 0;
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

bool panel_mup(PANEL *p) {
    bool    draw = p->type ? mupfunc[p->type - 1](p) : 0;
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

bool panel_mleave(PANEL *p) {
    bool    draw = p->type ? mleavefunc[p->type - 1](p) : 0;
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
