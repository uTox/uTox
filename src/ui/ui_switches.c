#include "../main.h"

static void switch_set_colors(UISWITCH *s) {
    if (s->switch_on) {
        s->bg_color     = COLOR_BUTTON_SUCCESS_BACKGROUND;
        s->hover_color  = COLOR_BUTTON_SUCCESS_HOVER_BACKGROUND;
        s->press_color  = COLOR_BUTTON_SUCCESS_HOVER_BACKGROUND;
    } else {
        s->bg_color     = COLOR_BUTTON_DANGER_BACKGROUND;
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


UISWITCH switch_logging = {
    .style        = BM_SWITCH,
    .update       = switch_set_colors,
    .onpress      = switchfxn_logging,
};

UISWITCH switch_mini_contacts = {
    .style        = BM_SWITCH,
    .update       = switch_set_colors,
    .onpress      = switchfxn_mini_contacts,
};
