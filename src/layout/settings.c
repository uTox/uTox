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
void draw_settings_header(int UNUSED(x), int UNUSED(y), int UNUSED(w), int UNUSED(height)) {
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

void draw_settings_sub_header(int x, int y, int UNUSED(w), int UNUSED(height)) {
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
void draw_settings_text_profile(int UNUSED(x), int y, int UNUSED(w), int UNUSED(h)) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(10), NAME);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(60), STATUSMESSAGE);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(110), TOXID);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(160), LANGUAGE);
}

// Devices settings page
void draw_settings_text_devices(int UNUSED(x), int y, int UNUSED(w), int UNUSED(h)) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(10), DEVICES_ADD_NEW);

    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(60), DEVICES_NUMBER);

    char   str[10];
    size_t strlen = snprintf(str, 10, "%zu", self.device_list_count);

    drawtext(MAIN_LEFT + SCALE(10), y + SCALE(75), str, strlen);
}

void draw_settings_text_password(int UNUSED(x), int y, int UNUSED(w), int UNUSED(h)) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(225), PROFILE_PASSWORD);

    setfont(FONT_MISC);
    setcolor(C_RED);
    drawstr(MAIN_LEFT + SCALE(75), y + SCALE(275), PROFILE_PW_WARNING);
    drawstr(MAIN_LEFT + SCALE(75), y + SCALE(289), PROFILE_PW_NO_RECOVER);
}

void draw_nospam_settings(int UNUSED(x), int y, int UNUSED(w), int UNUSED(h)){
    setfont(FONT_MISC);
    setcolor(C_RED);
    drawstr(MAIN_LEFT + SCALE(80), y + SCALE(230), NOSPAM_WARNING);

    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);

    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(225), NOSPAM);
}

// UI settings page
void draw_settings_text_ui(int UNUSED(x), int y, int UNUSED(w), int UNUSED(height)) {
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
void draw_settings_text_av(int UNUSED(x), int y, int UNUSED(w), int UNUSED(height)) {
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
void draw_settings_text_notifications(int UNUSED(x), int y, int UNUSED(w), int UNUSED(height)) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);

    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(10), RINGTONE);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(40), STATUS_NOTIFICATIONS);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(70), SEND_TYPING_NOTIFICATIONS);
    drawstr(MAIN_LEFT + SCALE(10), y + SCALE(100), GROUP_NOTIFICATIONS);
}

void draw_settings_text_adv(int UNUSED(x), int y, int UNUSED(w), int UNUSED(height)) {
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


void draw_friend_settings(int UNUSED(x), int y, int UNUSED(width), int UNUSED(height)) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);

    drawstr(MAIN_LEFT + SCALE(10), y + MAIN_TOP + SCALE(10), FRIEND_PUBLIC_KEY);
    drawstr(MAIN_LEFT + SCALE(10), y + MAIN_TOP + SCALE(60), FRIEND_ALIAS);
    drawstr(MAIN_LEFT + SCALE(10), y + MAIN_TOP + SCALE(110), FRIEND_AUTOACCEPT);
}

extern SCROLLABLE scrollbar_settings;

static void button_settings_on_mup(void) {
    if (tox_thread_init == UTOX_TOX_THREAD_INIT_SUCCESS) {
        flist_selectsettings();
    }
}

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

static void button_add_device_to_self_mdown(void) {
#ifdef ENABLE_MULTIDEVICE
    devices_self_add(edit_add_new_device_to_self.data, edit_add_new_device_to_self.length);
    edit_resetfocus();
#endif
}

BUTTON
    button_settings = {
        .bm2          = BM_SETTINGS,
        .bw           = _BM_ADD_WIDTH,
        .bh           = _BM_ADD_WIDTH,
        .update       = button_bottommenu_update,
        .on_mup      = button_settings_on_mup,
        .disabled     = false,
        .nodraw       = false,
        .tooltip_text = {.i18nal = STR_USERSETTINGS },
    },

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
