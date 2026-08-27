#include "test.h"
#include "mock/mock_ui.h"

#include <stdio.h>
#include <string.h>

#include "../src/ui/tooltip.h"

#include "../src/ui/tooltip.c"

static MAYBE_I18NAL_STRING tip_str;

static void set_tip(const char *text) {
    maybe_i18nal_string_set_plain(&tip_str, (char *)text, (uint16_t)strlen(text));
}

bool test_tooltip_new_show_hide(void) {
    mock_ui_reset();
    tooltip_reset();
    set_tip("hello");
    mouse.x = 40;
    mouse.y = 50;
    settings.window_width  = 640;
    settings.window_height = 480;

    tooltip_new(&tip_str);
    if (mock_thread_count != 1) {
        FAIL("first tooltip_new should start the delay thread");
    }

    tooltip_show();
    mock_draw_fill_count  = 0;
    mock_draw_frame_count = 0;
    mock_draw_text_count  = 0;
    tooltip_draw();
    if (mock_draw_fill_count < 1 || mock_draw_text_count < 1) {
        FAIL("visible tooltip should paint");
    }

    if (!tooltip_mmove()) {
        FAIL("move should hide a visible tooltip");
    }
    int fills = mock_draw_fill_count;
    tooltip_draw();
    if (mock_draw_fill_count != fills) {
        FAIL("hidden tooltip should not paint");
    }
    if (tooltip_mmove()) {
        FAIL("move while hidden should return false");
    }
    return true;
}

bool test_tooltip_mdown_blocks_show(void) {
    mock_ui_reset();
    tooltip_reset();
    set_tip("hello");

    if (tooltip_mdown()) {
        FAIL("mdown returns false");
    }
    tooltip_new(&tip_str);
    tooltip_show(); /* mouse_down: can_show was cleared by mdown, so still hidden */
    mock_draw_fill_count = 0;
    tooltip_draw();
    if (mock_draw_fill_count != 0) {
        FAIL("tooltip must not show while mouse is down");
    }

    if (tooltip_mup()) {
        FAIL("mup returns false");
    }
    tooltip_new(&tip_str);
    tooltip_show();
    tooltip_draw();
    if (mock_draw_fill_count < 1) {
        FAIL("after mup, tooltip_new + show should paint");
    }
    return true;
}

bool test_tooltip_reset(void) {
    mock_ui_reset();
    tooltip_reset();
    set_tip("hello");
    tooltip_new(&tip_str);
    tooltip_show();
    tooltip_reset();
    mock_draw_fill_count = 0;
    tooltip_draw();
    if (mock_draw_fill_count != 0) {
        FAIL("reset should hide");
    }
    return true;
}

bool test_tooltip_clamps_to_window(void) {
    mock_ui_reset();
    tooltip_reset();
    char long_tip[] = "this-tooltip-is-wide";
    maybe_i18nal_string_set_plain(&tip_str, long_tip, (uint16_t)strlen(long_tip));

    settings.window_width  = 30;
    settings.window_height = 40;
    mouse.x                = 25;
    mouse.y                = 35;
    tooltip_new(&tip_str);
    tooltip_show();
    tooltip_draw(); /* width/position clamp paths in calculate_pos_and_width */

    mouse.y = 0;
    tooltip_reset();
    tooltip_new(&tip_str);
    tooltip_show();
    tooltip_draw();
    return true;
}

int main(void) {
    int result = 0;
    RUN_TEST(test_tooltip_new_show_hide)
    RUN_TEST(test_tooltip_mdown_blocks_show)
    RUN_TEST(test_tooltip_reset)
    RUN_TEST(test_tooltip_clamps_to_window)
    return result;
}
