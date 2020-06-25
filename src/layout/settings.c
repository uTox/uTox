#include "settings.h"

#include "../debug.h"
#include "../flist.h"
#include "../macros.h"
#include "../self.h"
#include "../theme.h"
#include "../tox.h"

#include "../av/video.h"

#include "../native/clipboard.h"
#include "../native/dialog.h"
#include "../native/filesys.h"
#include "../native/keyboard.h"
#include "../native/notify.h"
#include "../native/os.h"

#include "../ui/button.h"
#include "../ui/contextmenu.h"
#include "../ui/draw.h"
#include "../ui/dropdown.h"
#include "../ui/edit.h"
#include "../ui/scrollable.h"
#include "../ui/svg.h"
#include "../ui/switch.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Top bar for user settings */
static void draw_settings_header(int x, int y, int w, int UNUSED(height)) {
    (void)w; // Silence an irreverent warning when GIT_VERSION is undefined
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(x + SCALE(10), y + SCALE(10), UTOX_SETTINGS);
#ifdef GIT_VERSION
    setfont(FONT_TEXT);
    char ver_string[64];
    int  ver_string_len;

    snprintf(ver_string, sizeof(ver_string), "Toxcore v%u.%u.%u",
             tox_version_major(), tox_version_minor(), tox_version_patch());
    ver_string_len = strnlen(ver_string, sizeof(ver_string) - 1);

    drawtextwidth_right(x + w - textwidth(ver_string, ver_string_len),
                        textwidth(ver_string, ver_string_len), SCALE(10),
                        ver_string, ver_string_len);

    setfont(FONT_SELF_NAME); // x adjustment depends on the font type being set first
    x += SCALE(25) + UTOX_STR_WIDTH(UTOX_SETTINGS);
    setfont(FONT_TEXT);
    drawtext(x, SCALE(10), GIT_VERSION, strlen(GIT_VERSION));
#endif
}

#define DRAW_UNDERLINE() drawhline(x, y + SCALE(28), next_x, COLOR_EDGE_NORMAL)
#define DRAW_OVERLINE()  drawhline(x, y + SCALE(0), next_x, COLOR_EDGE_ACTIVE); \
                         drawhline(x, y + SCALE(1), next_x, COLOR_EDGE_ACTIVE)

static void draw_settings_sub_header(int x, int y, int width, int UNUSED(height)) {
    drawhline(x, y, x + width, COLOR_EDGE_NORMAL);

    setfont(FONT_SELF_NAME);
    int last_x = x + width;

    /* Draw the text and bars for general settings */
    setcolor(!button_settings_sub_profile.mouseover ? COLOR_MAIN_TEXT : COLOR_MAIN_TEXT_SUBTEXT);
    int next_x = x + SCALE(20) + UTOX_STR_WIDTH(PROFILE_BUTTON);
    drawstr(x + SCALE(10), y + SCALE(10), PROFILE_BUTTON);

    if (panel_settings_profile.disabled) {
        DRAW_UNDERLINE();
    } else {
        DRAW_OVERLINE();
    }
    drawvline(next_x, y, y + SCALE(28), COLOR_EDGE_NORMAL);
    x = next_x;

    /* Draw the text and bars for device settings */
    #ifdef ENABLE_MULTIDEVICE
    setcolor(!button_settings_sub_devices.mouseover ? COLOR_MAIN_TEXT : COLOR_MAIN_TEXT_SUBTEXT);
    next_x += SCALE(20) + UTOX_STR_WIDTH(DEVICES_BUTTON);
    drawstr(x + SCALE(10), y, + SCALE(10), DEVICES_BUTTON);
    if (panel_settings_devices.disabled) {
        DRAW_UNDERLINE();
    } else {
        DRAW_OVERLINE();
    }
    drawvline(next_x, y, y, + SCALE(28), COLOR_EDGE_NORMAL);
    x = next_x;
    #endif

    /* Draw the text and bars for User interface settings */
    setcolor(!button_settings_sub_ui.mouseover ? COLOR_MAIN_TEXT : COLOR_MAIN_TEXT_SUBTEXT);
    next_x += SCALE(20) + UTOX_STR_WIDTH(USER_INTERFACE_BUTTON);
    drawstr(x + SCALE(10), y + SCALE(10), USER_INTERFACE_BUTTON);
    if (panel_settings_ui.disabled) {
        DRAW_UNDERLINE();
    } else {
        DRAW_OVERLINE();
    }
    drawvline(next_x, y, y + SCALE(28), COLOR_EDGE_NORMAL);
    x = next_x;

    /* Draw the text and bars for A/V settings */
    setcolor(!button_settings_sub_av.mouseover ? COLOR_MAIN_TEXT : COLOR_MAIN_TEXT_SUBTEXT);
    next_x += SCALE(20) + UTOX_STR_WIDTH(AUDIO_VIDEO_BUTTON);
    drawstr(x + SCALE(10), y + SCALE(10), AUDIO_VIDEO_BUTTON);
    if (panel_settings_av.disabled) {
        DRAW_UNDERLINE();
    } else {
        DRAW_OVERLINE();
    }
    drawvline(next_x, y, y + SCALE(28), COLOR_EDGE_NORMAL);
    x = next_x;

    /* Draw the text and bars for notification settings */
    setcolor(!button_settings_sub_notifications.mouseover ? COLOR_MAIN_TEXT : COLOR_MAIN_TEXT_SUBTEXT);
    next_x += SCALE(20) + UTOX_STR_WIDTH(NOTIFICATIONS_BUTTON);
    drawstr(x + SCALE(10), y + SCALE(10), NOTIFICATIONS_BUTTON);
    if (panel_settings_notifications.disabled) {
        DRAW_UNDERLINE();
    } else {
        DRAW_OVERLINE();
    }
    drawvline(next_x, y, y + SCALE(28), COLOR_EDGE_NORMAL);
    x = next_x;

    /* Draw the text and bars for advanced settings */
    setcolor(!button_settings_sub_adv.mouseover ? COLOR_MAIN_TEXT : COLOR_MAIN_TEXT_SUBTEXT);
    next_x += SCALE(20) + UTOX_STR_WIDTH(ADVANCED_BUTTON);
    drawstr(x + SCALE(10), y + SCALE(10), ADVANCED_BUTTON);
    if (panel_settings_adv.disabled) {
        DRAW_UNDERLINE();
    } else {
        DRAW_OVERLINE();
    }
    drawvline(next_x, y, y + SCALE(28), COLOR_EDGE_NORMAL);
    x = next_x;

    next_x = last_x;
    DRAW_UNDERLINE();
}

/* draw switch profile top bar */
/* Text content for settings page */
static void draw_settings_text_profile(int x, int y, int UNUSED(w), int UNUSED(h)) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(x + SCALE(10), y + SCALE(10), NAME);
    drawstr(x + SCALE(10), y + SCALE(65), STATUSMESSAGE);
    drawstr(x + SCALE(10), y + SCALE(120), TOXID);

    if (self.qr_image && !button_qr.disabled) {
        // Enlarge original QR for better recognition
        const double image_scale = SCALE(4);
        const uint32_t image_size = self.qr_image_size * image_scale;

        button_qr.panel.width = button_qr.panel.height = UN_SCALE(self.qr_image_size * image_scale);

        image_set_scale(self.qr_image, image_scale);
        draw_image(self.qr_image, x + SCALE(10), y + SCALE(175), image_size, image_size, 0, 0);
        image_set_scale(self.qr_image, 1.0);
    }
}

// Devices settings page
static void draw_settings_text_devices(int x, int y, int UNUSED(w), int UNUSED(h)) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(x + SCALE(10), y + SCALE(10), DEVICES_ADD_NEW);

    drawstr(x + SCALE(10), y + SCALE(60), DEVICES_NUMBER);

    char   str[10];

    snprintf(str, sizeof(str), "%zu", self.device_list_count);
    size_t str_len = strnlen(str, sizeof(str) - 1);

    drawtext(x + SCALE(10), y + SCALE(75), str, str_len);
}

static void draw_settings_text_password(int x, int y, int UNUSED(w), int UNUSED(h)) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(x + SCALE(10), y + SCALE(215), PROFILE_PASSWORD);

    setfont(FONT_MISC);
    setcolor(C_RED);
    drawstr(x + SCALE(10), y + SCALE(289), PROFILE_PW_WARNING);
    drawstr(x + SCALE(10), y + SCALE(301), PROFILE_PW_NO_RECOVER);
}

static void draw_nospam_settings(int x, int y, int UNUSED(w), int UNUSED(h)){
    setfont(FONT_MISC);
    setcolor(C_RED);
    drawstr(x + SCALE(95), y + SCALE(218), NOSPAM_WARNING);

    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);

    drawstr(x + SCALE(10), y + SCALE(215), NOSPAM);
}

// UI settings page
static void draw_settings_text_ui(int x, int y, int UNUSED(w), int UNUSED(height)) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(x + SCALE(10), y + SCALE(10), LANGUAGE);
    drawstr(x + SCALE(150), y + SCALE(65),  DPI);
    drawstr(x + SCALE(10),  y + SCALE(65),  THEME);
    drawstr(x + SCALE(20) + BM_SWITCH_WIDTH,  y + SCALE(120),  SAVE_CHAT_HISTORY);
    drawstr(x + SCALE(20) + BM_SWITCH_WIDTH,  y + SCALE(150),  CLOSE_TO_TRAY);
    drawstr(x + SCALE(20) + BM_SWITCH_WIDTH,  y + SCALE(180), START_IN_TRAY);
    drawstr(x + SCALE(20) + BM_SWITCH_WIDTH,  y + SCALE(210), AUTO_STARTUP);
    drawstr(x + SCALE(20) + BM_SWITCH_WIDTH,  y + SCALE(240), SETTINGS_UI_MINI_ROSTER);
    #if PLATFORM_ANDROID
        drawstr(x + SCALE(20) + BM_SWITCH_WIDTH, y + SCALE(270), SETTINGS_UI_AUTO_HIDE_SIDEBAR);
    #endif
}

// Audio/Video settings page
static void draw_settings_text_av(int x, int y, int UNUSED(w), int UNUSED(height)) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);

    // The element is draw_pos_y_inc units apart and they start draw_pos_y down.
    uint16_t draw_pos_y = 15;
    uint16_t draw_pos_y_inc = 30;

    drawstr(x + SCALE(20) + BM_SWITCH_WIDTH, y + SCALE(draw_pos_y), PUSH_TO_TALK);
    draw_pos_y += draw_pos_y_inc;
#ifdef AUDIO_FILTERING
    drawstr(x + SCALE(20) + BM_SWITCH_WIDTH, y + SCALE(draw_pos_y), AUDIOFILTERING);
    draw_pos_y += draw_pos_y_inc;
#endif

    // These are 60 apart as there needs to be room for a dropdown between them.
    draw_pos_y_inc = 60;

    drawstr(x + SCALE(10), y + SCALE(draw_pos_y), AUDIOINPUTDEVICE);
    draw_pos_y += draw_pos_y_inc;
    drawstr(x + SCALE(10), y + SCALE(draw_pos_y - 7), AUDIOOUTPUTDEVICE);
    draw_pos_y += draw_pos_y_inc;
    drawstr(x + SCALE(10), y + SCALE(draw_pos_y - 15), VIDEOFRAMERATE);
    draw_pos_y += draw_pos_y_inc;
    drawstr(x + SCALE(10), y + SCALE(draw_pos_y - 23), VIDEOINPUTDEVICE);
    draw_pos_y += draw_pos_y_inc;
    drawstr(x + SCALE(10), y + SCALE(draw_pos_y - 30), PREVIEW);
}

// Notification settings page
static void draw_settings_text_notifications(int x, int y, int UNUSED(w), int UNUSED(height)) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);

    drawstr(x + SCALE(20) + BM_SWITCH_WIDTH, y + SCALE(15), RINGTONE);
    drawstr(x + SCALE(20) + BM_SWITCH_WIDTH, y + SCALE(45), STATUS_NOTIFICATIONS);
    drawstr(x + SCALE(20) + BM_SWITCH_WIDTH, y + SCALE(75), SEND_TYPING_NOTIFICATIONS);
    drawstr(x + SCALE(10), y + SCALE(105), GROUP_NOTIFICATIONS);
}

static void draw_settings_text_adv(int x, int y, int UNUSED(w), int UNUSED(height)) {
    setfont(FONT_MISC);
    setcolor(C_RED);
    drawstr(x + SCALE(10), y + SCALE(5), WARNING);

    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);

    drawstr(x + SCALE(20) + BM_SWITCH_WIDTH, y + SCALE(30),  IPV6);
    drawstr(x + SCALE(20) + BM_SWITCH_WIDTH, y + SCALE(60),  UDP);
    drawstr(x + SCALE(20) + BM_SWITCH_WIDTH, y + SCALE(90),  PROXY);
    drawstr(x + SCALE(20) + BM_SWITCH_WIDTH, y + SCALE(120), PROXY_FORCE); // TODO draw ONLY when settings.proxyenable = true
    drawtext(x + SCALE(353), y + SCALE(89), ":", 1); // Little addr port separator

    drawstr(x + SCALE(20)+ BM_SWITCH_WIDTH, y + SCALE(150), BLOCK_FRIEND_REQUESTS);
}


SCROLLABLE scrollbar_settings = {
    .panel = { .type = PANEL_SCROLLABLE, },
    .color = C_SCROLL,
};

/* Draw the text for profile password window */
static void draw_profile_password(int x, int UNUSED(y), int UNUSED(w), int UNUSED(height)) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(x + SCALE(10), SCALE(10), PROFILE_PASSWORD);

    setcolor(COLOR_MAIN_TEXT_SUBTEXT);
    setfont(FONT_TEXT);
    drawstr(x + SCALE(10), SCALE(MAIN_TOP + 5), PROFILE_PASSWORD);
}

PANEL
boxfor_password_entry_login = {
    .type = PANEL_NONE,
    .x = 0, .y = 0,
    .child = (PANEL*[]) {
        (PANEL*)&edit_profile_password,
        NULL
    }
},
boxfor_password_entry_change = {
    .type = PANEL_NONE,
    .x = 0, .y = 180,
    .child = (PANEL*[]) {
        (PANEL*)&edit_profile_password,
        NULL
    }
},
panel_profile_password = {
    .type = PANEL_NONE,
    .disabled = 0,
    .drawfunc = draw_profile_password,
    .child = (PANEL*[]) {
        (PANEL*)&boxfor_password_entry_login,
        NULL
    }
},
panel_nospam_settings = {
    .type = PANEL_NONE,
    .disabled = true,
    .drawfunc = draw_nospam_settings,
    .content_scroll = &scrollbar_settings,
    .child = (PANEL*[]) {
        (PANEL*)&edit_nospam,
        (PANEL*)&button_change_nospam,
        (PANEL*)&button_revert_nospam,
        NULL
     }
},
panel_profile_password_settings = {
    .type     = PANEL_NONE,
    .disabled = 1,
    .drawfunc = draw_settings_text_password,
    .child = (PANEL*[]) {
        (PANEL*)&boxfor_password_entry_change,
        (PANEL*)&button_lock_uTox,
        NULL
    }
},
panel_settings_master = {
    .type = PANEL_NONE,
    .disabled = 1,
    .drawfunc = draw_settings_header,
    .child = (PANEL*[]) {
        &panel_settings_subheader,
        NULL
    }
},
    panel_settings_subheader = {
        .y = MAIN_TOP_FRAME_THIN,
        .type = PANEL_NONE,
        .disabled = 0,
        .drawfunc = draw_settings_sub_header,
        .child = (PANEL*[]) {
            (PANEL*)&button_settings_sub_profile,
            (PANEL*)&button_settings_sub_devices,
            (PANEL*)&button_settings_sub_ui,
            (PANEL*)&button_settings_sub_av,
            (PANEL*)&button_settings_sub_notifications,
            (PANEL*)&button_settings_sub_adv,
            (PANEL*)&scrollbar_settings,
            &panel_settings_profile,
            &panel_settings_devices,
            &panel_settings_ui,
            &panel_settings_av,
            &panel_settings_notifications,
            &panel_settings_adv,
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
            (PANEL*)&edit_name,
            (PANEL*)&edit_status_msg,
            // Text: Tox ID
            (PANEL*)&edit_toxid,
            (PANEL*)&button_copyid,
            (PANEL*)&button_show_qr,
            (PANEL*)&button_qr,
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
    panel_settings_ui = {
        .type = PANEL_NONE,
        .drawfunc = draw_settings_text_ui,
        .disabled = 1,
        .content_scroll = &scrollbar_settings,
        .child = (PANEL*[]) {
            (PANEL*)&dropdown_language,
            (PANEL*)&dropdown_dpi,
            (PANEL*)&dropdown_theme,
            (PANEL*)&switch_save_chat_history,
            (PANEL*)&switch_close_to_tray,
            (PANEL*)&switch_start_in_tray,
            (PANEL*)&switch_auto_startup,
            (PANEL*)&switch_mini_contacts,
            #if PLATFORM_ANDROID
                (PANEL*)&switch_magic_sidebar,
            #endif
            NULL
        }
    },
    panel_settings_av = {
        .type = PANEL_NONE,
        .disabled = 1,
        .drawfunc = draw_settings_text_av,
        .content_scroll = &scrollbar_settings,
        .child = (PANEL*[]) {
            (PANEL*)&button_callpreview,
            (PANEL*)&switch_push_to_talk,
            (PANEL*)&button_videopreview,
            (PANEL*)&edit_video_fps,
            (PANEL*)&dropdown_audio_in,
            (PANEL*)&dropdown_audio_out,
            (PANEL*)&dropdown_video,
            (PANEL*)&switch_audio_filtering,
            NULL
        }
    },

    panel_settings_notifications = {
        .type = PANEL_NONE,
        .disabled = true,
        .drawfunc = draw_settings_text_notifications,
        .content_scroll = &scrollbar_settings,
        .child = (PANEL*[]) {
            (PANEL*)&dropdown_global_group_notifications,
            (PANEL*)&switch_status_notifications,
            (PANEL*)&switch_typing_notes,
            (PANEL*)&switch_audible_notifications,
            NULL
        }
    },

    panel_settings_adv = {
        .type = PANEL_NONE,
        .disabled = true,
        .drawfunc = draw_settings_text_adv,
        .content_scroll = &scrollbar_settings,
        .child = (PANEL*[]) {
            (PANEL*)&edit_proxy_ip,
            (PANEL*)&edit_proxy_port,
            (PANEL*)&switch_proxy,
            (PANEL*)&switch_proxy_force,
            (PANEL*)&switch_ipv6,
            (PANEL*)&switch_udp,
            (PANEL*)&button_show_password_settings,
            &panel_profile_password_settings,
            (PANEL*)&switch_block_friend_requests,
            (PANEL*)&button_show_nospam,
            &panel_nospam_settings,
            NULL,
        }
    };


extern SCROLLABLE scrollbar_settings;

static void button_bottommenu_update(BUTTON *b) {
    b->c1  = COLOR_BKGRND_MENU;
    b->c2  = COLOR_BKGRND_MENU_HOVER;
    b->c3  = COLOR_BKGRND_MENU_ACTIVE;
    b->ct1 = COLOR_MENU_TEXT;
    b->ct2 = COLOR_MENU_TEXT;
    if (b->mousedown || b->disabled) {
        b->ct1 = COLOR_MENU_TEXT_ACTIVE;
        b->ct2 = COLOR_MENU_TEXT_ACTIVE;
    }
    b->cd = COLOR_BKGRND_MENU_ACTIVE;
}

static void button_settings_on_mup(void) {
    if (tox_thread_init == UTOX_TOX_THREAD_INIT_SUCCESS) {
        flist_selectsettings();
    }
}

static void update_show_nospam_button_text(void) {
    if (panel_nospam_settings.disabled) {
        maybe_i18nal_string_set_i18nal(&button_show_nospam.button_text, STR_SHOW_NOSPAM);
    } else {
        maybe_i18nal_string_set_i18nal(&button_show_nospam.button_text, STR_HIDE_NOSPAM);
    }
}

static void update_show_password_button_text(void) {
    if (panel_profile_password_settings.disabled) {
        maybe_i18nal_string_set_i18nal(&button_show_password_settings.button_text, STR_SHOW_UI_PASSWORD);
        maybe_i18nal_string_set_i18nal(&button_show_password_settings.tooltip_text, STR_SHOW_UI_PASSWORD_TOOLTIP);
    } else {
        maybe_i18nal_string_set_i18nal(&button_show_password_settings.button_text, STR_HIDE_UI_PASSWORD);
        maybe_i18nal_string_set_i18nal(&button_show_password_settings.tooltip_text, STR_HIDE_UI_PASSWORD_TOOLTIP);
    }
}

BUTTON button_settings = {
    .panel = {
        .type   = PANEL_BUTTON,
        .x      = SIDEBAR_BUTTON_LEFT,
        .y      = ROSTER_BOTTOM,
        .width  = SIDEBAR_BUTTON_WIDTH,
        .height = SIDEBAR_BUTTON_HEIGHT,
    },
    .bm_icon          = BM_SETTINGS,
    .icon_w       = _BM_ADD_WIDTH,
    .icon_h       = _BM_ADD_WIDTH,
    .update       = button_bottommenu_update,
    .on_mup       = button_settings_on_mup,
    .disabled     = false,
    .nodraw       = false,
    .tooltip_text = {.i18nal = STR_USERSETTINGS },
};

static void close_dropdowns(PANEL *p) {
    if (!p->child) {
        return;
    }

    PANEL *ch;
    for (int i = 0; (ch = p->child[i]); ++i)
    {
        PANEL_TYPE type = ch->type;
        if (type == PANEL_DROPDOWN) {
            dropdown_close((DROPDOWN*)ch);
        } else if (type == PANEL_NONE) {
            close_dropdowns(ch);
        }
    }
}

void reset_settings_controls(void) {
    update_show_nospam_button_text();
    update_show_password_button_text();
    close_dropdowns(&panel_settings_master);
}

static void disable_all_setting_sub(void) {
    flist_selectsettings();
    panel_settings_profile.disabled         = true;
    panel_settings_devices.disabled         = true;
    panel_settings_ui.disabled              = true;
    panel_settings_av.disabled              = true;
    panel_settings_notifications.disabled   = true;
    panel_settings_adv.disabled             = true;
    reset_settings_controls();
}

static void button_settings_sub_profile_on_mup(void) {
    scrollbar_settings.content_height = SCALE(260);
    disable_all_setting_sub();
    panel_settings_profile.disabled = false;
}

static void button_settings_sub_devices_on_mup(void) {
    scrollbar_settings.content_height = SCALE(260);
    disable_all_setting_sub();
    panel_settings_devices.disabled = false;
}

static void button_settings_sub_ui_on_mup(void) {
    scrollbar_settings.content_height = SCALE(280);
    disable_all_setting_sub();
    panel_settings_ui.disabled = false;
}

static void button_settings_sub_av_on_mup(void) {
    scrollbar_settings.content_height = SCALE(350);
    disable_all_setting_sub();
    panel_settings_av.disabled = false;
}

static void button_settings_sub_adv_on_mup(void) {
    scrollbar_settings.content_height = SCALE(300);
    disable_all_setting_sub();
    panel_settings_adv.disabled = false;
}

static void button_settings_sub_notifications_on_mup(void){
    scrollbar_settings.content_height = SCALE(300);
    disable_all_setting_sub();
    panel_settings_notifications.disabled = false;
}


static void button_add_device_to_self_mdown(void) {
#ifdef ENABLE_MULTIDEVICE
    devices_self_add(edit_add_new_device_to_self.data, edit_add_new_device_to_self.length);
    edit_resetfocus();
#endif
}

BUTTON
button_settings_sub_profile = {
    .nodraw = true,
    .on_mup = button_settings_sub_profile_on_mup,
    .tooltip_text = {.i18nal = STR_PROFILE_BUTTON },
    .button_text  = {.i18nal = STR_PROFILE_BUTTON },
},

button_settings_sub_devices = {
    .nodraw = true,
    .on_mup = button_settings_sub_devices_on_mup,
    .tooltip_text = {.i18nal = STR_DEVICES_BUTTON },
    .button_text  = {.i18nal = STR_DEVICES_BUTTON },
},

button_settings_sub_ui = {
    .nodraw = true,
    .on_mup = button_settings_sub_ui_on_mup,
    .tooltip_text = {.i18nal = STR_USER_INTERFACE_BUTTON },
    .button_text  = {.i18nal = STR_USER_INTERFACE_BUTTON },
},

button_settings_sub_av = {
    .nodraw = true,
    .on_mup = button_settings_sub_av_on_mup,
    .tooltip_text = {.i18nal = STR_AUDIO_VIDEO_BUTTON },
    .button_text  = {.i18nal = STR_AUDIO_VIDEO_BUTTON },
},

button_settings_sub_notifications = {
    .nodraw = true,
    .on_mup = button_settings_sub_notifications_on_mup,
    .tooltip_text = {.i18nal = STR_NOTIFICATIONS_BUTTON },
    .button_text  = {.i18nal = STR_NOTIFICATIONS_BUTTON },
},

button_settings_sub_adv = {
    .nodraw = true,
    .on_mup = button_settings_sub_adv_on_mup,
    .tooltip_text = {.i18nal = STR_ADVANCED_BUTTON },
    .button_text  = {.i18nal = STR_ADVANCED_BUTTON },
},

button_add_new_device_to_self = {
    .bm_fill         = BM_SBUTTON,
    .button_text = {.i18nal = STR_ADD },
    // .update   = button_setcolors_success,
    .on_mup      = button_add_device_to_self_mdown,
};

#include "../tox.h"
static void button_lock_uTox_on_mup(void) {
    if (tox_thread_init && edit_profile_password.length > 3) {
        flist_selectsettings();
        panel_profile_password.disabled = false;
        panel_settings_master.disabled  = true;
        tox_settingschanged();
    } else {
        show_messagebox(NULL, 0, S(PASSWORD_TOO_SHORT), SLEN(PASSWORD_TOO_SHORT));
        memset(edit_profile_password.data, '\0', edit_profile_password.data_size);
        edit_profile_password.length = 0;
    }
    button_show_password_settings.disabled = false;
    button_show_password_settings.nodraw = false;
    update_show_nospam_button_text();
    update_show_password_button_text();
}

static void button_show_password_settings_on_mup(void) {
    panel_nospam_settings.disabled = true;
    panel_profile_password_settings.disabled = !panel_profile_password_settings.disabled;
    update_show_nospam_button_text();
    update_show_password_button_text();
}


#include "../chatlog.h"
#include "../flist.h"
#include "../friend.h"
static void button_export_chatlog_on_mup(void) {
    FRIEND *f = flist_get_sel_friend();
    if (!f) {
        LOG_ERR("Settings", "Could not get selected friend.");
        return;
    }
    utox_export_chatlog_init(f->number);
}

static void button_change_nospam_on_mup(void) {
    button_revert_nospam.disabled = false;
    long int nospam = rand() | rand() << 16;
    postmessage_toxcore(TOX_SELF_CHANGE_NOSPAM, nospam, 0, NULL);
}

static void button_revert_nospam_on_mup(void) {
    if (button_revert_nospam.disabled) {
        return;
    }

    if (self.old_nospam == 0 || self.nospam == self.old_nospam) { //nospam can not be 0
        LOG_ERR("Settings", "Invalid or current nospam: %u.", self.old_nospam);
        return;
    }
    postmessage_toxcore(TOX_SELF_CHANGE_NOSPAM, self.old_nospam, 0, NULL);
    button_revert_nospam.disabled = true;
}

static void button_revert_nospam_on_update(BUTTON *b) {
    if (button_revert_nospam.disabled) {
        button_setcolors_disabled(b);
    } else {
        button_setcolors_success(b);
    }
}

static void button_show_nospam_on_mup(void) {
    panel_profile_password_settings.disabled = true;
    panel_nospam_settings.disabled = !panel_nospam_settings.disabled;
    update_show_nospam_button_text();
    update_show_password_button_text();
}

static void button_copyid_on_mup(void) {
    edit_setfocus(&edit_toxid);
    copy(0);
}

static void button_show_qr_on_mup(void) {
    if (button_qr.disabled) {
        maybe_i18nal_string_set_i18nal(&button_show_qr.button_text, STR_HIDE_QR);
    } else {
        maybe_i18nal_string_set_i18nal(&button_show_qr.button_text, STR_SHOW_QR);
    }

    button_qr.disabled = !button_qr.disabled;
    button_qr.panel.disabled = button_qr.disabled;
}

static void contextmenu_qr_onselect(uint8_t i) {
    if (i == 0) {
        if (!native_save_image_png(self.name, self.qr_data, self.qr_data_size)) {
            LOG_ERR("Self", "Unable to save QR code.");
        }
    }
}

static void button_qr_on_mright(void) {
    static UTOX_I18N_STR menu[] = { STR_SAVE_QR };
    contextmenu_new(COUNTOF(menu), menu, contextmenu_qr_onselect);
}

#include "../settings.h"
#include "../av/utox_av.h"
static void button_audiopreview_on_mup(void) {
    if (!settings.audio_preview) {
        postmessage_utoxav(UTOXAV_START_AUDIO, 1, 0, NULL);
    } else {
        postmessage_utoxav(UTOXAV_STOP_AUDIO, 1, 0, NULL);
    }
    settings.audio_preview = !settings.audio_preview;
}

static void button_audiopreview_update(BUTTON *b) {
    if (settings.audio_preview) {
        button_setcolors_danger(b);
    } else {
        button_setcolors_success(b);
    }
}

static void button_videopreview_on_mup(void) {
    if (settings.video_preview) {
        postmessage_utoxav(UTOXAV_STOP_VIDEO, UINT16_MAX, 1, NULL);
    } else if (video_width && video_height) {
        postmessage_utoxav(UTOXAV_START_VIDEO, UINT16_MAX, 1, NULL);
    } else {
        LOG_ERR("Button", "Video_width = 0, can't preview\n");
        return;
    }
    settings.video_preview = !settings.video_preview;
}

static void button_videopreview_update(BUTTON *b) {
    if (settings.video_preview) {
        button_setcolors_danger(b);
    } else {
        button_setcolors_success(b);
    }
}

static void button_show_qr_update(BUTTON *b) {
    button_setcolors_success(b);
    b->panel.x = 85 + UN_SCALE(UTOX_STR_WIDTH(COPY_TOX_ID));
}

BUTTON button_copyid = {
    .panel = {
        .type   = PANEL_BUTTON,
        .x      =  66,
        .y      = 117,
        .width  = _BM_SBUTTON_WIDTH,
        .height = _BM_SBUTTON_HEIGHT,
    },
    .bm_fill  = BM_SBUTTON,
    .update   = button_setcolors_success,
    .on_mup   = button_copyid_on_mup,
    .disabled = false,
    .button_text = {.i18nal = STR_COPY_TOX_ID },
};

BUTTON button_show_qr = {
    .panel = {
        .type   = PANEL_BUTTON,
        .x      = 85 + _BM_SBUTTON_WIDTH,
        .y      = 117,
        .width  = _BM_SBUTTON_WIDTH,
        .height = _BM_SBUTTON_HEIGHT
    },
    .bm_fill  = BM_SBUTTON,
    .update   = button_show_qr_update,
    .on_mup   = button_show_qr_on_mup,
    .disabled = false,
    .button_text = {.i18nal = STR_SHOW_QR },
};

BUTTON button_qr = {
    .panel = {
        .type   = PANEL_BUTTON,
        .x      = 10,
        .y      = 175,
        .disabled = true,
    },
    .nodraw   = true,
    .disabled = true,
    .onright  = button_qr_on_mright,
};

BUTTON button_callpreview = {
    .bm_fill  = BM_LBUTTON,
    .bm_icon  = BM_CALL,
    .icon_w   = _BM_LBICON_WIDTH,
    .icon_h   = _BM_LBICON_HEIGHT,
    .on_mup   = button_audiopreview_on_mup,
    .update   = button_audiopreview_update,
    .disabled = false,
};

BUTTON button_videopreview = {
    .bm_fill  = BM_LBUTTON,
    .bm_icon  = BM_VIDEO,
    .icon_w   = _BM_LBICON_WIDTH,
    .icon_h   = _BM_LBICON_HEIGHT,
    .on_mup   = button_videopreview_on_mup,
    .update   = button_videopreview_update,
    .disabled = false,
};

BUTTON button_lock_uTox = {
    .panel = {
        .type   = PANEL_BUTTON,
        .x      =  10,
        .y      = 265,
        .width  = _BM_SBUTTON_WIDTH,
        .height = _BM_SBUTTON_HEIGHT,
    },
    .bm_fill      = BM_SBUTTON,
    .update       = button_setcolors_success,
    .on_mup       = button_lock_uTox_on_mup,
    .button_text  = {.i18nal = STR_LOCK },
    .tooltip_text = {.i18nal = STR_LOCK_UTOX },
};

BUTTON button_show_password_settings = {
    .panel = {
        .type   = PANEL_BUTTON,
        .x      =  10,
        .y      = 177,
        .width  = _BM_SBUTTON_WIDTH,
        .height = _BM_SBUTTON_HEIGHT,
    },
    .bm_fill      = BM_SBUTTON,
    .update       = button_setcolors_success,
    .on_mup       = button_show_password_settings_on_mup,
    .button_text  = {.i18nal = STR_SHOW_UI_PASSWORD },
    .tooltip_text = {.i18nal = STR_SHOW_UI_PASSWORD_TOOLTIP },
};

BUTTON button_export_chatlog = {
    .panel = {
        .type   = PANEL_BUTTON,
        .x      =  10,
        .y      = 208,
        .width  = _BM_SBUTTON_WIDTH,
        .height = _BM_SBUTTON_HEIGHT,
    },
    .bm_fill      = BM_SBUTTON,
    .update       = button_setcolors_success,
    .on_mup       = button_export_chatlog_on_mup,
    .disabled     = false,
    .button_text  = {.i18nal = STR_FRIEND_EXPORT_CHATLOG },
    .tooltip_text = {.i18nal = STR_FRIEND_EXPORT_CHATLOG },
};

BUTTON button_change_nospam = {
    .panel = {
        .type   = PANEL_BUTTON,
        .x      =  10,
        .y      = 265,
        .width  = _BM_SBUTTON_WIDTH,
        .height = _BM_SBUTTON_HEIGHT,
    },
    .bm_fill      = BM_SBUTTON,
    .update       = button_setcolors_success,
    .on_mup       = button_change_nospam_on_mup,
    .button_text  = {.i18nal = STR_RANDOMIZE_NOSPAM},
    .tooltip_text = {.i18nal = STR_RANDOMIZE_NOSPAM},
};

BUTTON button_revert_nospam = {
    .bm_fill      = BM_SBUTTON,
    .update       = button_revert_nospam_on_update,
    .on_mup       = button_revert_nospam_on_mup,
    .disabled     = true,
    .button_text  = {.i18nal = STR_REVERT_NOSPAM},
    .tooltip_text = {.i18nal = STR_REVERT_NOSPAM},
};

BUTTON button_show_nospam = {
    .bm_fill      = BM_SBUTTON,
    .update       = button_setcolors_success,
    .tooltip_text = {.i18nal = STR_SHOW_NOSPAM},
    .button_text  = {.i18nal = STR_SHOW_NOSPAM},
    .on_mup       = button_show_nospam_on_mup,
};

static void switchfxn_logging(void) {
    settings.logging_enabled = !settings.logging_enabled;
}

static void switchfxn_mini_contacts(void) {
    settings.use_mini_flist = !settings.use_mini_flist;
    flist_re_scale();
}

static void switchfxn_ipv6(void) {
    settings.enableipv6 = !settings.enableipv6;
    tox_settingschanged();
}

static void switchfxn_udp(void) {
    settings.disableudp = !settings.disableudp;
    tox_settingschanged();
}

static void switchfxn_close_to_tray(void) {
    settings.close_to_tray = !settings.close_to_tray;
}

static void switchfxn_start_in_tray(void) {
    settings.start_in_tray = !settings.start_in_tray;
}

static void switchfxn_auto_startup(void) {
    settings.auto_startup = !settings.auto_startup;
    launch_at_startup(settings.auto_startup);
}

static void switchfxn_typing_notes(void) {
    settings.no_typing_notifications = !settings.no_typing_notifications;
}

static void switchfxn_audible_notifications(void) { settings.audible_notifications_enabled = !settings.audible_notifications_enabled; }

static void switchfxn_push_to_talk(void) {
    if (!settings.push_to_talk) {
        init_ptt();
    } else {
        exit_ptt();
    }
}

static void switchfxn_audio_filtering(void) { settings.audio_filtering_enabled = !settings.audio_filtering_enabled; }

static void switchfxn_status_notifications(void) { settings.status_notifications = !settings.status_notifications; }

static void switchfxn_block_friend_requests(void) { settings.block_friend_requests = !settings.block_friend_requests; }

UISWITCH switch_save_chat_history = {
    .panel = {
        .type   = PANEL_SWITCH,
        .x      =  10,
        .y      = 115,
        .width  = _BM_SWITCH_WIDTH,
        .height = _BM_SWITCH_HEIGHT,
    },
    .style_outer    = BM_SWITCH,
    .style_toggle   = BM_SWITCH_TOGGLE,
    .style_icon_off = BM_NO,
    .style_icon_on  = BM_YES,
    .update         = switch_update,
    .on_mup         = switchfxn_logging,
    .tooltip_text   = {.i18nal = STR_SAVE_CHAT_HISTORY },
};

UISWITCH switch_mini_contacts = {
    .panel = {
        .type   = PANEL_SWITCH,
        .x      =  10,
        .y      = 235,
        .width  = _BM_SWITCH_WIDTH,
        .height = _BM_SWITCH_HEIGHT,
    },
    .style_outer    = BM_SWITCH,
    .style_toggle   = BM_SWITCH_TOGGLE,
    .style_icon_off = BM_NO,
    .style_icon_on  = BM_YES,
    .update         = switch_update,
    .on_mup         = switchfxn_mini_contacts,
    .tooltip_text   = {.i18nal = STR_SETTINGS_UI_MINI_ROSTER },
};


static void switchfxn_magic_sidebar(void) {
    settings.magic_flist_enabled = !settings.magic_flist_enabled;
}

UISWITCH switch_magic_sidebar = {
    .panel = {
        .type   = PANEL_SWITCH,
        .x      = -10 - _BM_SWITCH_WIDTH,
        .y      = 210,
        .width  = _BM_SWITCH_WIDTH,
        .height = _BM_SWITCH_HEIGHT
    },
    .style_outer    = BM_SWITCH,
    .style_toggle   = BM_SWITCH_TOGGLE,
    .style_icon_off = BM_NO,
    .style_icon_on  = BM_YES,
    .update         = switch_update,
    .on_mup         = switchfxn_magic_sidebar,
    .tooltip_text   = {.i18nal = STR_SETTINGS_UI_AUTO_HIDE_SIDEBAR },
};

UISWITCH switch_ipv6 = {
    .panel = {
        .type   = PANEL_SWITCH,
        .x      = 10,
        .y      = 27,
        .width  = _BM_SWITCH_WIDTH,
        .height = _BM_SWITCH_HEIGHT,
    },
    .style_outer    = BM_SWITCH,
    .style_toggle   = BM_SWITCH_TOGGLE,
    .style_icon_off = BM_NO,
    .style_icon_on  = BM_YES,
    .update         = switch_update,
    .on_mup         = switchfxn_ipv6,
    .tooltip_text   = {.i18nal = STR_IPV6 },
};

UISWITCH switch_udp = {
    .panel = {
        .type   = PANEL_SWITCH,
        .x      = 10,
        .y      = 57,
        .width  = _BM_SWITCH_WIDTH,
        .height = _BM_SWITCH_HEIGHT,
    },
    .style_outer    = BM_SWITCH,
    .style_toggle   = BM_SWITCH_TOGGLE,
    .style_icon_off = BM_NO,
    .style_icon_on  = BM_YES,
    .update         = switch_update,
    .on_mup         = switchfxn_udp,
    .tooltip_text   = {.i18nal = STR_UDP },
};

UISWITCH switch_close_to_tray = {
    .panel = {
        .type   = PANEL_SWITCH,
        .x      =  10,
        .y      = 145,
        .width  = _BM_SWITCH_WIDTH,
        .height = _BM_SWITCH_HEIGHT,
    },
    .style_outer    = BM_SWITCH,
    .style_toggle   = BM_SWITCH_TOGGLE,
    .style_icon_off = BM_NO,
    .style_icon_on  = BM_YES,
    .update         = switch_update,
    .on_mup         = switchfxn_close_to_tray,
    .tooltip_text   = {.i18nal = STR_CLOSE_TO_TRAY },
};

UISWITCH switch_start_in_tray = {
    .panel = {
        .type   = PANEL_SWITCH,
        .x      =  10,
        .y      = 175,
        .width  = _BM_SWITCH_WIDTH,
        .height = _BM_SWITCH_HEIGHT,
    },
    .style_outer    = BM_SWITCH,
    .style_toggle   = BM_SWITCH_TOGGLE,
    .style_icon_off = BM_NO,
    .style_icon_on  = BM_YES,
    .update         = switch_update,
    .on_mup         = switchfxn_start_in_tray,
    .tooltip_text   = {.i18nal = STR_START_IN_TRAY },
};

UISWITCH switch_auto_startup = {
    .panel = {
        .type   = PANEL_SWITCH,
        .x      =  10,
        .y      = 205,
        .width  = _BM_SWITCH_WIDTH,
        .height = _BM_SWITCH_HEIGHT,
    },
    .style_outer    = BM_SWITCH,
    .style_toggle   = BM_SWITCH_TOGGLE,
    .style_icon_off = BM_NO,
    .style_icon_on  = BM_YES,
    .update         = switch_update,
    .on_mup         = switchfxn_auto_startup,
    .tooltip_text   = {.i18nal = STR_AUTO_STARTUP },
};

UISWITCH switch_typing_notes = {
    .panel = {
        .type   = PANEL_SWITCH,
        .x      = 10,
        .y      = 70,
        .width  = _BM_SWITCH_WIDTH,
        .height = _BM_SWITCH_HEIGHT,
    },
    .style_outer    = BM_SWITCH,
    .style_toggle   = BM_SWITCH_TOGGLE,
    .style_icon_off = BM_NO,
    .style_icon_on  = BM_YES,
    .update         = switch_update,
    .on_mup         = switchfxn_typing_notes,
    .tooltip_text   = {.i18nal = STR_SEND_TYPING_NOTIFICATIONS },
};

UISWITCH switch_audible_notifications = {
    .panel = {
        .type   = PANEL_SWITCH,
        .x      = 10,
        .y      = 10,
        .width  = _BM_SWITCH_WIDTH,
        .height = _BM_SWITCH_HEIGHT,
    },
    .style_outer    = BM_SWITCH,
    .style_toggle   = BM_SWITCH_TOGGLE,
    .style_icon_off = BM_NO,
    .style_icon_on  = BM_YES,
    .update         = switch_update,
    .on_mup         = switchfxn_audible_notifications,
    .tooltip_text   = {.i18nal = STR_AUDIONOTIFICATIONS },
};

UISWITCH switch_push_to_talk = {
    .panel = {
        .type   = PANEL_SWITCH,
        .x      = 10,
        .y      = 10,
        .width  = _BM_SWITCH_WIDTH,
        .height = _BM_SWITCH_HEIGHT,
    },
    .style_outer    = BM_SWITCH,
    .style_toggle   = BM_SWITCH_TOGGLE,
    .style_icon_off = BM_NO,
    .style_icon_on  = BM_YES,
    .update         = switch_update,
    .on_mup         = switchfxn_push_to_talk,
    .tooltip_text   = {.i18nal = STR_PUSH_TO_TALK },
};

UISWITCH switch_audio_filtering = {
    .style_outer    = BM_SWITCH,
    .style_toggle   = BM_SWITCH_TOGGLE,
    .style_icon_off = BM_NO,
    .style_icon_on  = BM_YES,
    .update         = switch_update,
    .on_mup         = switchfxn_audio_filtering,
    .tooltip_text   = {.i18nal = STR_AUDIOFILTERING },
};

UISWITCH switch_status_notifications = {
    .panel = {
        .type   = PANEL_SWITCH,
        .x      = 10,
        .y      = 40,
        .width  = _BM_SWITCH_WIDTH,
        .height = _BM_SWITCH_HEIGHT,
    },
    .style_outer    = BM_SWITCH,
    .style_toggle   = BM_SWITCH_TOGGLE,
    .style_icon_off = BM_NO,
    .style_icon_on  = BM_YES,
    .update         = switch_update,
    .on_mup         = switchfxn_status_notifications,
    .tooltip_text   = {.i18nal = STR_STATUS_NOTIFICATIONS },
};

UISWITCH switch_block_friend_requests = {
    .panel = {
        .type   = PANEL_SWITCH,
        .x      =  10,
        .y      = 147,
        .width  = _BM_SWITCH_WIDTH,
        .height = _BM_SWITCH_HEIGHT,
    },
    .style_outer    = BM_SWITCH,
    .style_toggle   = BM_SWITCH_TOGGLE,
    .style_icon_off = BM_NO,
    .style_icon_on  = BM_YES,
    .update         = switch_update,
    .on_mup         = switchfxn_block_friend_requests,
    .tooltip_text   = {.i18nal = STR_BLOCK_FRIEND_REQUESTS },
};

static void switchfxn_proxy(void) {
    settings.proxyenable   = !settings.proxyenable;

    if (settings.proxyenable) {
        switch_proxy_force.panel.disabled = false;
    } else {
        settings.force_proxy              = false;
        switch_proxy_force.switch_on      = false;
        switch_proxy_force.panel.disabled = true;
        switch_udp.panel.disabled         = false;
    }

    tox_settingschanged();
}

static void switchfxn_proxy_force(void) {
    settings.force_proxy = !settings.force_proxy;

    if (settings.force_proxy) {
        switch_udp.switch_on      = false;
        settings.disableudp       = true;
        switch_udp.panel.disabled = true;
    } else {
        switch_udp.panel.disabled = false;
    }

    tox_settingschanged();
}

UISWITCH switch_proxy = {
    .panel = {
        .type   = PANEL_SWITCH,
        .x      = 10,
        .y      = 87,
        .width  = _BM_SWITCH_WIDTH,
        .height = _BM_SWITCH_HEIGHT,
    },
    .style_outer    = BM_SWITCH,
    .style_toggle   = BM_SWITCH_TOGGLE,
    .style_icon_off = BM_NO,
    .style_icon_on  = BM_YES,
    .update         = switch_update,
    .on_mup         = switchfxn_proxy,
    .tooltip_text   = {.i18nal = STR_PROXY }
};

UISWITCH switch_proxy_force = {
    .panel = {
        .type   = PANEL_SWITCH,
        .x      =  10,
        .y      = 117,
        .width  = _BM_SWITCH_WIDTH,
        .height = _BM_SWITCH_HEIGHT,
    },
    .style_outer    = BM_SWITCH,
    .style_toggle   = BM_SWITCH_TOGGLE,
    .style_icon_off = BM_NO,
    .style_icon_on  = BM_YES,
    .update         = switch_update,
    .on_mup         = switchfxn_proxy_force,
    .tooltip_text   = {.i18nal = STR_PROXY_FORCE },
};

static void dropdown_audio_in_onselect(uint16_t i, const DROPDOWN *dm) {
    DROP_ELEMENT *e      = &((DROP_ELEMENT *)dm->userdata)[i];
    void *        handle = e->handle;
    postmessage_utoxav(UTOXAV_SET_AUDIO_IN, 0, 0, handle);
}

static void dropdown_audio_out_onselect(uint16_t i, const DROPDOWN *dm) {
    DROP_ELEMENT *e      = &((DROP_ELEMENT *)dm->userdata)[i];
    void *        handle = e->handle;
    postmessage_utoxav(UTOXAV_SET_AUDIO_OUT, 0, 0, handle);
}

static void edit_video_fps_onlosefocus(EDIT *UNUSED(edit)) {
    edit_video_fps.data[edit_video_fps.length] = '\0';

    char *temp;
    uint16_t value = strtol((char *)edit_video_fps.data, &temp, 10);

    if (*temp == '\0' && value >= 1 && value <= UINT8_MAX) {
        settings.video_fps = value;
        return;
    }

    LOG_WARN("Settings", "FPS value (%s) is invalid. It must be integer in range of [1,%u]. Setting default value (%u).",
             edit_video_fps.data, UINT8_MAX, DEFAULT_FPS);

    settings.video_fps = DEFAULT_FPS;
    snprintf((char *)edit_video_fps.data, edit_video_fps.data_size,
             "%u", DEFAULT_FPS);
    edit_video_fps.length = strnlen((char *)edit_video_fps.data, edit_video_fps.data_size - 1);
}

#include "../screen_grab.h"
static void dropdown_video_onselect(uint16_t i, const DROPDOWN *UNUSED(dm)) {
    if (i == 1) {
        utox_screen_grab_desktop(1);
    } else {
        postmessage_utoxav(UTOXAV_SET_VIDEO_IN, i, 0, NULL);
    }
}

static void dropdown_dpi_onselect(uint16_t i, const DROPDOWN *UNUSED(dm)) {
    ui_rescale(i + 5);
    settings.scale = ui_scale;
}

static void dropdown_language_onselect(uint16_t i, const DROPDOWN *UNUSED(dm)) {
    settings.language = (UTOX_LANG)i;
    /* The draw functions need the fonts' and scale to be reset when changing languages. */
    ui_rescale(ui_scale);
}
static STRING *dropdown_language_ondisplay(uint16_t i, const DROPDOWN *UNUSED(dm)) {
    UTOX_LANG tmp_language = (UTOX_LANG)i;
    return SPTRFORLANG(tmp_language, STR_LANG_NATIVE_NAME);
}

static void dropdown_theme_onselect(const uint16_t i, const DROPDOWN *UNUSED(dm)) {
    theme_load(i);
    settings.theme = i;
}

#include"../groups.h"
static void dropdown_notify_groupchats_onselect(const uint16_t i, const DROPDOWN *UNUSED(dm)) {
    GROUPCHAT *g = flist_get_sel_group();
    if (!g) {
        LOG_ERR("Settings", "Could not get selected groupchat.");
        return;
    }

    g->notify    = i;
    LOG_INFO("Settings", "g->notify = %u\n", i);
}

static void dropdown_global_group_notifications_onselect(const uint16_t i, const DROPDOWN *UNUSED(dm)) {
    settings.group_notifications = i;
}

static UTOX_I18N_STR dpidrops[] = {
    STR_DPI_TINY, STR_DPI_060,   STR_DPI_070, STR_DPI_080, STR_DPI_090, STR_DPI_NORMAL, STR_DPI_110,
    STR_DPI_120,  STR_DPI_130,   STR_DPI_140, STR_DPI_BIG, STR_DPI_160, STR_DPI_170,    STR_DPI_180,
    STR_DPI_190,  STR_DPI_LARGE, STR_DPI_210, STR_DPI_220, STR_DPI_230, STR_DPI_240,    STR_DPI_HUGE,
};

DROPDOWN dropdown_audio_in = {
    .ondisplay = dropdown_list_ondisplay,
    .onselect = dropdown_audio_in_onselect
};

DROPDOWN dropdown_audio_out = {
    .ondisplay = dropdown_list_ondisplay,
    .onselect = dropdown_audio_out_onselect
};

DROPDOWN dropdown_video = {
    .ondisplay = dropdown_list_ondisplay,
    .onselect = dropdown_video_onselect,
};

DROPDOWN dropdown_dpi = {
    .panel = {
        .type   = PANEL_DROPDOWN,
        .x      = 150,
        .y      =  85,
        .width  = 200,
        .height =  24,
    },
    .ondisplay = simple_dropdown_ondisplay,
    .onselect  = dropdown_dpi_onselect,
    .dropcount = COUNTOF(dpidrops),
    .userdata  = dpidrops
};

DROPDOWN dropdown_language = {
    .panel = {
        .type   = PANEL_DROPDOWN,
        .x      = 10,
        .y      = 30,
        .width  = -10,
        .height = 24
    },
    .ondisplay = dropdown_language_ondisplay,
    .onselect  = dropdown_language_onselect,
    .dropcount = NUM_LANGS,
};


static UTOX_I18N_STR themedrops[] = {
    STR_THEME_DEFAULT,
    STR_THEME_LIGHT,
    STR_THEME_DARK,
    STR_THEME_HIGHCONTRAST,
    STR_THEME_CUSTOM,
    STR_THEME_ZENBURN,
    STR_THEME_SOLARIZED_LIGHT,
    STR_THEME_SOLARIZED_DARK,
};

DROPDOWN dropdown_theme = {
    .panel = {
        .type   = PANEL_DROPDOWN,
        .x      =  10,
        .y      =  85,
        .width  = 120,
        .height =  24,
    },
    .ondisplay = simple_dropdown_ondisplay,
    .onselect  = dropdown_theme_onselect,
    .dropcount = COUNTOF(themedrops),
    .userdata  = themedrops
};

static UTOX_I18N_STR notifydrops[] = {
    STR_GROUP_NOTIFICATIONS_OFF, STR_GROUP_NOTIFICATIONS_MENTION, STR_GROUP_NOTIFICATIONS_ON,
};

DROPDOWN dropdown_notify_groupchats = {
    .panel = {
        .type   = PANEL_DROPDOWN,
        .x      = 10,
        .y      = 138,
        .width  = 100,
        .height = 24
    },
    .ondisplay = simple_dropdown_ondisplay,
    .onselect  = dropdown_notify_groupchats_onselect,
    .dropcount = COUNTOF(notifydrops),
    .userdata  = notifydrops
};

DROPDOWN dropdown_global_group_notifications = {
    .panel = {
        .type   = PANEL_DROPDOWN,
        .x      =  10,
        .y      = 125,
        .width  = 100,
        .height =  24,
    },
    .ondisplay = simple_dropdown_ondisplay,
    .onselect  = dropdown_global_group_notifications_onselect,
    .dropcount = COUNTOF(notifydrops),
    .userdata  = notifydrops
};

static char edit_name_data[128],
            edit_status_msg_data[128],
            edit_proxy_ip_data[256],
            edit_proxy_port_data[8],
            edit_video_fps_data[3 + 1], /* range is [1-255] */
            edit_profile_password_data[65535],
            edit_nospam_data[(sizeof(uint32_t) * 2) + 1] = { 0 };
#ifdef ENABLE_MULTIDEVICE
static char edit_add_self_device_data[TOX_ADDRESS_SIZE * 4];
#endif


static void edit_name_onenter(EDIT *edit) {
    char *   data   = edit->data;
    uint16_t length = edit->length;

    memcpy(self.name, data, length);
    self.name_length = length;
    update_tray();

    postmessage_toxcore(TOX_SELF_SET_NAME, length, 0, self.name);
}

static void edit_name_ontab(EDIT *UNUSED(edit)) {
    edit_setfocus(&edit_status_msg);
}

static void edit_name_onshifttab(EDIT *UNUSED(edit)) {
    edit_setfocus(&edit_toxid);
}

EDIT edit_name = {
    .panel = {
        .type   = PANEL_EDIT,
        .x      =  10,
        .y      =  30,
        .width  = -10,
        .height =  24,
    },
    .data        = edit_name_data,
    .data_size   = sizeof edit_name_data,
    .onenter     = edit_name_onenter,
    .onlosefocus = edit_name_onenter,
    .ontab       = edit_name_ontab,
    .onshifttab  = edit_name_onshifttab,
};

static void edit_status_msg_onenter(EDIT *edit) {
    char *   data   = edit->data;
    uint16_t length = edit->length;

    if (length) {
        length = (length <= TOX_MAX_STATUS_MESSAGE_LENGTH) ? length : TOX_MAX_STATUS_MESSAGE_LENGTH;
        memcpy(self.statusmsg, data, length);
        self.statusmsg_length = length;
    } else {
        self.statusmsg_length = length;
    }

    update_tray();

    postmessage_toxcore(TOX_SELF_SET_STATUS, length, 0, self.statusmsg); //!
}

static void edit_status_msg_ontab(EDIT *UNUSED(edit)) {
    edit_setfocus(&edit_toxid);
}

static void edit_status_msg_onshifttab(EDIT *UNUSED(edit)) {
    edit_setfocus(&edit_name);
}

EDIT edit_status_msg = {
    .panel = {
        .type   = PANEL_EDIT,
        .x      =  10,
        .y      =  85,
        .width  = -10,
        .height =  24,
    },
    .data        = edit_status_msg_data,
    .data_size   = sizeof edit_status_msg_data,
    .onenter     = edit_status_msg_onenter,
    .onlosefocus = edit_status_msg_onenter,
    .ontab       = edit_status_msg_ontab,
    .onshifttab  = edit_status_msg_onshifttab,
};


static void edit_proxy_ip_port_onlosefocus(EDIT *UNUSED(edit)) {
    edit_proxy_port.data[edit_proxy_port.length] = 0;
    settings.proxy_port = strtol((char *)edit_proxy_port.data, NULL, 0);

    memcpy(settings.proxy_ip, edit_proxy_ip.data, edit_proxy_ip.length);
    settings.proxy_ip[edit_proxy_ip.length] = '\0';

    if (settings.proxyenable) {
        tox_settingschanged();
    }
}

static void edit_proxy_ip_ontab(EDIT *UNUSED(edit)) {
    edit_setfocus(&edit_proxy_port);
}

EDIT edit_proxy_ip = {
    .panel = {
        .type   = PANEL_EDIT,
        .x      = 230,
        .y      =  87,
        .width  = 120,
        .height =  24,
    },
    .data        = edit_proxy_ip_data,
    .data_size   = sizeof edit_proxy_ip_data,
    .empty_str = {.i18nal = STR_PROXY_EDIT_HINT_IP },
    .onlosefocus = edit_proxy_ip_port_onlosefocus,
    .ontab       = edit_proxy_ip_ontab,
    .onshifttab  = edit_proxy_ip_ontab,
};

static void edit_proxy_port_ontab(EDIT *UNUSED(edit)) {
    edit_setfocus(&edit_proxy_ip);
}

EDIT edit_proxy_port = {
    .panel = {
        .type   = PANEL_EDIT,
        .x      = 360,
        .y      =  87,
        .width  =  60,
        .height =  24,
    },
    .data        = edit_proxy_port_data,
    .data_size   = sizeof edit_proxy_port_data,
    .empty_str = {.i18nal = STR_PROXY_EDIT_HINT_PORT },
    .onlosefocus = edit_proxy_ip_port_onlosefocus,
    .ontab       = edit_proxy_port_ontab,
    .onshifttab  = edit_proxy_port_ontab,
};

EDIT edit_video_fps = {
    .data        = edit_video_fps_data,
    .data_size   = sizeof edit_video_fps_data,
    .onlosefocus = edit_video_fps_onlosefocus,
    /* .empty_str = {.i18nal = STR_PROXY_EDIT_HINT_PORT }, */
};

static void edit_profile_password_update(EDIT *UNUSED(edit)) {
    if (tox_thread_init) {
        postmessage_toxcore(TOX_SAVE, 0, 0, NULL);
    }
}

EDIT edit_profile_password = {
    .panel = {
        .type   = PANEL_EDIT,
        .x      =  10,
        .y      =  55,
        .width  = -10,
        .height =  24,
    },
    .data_size = sizeof edit_profile_password_data,
    .data      = edit_profile_password_data,
    // .onchange    = edit_profile_password_update,
    .onlosefocus = edit_profile_password_update,
    .password    = 1,
};

static void edit_toxid_ontab(EDIT *UNUSED(edit)) {
    edit_setfocus(&edit_name);
}

static void edit_toxid_onshifttab(EDIT *UNUSED(edit)) {
    edit_setfocus(&edit_status_msg);
}

EDIT edit_toxid = {
    .panel = {
        .type   = PANEL_EDIT,
        .x      =  10,
        .y      = 140,
        .width  = -10,
        .height =  24,
    },
    .length = TOX_ADDRESS_SIZE * 2,
    .data = self.id_str,
    .readonly = 1,
    .noborder = 0,
    .select_completely = 1,
    .ontab      = edit_toxid_ontab,
    .onshifttab = edit_toxid_onshifttab,
};

static void edit_change_nospam_onenter(EDIT *edit) {
    char *endptr;
    edit_nospam_data[edit->length] = '\0';
    long int nospam = strtol(edit_nospam_data, &endptr, 16);
    if (nospam == 0 || nospam < 0) {
        LOG_ERR("Nospam", "Invalid nospam value: %lu", nospam);
        return;
    } else if (endptr == edit_nospam_data) {
        LOG_ERR("Nospam", "No numbers found.");
        return;
    }
    postmessage_toxcore(TOX_SELF_CHANGE_NOSPAM, nospam, 0, NULL);
}

EDIT edit_nospam = {
    .panel = {
        .type   = PANEL_EDIT,
        .x      =  10,
        .y      = 235,
        .width  = -10,
        .height =  24,
    },
    .data_size    = sizeof edit_nospam_data,
    .data         = edit_nospam_data,
    .noborder     = false,
    .onenter      = edit_change_nospam_onenter,
    .onlosefocus  = edit_change_nospam_onenter,
};


static char edit_add_new_device_to_self_data[TOX_ADDRESS_SIZE * 4];
static void edit_add_new_device_to_self_onenter(EDIT *UNUSED(edit)) {
#ifdef ENABLE_MULTIDEVICE
    devices_self_add(edit_add_new_device_to_self.data, edit_add_new_device_to_self.length);
#endif
}

EDIT edit_add_new_device_to_self = {
    .data      = edit_add_new_device_to_self_data,
    .data_size = sizeof edit_add_new_device_to_self_data,
    .onenter   = edit_add_new_device_to_self_onenter,
};
