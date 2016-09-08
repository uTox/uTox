#include "../main.h"

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

static void switchfxn_logging(void) {
    settings.logging_enabled = !settings.logging_enabled;
}

static void switchfxn_mini_contacts(void) {
    settings.use_mini_roster = !settings.use_mini_roster;
    roster_re_scale();
}

static void switchfxn_ipv6(void) {
    settings.enable_ipv6 = !settings.enable_ipv6;
    tox_settingschanged();
}

static void switchfxn_udp(void) {
    settings.enable_udp = !settings.enable_udp;
    tox_settingschanged();
}

static void switchfxn_close_to_tray(void) {
    settings.close_to_tray = !settings.close_to_tray;
}

static void switchfxn_start_in_tray(void) {
    settings.start_in_tray = !settings.start_in_tray;
}

static void switchfxn_auto_startup(void) {
    settings.start_with_system = !settings.start_with_system;
}

static void switchfxn_typing_notes(void) {
    settings.send_typing_status = !settings.send_typing_status;
}

static void switchfxn_audible_notifications(void) {
    settings.ringtone_enabled = !settings.ringtone_enabled;
}

static void switchfxn_push_to_talk(void) {
    if (!settings.push_to_talk) {
        init_ptt();
    } else {
        exit_ptt();
    }
}

static void switchfxn_audio_filtering(void) {
    settings.audiofilter_enabled = !settings.audiofilter_enabled;
}

UISWITCH switch_logging = {
    .style_outer  = BM_SWITCH,
    .style_toggle = BM_SWITCH_TOGGLE,
    .update       = switch_set_colors,
    .onpress      = switchfxn_logging,
    .tooltip_text = {.i18nal = STR_LOGGING},
};

UISWITCH switch_mini_contacts = {
    .style_outer  = BM_SWITCH,
    .style_toggle = BM_SWITCH_TOGGLE,
    .update       = switch_set_colors,
    .onpress      = switchfxn_mini_contacts,
    .tooltip_text = {.i18nal = STR_SETTINGS_UI_MINI_ROSTER},
};

UISWITCH switch_ipv6 = {
    .style_outer  = BM_SWITCH,
    .style_toggle = BM_SWITCH_TOGGLE,
    .update       = switch_set_colors,
    .onpress      = switchfxn_ipv6,
    .tooltip_text = {.i18nal = STR_IPV6},
};

UISWITCH switch_udp = {
    .style_outer  = BM_SWITCH,
    .style_toggle = BM_SWITCH_TOGGLE,
    .update       = switch_set_colors,
    .onpress      = switchfxn_udp,
    .tooltip_text = {.i18nal = STR_UDP},
};

UISWITCH switch_close_to_tray = {
    .style_outer  = BM_SWITCH,
    .style_toggle = BM_SWITCH_TOGGLE,
    .update       = switch_set_colors,
    .onpress      = switchfxn_close_to_tray,
    .tooltip_text = {.i18nal = STR_CLOSE_TO_TRAY},
};

UISWITCH switch_start_in_tray = {
    .style_outer  = BM_SWITCH,
    .style_toggle = BM_SWITCH_TOGGLE,
    .update       = switch_set_colors,
    .onpress      = switchfxn_start_in_tray,
    .tooltip_text = {.i18nal = STR_START_IN_TRAY},
};

UISWITCH switch_auto_startup = {
    .style_outer  = BM_SWITCH,
    .style_toggle = BM_SWITCH_TOGGLE,
    .update       = switch_set_colors,
    .onpress      = switchfxn_auto_startup,
    .tooltip_text = {.i18nal = STR_AUTO_STARTUP},
};

UISWITCH switch_typing_notes = {
    .style_outer  = BM_SWITCH,
    .style_toggle = BM_SWITCH_TOGGLE,
    .update       = switch_set_colors,
    .onpress      = switchfxn_typing_notes,
    .tooltip_text = {.i18nal = STR_SEND_TYPING_NOTIFICATIONS},
};

UISWITCH switch_audible_notifications = {
    .style_outer  = BM_SWITCH,
    .style_toggle = BM_SWITCH_TOGGLE,
    .update       = switch_set_colors,
    .onpress      = switchfxn_audible_notifications,
    .tooltip_text = {.i18nal = STR_AUDIONOTIFICATIONS},
};

UISWITCH switch_push_to_talk = {
    .style_outer  = BM_SWITCH,
    .style_toggle = BM_SWITCH_TOGGLE,
    .update       = switch_set_colors,
    .onpress      = switchfxn_push_to_talk,
    .tooltip_text = {.i18nal = STR_PUSH_TO_TALK},
};

UISWITCH switch_audio_filtering = {
    .style_outer  = BM_SWITCH,
    .style_toggle = BM_SWITCH_TOGGLE,
    .update       = switch_set_colors,
    .onpress      = switchfxn_audio_filtering,
    .tooltip_text = {.i18nal = STR_AUDIOFILTERING},
};
