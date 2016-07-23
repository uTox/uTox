#include "../main.h"

static void switch_set_colors(UISWITCH *s) {
    if (s->switch_on) {
        s->bg_color     = COLOR_BUTTON_SUCCESS_BACKGROUND;
        s->sw_color     = COLOR_BUTTON_SUCCESS_TEXT;
        s->press_color  = COLOR_BUTTON_SUCCESS_HOVER_BACKGROUND;
        s->hover_color  = COLOR_BUTTON_SUCCESS_HOVER_BACKGROUND;
    } else {
        s->bg_color     = COLOR_BUTTON_DANGER_BACKGROUND;
        s->sw_color     = COLOR_BUTTON_DANGER_TEXT;
        s->hover_color  = COLOR_BUTTON_DANGER_HOVER_BACKGROUND;
        s->press_color  = COLOR_BUTTON_DANGER_HOVER_BACKGROUND;
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


UISWITCH switch_logging = {
    .style        = BM_SWITCH,
    .update       = switch_set_colors,
    .onpress      = switchfxn_logging,
    .tooltip_text = { .i18nal = STR_LOGGING },
};

UISWITCH switch_mini_contacts = {
    .style        = BM_SWITCH,
    .update       = switch_set_colors,
    .onpress      = switchfxn_mini_contacts,
    .tooltip_text = { .i18nal = STR_SETTINGS_UI_MINI_ROSTER },
};

UISWITCH switch_ipv6 = {
    .style        = BM_SWITCH,
    .update       = switch_set_colors,
    .onpress      = switchfxn_ipv6,
    .tooltip_text = { .i18nal = STR_IPV6 },
};

UISWITCH switch_udp = {
    .style        = BM_SWITCH,
    .update       = switch_set_colors,
    .onpress      = switchfxn_udp,
    .tooltip_text = { .i18nal = STR_UDP },
};
