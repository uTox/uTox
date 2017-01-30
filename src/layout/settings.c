#include "settings.h"

#include "tree.h"

#include "../macros.h"
#include "../theme.h"
#include "../self.h"
#include "../flist.h"

#include "../ui/draw.h"
#include "../ui/svg.h"
#include "../ui/scrollable.h"

#include <stdio.h>

#include "../main.h" // tox_thread

/* Top bar for user settings */
static void draw_settings_header(int UNUSED(x), int UNUSED(y), int UNUSED(w), int UNUSED(height)) {
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

#define DRAW_UNDERLINE() drawhline(x, y + SCALE(30), x_right_edge, COLOR_EDGE_NORMAL)
#define DRAW_OVERLINE()                                   \
    drawhline(x, y + 0, x_right_edge, COLOR_EDGE_ACTIVE); \
    drawhline(x, y + 1, x_right_edge, COLOR_EDGE_ACTIVE)

static void draw_settings_sub_header(int x, int y, int UNUSED(w), int UNUSED(height)) {
    setfont(FONT_SELF_NAME);

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

    /* Draw the text and bars for device settings */
    #ifdef ENABLE_MULTIDEVICE
    setcolor(!button_settings_sub_devices.mouseover ? COLOR_MAIN_TEXT : COLOR_MAIN_TEXT_SUBTEXT);
    x            = x_right_edge;
    x_right_edge = x_right_edge + SCALE(10) + UTOX_STR_WIDTH(DEVICES_BUTTON) + SCALE(10);
    drawstr(x + SCALE(10), y + SCALE(10), DEVICES_BUTTON);

    if (panel_settings_devices.disabled) {
        DRAW_UNDERLINE();
    } else {
        DRAW_OVERLINE();
    }
    drawvline(x_right_edge, y + SCALE(0), y + SCALE(30), COLOR_EDGE_NORMAL);
    #endif

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
    x_right_edge = x_right_edge + SCALE(10) + UTOX_STR_WIDTH(AUDIO_VIDEO_BUTTON) + SCALE(10);
    drawstr(x + SCALE(10), y + SCALE(10), AUDIO_VIDEO_BUTTON);

    if (panel_settings_av.disabled) {
        DRAW_UNDERLINE();
    } else {
        DRAW_OVERLINE();
    }
    drawvline(x_right_edge, y + SCALE(0), y + SCALE(30), COLOR_EDGE_NORMAL);

    /* Draw the text and bars for notification settings */
    setcolor(!button_settings_sub_notifications.mouseover ? COLOR_MAIN_TEXT : COLOR_MAIN_TEXT_SUBTEXT);
    x            = x_right_edge;
    x_right_edge = x_right_edge + SCALE(10) + UTOX_STR_WIDTH(NOTIFICATIONS_BUTTON) + SCALE(10);
    drawstr(x + SCALE(10), y + SCALE(10), NOTIFICATIONS_BUTTON);

    if (panel_settings_notifications.disabled) {
        DRAW_UNDERLINE();
    } else {
        DRAW_OVERLINE();
    }
    drawvline(x_right_edge, y + SCALE(0), y + SCALE(30), COLOR_EDGE_NORMAL);

    /* Draw the text and bars for advanced settings */
    setcolor(!button_settings_sub_adv.mouseover ? COLOR_MAIN_TEXT : COLOR_MAIN_TEXT_SUBTEXT);
    x            = x_right_edge;
    x_right_edge = x_right_edge + SCALE(10) + UTOX_STR_WIDTH(ADVANCED_BUTTON) + SCALE(10);
    drawstr(x + SCALE(10), y + SCALE(10), ADVANCED_BUTTON);

    if (panel_settings_adv.disabled) {
        DRAW_UNDERLINE();
    } else {
        DRAW_OVERLINE();
    }
    drawvline(x_right_edge, y + SCALE(0), y + SCALE(30), COLOR_EDGE_NORMAL);

    x            = x_right_edge;
    x_right_edge = x_right_edge + SCALE(1000);
    DRAW_UNDERLINE();
}

/* draw switch profile top bar */
/* Text content for settings page */
static void draw_settings_text_profile(int UNUSED(x), int y, int UNUSED(w), int UNUSED(h)) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(10), NAME);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(60), STATUSMESSAGE);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(110), TOXID);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(160), LANGUAGE);
}

// Devices settings page
static void draw_settings_text_devices(int UNUSED(x), int y, int UNUSED(w), int UNUSED(h)) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(10), DEVICES_ADD_NEW);

    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(60), DEVICES_NUMBER);

    char   str[10];
    size_t strlen = snprintf(str, 10, "%zu", self.device_list_count);

    drawtext(MAIN_LEFT + SCALE(10), y + SCALE(75), str, strlen);
}

static void draw_settings_text_password(int UNUSED(x), int y, int UNUSED(w), int UNUSED(h)) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(225), PROFILE_PASSWORD);

    setfont(FONT_MISC);
    setcolor(C_RED);
    drawstr(MAIN_LEFT + SCALE(75), y + SCALE(275), PROFILE_PW_WARNING);
    drawstr(MAIN_LEFT + SCALE(75), y + SCALE(289), PROFILE_PW_NO_RECOVER);
}

static void draw_nospam_settings(int UNUSED(x), int y, int UNUSED(w), int UNUSED(h)){
    setfont(FONT_MISC);
    setcolor(C_RED);
    drawstr(MAIN_LEFT + SCALE(80), y + SCALE(230), NOSPAM_WARNING);

    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);

    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(225), NOSPAM);
}

// UI settings page
static void draw_settings_text_ui(int UNUSED(x), int y, int UNUSED(w), int UNUSED(height)) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(MAIN_LEFT + SCALE(150), y + SCALE(10), DPI);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(10), THEME);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(65), SAVE_CHAT_HISTORY);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(95), CLOSE_TO_TRAY);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(125), START_IN_TRAY);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(155), AUTO_STARTUP);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(185), SETTINGS_UI_MINI_ROSTER);
}

// Audio/Video settings page
static void draw_settings_text_av(int UNUSED(x), int y, int UNUSED(w), int UNUSED(height)) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);

    // The element is draw_pos_y_inc units apart and they start draw_pos_y down.
    uint16_t draw_pos_y = 10;
    uint16_t draw_pos_y_inc = 30;

    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(draw_pos_y), PUSH_TO_TALK);
    draw_pos_y += draw_pos_y_inc;
#ifdef AUDIO_FILTERING
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(draw_pos_y), AUDIOFILTERING);
    draw_pos_y += draw_pos_y_inc;
#endif

    // These are 60 apart as there needs to be room for a dropdown between them.
    draw_pos_y_inc = 60;

    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(draw_pos_y), AUDIOINPUTDEVICE);
    draw_pos_y += draw_pos_y_inc;
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(draw_pos_y), AUDIOOUTPUTDEVICE);
    draw_pos_y += draw_pos_y_inc;
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(draw_pos_y), VIDEOINPUTDEVICE);
    draw_pos_y += draw_pos_y_inc;
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(draw_pos_y), PREVIEW);
}

// Notification settings page
static void draw_settings_text_notifications(int UNUSED(x), int y, int UNUSED(w), int UNUSED(height)) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);

    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(10), RINGTONE);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(40), STATUS_NOTIFICATIONS);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(70), SEND_TYPING_NOTIFICATIONS);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(100), GROUP_NOTIFICATIONS);
}

static void draw_settings_text_adv(int UNUSED(x), int y, int UNUSED(w), int UNUSED(height)) {
    setfont(FONT_MISC);
    setcolor(C_RED);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(10), WARNING);

    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(30), IPV6);

    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(60), UDP);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(90), PROXY);
    drawtext(MAIN_LEFT + SCALE(264), y + SCALE(114), ":", 1);

    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(140), AUTO_UPDATE);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(170), BLOCK_FRIEND_REQUESTS);
}


SCROLLABLE scrollbar_settings = {
    .panel = { .type = PANEL_SCROLLABLE, },
    .color = C_SCROLL,
};

#include "../ui/edits.h"
#include "../ui/switches.h"
#include "../ui/dropdowns.h"


/* Draw the text for profile password window */
static void draw_profile_password(int UNUSED(x), int UNUSED(y), int UNUSED(w), int UNUSED(height)) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(MAIN_LEFT + SCALE(10), SCALE(20), PROFILE_PASSWORD);

    setcolor(COLOR_MAIN_TEXT_SUBTEXT);
    setfont(FONT_TEXT);
    drawstr(MAIN_LEFT + SCALE(10), MAIN_TOP + SCALE(10), PROFILE_PASSWORD);
}

PANEL panel_profile_password = {
            .type = PANEL_NONE,
            .disabled = 0,
            .drawfunc = draw_profile_password,
            .child = (PANEL*[]) {
                (PANEL*)&edit_profile_password,
                NULL
            }
        };

PANEL   panel_nospam_settings = {
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
        };

PANEL   panel_settings_master = {
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
                    (PANEL*)&edit_status,
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
                    (PANEL*)&dropdown_proxy,
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
    .on_mup      = button_settings_on_mup,
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
        // .update  = button_setcolors_success,
        .on_mup = button_add_device_to_self_mdown,
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
    utox_export_chatlog_init(((FRIEND *)flist_get_selected()->data)->number);
}

static void button_change_nospam_on_mup(void) {
    button_revert_nospam.disabled = false;
    postmessage_toxcore(TOX_SELF_CHANGE_NOSPAM, 1, 0, NULL);
}

#include "../logging_native.h"
static void button_revert_nospam_on_mup(void) {
    if (self.old_nospam == 0 || self.nospam == self.old_nospam) { //nospam can not be 0
        debug_error("Invalid or current nospam: %u.\n", self.old_nospam);
        return;
    }
    postmessage_toxcore(TOX_SELF_CHANGE_NOSPAM, 0, 0, NULL);
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


// TODO delete button_setcolor_* and move this setting and logic to the struct
/* Quick color change functions */
static void button_setcolors_success(BUTTON *b) {
    b->c1  = COLOR_BTN_SUCCESS_BKGRND;
    b->c2  = COLOR_BTN_SUCCESS_BKGRND_HOVER;
    b->c3  = COLOR_BTN_SUCCESS_BKGRND_HOVER;
    b->ct1 = COLOR_BTN_SUCCESS_TEXT;
    b->ct2 = COLOR_BTN_SUCCESS_TEXT_HOVER;
}

static void button_setcolors_danger(BUTTON *b) {
    b->c1  = COLOR_BTN_DANGER_BACKGROUND;
    b->c2  = COLOR_BTN_DANGER_BKGRND_HOVER;
    b->c3  = COLOR_BTN_DANGER_BKGRND_HOVER;
    b->ct1 = COLOR_BTN_DANGER_TEXT;
    b->ct2 = COLOR_BTN_DANGER_TEXT_HOVER;
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
        debug("Button ERR:\tVideo_width = 0, can't preview\n");
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
    .bm          = BM_SBUTTON,
    .button_text = {.i18nal = STR_COPY_TOX_ID },
    .update   = button_setcolors_success,
    .on_mup  = button_copyid_on_mup,
    .disabled = false,
};

BUTTON button_callpreview = {
    .bm       = BM_LBUTTON,
    .bm2      = BM_CALL,
    .bw       = _BM_LBICON_WIDTH,
    .bh       = _BM_LBICON_HEIGHT,
    .on_mup  = button_audiopreview_on_mup,
    .update   = button_audiopreview_update,
    .disabled = false,
};

BUTTON button_videopreview = {
    .bm       = BM_LBUTTON,
    .bm2      = BM_VIDEO,
    .bw       = _BM_LBICON_WIDTH,
    .bh       = _BM_LBICON_HEIGHT,
    .on_mup  = button_videopreview_on_mup,
    .update   = button_videopreview_update,
    .disabled = false,
};

BUTTON button_lock_uTox = {
    .bm          = BM_SBUTTON,
    .update      = button_setcolors_success,
    .on_mup     = button_lock_uTox_on_mup,
    .button_text = {.i18nal = STR_LOCK },
    .tooltip_text = {.i18nal = STR_LOCK_UTOX },
};

BUTTON button_show_password_settings = {
    .bm          = BM_SBUTTON,
    .update      = button_setcolors_success,
    .on_mup     = button_show_password_settings_on_mup,
    .button_text = {.i18nal = STR_SHOW_UI_PASSWORD },
    .tooltip_text = {.i18nal = STR_SHOW_UI_PASSWORD_TOOLTIP },
};

BUTTON button_export_chatlog = {
    .bm          = BM_SBUTTON,
    .button_text = {.i18nal = STR_FRIEND_EXPORT_CHATLOG },
    .update   = button_setcolors_success,
    .on_mup  = button_export_chatlog_on_mup,
    .disabled = false,
};

BUTTON button_change_nospam = {
    .bm           = BM_SBUTTON,
    .update       = button_setcolors_success,
    .tooltip_text = {.i18nal = STR_RANDOMIZE_NOSPAM},
    .button_text  = {.i18nal = STR_RANDOMIZE_NOSPAM},
    .on_mup       = button_change_nospam_on_mup,
};

BUTTON button_revert_nospam = {
    .disabled     = true,
    .bm           = BM_SBUTTON,
    .update       = button_setcolors_success,
    .tooltip_text = {.i18nal = STR_REVERT_NOSPAM},
    .button_text  = {.i18nal = STR_REVERT_NOSPAM},
    .on_mup       = button_revert_nospam_on_mup,
};

BUTTON button_show_nospam = {
    .bm           = BM_SBUTTON,
    .update       = button_setcolors_success,
    .tooltip_text = {.i18nal = STR_SHOW_NOSPAM},
    .button_text  = {.i18nal = STR_SHOW_NOSPAM},
    .on_mup       = button_show_nospam_on_mup,
};
