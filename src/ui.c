#include "ui.h"

#include "flist.h"
#include "inline_video.h"
#include "macros.h"
#include "messages.h"

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

/***********************************************************************
 *                                                                     *
 * Panel layout size set functions.                                    *
 *                                                                     *
 **********************************************************************/
static void sidepanel_USERBADGE(void) {
    panel_side_bar.x     = 0;
    panel_side_bar.y     = 0;
    panel_side_bar.width = SIDEBAR_WIDTH;

    CREATE_BUTTON(avatar, SIDEBAR_AVATAR_LEFT, SIDEBAR_AVATAR_TOP, BM_CONTACT_WIDTH, BM_CONTACT_WIDTH);
    CREATE_BUTTON(name, SIDEBAR_NAME_LEFT, SIDEBAR_NAME_TOP, SIDEBAR_NAME_WIDTH, SIDEBAR_NAME_HEIGHT - SCALE(2));
    CREATE_BUTTON(status_msg, SIDEBAR_STATUSMSG_LEFT, SIDEBAR_STATUSMSG_TOP,
                  (SELF_STATUS_ICON_LEFT - SIDEBAR_STATUSMSG_LEFT - SCALE(2)), SIDEBAR_STATUSMSG_HEIGHT - SCALE(2));
    CREATE_BUTTON(usr_state, SELF_STATUS_ICON_LEFT, SELF_STATUS_ICON_TOP, BM_STATUSAREA_WIDTH, BM_STATUSAREA_HEIGHT);
}

static void sidepanel_FLIST(void) {
    scrollbar_flist.panel.y      = ROSTER_TOP;
    scrollbar_flist.panel.width  = MAIN_LEFT;
    scrollbar_flist.panel.height = ROSTER_BOTTOM;

    panel_flist.x      = 0;
    panel_flist.y      = ROSTER_TOP;
    panel_flist.width  = MAIN_LEFT;
    panel_flist.height = ROSTER_BOTTOM;


    CREATE_BUTTON(filter_friends, SIDEBAR_FILTER_FRIENDS_LEFT, SIDEBAR_FILTER_FRIENDS_TOP, SIDEBAR_FILTER_FRIENDS_WIDTH,
                  SIDEBAR_FILTER_FRIENDS_HEIGHT);
    CREATE_EDIT(search, SIDEBAR_SEARCH_LEFT, SIDEBAR_SEARCH_TOP, SIDEBAR_SEARCH_WIDTH, SIDEBAR_SEARCH_HEIGHT);

    CREATE_BUTTON(settings, SIDEBAR_BUTTON_LEFT, ROSTER_BOTTOM, SIDEBAR_BUTTON_WIDTH, SIDEBAR_BUTTON_HEIGHT);
    CREATE_BUTTON(add_new_contact, SIDEBAR_BUTTON_LEFT, ROSTER_BOTTOM, SIDEBAR_BUTTON_WIDTH, SIDEBAR_BUTTON_HEIGHT);
    button_add_new_contact.panel.disabled = true;
}


static void settings_PROFILE(void) {
    panel_settings_profile.y = PANEL_SETTINGS_OFFSET_Y;

    int step_y = SCALE(55);
    int offset_y = SCALE(30);

    CREATE_EDIT(name,       UI_OFFSET_X, offset_y + step_y*0, SCALE(-10), SCALE(24));
    CREATE_EDIT(status_msg, UI_OFFSET_X, offset_y + step_y*1, SCALE(-10), SCALE(24));
    CREATE_EDIT(toxid,      UI_OFFSET_X, offset_y + step_y*2, SCALE(-10), SCALE(24));

    CREATE_BUTTON(copyid, UI_OFFSET_X + SCALE(60), PANEL_SETTINGS_OFFSET_Y + offset_y + step_y, BM_SBUTTON_WIDTH,
                  BM_SBUTTON_HEIGHT);

    CREATE_DROPDOWN(language, UI_OFFSET_X, offset_y + step_y*3, SCALE(24), SCALE(-10));
}

static void settings_UI(void) {
    panel_settings_ui.y = PANEL_SETTINGS_OFFSET_Y;

    CREATE_DROPDOWN(theme, UI_OFFSET_X, UI_OFFSET_Y, SCALE(24), SCALE(120));
    CREATE_DROPDOWN(dpi,   SCALE(150),  UI_OFFSET_Y, SCALE(24), SCALE(200));

    CREATE_SWITCH(save_chat_history, UI_OFFSET_X, UI_OFFSET_Y*2, BM_SWITCH_WIDTH, BM_SWITCH_HEIGHT);
    CREATE_SWITCH(close_to_tray,     UI_OFFSET_X, UI_OFFSET_Y*3, BM_SWITCH_WIDTH, BM_SWITCH_HEIGHT);
    CREATE_SWITCH(start_in_tray,     UI_OFFSET_X, UI_OFFSET_Y*4, BM_SWITCH_WIDTH, BM_SWITCH_HEIGHT);
    CREATE_SWITCH(auto_startup,      UI_OFFSET_X, UI_OFFSET_Y*5, BM_SWITCH_WIDTH, BM_SWITCH_HEIGHT);
    CREATE_SWITCH(mini_contacts,     UI_OFFSET_X, UI_OFFSET_Y*6, BM_SWITCH_WIDTH, BM_SWITCH_HEIGHT);
}

static void settings_AV(void) {
    panel_settings_av.y = PANEL_SETTINGS_OFFSET_Y;

    int offset_y = SCALE(10);

    CREATE_SWITCH(push_to_talk, UI_OFFSET_X, offset_y, BM_SWITCH_WIDTH, BM_SWITCH_HEIGHT);

    uint16_t start_draw_y = UI_OFFSET_Y + SCALE(5);
    uint16_t button_offset_y = SCALE(245);

    #ifdef AUDIO_FILTERING
        start_draw_y += UI_OFFSET_Y;
        button_offset_y += UI_OFFSET_Y;
        CREATE_SWITCH(audio_filtering, UI_OFFSET_X, offset_y + UI_OFFSET_Y, BM_SWITCH_WIDTH, BM_SWITCH_HEIGHT);
    #endif

    CREATE_DROPDOWN(audio_in,  UI_OFFSET_X, start_draw_y + UI_OFFSET_Y*1, SCALE(24), SCALE(360));
    CREATE_DROPDOWN(audio_out, UI_OFFSET_X, start_draw_y + UI_OFFSET_Y*3, SCALE(24), SCALE(360));
    CREATE_DROPDOWN(video,     UI_OFFSET_X, start_draw_y + UI_OFFSET_Y*5, SCALE(24), SCALE(360));

    CREATE_BUTTON(callpreview, UI_OFFSET_X, button_offset_y, BM_LBUTTON_WIDTH, BM_LBUTTON_HEIGHT);
    CREATE_BUTTON(videopreview, UI_OFFSET_X*2 + BM_LBUTTON_WIDTH, button_offset_y, BM_LBUTTON_WIDTH, BM_LBUTTON_HEIGHT);
}

static void settings_NOTIFY(void) {
    panel_settings_notifications.y = PANEL_SETTINGS_OFFSET_Y;

    int offset_y = SCALE(10);

    CREATE_SWITCH(audible_notifications, UI_OFFSET_X, offset_y + UI_OFFSET_Y*0, BM_SWITCH_WIDTH, BM_SWITCH_HEIGHT);
    CREATE_SWITCH(status_notifications,  UI_OFFSET_X, offset_y + UI_OFFSET_Y*1, BM_SWITCH_WIDTH, BM_SWITCH_HEIGHT);
    CREATE_SWITCH(typing_notes,          UI_OFFSET_X, offset_y + UI_OFFSET_Y*2, BM_SWITCH_WIDTH, BM_SWITCH_HEIGHT);

    CREATE_DROPDOWN(global_group_notifications, UI_OFFSET_X, offset_y + UI_OFFSET_Y*4 - SCALE(5), SCALE(24), SCALE(100));
}

static void settings_ADV(void) {
    panel_settings_adv.y = PANEL_SETTINGS_OFFSET_Y;

    CREATE_SWITCH(ipv6, UI_OFFSET_X, UI_OFFSET_Y,   BM_SWITCH_WIDTH, BM_SWITCH_HEIGHT);
    CREATE_SWITCH(udp,  UI_OFFSET_X, UI_OFFSET_Y*2, BM_SWITCH_WIDTH, BM_SWITCH_HEIGHT);

    CREATE_SWITCH(proxy,       UI_OFFSET_X, UI_OFFSET_Y*3, BM_SWITCH_WIDTH, BM_SWITCH_HEIGHT);
    CREATE_SWITCH(proxy_force, UI_OFFSET_X, UI_OFFSET_Y*4, BM_SWITCH_WIDTH, BM_SWITCH_HEIGHT);

    int proxy_edit_offset_x = UI_OFFSET_X + BM_SWITCH_WIDTH + SCALE(160);
    int proxy_edit_offset_y = SCALE(90);
    int proxy_ip_width = SCALE(120);

    CREATE_EDIT(proxy_ip, proxy_edit_offset_x, proxy_edit_offset_y, proxy_ip_width, SCALE(24));
    CREATE_EDIT(proxy_port, proxy_edit_offset_x + proxy_ip_width + UI_OFFSET_X, proxy_edit_offset_y, SCALE(60), SCALE(24));

    CREATE_SWITCH(auto_update,           UI_OFFSET_X, UI_OFFSET_Y*5, BM_SWITCH_WIDTH, BM_SWITCH_HEIGHT);
    CREATE_SWITCH(block_friend_requests, UI_OFFSET_X, UI_OFFSET_Y*6, BM_SWITCH_WIDTH, BM_SWITCH_HEIGHT);

    CREATE_BUTTON(show_password_settings, UI_OFFSET_X,  UI_OFFSET_Y*7, BM_SBUTTON_WIDTH, BM_SBUTTON_HEIGHT);
    CREATE_BUTTON(show_nospam, UI_OFFSET_X + BM_SWITCH_WIDTH + SCALE(160), UI_OFFSET_Y*7, BM_SBUTTON_WIDTH, BM_SBUTTON_HEIGHT);

    CREATE_EDIT(nospam,           UI_OFFSET_X,  SCALE(265), SCALE(-10), SCALE(24));
    CREATE_BUTTON(change_nospam,  UI_OFFSET_X,  SCALE(295), BM_SBUTTON_WIDTH, BM_SBUTTON_HEIGHT);
    CREATE_BUTTON(revert_nospam,  UI_OFFSET_X + BM_SWITCH_WIDTH + SCALE(160), SCALE(295), BM_SBUTTON_WIDTH, BM_SBUTTON_HEIGHT);

    CREATE_EDIT(profile_password, UI_OFFSET_X,  SCALE(88) + (SCALE(177) * panel_profile_password.disabled), SCALE(-10), SCALE(24));
    CREATE_BUTTON(lock_uTox,      UI_OFFSET_X,  SCALE(295), BM_SBUTTON_WIDTH, BM_SBUTTON_HEIGHT);
}

void ui_set_scale(uint8_t scale) {
    if (scale >= 6 && scale <= 26) {
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

    panel_main.x = MAIN_LEFT;
    panel_main.y = 0;

    scrollbar_settings.panel.y        = PANEL_SETTINGS_OFFSET_Y;
    scrollbar_settings.content_height = SCALE(300); /* TODO magic numbers are bad */

    panel_settings_master.y        = MAIN_TOP_FRAME_THIN;
    panel_settings_devices.y       = PANEL_SETTINGS_OFFSET_Y;
    panel_settings_adv.y           = PANEL_SETTINGS_OFFSET_Y;

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

    sidepanel_USERBADGE();
    sidepanel_FLIST();

    settings_PROFILE();
    settings_UI();
    settings_AV();
    settings_NOTIFY();
    settings_ADV();

    // FIXME for testing, remove
    CREATE_BUTTON(notify_create, SCALE(2), SCALE(2), BM_SBUTTON_WIDTH, BM_SBUTTON_HEIGHT);
    CREATE_BUTTON(notify_one, SCALE(0), SCALE(-50), SCALE(40), SCALE(50));
    CREATE_BUTTON(notify_two, SCALE(200), SCALE(-50), SCALE(40), SCALE(50));
    CREATE_BUTTON(notify_three, SCALE(-40), SCALE(-50), SCALE(40), SCALE(50));

    CREATE_BUTTON(move_notify, SCALE(-40), SCALE(-40), SCALE(40), SCALE(40));


    /* Setting pages */
    CREATE_BUTTON(settings_sub_profile, 1, 1, SCALE(18) + UTOX_STR_WIDTH(PROFILE_BUTTON), SCALE(28));
    uint32_t settings_tab_x = SCALE(22) + UTOX_STR_WIDTH(PROFILE_BUTTON);

#ifdef ENABLE_MULTIDEVICE
    CREATE_BUTTON(settings_sub_devices, settings_tab_x, 1, SCALE(22) + UTOX_STR_WIDTH(DEVICES_BUTTON), SCALE(28));
    settings_tab_x += SCALE(22) + UTOX_STR_WIDTH(DEVICES_BUTTON);
#endif

    CREATE_BUTTON(settings_sub_ui, settings_tab_x, 1, SCALE(18) + UTOX_STR_WIDTH(USER_INTERFACE_BUTTON), SCALE(28));
    settings_tab_x += SCALE(20) + UTOX_STR_WIDTH(USER_INTERFACE_BUTTON);

    CREATE_BUTTON(settings_sub_av, settings_tab_x, 1, SCALE(18) + UTOX_STR_WIDTH(AUDIO_VIDEO_BUTTON), SCALE(28));
    settings_tab_x += SCALE(20) + UTOX_STR_WIDTH(USER_INTERFACE_BUTTON);

    CREATE_BUTTON(settings_sub_notifications, settings_tab_x, 1, SCALE(18) + UTOX_STR_WIDTH(NOTIFICATIONS_BUTTON), SCALE(28));
    settings_tab_x += SCALE(20) + UTOX_STR_WIDTH(USER_INTERFACE_BUTTON);

    CREATE_BUTTON(settings_sub_adv, settings_tab_x, 1, SCALE(18) + UTOX_STR_WIDTH(ADVANCED_BUTTON), SCALE(28));


    /* Devices */
    CREATE_BUTTON(add_new_device_to_self, SCALE(-10) - BM_SBUTTON_WIDTH, SCALE(28), BM_SBUTTON_WIDTH, BM_SBUTTON_HEIGHT);

    CREATE_EDIT(add_new_device_to_self, SCALE(10), SCALE(27), SCALE(0) - UTOX_STR_WIDTH(ADD) - BM_SBUTTON_WIDTH,
                SCALE(24));


    /* Friend Add Page */
    CREATE_BUTTON(send_friend_request, SCALE(-10) - BM_SBUTTON_WIDTH, MAIN_TOP + SCALE(168), BM_SBUTTON_WIDTH,
                  BM_SBUTTON_HEIGHT);
    CREATE_BUTTON(accept_friend, SCALE(-60), SCALE(-80), BM_SBUTTON_WIDTH, BM_SBUTTON_HEIGHT);

    /* Friend Settings Page */
    CREATE_BUTTON(export_chatlog, SCALE(10), SCALE(220), BM_SBUTTON_WIDTH, BM_SBUTTON_HEIGHT);

    CREATE_EDIT(friend_pubkey,          SCALE(10), SCALE(88), SCALE(-10), SCALE(24));
    CREATE_EDIT(friend_alias,           SCALE(10), SCALE(138), SCALE(-10), SCALE(24));

    CREATE_SWITCH(friend_autoaccept_ft, SCALE(-10) -BM_SWITCH_WIDTH, SCALE(170), BM_SWITCH_WIDTH, BM_SWITCH_HEIGHT);

    /* Group Settings */
    CREATE_EDIT(group_topic, SCALE(10), SCALE(95), SCALE(-10), SCALE(24));

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

    CREATE_BUTTON(chat_send_friend, SCALE(-6) - BM_CHAT_SEND_WIDTH, SCALE(-46), BM_CHAT_SEND_WIDTH, BM_CHAT_SEND_HEIGHT);
    CREATE_BUTTON(chat_send_group,  SCALE(-6) - BM_CHAT_SEND_WIDTH, SCALE(-46), BM_CHAT_SEND_WIDTH, BM_CHAT_SEND_HEIGHT);

    setfont(FONT_TEXT);

    // Add friend panel
    CREATE_EDIT(add_new_friend_id, SCALE(10), SCALE(28) + MAIN_TOP, SCALE(-10), SCALE(24));
    CREATE_EDIT(add_new_friend_msg, SCALE(10), SCALE(76) + MAIN_TOP, SCALE(-10), SCALE(84));

    /* Message entry box for friends and groups */
    CREATE_EDIT(chat_msg_friend, SCALE(10) + BM_CHAT_BUTTON_WIDTH * 2, /* Make space for the left button  */
                SCALE(-46), SCALE(-64), SCALE(40)); /* text is 8 high. 8 * 2.5 = 20. */

    CREATE_EDIT(chat_msg_group, SCALE(6), SCALE(-46), SCALE(-10) - BM_CHAT_SEND_WIDTH, SCALE(40));

    /* Confirm deletion */
    CREATE_BUTTON(confirm_deletion, SCALE(10), MAIN_TOP + SCALE(40), BM_SBUTTON_WIDTH, BM_SBUTTON_HEIGHT);
    CREATE_BUTTON(deny_deletion,    SCALE(80), MAIN_TOP + SCALE(40), BM_SBUTTON_WIDTH, BM_SBUTTON_HEIGHT);

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

        default: {
            break;
        }
    }

    PANEL **pp = p->child;
    if (pp) {
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

    PANEL **pp = p->child;
    if (pp) {
        PANEL *subp;
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

    PANEL **pp = p->child;
    if (pp) {
        PANEL *subp;
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
        draw |= contextmenu_mup();
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
