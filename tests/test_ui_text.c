#include "test.h"
#include "mock/mock_ui.h"

#include <stdio.h>
#include <string.h>

#include "../src/ui/text.h"
#include "../src/ui/scrollable.h"

#include "../src/text.c"
#include "../src/ui/text.c"

bool test_ui_text_height_and_hit(void) {
    mock_ui_reset();
    const uint16_t lh = 12;

    if (text_height(100, lh, "", 0) != lh) {
        FAIL("empty text is one line");
    }
    if (text_height(100, lh, "hello", 5) != lh) {
        FAIL("short unwrapped line");
    }

    char nl[] = "ab\ncd";
    if (text_height(100, lh, nl, 5) != 2 * lh) {
        FAIL("newline should add a line");
    }

    char wrap[] = "abcdef";
    if (text_height(3, lh, wrap, 6) != 2 * lh) {
        FAIL("1px/char wrap at width 3 should be two lines, got %d", text_height(3, lh, wrap, 6));
    }

    if (hittextmultiline(0, 100, -1, 100, lh, "hello", 5, true) != 0) {
        FAIL("above box -> 0");
    }
    if (hittextmultiline(0, 100, 100, 100, lh, "hello", 5, true) != 5) {
        FAIL("below box -> length");
    }

    uint16_t at2 = hittextmultiline(2, 100, 0, 100, lh, "hello", 5, true);
    if (at2 != 2) {
        FAIL("hit x=2 on 1px/char should be index 2, got %u", at2);
    }

    uint16_t wrapped = hittextmultiline(1, 3, lh, 100, lh, wrap, 6, true);
    if (wrapped != 4) {
        FAIL("second wrapped line x=1 should be index 4, got %u", wrapped);
    }

    uint16_t at_nl = hittextmultiline(0, 100, 0, 100, lh, nl, 5, true);
    if (at_nl != 0) {
        FAIL("first line of ab\\ncd, got %u", at_nl);
    }
    uint16_t second = hittextmultiline(0, 100, lh, 100, lh, nl, 5, true);
    if (second != 3) {
        FAIL("start of second line should be 3, got %u", second);
    }
    return true;
}

bool test_ui_text_lineup_linedown(void) {
    mock_ui_reset();
    SCROLLABLE scroll;
    memset(&scroll, 0, sizeof scroll);
    char str[] = "ab\ncd";
    const uint16_t lh = 12;

    uint16_t down = text_linedown(100, 40, 0, lh, str, 5, &scroll);
    if (down < 3) {
        FAIL("linedown from start should reach second line, got %u", down);
    }

    uint16_t up = text_lineup(100, 40, 4, lh, str, 5, &scroll);
    if (up >= 3) {
        FAIL("lineup from end should reach first line, got %u", up);
    }

    uint16_t top = text_lineup(100, 40, 0, lh, str, 5, &scroll);
    if (top != 0 || scroll.d != 0.0) {
        FAIL("lineup at top stays at 0");
    }
    return true;
}

bool test_ui_text_draw_smoke(void) {
    mock_ui_reset();
    const uint16_t lh = 12;
    mock_draw_text_count = 0;

    utox_draw_text_multiline_within_box(0, 0, 80, 0, 80, lh, "hello", 5, 1, 2, 0, 0, false);
    utox_draw_text_multiline_within_box(0, 0, 80, 0, 80, lh, ">quote", 6, 0, 0, 0, 0, true);
    utox_draw_text_multiline_within_box(0, 0, 120, 0, 80, lh, "http://x https://y tox:z", 24, 0, 0, 0, 0, true);
    utox_draw_text_multiline_within_box(0, 0, 80, 0, 80, lh, "red<\nnext", 9, 0, 0, 1, 1, true);

    if (mock_draw_text_count < 1) {
        FAIL("draw helpers should call drawtext");
    }
    return true;
}

int main(void) {
    int result = 0;
    RUN_TEST(test_ui_text_height_and_hit)
    RUN_TEST(test_ui_text_lineup_linedown)
    RUN_TEST(test_ui_text_draw_smoke)
    return result;
}
