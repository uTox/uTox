#include "ui.h"

#include "flist.h"
#include "inline_video.h"
#include "macros.h"
#include "messages.h"
#include "settings.h"

#include "layout/background.h"
#include "layout/create.h"
#include "layout/friend.h"
#include "layout/group.h"
#include "layout/notify.h"
#include "layout/settings.h"
#include "layout/sidebar.h"

#include "native/image.h"
#include "native/ui.h"

#include "ui/button.h"
#include "ui/contextmenu.h"
#include "ui/draw.h"
#include "ui/dropdown.h"
#include "ui/edit.h"
#include "ui/panel.h"
#include "ui/scrollable.h"
#include "ui/switch.h"
#include "ui/text.h"
#include "ui/tooltip.h"

/* These remain for legacy reasons, PANEL_MAIN calls these by default when not given it's own function to call */
static void background_draw(PANEL *UNUSED(p), int UNUSED(x), int UNUSED(y), int UNUSED(width), int UNUSED(height)) {
    return;
}

static bool background_mmove(PANEL *UNUSED(p), int UNUSED(x), int UNUSED(y), int UNUSED(width), int UNUSED(height),
                      int UNUSED(mx), int UNUSED(my), int UNUSED(dx), int UNUSED(dy)) {
    return false;
}

static bool background_mdown(PANEL *UNUSED(p)) {
    return false;
}

static bool background_mright(PANEL *UNUSED(p)) {
    return false;
}

static bool background_mwheel(PANEL *UNUSED(p), int UNUSED(height), double UNUSED(d), bool UNUSED(smooth)) {
    return false;
}

static bool background_mup(PANEL *UNUSED(p)) {
    return false;
}

static bool background_mleave(PANEL *UNUSED(p)) {
    return false;
}

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
    }

    return SPTRFORLANG(settings.language, mis->i18nal);
}

bool maybe_i18nal_string_is_valid(MAYBE_I18NAL_STRING *mis) {
    return (mis->plain.str || ((UI_STRING_ID_INVALID != mis->i18nal) && (mis->i18nal < NUM_STRS)));
}

/***********************************************************************
 *                                                                     *
 * Panel layout size set functions.                                    *
 *                                                                     *
 **********************************************************************/
static void sidepanel_USERBADGE(void) {
    // Converting DEFINES to magic becaues this will be moved to layout/
    // and will then get a different format/selection
    CREATE_BUTTON(avatar, SIDEBAR_AVATAR_LEFT, SIDEBAR_AVATAR_TOP, 40, 40);
    CREATE_BUTTON(name, SIDEBAR_NAME_LEFT, SIDEBAR_NAME_TOP, SIDEBAR_NAME_WIDTH, SIDEBAR_NAME_HEIGHT - 2);
    CREATE_BUTTON(status_msg, SIDEBAR_STATUSMSG_LEFT, SIDEBAR_STATUSMSG_TOP, SIDEBAR_STATUSMSG_WIDTH, SIDEBAR_STATUSMSG_HEIGHT - 2);
    CREATE_BUTTON(usr_state, 200, 10, 25, 45);
}

static void sidepanel_FLIST(void) {
    scrollbar_flist.panel.y      = 0;
    // scrollbar_flist.panel.width  = 230; // TODO remove?
    scrollbar_flist.panel.height = -1;


    CREATE_BUTTON(filter_friends, SIDEBAR_FILTER_FRIENDS_LEFT, SIDEBAR_FILTER_FRIENDS_TOP, SIDEBAR_FILTER_FRIENDS_WIDTH,
                  SIDEBAR_FILTER_FRIENDS_HEIGHT);
    CREATE_EDIT(search, SIDEBAR_SEARCH_LEFT, SIDEBAR_SEARCH_TOP, SIDEBAR_SEARCH_WIDTH, SIDEBAR_SEARCH_HEIGHT);

    CREATE_BUTTON(settings, SIDEBAR_BUTTON_LEFT, ROSTER_BOTTOM, SIDEBAR_BUTTON_WIDTH, SIDEBAR_BUTTON_HEIGHT);
    CREATE_BUTTON(add_new_contact, SIDEBAR_BUTTON_LEFT, ROSTER_BOTTOM, SIDEBAR_BUTTON_WIDTH, SIDEBAR_BUTTON_HEIGHT);
    button_add_new_contact.panel.disabled = true;
}


static void settings_PROFILE(void) {
    panel_settings_profile.y = 32;

    CREATE_EDIT(name, 10, 30, -10, 24);

    CREATE_EDIT(status_msg, 10, 85, -10, 24);

    CREATE_EDIT(toxid, 10, 140, -10, 24);
    CREATE_BUTTON(copyid, 66, 117, _BM_SBUTTON_WIDTH, _BM_SBUTTON_HEIGHT);

    CREATE_DROPDOWN(language, 10, 195, 24, -10);
}

static void settings_UI(void) {
    panel_settings_ui.y            = 32;

    CREATE_DROPDOWN(theme, 10, 30, 24, 120);

    CREATE_DROPDOWN(dpi,   150, 30, 24, 200);

    CREATE_SWITCH(save_chat_history, 10, 60,  _BM_SWITCH_WIDTH, _BM_SWITCH_HEIGHT);
    CREATE_SWITCH(close_to_tray,     10, 90,  _BM_SWITCH_WIDTH, _BM_SWITCH_HEIGHT);
    CREATE_SWITCH(start_in_tray,     10, 120, _BM_SWITCH_WIDTH, _BM_SWITCH_HEIGHT);
    CREATE_SWITCH(auto_startup,      10, 150, _BM_SWITCH_WIDTH, _BM_SWITCH_HEIGHT);
    CREATE_SWITCH(mini_contacts,     10, 180, _BM_SWITCH_WIDTH, _BM_SWITCH_HEIGHT);
}

static void settings_AV(void) {
    panel_settings_av.y = 32;

    CREATE_SWITCH(push_to_talk, 10, 10, _BM_SWITCH_WIDTH, _BM_SWITCH_HEIGHT);

    #ifndef AUDIO_FILTERING
        const uint16_t start_draw_y = 30;
        const uint16_t preview_button_pos_y = 245;
    #else
        const uint16_t start_draw_y = 60;
        const uint16_t preview_button_pos_y = 275;
        CREATE_SWITCH(audio_filtering, 10, 40, _BM_SWITCH_WIDTH, _BM_SWITCH_HEIGHT);
    #endif


    const uint16_t draw_y_vect = 30;
    CREATE_DROPDOWN(audio_in,  10, (start_draw_y + draw_y_vect + 5), 24, 360);
    CREATE_DROPDOWN(audio_out, 10, (start_draw_y + draw_y_vect + 57), 24, 360);
    CREATE_EDIT(video_fps,     10, (start_draw_y + draw_y_vect + 110), 360, 24);
    CREATE_DROPDOWN(video,     10, (start_draw_y + draw_y_vect + 162), 24, 360);

    CREATE_BUTTON(callpreview,  10, (preview_button_pos_y + 35), _BM_LBUTTON_WIDTH, _BM_LBUTTON_HEIGHT);
    CREATE_BUTTON(videopreview, 70, (preview_button_pos_y + 35), _BM_LBUTTON_WIDTH, _BM_LBUTTON_HEIGHT);
}

static void settings_NOTIFY(void) {
    panel_settings_notifications.y = 32;

    CREATE_SWITCH(audible_notifications,        10,  10, _BM_SWITCH_WIDTH, _BM_SWITCH_HEIGHT);
    CREATE_SWITCH(status_notifications,         10,  40, _BM_SWITCH_WIDTH, _BM_SWITCH_HEIGHT);
    CREATE_SWITCH(typing_notes,                 10,  70, _BM_SWITCH_WIDTH, _BM_SWITCH_HEIGHT);
    CREATE_DROPDOWN(global_group_notifications, 10, 125,               24,               100);
}

static void settings_ADV(void) {
    panel_settings_adv.y = 32;

    CREATE_SWITCH(ipv6, 10, 27, _BM_SWITCH_WIDTH, _BM_SWITCH_HEIGHT);
    CREATE_SWITCH(udp,  10, 57, _BM_SWITCH_WIDTH, _BM_SWITCH_HEIGHT);

    CREATE_SWITCH(proxy,       10, 87,  _BM_SWITCH_WIDTH, _BM_SWITCH_HEIGHT);
    CREATE_EDIT(proxy_ip,      230, 87, 120, 24);
    CREATE_EDIT(proxy_port,    360, 87, 60,  24);
    CREATE_SWITCH(proxy_force, 10, 117, _BM_SWITCH_WIDTH, _BM_SWITCH_HEIGHT);

    CREATE_SWITCH(auto_update,           10, 147, _BM_SWITCH_WIDTH, _BM_SWITCH_HEIGHT);
    CREATE_SWITCH(block_friend_requests, 10, 177, _BM_SWITCH_WIDTH, _BM_SWITCH_HEIGHT);

    CREATE_BUTTON(show_password_settings, 10,  207, _BM_SBUTTON_WIDTH, _BM_SBUTTON_HEIGHT);

    const int show_nospam_x = 30 + UN_SCALE(MAX(UTOX_STR_WIDTH(SHOW_UI_PASSWORD), UTOX_STR_WIDTH(HIDE_UI_PASSWORD)));
    CREATE_BUTTON(show_nospam, show_nospam_x, 207, _BM_SBUTTON_WIDTH, _BM_SBUTTON_HEIGHT);

    CREATE_EDIT(nospam,           10,  265, -10, 24);
    CREATE_BUTTON(change_nospam,  10,  295, _BM_SBUTTON_WIDTH, _BM_SBUTTON_HEIGHT);

    const int revert_nospam_x = 30 + UN_SCALE(UTOX_STR_WIDTH(RANDOMIZE_NOSPAM));
    CREATE_BUTTON(revert_nospam, revert_nospam_x, 295, _BM_SBUTTON_WIDTH, _BM_SBUTTON_HEIGHT);

    CREATE_EDIT(profile_password, 10,  85, -10, 24);
    CREATE_BUTTON(lock_uTox,      10,  295, _BM_SBUTTON_WIDTH, _BM_SBUTTON_HEIGHT);
}

void ui_set_scale(uint8_t scale) {
    if (scale >= 5 && scale <= 25) {
        ui_scale = scale;
    } else if (scale != 0) {
        return ui_set_scale(10);
    }
}

void ui_rescale(uint8_t scale) {
    ui_set_scale(scale);

    flist_re_scale();
    setscale_fonts();
    setfont(FONT_SELF_NAME);

    /* DEFAULT positions */

    panel_main.y = 0;

    scrollbar_settings.panel.y        = 32;  /* TODO magic numbers are bad */
    scrollbar_settings.content_height = 300; /* TODO magic numbers are bad */

    panel_settings_master.y        = 0;
    panel_settings_devices.y       = 32;
    panel_settings_adv.y           = 32;

    scrollbar_friend.panel.y      = MAIN_TOP;
    scrollbar_friend.panel.height = CHAT_BOX_TOP;
    messages_friend.y             = MAIN_TOP;
    messages_friend.height        = CHAT_BOX_TOP - 10;
    messages_friend.width         = -SCROLL_WIDTH;

    scrollbar_group.panel.y      = MAIN_TOP;
    scrollbar_group.panel.height = CHAT_BOX_TOP;
    messages_group.y             = MAIN_TOP;
    messages_group.height        = CHAT_BOX_TOP;
    messages_group.width         = -SCROLL_WIDTH;

    setfont(FONT_SELF_NAME);

    sidepanel_USERBADGE();
    sidepanel_FLIST();

    settings_PROFILE();
    settings_UI();
    settings_AV();
    settings_NOTIFY();
    settings_ADV();

    // FIXME for testing, remove
    CREATE_BUTTON(notify_create, 2, 2, BM_SBUTTON_WIDTH, BM_SBUTTON_HEIGHT);
    CREATE_BUTTON(notify_one, 0, -50, 40, 50);
    CREATE_BUTTON(notify_two, 200, -50, 40, 50);
    CREATE_BUTTON(notify_three, -40, -50, 40, 50);

    CREATE_BUTTON(move_notify, -40, -40, 40, 40);


    /* Setting pages */
    uint32_t settings_x = 4;
    CREATE_BUTTON(settings_sub_profile,         settings_x, 0, 12, 28);
    settings_x += 20 + UN_SCALE(UTOX_STR_WIDTH(PROFILE_BUTTON));

#ifdef ENABLE_MULTIDEVICE
    CREATE_BUTTON(settings_sub_devices,         settings_x, 0, 12, 28);
    settings_x += 20 + UN_SCALE(UTOX_STR_WIDTH(DEVICES_BUTTON));
#endif

    CREATE_BUTTON(settings_sub_ui,              settings_x, 0, 12, 28);
    settings_x += 20 + UN_SCALE(UTOX_STR_WIDTH(USER_INTERFACE_BUTTON));

    CREATE_BUTTON(settings_sub_av,              settings_x, 0, 12, 28);
    settings_x += 20 + UN_SCALE(UTOX_STR_WIDTH(AUDIO_VIDEO_BUTTON));

    CREATE_BUTTON(settings_sub_notifications,   settings_x, 0, 12, 28);
    settings_x += 20 + UN_SCALE(UTOX_STR_WIDTH(NOTIFICATIONS_BUTTON));

    CREATE_BUTTON(settings_sub_adv,             settings_x, 0, 12, 28);


    /* Devices */
    CREATE_BUTTON(add_new_device_to_self, -10 - BM_SBUTTON_WIDTH, 28, BM_SBUTTON_WIDTH, BM_SBUTTON_HEIGHT);

    CREATE_EDIT(add_new_device_to_self, 10, 27, 0 - UTOX_STR_WIDTH(ADD) - BM_SBUTTON_WIDTH, 24);


    /* Friend Add Page */
    CREATE_BUTTON(send_friend_request, -10 - _BM_SBUTTON_WIDTH, MAIN_TOP + 168, _BM_SBUTTON_WIDTH,
                  _BM_SBUTTON_HEIGHT);
    CREATE_BUTTON(accept_friend, -60, -80, _BM_SBUTTON_WIDTH, _BM_SBUTTON_HEIGHT);

    /* Friend Settings Page */
    CREATE_BUTTON(export_chatlog, 10, 208, _BM_SBUTTON_WIDTH, _BM_SBUTTON_HEIGHT);

    CREATE_EDIT(friend_pubkey,          10, 88, -10, 24);
    CREATE_EDIT(friend_alias,           10, 138, -10, 24);

    CREATE_SWITCH(friend_autoaccept_ft, 10, 168, _BM_SWITCH_WIDTH, _BM_SWITCH_HEIGHT);

    /* Group Settings */
    CREATE_EDIT(group_topic, 10, 95, -10, 24);

    /* Friend / Group Page  */
    CREATE_BUTTON(call_decline, -186, 10, _BM_LBUTTON_WIDTH, _BM_LBUTTON_HEIGHT);
    CREATE_BUTTON(call_audio,   -124, 10, _BM_LBUTTON_WIDTH, _BM_LBUTTON_HEIGHT);
    CREATE_BUTTON(call_video,    -62, 10, _BM_LBUTTON_WIDTH, _BM_LBUTTON_HEIGHT);
    CREATE_BUTTON(group_audio,   -62, 10, _BM_LBUTTON_WIDTH, _BM_LBUTTON_HEIGHT);

    CREATE_BUTTON(send_file,         6, -46, _BM_CHAT_BUTTON_WIDTH, _BM_CHAT_BUTTON_HEIGHT);
    CREATE_BUTTON(send_screenshot,   8 + _BM_CHAT_BUTTON_WIDTH, -46, _BM_CHAT_BUTTON_WIDTH, _BM_CHAT_BUTTON_HEIGHT);

    CREATE_BUTTON(chat_send_friend, -6 - _BM_CHAT_SEND_WIDTH, -46, _BM_CHAT_SEND_WIDTH, _BM_CHAT_SEND_HEIGHT);
    CREATE_BUTTON(chat_send_group,  -6 - _BM_CHAT_SEND_WIDTH, -46, _BM_CHAT_SEND_WIDTH, _BM_CHAT_SEND_HEIGHT);

    setfont(FONT_TEXT);

    // Add friend panel
    CREATE_EDIT(add_new_friend_id, 10, 28 + MAIN_TOP, -10, 24);
    CREATE_EDIT(add_new_friend_msg, 10, 76 + MAIN_TOP, -10, 84);

    /* Message entry box for friends and groups */
    CREATE_EDIT(chat_msg_friend, 10 + _BM_CHAT_BUTTON_WIDTH * 2, /* Make space for the left button  */
                -46, -64, 40); /* text is 8 high. 8 * 2.5 = 20. */

    CREATE_EDIT(chat_msg_group, 6, -46, -10 - BM_CHAT_SEND_WIDTH, 40);

    /* Confirm deletion */
    CREATE_BUTTON(confirm_deletion, 10, MAIN_TOP + 40, _BM_SBUTTON_WIDTH, _BM_SBUTTON_HEIGHT);
    CREATE_BUTTON(deny_deletion,    110, MAIN_TOP + 40, _BM_SBUTTON_WIDTH, _BM_SBUTTON_HEIGHT);

    setscale();
}

/* Use the preprocessor to build function prototypes for all user interactions
 * These are functions that are (must be) defined elsewehere. The preprocessor in this case creates the prototypes that
 * will then be used by panel_draw_core to call the correct function
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
#define FIX_XY_CORDS_FOR_SUBPANELS() \
    { \
        int relx = (p->x < 0) ? width + SCALE(p->x) : SCALE(p->x); \
        int rely = (p->y < 0) ? height + SCALE(p->y) : SCALE(p->y); \
        x += relx; \
        y += rely; \
        width  = (p->width <= 0) ? width + SCALE(p->width) - relx : MIN(width, SCALE(p->width)); \
        height = (p->height <= 0) ? height + SCALE(p->height) - rely : MIN(height, SCALE(p->height)); \
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

        default: {
            break;
        }
    }

    PANEL **pp = p->child;
    if (pp) {
        if (p->update) {
            p->update(width, height, ui_scale);
        }

        PANEL *subp;
        while ((subp = *pp++)) {
            panel_update(subp, x, y, width, height);
        }
    }
}

void draw_avatar_image(NATIVE_IMAGE *image, int x, int y, uint32_t width, uint32_t height, uint32_t targetwidth,
                       uint32_t targetheight)
{
    /* get smallest of width or height */
    const double scale = (width > height) ? (double)targetheight / height : (double)targetwidth / width;

    image_set_scale(image, scale);
    image_set_filter(image, FILTER_BILINEAR);

    /* set position to show the middle of the image in the center  */
    const int xpos = (int)((double)width * scale / 2 - (double)targetwidth / 2);
    const int ypos = (int)((double)height * scale / 2 - (double)targetheight / 2);

    draw_image(image, x, y, targetwidth, targetheight, xpos, ypos);

    image_set_scale(image, 1.0);
    image_set_filter(image, FILTER_NEAREST);
}

void ui_size(int width, int height) {
    panel_update(&panel_root, 0, 0, width, height);
    tooltip_reset();

    panel_side_bar.disabled = false;
    panel_main.x = 50;

    if (settings.magic_flist_enabled) {
        if (width <= panel_flist.width * 2 || height > width) {
            panel_side_bar.disabled = true;
            panel_main.x = 0;
        }
    }
}

void ui_mouseleave(void) {
    panel_mleave(&panel_root);
    tooltip_reset();
    redraw();
}

static void panel_draw_core(PANEL *p, int x, int y, int width, int height) {
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

    PANEL **pp = p->child;
    if (pp) {
        PANEL *subp;
        while ((subp = *pp++)) {
            if (!subp->disabled) {
                panel_draw_core(subp, x, y, width, height);
            }
        }
    }

    if (p->content_scroll) {
        popclip();
    }
}

void panel_draw(PANEL *p, int x, int y, int width, int height) {
    FIX_XY_CORDS_FOR_SUBPANELS();

    panel_draw_core(p, x, y, width, height);

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

    mx -= (p->x < 0) ? width + SCALE(p->x) : SCALE(p->x);
    my -= (p->y < 0) ? height + SCALE(p->y) : SCALE(p->y);
    FIX_XY_CORDS_FOR_SUBPANELS();

    int mmy = my;

    if (p->content_scroll) {
        const int scroll_y = scroll_gety(p->content_scroll, height);
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

    bool draw = p->type ? mmovefunc[p->type - 1](p, x, y, width, height, mx, mmy, dx, dy) : false;
    // Has to be called before children mmove
    if (p == &panel_root) {
        draw |= tooltip_mmove();
    }
    PANEL **pp = p->child;
    if (pp) {
        PANEL *subp;
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
        return true;
    }

    PANEL **pp = p->child;
    if (pp) {
        PANEL *subp;
        while ((subp = *pp++)) {
            if (!subp->disabled) {
                if (panel_mdown_sub(subp)) {
                    return true;
                }
            }
        }
    }

    return false;
}

void panel_mdown(PANEL *p) {
    if (contextmenu_mdown() || tooltip_mdown()) {
        redraw();
        return;
    }

    bool draw = edit_active();

    PANEL **pp = p->child;

    if (pp) {
        PANEL *subp;
        while ((subp = *pp++)) {
            if (!subp->disabled) {
                draw |= panel_mdown_sub(subp);
            }
        }
    }

    if (draw) {
        redraw();
    }
}

bool panel_dclick(PANEL *p, bool triclick) {
    bool draw = false;
    if (p->type == PANEL_EDIT) {
        draw = edit_dclick((EDIT *)p, triclick);
    } else if (p->type == PANEL_MESSAGES) {
        draw = messages_dclick(p, triclick);
    }

    PANEL **pp = p->child;
    if (pp) {
        PANEL *subp;
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
    bool draw = p->type ? mrightfunc[p->type - 1](p) : false;
    PANEL **pp = p->child;
    if (pp) {
        PANEL *subp;
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

    bool draw = p->type ? mwheelfunc[p->type - 1](p, height, d) : false;
    PANEL **pp = p->child;
    if (pp) {
        PANEL *subp;
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
    if (p == &panel_root && contextmenu_mup()) {
        tooltip_mup();
        redraw();
        return true;
    }

    bool draw = p->type ? mupfunc[p->type - 1](p) : false;
    PANEL **pp = p->child;
    if (pp) {
        PANEL *subp;
        while ((subp = *pp++)) {
            if (!subp->disabled) {
                draw |= panel_mup(subp);
            }
        }
    }

    if (p == &panel_root) {
        tooltip_mup();
        if (draw) {
            redraw();
        }
    }

    return draw;
}

bool panel_mleave(PANEL *p) {
    bool    draw = p->type ? mleavefunc[p->type - 1](p) : false;
    PANEL **pp   = p->child;
    if (pp) {
        PANEL *subp;
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
