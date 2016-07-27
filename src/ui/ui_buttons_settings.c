#include "../main.h"

extern SCROLLABLE scrollbar_settings;

static void button_settings_onpress(void) {
    if (tox_thread_init) {
        list_selectsettings();
    }
}

static void disable_all_setting_sub(void) {
    list_selectsettings();
    panel_settings_profile.disabled = 1;
    panel_settings_devices.disabled = 1;
    panel_settings_net.disabled     = 1;
    panel_settings_ui.disabled      = 1;
    panel_settings_av.disabled      = 1;
}

static void button_settings_sub_profile_onpress(void){
    scrollbar_settings.content_height = SCALE(260);
    disable_all_setting_sub();
    panel_settings_profile.disabled = 0;
}

static void button_settings_sub_devices_onpress(void){
    scrollbar_settings.content_height = SCALE(260);
    disable_all_setting_sub();
    panel_settings_devices.disabled = 0;
}

static void button_settings_sub_net_onpress(void){
    scrollbar_settings.content_height = SCALE(180);
    disable_all_setting_sub();
    panel_settings_net.disabled = 0;
}

static void button_settings_sub_ui_onpress(void){
    scrollbar_settings.content_height = SCALE(280);
    disable_all_setting_sub();
    panel_settings_ui.disabled = 0;
}

static void button_settings_sub_av_onpress(void){
    scrollbar_settings.content_height = SCALE(300);
    disable_all_setting_sub();
    panel_settings_av.disabled = 0;
}

static void button_bottommenu_update(BUTTON *b) {
    b->c1 = COLOR_BACKGROUND_MENU;
    b->c2 = COLOR_BACKGROUND_MENU_HOVER;
    b->c3 = COLOR_BACKGROUND_MENU_ACTIVE;
    b->ct1 = COLOR_MENU_TEXT;
    b->ct2 = COLOR_MENU_TEXT;
    if (b->mousedown || b->disabled) {
        b->ct1 = COLOR_MENU_ACTIVE_TEXT;
        b->ct2 = COLOR_MENU_ACTIVE_TEXT;
    }
    b->cd = COLOR_BACKGROUND_MENU_ACTIVE;
}

static void button_add_device_to_self_mdown(void) {
    #ifdef ENABLE_MULTIDEVICE
    devices_self_add(edit_add_new_device_to_self.data, edit_add_new_device_to_self.length);
    edit_resetfocus();
    #endif
}

BUTTON button_settings = {
    .bm2          = BM_SETTINGS,
    .bw           = _BM_ADD_WIDTH,
    .bh           = _BM_ADD_WIDTH,
    .update       = button_bottommenu_update,
    .onpress      = button_settings_onpress,
    .disabled     = 0,
    .nodraw       = 0,
    .tooltip_text = { .i18nal = STR_USERSETTINGS },
},

button_settings_sub_profile = {
    .nodraw       = 1,
    .onpress      = button_settings_sub_profile_onpress,
    .tooltip_text = { .i18nal = STR_UTOX_SETTINGS },
},

button_settings_sub_devices = {
    .nodraw       = 1,
    .onpress      = button_settings_sub_devices_onpress,
    .tooltip_text = { .i18nal = STR_UTOX_SETTINGS },
},

button_settings_sub_net = {
    .nodraw       = 1,
    .onpress      = button_settings_sub_net_onpress,
    .tooltip_text = { .i18nal = STR_NETWORK_SETTINGS },
},

button_settings_sub_ui = {
    .nodraw       = 1,
    .onpress      = button_settings_sub_ui_onpress,
    .tooltip_text = { .i18nal = STR_USERSETTINGS },
},

button_settings_sub_av = {
    .nodraw       = 1,
    .onpress      = button_settings_sub_av_onpress,
    .tooltip_text = { .i18nal = STR_AUDIO_VIDEO },
},

button_add_new_device_to_self = {
    .bm             = BM_SBUTTON,
    .button_text    = { .i18nal = STR_ADD },
    .update         = button_setcolors_success,
    .onpress        = button_add_device_to_self_mdown,
};
