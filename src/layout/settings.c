#include "settings.h"

#include "../debug.h"
#include "../flist.h"
#include "../macros.h"
#include "../self.h"
#include "../theme.h"

#include "../ui/button.h"
#include "../ui/draw.h"
#include "../ui/dropdown.h"
#include "../ui/edit.h"
#include "../ui/scrollable.h"
#include "../ui/svg.h"
#include "../ui/switch.h"

#include <stdio.h>

#include "../main.h" // tox_thread

/* Top bar for user settings */
static void draw_settings_header(int x, int y, int w, int UNUSED(height)) {
    (void)w; // Silence an irreverent warning when GIT_VERSION is undefined
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(SCALE(x + 10), SCALE(y + 10), UTOX_SETTINGS);
#ifdef GIT_VERSION
    x += SCALE(20) + UTOX_STR_WIDTH(UTOX_SETTINGS);
    setfont(FONT_TEXT);
    drawtext(SCALE(x), SCALE(10), GIT_VERSION, strlen(GIT_VERSION));
    char ver_string[64];
    int  count;
    count = snprintf(ver_string, 64, "Toxcore v%u.%u.%u", tox_version_major(), tox_version_minor(), tox_version_patch());
    drawtextwidth_right(w - textwidth(ver_string, count), textwidth(ver_string, count), SCALE(10),
                        ver_string, count);
#endif
}

#define DRAW_UNDERLINE() drawhline(x, SCALE(y + 30), next_x, COLOR_EDGE_NORMAL)
#define DRAW_OVERLINE()  drawhline(x, SCALE(y + 0), next_x, COLOR_EDGE_ACTIVE); \
                         drawhline(x, SCALE(y + 1), next_x, COLOR_EDGE_ACTIVE)

static void draw_settings_sub_header(int x, int y, int width, int UNUSED(height)) {
    setfont(FONT_SELF_NAME);
    int last_x = x + width;

    /* Draw the text and bars for general settings */
    setcolor(!button_settings_sub_profile.mouseover ? COLOR_MAIN_TEXT : COLOR_MAIN_TEXT_SUBTEXT);
    int next_x = SCALE(x + 20) + UTOX_STR_WIDTH(PROFILE_BUTTON);
    drawstr(SCALE(x + 10), SCALE(y + 10), PROFILE_BUTTON);

    if (panel_settings_profile.disabled) {
        DRAW_UNDERLINE();
    } else {
        DRAW_OVERLINE();
    }
    drawvline(next_x, SCALE(y), SCALE(y + 30), COLOR_EDGE_NORMAL);
    x = next_x;

    /* Draw the text and bars for device settings */
    #ifdef ENABLE_MULTIDEVICE
    setcolor(!button_settings_sub_devices.mouseover ? COLOR_MAIN_TEXT : COLOR_MAIN_TEXT_SUBTEXT);
    next_x += SCALE(20) + UTOX_STR_WIDTH(DEVICES_BUTTON);
    drawstr(SCALE(x + 10), SCALE(y + 10), DEVICES_BUTTON);
    if (panel_settings_devices.disabled) {
        DRAW_UNDERLINE();
    } else {
        DRAW_OVERLINE();
    }
    drawvline(next_x, SCALE(y), SCALE(y + 30), COLOR_EDGE_NORMAL);
    x = next_x;
    #endif

    /* Draw the text and bars for User interface settings */
    setcolor(!button_settings_sub_ui.mouseover ? COLOR_MAIN_TEXT : COLOR_MAIN_TEXT_SUBTEXT);
    next_x += SCALE(20) + UTOX_STR_WIDTH(USER_INTERFACE_BUTTON);
    drawstr(SCALE(x + 10), SCALE(y + 10), USER_INTERFACE_BUTTON);
    if (panel_settings_ui.disabled) {
        DRAW_UNDERLINE();
    } else {
        DRAW_OVERLINE();
    }
    drawvline(next_x, SCALE(y), SCALE(y + 30), COLOR_EDGE_NORMAL);
    x = next_x;

    /* Draw the text and bars for A/V settings */
    setcolor(!button_settings_sub_av.mouseover ? COLOR_MAIN_TEXT : COLOR_MAIN_TEXT_SUBTEXT);
    next_x += SCALE(20) + UTOX_STR_WIDTH(AUDIO_VIDEO_BUTTON);
    drawstr(SCALE(x + 10), SCALE(y + 10), AUDIO_VIDEO_BUTTON);
    if (panel_settings_av.disabled) {
        DRAW_UNDERLINE();
    } else {
        DRAW_OVERLINE();
    }
    drawvline(next_x, SCALE(y), SCALE(y + 30), COLOR_EDGE_NORMAL);
    x = next_x;

    /* Draw the text and bars for notification settings */
    setcolor(!button_settings_sub_notifications.mouseover ? COLOR_MAIN_TEXT : COLOR_MAIN_TEXT_SUBTEXT);
    next_x += SCALE(20) + UTOX_STR_WIDTH(NOTIFICATIONS_BUTTON);
    drawstr(SCALE(x + 10), SCALE(y + 10), NOTIFICATIONS_BUTTON);
    if (panel_settings_notifications.disabled) {
        DRAW_UNDERLINE();
    } else {
        DRAW_OVERLINE();
    }
    drawvline(next_x, SCALE(y), SCALE(y + 30), COLOR_EDGE_NORMAL);
    x = next_x;

    /* Draw the text and bars for advanced settings */
    setcolor(!button_settings_sub_adv.mouseover ? COLOR_MAIN_TEXT : COLOR_MAIN_TEXT_SUBTEXT);
    next_x += SCALE(20) + UTOX_STR_WIDTH(ADVANCED_BUTTON);
    drawstr(SCALE(x + 10), SCALE(y + 10), ADVANCED_BUTTON);
    if (panel_settings_adv.disabled) {
        DRAW_UNDERLINE();
    } else {
        DRAW_OVERLINE();
    }
    drawvline(next_x, SCALE(y), SCALE(y + 30), COLOR_EDGE_NORMAL);
    x = next_x;

    next_x = last_x;
    DRAW_UNDERLINE();
}

/* draw switch profile top bar */
/* Text content for settings page */
static void draw_settings_text_profile(int x, int y, int UNUSED(w), int UNUSED(h)) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(SCALE(x + 10), SCALE(y + 10), NAME);
    drawstr(SCALE(x + 10), SCALE(y + 60), STATUSMESSAGE);
    drawstr(SCALE(x + 10), SCALE(y + 110), TOXID);
    drawstr(SCALE(x + 10), SCALE(y + 160), LANGUAGE);
}

// Devices settings page
static void draw_settings_text_devices(int x, int y, int UNUSED(w), int UNUSED(h)) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(SCALE(x + 10), SCALE(y + 10), DEVICES_ADD_NEW);

    drawstr(SCALE(x + 10), SCALE(y + 60), DEVICES_NUMBER);

    char   str[10];
    size_t strlen = snprintf(str, 10, "%zu", self.device_list_count);

    drawtext(SCALE(x + 10), SCALE(y + 75), str, strlen);
}

static void draw_settings_text_password(int x, int y, int UNUSED(w), int UNUSED(h)) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(SCALE(x + 10), SCALE(y + 235), PROFILE_PASSWORD);

    setfont(FONT_MISC);
    setcolor(C_RED);
    drawstr(x + 75, y + 285, PROFILE_PW_WARNING);
    drawstr(x + 75, y + 299, PROFILE_PW_NO_RECOVER);
}

static void draw_nospam_settings(int x, int y, int UNUSED(w), int UNUSED(h)){
    setfont(FONT_MISC);
    setcolor(C_RED);
    drawstr(x + 75, y + 240, NOSPAM_WARNING);

    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);

    drawstr(SCALE(x + 10), SCALE(y + 235), NOSPAM);
}

// UI settings page
static void draw_settings_text_ui(int x, int y, int UNUSED(w), int UNUSED(height)) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(x + 150, y + 10,  DPI);
    drawstr(SCALE(x + 10),  y + 10,  THEME);
    drawstr(SCALE(x + 10),  y + 65,  SAVE_CHAT_HISTORY);
    drawstr(SCALE(x + 10),  y + 95,  CLOSE_TO_TRAY);
    drawstr(SCALE(x + 10),  y + 125, START_IN_TRAY);
    drawstr(SCALE(x + 10),  y + 155, AUTO_STARTUP);
    drawstr(SCALE(x + 10),  y + 185, SETTINGS_UI_MINI_ROSTER);
}

// Audio/Video settings page
static void draw_settings_text_av(int x, int y, int UNUSED(w), int UNUSED(height)) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);

    // The element is draw_pos_y_inc units apart and they start draw_pos_y down.
    uint16_t draw_pos_y = 10;
    uint16_t draw_pos_y_inc = 30;

    drawstr(SCALE(x + 10), SCALE(y + draw_pos_y), PUSH_TO_TALK);
    draw_pos_y += draw_pos_y_inc;
#ifdef AUDIO_FILTERING
    drawstr(SCALE(x + 10), SCALE(y + draw_pos_y), AUDIOFILTERING);
    draw_pos_y += draw_pos_y_inc;
#endif

    // These are 60 apart as there needs to be room for a dropdown between them.
    draw_pos_y_inc = 60;

    drawstr(SCALE(x + 10), SCALE(y + draw_pos_y), AUDIOINPUTDEVICE);
    draw_pos_y += draw_pos_y_inc;
    drawstr(SCALE(x + 10), SCALE(y + draw_pos_y), AUDIOOUTPUTDEVICE);
    draw_pos_y += draw_pos_y_inc;
    drawstr(SCALE(x + 10), SCALE(y + draw_pos_y), VIDEOINPUTDEVICE);
    draw_pos_y += draw_pos_y_inc;
    drawstr(SCALE(x + 10), SCALE(y + draw_pos_y), PREVIEW);
}

// Notification settings page
static void draw_settings_text_notifications(int x, int y, int UNUSED(w), int UNUSED(height)) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);

    drawstr(SCALE(x + 10), SCALE(y + 10), RINGTONE);
    drawstr(SCALE(x + 10), SCALE(y + 40), STATUS_NOTIFICATIONS);
    drawstr(SCALE(x + 10), SCALE(y + 70), SEND_TYPING_NOTIFICATIONS);
    drawstr(SCALE(x + 10), SCALE(y + 100), GROUP_NOTIFICATIONS);
}

static void draw_settings_text_adv(int x, int y, int UNUSED(w), int UNUSED(height)) {
    setfont(FONT_MISC);
    setcolor(C_RED);
    drawstr(SCALE(x + 10), SCALE(y + 10), WARNING);

    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);

    drawstr(SCALE(x + 10), SCALE(y + 30),  IPV6);
    drawstr(SCALE(x + 10), SCALE(y + 60),  UDP);
    drawstr(SCALE(x + 10), SCALE(y + 90),  PROXY);
    drawtext(SCALE(x + 264), SCALE(y + 94), ":", 1); // Little addr port separator
    drawstr(SCALE(x + 10), SCALE(y + 120), PROXY_FORCE); // TODO draw ONLY when settings.use_proxy = true

    drawstr(SCALE(x + 10), SCALE(y + 150), AUTO_UPDATE);
    drawstr(SCALE(x + 10), SCALE(y + 180), BLOCK_FRIEND_REQUESTS);
}


SCROLLABLE scrollbar_settings = {
    .panel = { .type = PANEL_SCROLLABLE, },
    .color = C_SCROLL,
};

/* Draw the text for profile password window */
static void draw_profile_password(int x, int UNUSED(y), int UNUSED(w), int UNUSED(height)) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(SCALE(x + 10), 20, PROFILE_PASSWORD);

    setcolor(COLOR_MAIN_TEXT_SUBTEXT);
    setfont(FONT_TEXT);
    drawstr(SCALE(x + 10), MAIN_TOP + 10, PROFILE_PASSWORD);
}

PANEL
panel_profile_password = {
    .type = PANEL_NONE,
    .disabled = 0,
    .drawfunc = draw_profile_password,
    .child = (PANEL*[]) {
        (PANEL*)&edit_profile_password,
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
        (PANEL*)&edit_profile_password,
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
            (PANEL*)&dropdown_language,
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
            (PANEL*)&dropdown_dpi,
            (PANEL*)&dropdown_theme,
            (PANEL*)&switch_save_chat_history,
            (PANEL*)&switch_close_to_tray,
            (PANEL*)&switch_start_in_tray,
            (PANEL*)&switch_auto_startup,
            (PANEL*)&switch_mini_contacts,
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
            (PANEL*)&switch_auto_update,
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

BUTTON button_settings = {
    .bm2          = BM_SETTINGS,
    .bw           = _BM_ADD_WIDTH,
    .bh           = _BM_ADD_WIDTH,
    .update       = button_bottommenu_update,
    .on_mup       = button_settings_on_mup,
    .disabled     = false,
    .nodraw       = false,
    .tooltip_text = {.i18nal = STR_USERSETTINGS },
};

static void disable_all_setting_sub(void) {
    flist_selectsettings();
    panel_settings_profile.disabled         = true;
    panel_settings_devices.disabled         = true;
    panel_settings_ui.disabled              = true;
    panel_settings_av.disabled              = true;
    panel_settings_notifications.disabled   = true;
    panel_settings_adv.disabled             = true;
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
    .tooltip_text = {.i18nal = STR_UTOX_SETTINGS },
},

button_settings_sub_devices = {
    .nodraw = true,
    .on_mup = button_settings_sub_devices_on_mup,
    .tooltip_text = {.i18nal = STR_UTOX_SETTINGS },
},

button_settings_sub_ui = {
    .nodraw = true,
    .on_mup = button_settings_sub_ui_on_mup,
    .tooltip_text = {.i18nal = STR_USERSETTINGS },
},

button_settings_sub_av = {
    .nodraw = true,
    .on_mup = button_settings_sub_av_on_mup,
    .tooltip_text = {.i18nal = STR_AUDIO_VIDEO },
},

button_settings_sub_adv = {
    .nodraw = true,
    .on_mup = button_settings_sub_adv_on_mup,
    .tooltip_text = {.i18nal = STR_ADVANCED_BUTTON },
},

button_settings_sub_notifications = {
    .nodraw = true,
    .on_mup = button_settings_sub_notifications_on_mup,
    .tooltip_text = {.i18nal = STR_NOTIFICATIONS_BUTTON },
},

button_add_new_device_to_self = {
    .bm          = BM_SBUTTON,
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
    }
    button_show_password_settings.disabled = false;
    button_show_password_settings.nodraw = false;
}

static void button_show_password_settings_on_mup(void) {
    panel_nospam_settings.disabled = true;
    panel_profile_password_settings.disabled = !panel_profile_password_settings.disabled;
}


#include "../chatlog.h"
#include "../flist.h"
#include "../friend.h"
static void button_export_chatlog_on_mup(void) {
    FRIEND *f = flist_get_friend();
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
    if (self.old_nospam == 0 || self.nospam == self.old_nospam) { //nospam can not be 0
        LOG_ERR("Settings", "Invalid or current nospam: %u.", self.old_nospam);
        return;
    }
    postmessage_toxcore(TOX_SELF_CHANGE_NOSPAM, self.old_nospam, 0, NULL);
    button_revert_nospam.disabled = true;
}

static void button_show_nospam_on_mup(void) {
    panel_profile_password_settings.disabled = true;
    panel_nospam_settings.disabled = !panel_nospam_settings.disabled;
}

#include "../main_native.h"
static void button_copyid_on_mup(void) {
    edit_setfocus(&edit_toxid);
    copy(0);
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
        postmessage_utoxav(UTOXAV_STOP_VIDEO, 0, 1, NULL);
    } else if (video_width && video_height) {
        postmessage_utoxav(UTOXAV_START_VIDEO, 0, 1, NULL);
    } else {
        LOG_ERR("Button", "Video_width = 0, can't preview\n");
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
BUTTON button_copyid = {
    .bm       = BM_SBUTTON,
    .update   = button_setcolors_success,
    .on_mup   = button_copyid_on_mup,
    .disabled = false,
    .button_text = {.i18nal = STR_COPY_TOX_ID },
};

BUTTON button_callpreview = {
    .bm       = BM_LBUTTON,
    .bm2      = BM_CALL,
    .bw       = _BM_LBICON_WIDTH,
    .bh       = _BM_LBICON_HEIGHT,
    .on_mup   = button_audiopreview_on_mup,
    .update   = button_audiopreview_update,
    .disabled = false,
};

BUTTON button_videopreview = {
    .bm       = BM_LBUTTON,
    .bm2      = BM_VIDEO,
    .bw       = _BM_LBICON_WIDTH,
    .bh       = _BM_LBICON_HEIGHT,
    .on_mup   = button_videopreview_on_mup,
    .update   = button_videopreview_update,
    .disabled = false,
};

BUTTON button_lock_uTox = {
    .bm     = BM_SBUTTON,
    .update = button_setcolors_success,
    .on_mup = button_lock_uTox_on_mup,
    .button_text = {.i18nal = STR_LOCK },
    .tooltip_text = {.i18nal = STR_LOCK_UTOX },
};

BUTTON button_show_password_settings = {
    .bm     = BM_SBUTTON,
    .update = button_setcolors_success,
    .on_mup = button_show_password_settings_on_mup,
    .button_text = {.i18nal = STR_SHOW_UI_PASSWORD },
    .tooltip_text = {.i18nal = STR_SHOW_UI_PASSWORD_TOOLTIP },
};

BUTTON button_export_chatlog = {
    .bm     = BM_SBUTTON,
    .update = button_setcolors_success,
    .on_mup = button_export_chatlog_on_mup,
    .disabled = false,
    .button_text = {.i18nal = STR_FRIEND_EXPORT_CHATLOG },
};

BUTTON button_change_nospam = {
    .bm     = BM_SBUTTON,
    .update = button_setcolors_success,
    .on_mup = button_change_nospam_on_mup,
    .tooltip_text = {.i18nal = STR_RANDOMIZE_NOSPAM},
    .button_text  = {.i18nal = STR_RANDOMIZE_NOSPAM},
};

BUTTON button_revert_nospam = {
    .bm       = BM_SBUTTON,
    .update   = button_setcolors_success,
    .on_mup   = button_revert_nospam_on_mup,
    .disabled = true,
    .tooltip_text = {.i18nal = STR_REVERT_NOSPAM},
    .button_text  = {.i18nal = STR_REVERT_NOSPAM},
};

BUTTON button_show_nospam = {
    .bm           = BM_SBUTTON,
    .update       = button_setcolors_success,
    .tooltip_text = {.i18nal = STR_SHOW_NOSPAM},
    .button_text  = {.i18nal = STR_SHOW_NOSPAM},
    .on_mup       = button_show_nospam_on_mup,
};

static void switch_set_colors(UISWITCH *s) {
    if (s->switch_on) {
        s->bg_color    = COLOR_BTN_SUCCESS_BKGRND;
        s->sw_color    = COLOR_BTN_SUCCESS_TEXT;
        s->press_color = COLOR_BTN_SUCCESS_BKGRND_HOVER;
        s->hover_color = COLOR_BTN_SUCCESS_BKGRND_HOVER;
    } else {
        s->bg_color    = COLOR_BTN_DISABLED_BKGRND;
        s->sw_color    = COLOR_BTN_DISABLED_FORGRND;
        s->hover_color = COLOR_BTN_DISABLED_BKGRND_HOVER;
        s->press_color = COLOR_BTN_DISABLED_BKGRND_HOVER;
    }
}

static void switch_set_size(UISWITCH *s) {
    s->toggle_w   = BM_SWITCH_TOGGLE_WIDTH;
    s->toggle_h   = BM_SWITCH_TOGGLE_HEIGHT;
    s->icon_off_w = BM_FB_WIDTH;
    s->icon_off_h = BM_FB_HEIGHT;
    s->icon_on_w  = BM_FB_WIDTH;
    s->icon_on_h  = BM_FB_HEIGHT;
}

static void switch_update(UISWITCH *s) {
    switch_set_colors(s);
    switch_set_size(s);
}

static void switchfxn_logging(void) { settings.logging_enabled = !settings.logging_enabled; }

static void switchfxn_mini_contacts(void) {
    settings.use_mini_flist = !settings.use_mini_flist;
    flist_re_scale();
}

static void switchfxn_ipv6(void) {
    settings.enable_ipv6 = !settings.enable_ipv6;
    tox_settingschanged();
}

static void switchfxn_udp(void) {
    settings.enable_udp = !settings.enable_udp;
    tox_settingschanged();
}

static void switchfxn_close_to_tray(void) { settings.close_to_tray = !settings.close_to_tray; }

static void switchfxn_start_in_tray(void) { settings.start_in_tray = !settings.start_in_tray; }

static void switchfxn_auto_startup(void) { settings.start_with_system = !settings.start_with_system; }

static void switchfxn_typing_notes(void) { settings.send_typing_status = !settings.send_typing_status; }

static void switchfxn_audible_notifications(void) { settings.ringtone_enabled = !settings.ringtone_enabled; }

static void switchfxn_push_to_talk(void) {
    if (!settings.push_to_talk) {
        init_ptt();
    } else {
        exit_ptt();
    }
}

static void switchfxn_audio_filtering(void) { settings.audiofilter_enabled = !settings.audiofilter_enabled; }

static void switchfxn_status_notifications(void) { settings.status_notifications = !settings.status_notifications; }

static void switchfxn_auto_update(void) { settings.auto_update = !settings.auto_update; }

static void switchfxn_block_friend_requests(void) { settings.block_friend_requests = !settings.block_friend_requests; }

UISWITCH switch_save_chat_history = {
    .style_outer    = BM_SWITCH,
    .style_toggle   = BM_SWITCH_TOGGLE,
    .style_icon_off = BM_NO,
    .style_icon_on  = BM_YES,
    .update         = switch_update,
    .on_mup         = switchfxn_logging,
    .tooltip_text   = {.i18nal = STR_SAVE_CHAT_HISTORY },
};

UISWITCH switch_mini_contacts = {
    .style_outer    = BM_SWITCH,
    .style_toggle   = BM_SWITCH_TOGGLE,
    .style_icon_off = BM_NO,
    .style_icon_on  = BM_YES,
    .update         = switch_update,
    .on_mup         = switchfxn_mini_contacts,
    .tooltip_text   = {.i18nal = STR_SETTINGS_UI_MINI_ROSTER },
};

UISWITCH switch_ipv6 = {
    .style_outer    = BM_SWITCH,
    .style_toggle   = BM_SWITCH_TOGGLE,
    .style_icon_off = BM_NO,
    .style_icon_on  = BM_YES,
    .update         = switch_update,
    .on_mup         = switchfxn_ipv6,
    .tooltip_text   = {.i18nal = STR_IPV6 },
};

UISWITCH switch_udp = {
    .style_outer    = BM_SWITCH,
    .style_toggle   = BM_SWITCH_TOGGLE,
    .style_icon_off = BM_NO,
    .style_icon_on  = BM_YES,
    .update         = switch_update,
    .on_mup         = switchfxn_udp,
    .tooltip_text   = {.i18nal = STR_UDP },
};

UISWITCH switch_close_to_tray = {
    .style_outer    = BM_SWITCH,
    .style_toggle   = BM_SWITCH_TOGGLE,
    .style_icon_off = BM_NO,
    .style_icon_on  = BM_YES,
    .update         = switch_update,
    .on_mup         = switchfxn_close_to_tray,
    .tooltip_text   = {.i18nal = STR_CLOSE_TO_TRAY },
};

UISWITCH switch_start_in_tray = {
    .style_outer    = BM_SWITCH,
    .style_toggle   = BM_SWITCH_TOGGLE,
    .style_icon_off = BM_NO,
    .style_icon_on  = BM_YES,
    .update         = switch_update,
    .on_mup         = switchfxn_start_in_tray,
    .tooltip_text   = {.i18nal = STR_START_IN_TRAY },
};

UISWITCH switch_auto_startup = {
    .style_outer    = BM_SWITCH,
    .style_toggle   = BM_SWITCH_TOGGLE,
    .style_icon_off = BM_NO,
    .style_icon_on  = BM_YES,
    .update         = switch_update,
    .on_mup         = switchfxn_auto_startup,
    .tooltip_text   = {.i18nal = STR_AUTO_STARTUP },
};

UISWITCH switch_typing_notes = {
    .style_outer    = BM_SWITCH,
    .style_toggle   = BM_SWITCH_TOGGLE,
    .style_icon_off = BM_NO,
    .style_icon_on  = BM_YES,
    .update         = switch_update,
    .on_mup         = switchfxn_typing_notes,
    .tooltip_text   = {.i18nal = STR_SEND_TYPING_NOTIFICATIONS },
};

UISWITCH switch_audible_notifications = {
    .style_outer    = BM_SWITCH,
    .style_toggle   = BM_SWITCH_TOGGLE,
    .style_icon_off = BM_NO,
    .style_icon_on  = BM_YES,
    .update         = switch_update,
    .on_mup         = switchfxn_audible_notifications,
    .tooltip_text   = {.i18nal = STR_AUDIONOTIFICATIONS },
};

UISWITCH switch_push_to_talk = {
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
    .style_outer    = BM_SWITCH,
    .style_toggle   = BM_SWITCH_TOGGLE,
    .style_icon_off = BM_NO,
    .style_icon_on  = BM_YES,
    .update         = switch_update,
    .on_mup         = switchfxn_status_notifications,
    .tooltip_text   = {.i18nal = STR_STATUS_NOTIFICATIONS },
};

UISWITCH switch_auto_update = {
    .style_outer    = BM_SWITCH,
    .style_toggle   = BM_SWITCH_TOGGLE,
    .style_icon_off = BM_NO,
    .style_icon_on  = BM_YES,
    .update         = switch_update,
    .on_mup         = switchfxn_auto_update,
    .tooltip_text   = {.i18nal = STR_AUTO_UPDATE }
};

UISWITCH switch_block_friend_requests = {
    .style_outer    = BM_SWITCH,
    .style_toggle   = BM_SWITCH_TOGGLE,
    .style_icon_off = BM_NO,
    .style_icon_on  = BM_YES,
    .update         = switch_update,
    .on_mup         = switchfxn_block_friend_requests,
    .tooltip_text   = {.i18nal = STR_BLOCK_FRIEND_REQUESTS },
};

static void switchfxn_proxy(void) {
    settings.use_proxy   = !settings.use_proxy;
    if (settings.use_proxy) {
        settings.force_proxy = false;
        switch_proxy_force.panel.disabled = false;
    } else {
        switch_proxy_force.panel.disabled = true;
    }

    memcpy(proxy_address, edit_proxy_ip.data, edit_proxy_ip.length);
    proxy_address[edit_proxy_ip.length] = 0;

    edit_proxy_port.data[edit_proxy_port.length] = 0;
    settings.proxy_port = strtol((char *)edit_proxy_port.data, NULL, 0);

    tox_settingschanged();
}

static void switchfxn_proxy_force(void) {
    settings.force_proxy = !settings.force_proxy;

    if (settings.force_proxy) {
        switch_udp.disabled       = true;
        switch_udp.panel.disabled = true;
    }

    edit_proxy_port.data[edit_proxy_port.length] = 0;
    settings.proxy_port = strtol((char *)edit_proxy_port.data, NULL, 0);

    tox_settingschanged();
}

UISWITCH switch_proxy = {
    .style_outer    = BM_SWITCH,
    .style_toggle   = BM_SWITCH_TOGGLE,
    .style_icon_off = BM_NO,
    .style_icon_on  = BM_YES,
    .update         = switch_update,
    .on_mup         = switchfxn_proxy,
    .tooltip_text   = {.i18nal = STR_PROXY }
};

UISWITCH switch_proxy_force = {
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

#include "../screen_grab.h"
static void dropdown_video_onselect(uint16_t i, const DROPDOWN *UNUSED(dm)) {
    if (i == 1) {
        utox_screen_grab_desktop(1);
    } else {
        postmessage_utoxav(UTOXAV_SET_VIDEO_IN, i, 0, NULL);
    }
}

static void dropdown_dpi_onselect(uint16_t i, const DROPDOWN *UNUSED(dm)) {
    ui_set_scale(i + 6);
}

static void dropdown_language_onselect(uint16_t i, const DROPDOWN *UNUSED(dm)) {
    LANG = (UTOX_LANG)i;
    /* The draw functions need the fonts' and scale to be reset when changing languages. */
    ui_set_scale(ui_scale);
}
static STRING *dropdown_language_ondisplay(uint16_t i, const DROPDOWN *UNUSED(dm)) {
    UTOX_LANG l = (UTOX_LANG)i;
    return SPTRFORLANG(l, STR_LANG_NATIVE_NAME);
}

static void dropdown_theme_onselect(const uint16_t i, const DROPDOWN *UNUSED(dm)) {
    theme_load(i);
    settings.theme = i;
}

#include"../groups.h"
static void dropdown_notify_groupchats_onselect(const uint16_t i, const DROPDOWN *UNUSED(dm)) {
    GROUPCHAT *g = flist_get_groupchat();
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
    .ondisplay = simple_dropdown_ondisplay,
    .onselect  = dropdown_dpi_onselect,
    .dropcount = COUNTOF(dpidrops),
    .userdata  = dpidrops
};

DROPDOWN dropdown_language = {
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
    .ondisplay = simple_dropdown_ondisplay,
    .onselect  = dropdown_theme_onselect,
    .dropcount = COUNTOF(themedrops),
    .userdata  = themedrops
};

static UTOX_I18N_STR notifydrops[] = {
    STR_GROUP_NOTIFICATIONS_OFF, STR_GROUP_NOTIFICATIONS_MENTION, STR_GROUP_NOTIFICATIONS_ON,
};

DROPDOWN dropdown_notify_groupchats = {
    .ondisplay = simple_dropdown_ondisplay,
    .onselect  = dropdown_notify_groupchats_onselect,
    .dropcount = COUNTOF(notifydrops),
    .userdata  = notifydrops
};

DROPDOWN dropdown_global_group_notifications = {
    .ondisplay = simple_dropdown_ondisplay,
    .onselect  = dropdown_global_group_notifications_onselect,
    .dropcount = COUNTOF(notifydrops),
    .userdata  = notifydrops
};

static char edit_name_data[128],
            edit_status_msg_data[128],
            edit_proxy_ip_data[256],
            edit_proxy_port_data[8],
            edit_profile_password_data[65535],
            edit_nospam_data[sizeof(uint32_t) * 2];
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

EDIT edit_name = {
    .data        = edit_name_data,
    .maxlength   = sizeof edit_name_data - 1,
    .onenter     = edit_name_onenter,
    .onlosefocus = edit_name_onenter,
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

EDIT edit_status_msg = {
    .data        = edit_status_msg_data,
    .maxlength   = sizeof edit_status_msg_data - 1,
    .onenter     = edit_status_msg_onenter,
    .onlosefocus = edit_status_msg_onenter,
};


static void edit_proxy_ip_port_onlosefocus(EDIT *UNUSED(edit)) {
    edit_proxy_port.data[edit_proxy_port.length] = 0;

    settings.proxy_port = strtol((char *)edit_proxy_port.data, NULL, 0);

    if (memcmp(proxy_address, edit_proxy_ip.data, edit_proxy_ip.length) == 0 && proxy_address[edit_proxy_ip.length] == 0) {
        return;
    }

    memset(proxy_address, 0, 256); /* Magic number from toxcore */
    memcpy(proxy_address, edit_proxy_ip.data, edit_proxy_ip.length);
    proxy_address[edit_proxy_ip.length] = 0;


    if (settings.use_proxy) {
        tox_settingschanged();
    }
}

EDIT edit_proxy_ip = {
    .data        = edit_proxy_ip_data,
    .maxlength   = sizeof edit_proxy_ip_data - 1,
    .onlosefocus = edit_proxy_ip_port_onlosefocus,
    .empty_str = {.i18nal = STR_PROXY_EDIT_HINT_IP },
    /* TODO .ontab = change to proxy port field */
};

EDIT edit_proxy_port = {
    .data        = edit_proxy_port_data,
    .maxlength   = sizeof edit_proxy_port_data - 1,
    .onlosefocus = edit_proxy_ip_port_onlosefocus,
    .empty_str = {.i18nal = STR_PROXY_EDIT_HINT_PORT },
};

static void edit_profile_password_update(EDIT *UNUSED(edit)) {
    if (tox_thread_init) {
        postmessage_toxcore(TOX_SAVE, 0, 0, NULL);
    }
}

EDIT edit_profile_password = {
    .maxlength = sizeof(edit_profile_password) - 1,
    .data      = edit_profile_password_data,
    // .onchange    = edit_profile_password_update,
    .onlosefocus = edit_profile_password_update,
    .password    = 1,
};

EDIT edit_toxid = {
    .length = TOX_ADDRESS_SIZE * 2,
    .data = self.id_str,
    .readonly = 1,
    .noborder = 0,
    .select_completely = 1,
};

static void edit_change_nospam_onenter(EDIT *UNUSED(edit)) {
    long int nospam = strtol(edit_nospam_data, NULL, 16);
    if (nospam == 0 || nospam < 0) {
        LOG_ERR("Nospam", "Invalid nospam value: %lu", nospam);
        return;
    }
    postmessage_toxcore(TOX_SELF_CHANGE_NOSPAM, nospam, 0, NULL);
}

EDIT edit_nospam = {
    .maxlength         = sizeof(edit_nospam_data),
    .data              = edit_nospam_data,
    .noborder          = false,
    .onenter           = edit_change_nospam_onenter,
    .onlosefocus       = edit_change_nospam_onenter,
};


static char edit_add_new_device_to_self_data[TOX_ADDRESS_SIZE * 4];
static void edit_add_new_device_to_self_onenter(EDIT *UNUSED(edit)) {
#ifdef ENABLE_MULTIDEVICE
    devices_self_add(edit_add_new_device_to_self.data, edit_add_new_device_to_self.length);
#endif
}

EDIT edit_add_new_device_to_self = {
    .data      = edit_add_new_device_to_self_data,
    .maxlength = sizeof edit_add_new_device_to_self_data - 1,
    .onenter   = edit_add_new_device_to_self_onenter,
};
