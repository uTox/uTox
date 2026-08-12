#include "test.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../src/ui/draw.h"

/* Provided by scrollable.c via ui.h / draw.h — define before including it. */
double ui_scale;
int font_small_lineheight, font_msg_lineheight;

void drawrect(int x, int y, int right, int bottom, uint32_t color) {
    (void)x;
    (void)y;
    (void)right;
    (void)bottom;
    (void)color;
}

void drawalpha(int bm, int x, int y, int width, int height, uint32_t color) {
    (void)bm;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)color;
}

#include "../src/ui/scrollable.c"

static int expected_line_step(void) {
    int line = font_small_lineheight;
    const int design = SCALE(12);
    if (line < design) {
        line = design;
    }
    line += MESSAGES_SPACING;
    if (line < 1) {
        line = 1;
    }
    return line;
}

static SCROLLABLE make_scroll(int content_height, int viewport_height, double d) {
    SCROLLABLE s;
    memset(&s, 0, sizeof(s));
    s.content_height  = content_height;
    s.viewport_height = viewport_height;
    s.d               = d;
    s.mouseover2      = true;
    return s;
}

static int scroll_offset(const SCROLLABLE *s, int port) {
    const int range = s->content_height - port;
    if (range <= 0) {
        return 0;
    }
    return (int)(s->d * (double)range + 0.5);
}

static void setup_fonts(int lineheight) {
    ui_scale               = 10.0; /* SCALE(x) == x */
    font_small_lineheight  = lineheight;
    font_msg_lineheight    = lineheight;
}

bool test_wheel_step_matches_row(void) {
    setup_fonts(16);
    const int port = 400;
    const int line = expected_line_step();
    SCROLLABLE s   = make_scroll(port + 2000, port, 1.0);

    const int before = scroll_offset(&s, port);
    if (!scroll_mwheel(&s, port, 1.0, true)) {
        FAIL("expected scroll_mwheel to move");
    }
    const int after = scroll_offset(&s, port);
    const int dy    = before - after;

    if (dy != line) {
        FAIL("expected dy=%d (font+spacing), got %d", line, dy);
    }
    if (line != 16 + MESSAGES_SPACING) {
        FAIL("expected line step 16+%d, got %d", MESSAGES_SPACING, line);
    }
    return true;
}

bool test_wheel_step_independent_of_content_length(void) {
    setup_fonts(16);
    const int port = 400;
    const int line = expected_line_step();

    SCROLLABLE short_chat = make_scroll(port + 200, port, 1.0);
    SCROLLABLE long_chat  = make_scroll(port + 20000, port, 1.0);

    const int short_before = scroll_offset(&short_chat, port);
    const int long_before  = scroll_offset(&long_chat, port);

    if (!scroll_mwheel(&short_chat, port, 1.0, true)
        || !scroll_mwheel(&long_chat, port, 1.0, true)) {
        FAIL("expected both scrolls to move");
    }

    const int short_dy = short_before - scroll_offset(&short_chat, port);
    const int long_dy  = long_before - scroll_offset(&long_chat, port);

    if (short_dy != line || long_dy != line) {
        FAIL("steps differ or wrong size: short=%d long=%d expected=%d", short_dy, long_dy, line);
    }
    return true;
}

bool test_wheel_accumulates_fractional_deltas(void) {
    setup_fonts(16);
    const int port = 400;
    const int line = expected_line_step();
    SCROLLABLE s   = make_scroll(port + 2000, port, 1.0);

    const int before = scroll_offset(&s, port);

    /* Sub-pixel wheel events: each is less than one pixel of motion. */
    const double tiny = 0.6 / (double)line;
    if (scroll_mwheel(&s, port, tiny, true)) {
        FAIL("sub-pixel delta should not move yet");
    }
    if (!scroll_mwheel(&s, port, tiny, true)) {
        FAIL("accumulated sub-pixel deltas should move one pixel");
    }

    const int dy = before - scroll_offset(&s, port);
    if (dy != 1) {
        FAIL("expected accumulated dy=1, got %d", dy);
    }
    return true;
}

bool test_wheel_clamps_at_edges(void) {
    setup_fonts(16);
    const int port = 400;
    SCROLLABLE top = make_scroll(port + 1000, port, 0.0);
    SCROLLABLE bot = make_scroll(port + 1000, port, 1.0);

    /* Scrolling further "up" at top should stay at 0. */
    if (!scroll_mwheel(&top, port, 1.0, true)) {
        FAIL("top clamp still applies a step then clamps");
    }
    if (scroll_offset(&top, port) != 0 || top.d != 0.0) {
        FAIL("expected top clamp at 0, d=%g y=%d", top.d, scroll_offset(&top, port));
    }

    /* Drive past the bottom with a large negative delta. */
    if (!scroll_mwheel(&bot, port, -50.0, true)) {
        FAIL("expected large downward scroll");
    }
    const int range = bot.content_height - port;
    if (scroll_offset(&bot, port) != range) {
        FAIL("expected bottom clamp at %d, got %d", range, scroll_offset(&bot, port));
    }
    return true;
}

bool test_wheel_prefers_content_viewport(void) {
    setup_fonts(16);
    const int content_port = 400;
    const int chrome_port  = 410; /* old messages vs scrollbar mismatch */
    const int line         = expected_line_step();
    SCROLLABLE s           = make_scroll(content_port + 2000, content_port, 1.0);

    const int before = scroll_offset(&s, content_port);
    if (!scroll_mwheel(&s, chrome_port, 1.0, true)) {
        FAIL("expected scroll with mismatched chrome height");
    }
    const int after = scroll_offset(&s, content_port);
    if (before - after != line) {
        FAIL("expected dy=%d using content viewport, got %d", line, before - after);
    }
    return true;
}

bool test_wheel_ignores_without_mouseover(void) {
    setup_fonts(16);
    SCROLLABLE s = make_scroll(1000, 400, 0.5);
    s.mouseover2 = false;

    if (scroll_mwheel(&s, 400, 1.0, true)) {
        FAIL("wheel should be ignored without mouseover2");
    }
    if (s.d != 0.5) {
        FAIL("d should be unchanged, got %g", s.d);
    }
    return true;
}

bool test_wheel_noop_when_content_fits(void) {
    setup_fonts(16);
    SCROLLABLE s = make_scroll(300, 400, 0.0);

    if (scroll_mwheel(&s, 400, 1.0, true)) {
        FAIL("wheel should no-op when content fits in viewport");
    }
    return true;
}

bool test_wheel_uses_design_floor_when_font_small(void) {
    setup_fonts(8); /* below SCALE(12) */
    const int port = 400;
    const int line = expected_line_step();
    SCROLLABLE s   = make_scroll(port + 2000, port, 1.0);

    if (line != SCALE(12) + MESSAGES_SPACING) {
        FAIL("expected design floor step %d, got %d", SCALE(12) + MESSAGES_SPACING, line);
    }

    const int before = scroll_offset(&s, port);
    if (!scroll_mwheel(&s, port, 1.0, true)) {
        FAIL("expected scroll");
    }
    if (before - scroll_offset(&s, port) != line) {
        FAIL("dy mismatch");
    }
    return true;
}

int main(void) {
    int result = 0;
    RUN_TEST(test_wheel_step_matches_row)
    RUN_TEST(test_wheel_step_independent_of_content_length)
    RUN_TEST(test_wheel_accumulates_fractional_deltas)
    RUN_TEST(test_wheel_clamps_at_edges)
    RUN_TEST(test_wheel_prefers_content_viewport)
    RUN_TEST(test_wheel_ignores_without_mouseover)
    RUN_TEST(test_wheel_noop_when_content_fits)
    RUN_TEST(test_wheel_uses_design_floor_when_font_small)
    return result;
}
