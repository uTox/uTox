#include "test.h"
#include "mock/mock_ui.h"

#include <stdio.h>

#include "../src/ui/switch.h"

static int tip_count;

void tooltip_new(MAYBE_I18NAL_STRING *text) {
    (void)text;
    tip_count++;
}

#include "../src/ui/switch.c"

static int up_count;
static int right_count;
static int update_count;

static void on_up(void) {
    up_count++;
}

static void on_right(void) {
    right_count++;
}

static void on_update(UISWITCH *s) {
    (void)s;
    update_count++;
}

static UISWITCH make_switch(void) {
    UISWITCH s;
    memset(&s, 0, sizeof s);
    s.on_mup  = on_up;
    s.onright = on_right;
    return s;
}

static void reset_counts(void) {
    mock_ui_reset();
    tip_count    = 0;
    up_count     = 0;
    right_count  = 0;
    update_count = 0;
}

bool test_switch_toggle(void) {
    reset_counts();
    UISWITCH s = make_switch();
    const int w = 60, h = 25;

    if (switch_mdown(&s) || switch_mup(&s) || switch_mright(&s)) {
        FAIL("no-op without mouseover");
    }
    if (switch_mwheel(&s, h, 1.0, true)) {
        FAIL("wheel is unused");
    }

    if (!switch_mmove(&s, 0, 0, w, h, 2, 2, 0, 0) || !s.mouseover) {
        FAIL("hover");
    }
    if (cursor != CURSOR_HAND) {
        FAIL("hover should set hand cursor");
    }
    if (!switch_mdown(&s) || !s.mousedown) {
        FAIL("mdown");
    }
    if (switch_mdown(&s)) {
        FAIL("held mdown should not retrigger");
    }
    if (!switch_mup(&s) || s.switch_on != true || up_count != 1 || s.mousedown) {
        FAIL("mup should toggle on");
    }

    switch_mmove(&s, 0, 0, w, h, 2, 2, 0, 0);
    switch_mdown(&s);
    if (!switch_mup(&s) || s.switch_on != false || up_count != 2) {
        FAIL("second click should toggle off");
    }

    switch_mmove(&s, 0, 0, w, h, 2, 2, 0, 0);
    switch_mdown(&s);
    switch_mmove(&s, 0, 0, w, h, -1, 0, 0, 0);
    if (!switch_mup(&s) || s.switch_on != false || up_count != 2) {
        FAIL("release outside should not toggle");
    }

    switch_mmove(&s, 0, 0, w, h, 2, 2, 0, 0);
    if (!switch_mright(&s) || right_count != 1) {
        FAIL("right click");
    }
    if (!switch_mleave(&s) || s.mouseover) {
        FAIL("mleave");
    }
    return true;
}

bool test_switch_disabled(void) {
    reset_counts();
    UISWITCH s = make_switch();
    s.disabled = true;
    const int w = 60, h = 25;

    switch_mmove(&s, 0, 0, w, h, 2, 2, 0, 0);
    if (cursor != CURSOR_NONE) {
        FAIL("disabled hover should not set hand");
    }
    if (!switch_mdown(&s) || !s.mousedown) {
        FAIL("disabled still records mousedown");
    }
    if (switch_mup(&s) || s.switch_on || up_count != 0 || s.mousedown) {
        FAIL("disabled mup must not toggle and must clear mousedown");
    }
    return true;
}

bool test_switch_null_on_mup(void) {
    reset_counts();
    UISWITCH s = make_switch();
    s.on_mup = NULL;
    switch_mmove(&s, 0, 0, 60, 25, 2, 2, 0, 0);
    switch_mdown(&s);
    if (!switch_mup(&s) || !s.switch_on) {
        FAIL("NULL on_mup should still toggle");
    }
    return true;
}

bool test_switch_tooltip_draw_update(void) {
    reset_counts();
    UISWITCH s = make_switch();
    char tip[] = "tip";
    maybe_i18nal_string_set_plain(&s.tooltip_text, tip, 3);
    s.update         = on_update;
    s.style_outer    = 1;
    s.style_toggle   = 2;
    s.style_icon_off = 3;
    s.style_icon_on  = 4;

    if (!switch_mmove(&s, 0, 0, 60, 25, 1, 1, 0, 0) || tip_count != 1) {
        FAIL("tooltip_new on hover");
    }

    s.panel.x = -8;
    switch_mmove(&s, 0, 0, 60, 25, 1, 1, 0, 0);

    COLOR_BTN_SUCCESS_BKGRND       = 1;
    COLOR_BTN_SUCCESS_TEXT         = 2;
    COLOR_BTN_SUCCESS_BKGRND_HOVER = 3;
    COLOR_BTN_DISABLED_BKGRND      = 4;
    COLOR_BTN_DISABLED_FORGRND     = 5;
    COLOR_BTN_DISABLED_BKGRND_HOVER = 6;
    switch_update(&s);
    if (s.toggle_w != BM_SWITCH_TOGGLE_WIDTH) {
        FAIL("switch_update sets toggle size");
    }

    switch_draw(&s, 0, 0, 60, 25);
    if (update_count != 1) {
        FAIL("draw calls update");
    }

    s.switch_on = true;
    switch_draw(&s, 0, 0, 60, 25);
    switch_update(&s);

    s.nodraw = true;
    int alpha = mock_draw_alpha_count;
    switch_draw(&s, 0, 0, 60, 25);
    if (update_count != 2) {
        FAIL("nodraw skips update");
    }
    if (mock_draw_alpha_count != alpha) {
        FAIL("nodraw skips paint");
    }

    s.nodraw   = false;
    s.disabled = true;
    switch_draw(&s, 0, 0, 60, 25);
    return true;
}

int main(void) {
    int result = 0;
    RUN_TEST(test_switch_toggle)
    RUN_TEST(test_switch_disabled)
    RUN_TEST(test_switch_null_on_mup)
    RUN_TEST(test_switch_tooltip_draw_update)
    return result;
}
