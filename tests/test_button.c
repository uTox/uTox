#include "test.h"
#include "mock/mock_ui.h"

#include <stdio.h>
#include <string.h>

#include "../src/ui/button.h"

static int tip_count;
static MAYBE_I18NAL_STRING *last_tip;

void tooltip_new(MAYBE_I18NAL_STRING *text) {
    tip_count++;
    last_tip = text;
}

#include "../src/ui/button.c"

static int up_count;
static int down_count;
static int right_count;
static int update_count;

static void on_up(void) {
    up_count++;
}

static void on_down(void) {
    down_count++;
}

static void on_right(void) {
    right_count++;
}

static void on_update(BUTTON *b) {
    (void)b;
    update_count++;
}

static BUTTON make_button(void) {
    BUTTON b;
    memset(&b, 0, sizeof b);
    b.on_mup  = on_up;
    b.on_mdn  = on_down;
    b.onright = on_right;
    return b;
}

static void reset_counts(void) {
    mock_ui_reset();
    tip_count     = 0;
    last_tip      = NULL;
    up_count      = 0;
    down_count    = 0;
    right_count   = 0;
    update_count  = 0;
}

bool test_button_hit_and_leave(void) {
    reset_counts();
    BUTTON b = make_button();
    const int w = 80, h = 24;

    if (button_mmove(&b, 0, 0, w, h, -1, 0, 0, 0) || b.mouseover) {
        FAIL("outside left should miss");
    }
    if (!button_mmove(&b, 0, 0, w, h, 0, 0, 0, 0) || !b.mouseover) {
        FAIL("top-left corner is inside");
    }
    if (cursor != CURSOR_HAND) {
        FAIL("hover should set hand cursor");
    }
    if (button_mmove(&b, 0, 0, w, h, 10, 10, 0, 0)) {
        FAIL("still over should not request redraw");
    }
    if (!button_mmove(&b, 0, 0, w, h, w, 0, 0, 0) || b.mouseover) {
        FAIL("right edge is exclusive");
    }

    b.mouseover = true;
    if (!button_mleave(&b) || b.mouseover) {
        FAIL("mleave should clear mouseover");
    }
    if (button_mleave(&b)) {
        FAIL("second mleave should return false");
    }
    return true;
}

bool test_button_press_release(void) {
    reset_counts();
    BUTTON b = make_button();
    const int w = 80, h = 24;

    if (button_mdown(&b) || button_mup(&b) || button_mright(&b)) {
        FAIL("no-op without mouseover/mousedown");
    }
    if (button_mwheel(&b, h, 1.0, true)) {
        FAIL("wheel is unused");
    }

    button_mmove(&b, 0, 0, w, h, 4, 4, 0, 0);
    if (!button_mdown(&b) || !b.mousedown || down_count != 1) {
        FAIL("mdown on hover should press and call on_mdn");
    }
    if (button_mdown(&b) || down_count != 1) {
        FAIL("second mdown while held should not retrigger on_mdn");
    }
    if (!button_mup(&b) || b.mousedown || up_count != 1) {
        FAIL("mup on hover should click");
    }

    button_mmove(&b, 0, 0, w, h, 4, 4, 0, 0);
    button_mdown(&b);
    button_mmove(&b, 0, 0, w, h, -5, 0, 0, 0);
    if (!button_mup(&b) || up_count != 1) {
        FAIL("release outside should not click");
    }
    if (b.mousedown) {
        FAIL("release should clear mousedown");
    }

    button_mmove(&b, 0, 0, w, h, 4, 4, 0, 0);
    if (!button_mright(&b) || right_count != 1) {
        FAIL("right click on hover");
    }
    return true;
}

bool test_button_disabled(void) {
    reset_counts();
    BUTTON b = make_button();
    b.disabled = true;
    const int w = 80, h = 24;

    if (!button_mmove(&b, 0, 0, w, h, 4, 4, 0, 0) || !b.mouseover) {
        FAIL("disabled still tracks hover");
    }
    if (cursor != CURSOR_NONE) {
        FAIL("disabled hover should not set hand cursor");
    }
    if (!button_mdown(&b) || !b.mousedown || down_count != 1) {
        FAIL("disabled still receives mdown");
    }
    if (!button_mup(&b) || up_count != 1 || b.mousedown) {
        FAIL("disabled still receives mup");
    }
    if (!button_mright(&b) || right_count != 1) {
        FAIL("disabled still receives right-click");
    }
    return true;
}

bool test_button_tooltip_and_wide_text(void) {
    reset_counts();
    BUTTON b = make_button();
    char label[] = "a-quite-long-label";
    maybe_i18nal_string_set_plain(&b.button_text, label, (uint16_t)strlen(label));
    maybe_i18nal_string_set_plain(&b.tooltip_text, label, (uint16_t)strlen(label));

    const int w = 8, h = 20;
    if (!button_mmove(&b, 0, 0, w, h, 0, 0, 0, 0) || tip_count != 1) {
        FAIL("hover with tooltip_text should call tooltip_new");
    }
    if (last_tip != &b.tooltip_text) {
        FAIL("tooltip_new should get the button tooltip string");
    }

    /* textwidth == length (18) > 8, so hit box grows past the original width. */
    button_mmove(&b, 0, 0, w, h, 18, 0, 0, 0);
    if (!b.mouseover) {
        FAIL("expanded text width should still hit");
    }

    b.panel.x = -10;
    button_mmove(&b, 0, 0, w, h, -1, 0, 0, 0);
    if (!b.mouseover) {
        FAIL("right-aligned expansion should extend to the left");
    }
    return true;
}

bool test_button_draw_and_colors(void) {
    reset_counts();
    BUTTON b = make_button();
    b.update = on_update;
    char label[] = "ok";
    maybe_i18nal_string_set_plain(&b.button_text, label, 2);

    COLOR_BTN_SUCCESS_BKGRND       = 1;
    COLOR_BTN_SUCCESS_BKGRND_HOVER = 2;
    COLOR_BTN_SUCCESS_TEXT         = 3;
    COLOR_BTN_SUCCESS_TEXT_HOVER   = 4;
    button_setcolors_success(&b);
    if (b.c1 != 1 || b.c2 != 2 || b.ct1 != 3 || b.ct2 != 4) {
        FAIL("success colors");
    }

    COLOR_BTN_DANGER_BACKGROUND  = 5;
    COLOR_BTN_DANGER_BKGRND_HOVER = 6;
    COLOR_BTN_DANGER_TEXT        = 7;
    COLOR_BTN_DANGER_TEXT_HOVER  = 8;
    button_setcolors_danger(&b);

    COLOR_BTN_WARNING_BKGRND       = 9;
    COLOR_BTN_WARNING_BKGRND_HOVER = 10;
    COLOR_BTN_WARNING_TEXT         = 11;
    COLOR_BTN_WARNING_TEXT_HOVER   = 12;
    button_setcolors_warning(&b);

    COLOR_BTN_DISABLED_BKGRND = 13;
    COLOR_BTN_DISABLED_TEXT   = 14;
    button_setcolors_disabled(&b);
    if (b.c1 != 13 || b.ct1 != 14) {
        FAIL("disabled colors");
    }

    button_draw(&b, 0, 0, 40, 20);
    if (update_count != 1 || mock_draw_fill_count < 1 || mock_draw_text_count < 1) {
        FAIL("draw should update, fill, and paint text");
    }

    b.nodraw = true;
    int fills = mock_draw_fill_count;
    button_draw(&b, 0, 0, 40, 20);
    if (update_count != 2) {
        FAIL("nodraw still runs update");
    }
    if (mock_draw_fill_count != fills) {
        FAIL("nodraw should skip painting");
    }

    b.nodraw  = false;
    b.bm_fill = 1;
    b.bm_icon = 2;
    b.icon_w  = 8;
    b.icon_h  = 8;
    button_draw(&b, 0, 0, 10, 20); /* text wider than real_w: fill-extend loop */

    b.disabled = true;
    button_draw(&b, 0, 0, 40, 20);
    return true;
}

bool test_button_null_callbacks(void) {
    reset_counts();
    BUTTON b;
    memset(&b, 0, sizeof b);
    button_mmove(&b, 0, 0, 40, 20, 1, 1, 0, 0);
    if (!button_mdown(&b) || !button_mup(&b) || button_mright(&b)) {
        FAIL("NULL callbacks must not crash; right-click is a no-op");
    }
    return true;
}

int main(void) {
    int result = 0;
    RUN_TEST(test_button_hit_and_leave)
    RUN_TEST(test_button_press_release)
    RUN_TEST(test_button_disabled)
    RUN_TEST(test_button_tooltip_and_wide_text)
    RUN_TEST(test_button_draw_and_colors)
    RUN_TEST(test_button_null_callbacks)
    return result;
}
