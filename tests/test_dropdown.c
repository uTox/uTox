#include "test.h"
#include "mock/mock_ui.h"

#include <stdio.h>
#include <string.h>

#include "../src/ui/dropdown.h"

#include "../src/ui/dropdown.c"

static int select_count;
static uint16_t last_selected;

static void on_select(uint16_t i, const DROPDOWN *d) {
    (void)d;
    select_count++;
    last_selected = i;
}

static STRING *ondisplay(uint16_t i, const DROPDOWN *d) {
    static STRING s;
    static char buf[16];
    (void)d;
    snprintf(buf, sizeof buf, "item-%u", (unsigned)i);
    s.str    = buf;
    s.length = (uint16_t)strlen(buf);
    return &s;
}

static DROPDOWN make_drop(uint16_t n) {
    DROPDOWN d;
    memset(&d, 0, sizeof d);
    d.dropcount = n;
    d.onselect  = on_select;
    d.ondisplay = ondisplay;
    return d;
}

static void reset_counts(void) {
    mock_ui_reset();
    select_count  = 0;
    last_selected = 0xffff;
}

static bool open_at(DROPDOWN *d, int mx, int my, int w, int h) {
    dropdown_mmove(d, 0, 0, w, h, mx, my, 0, 0);
    return dropdown_mdown(d);
}

bool test_dropdown_open_select_close(void) {
    reset_counts();
    DROPDOWN d = make_drop(3);
    const int w = 100, h = 20;

    if (dropdown_mdown(&d) || d.open) {
        FAIL("mdown without hover should not open");
    }
    if (dropdown_mright(&d) || dropdown_mwheel(&d, h, 1.0, true)) {
        FAIL("right/wheel unused");
    }

    if (!open_at(&d, 4, 4, w, h) || !d.open) {
        FAIL("click should open");
    }

    /* Hover the third row (my is relative to the closed box; over = my/h + selected). */
    if (!dropdown_mmove(&d, 0, 0, w, h, 4, h * 2 + 1, 0, 0) || d.over != 2) {
        FAIL("hover row 2, over=%u", d.over);
    }
    if (!dropdown_mup(&d) || d.open || d.selected != 2 || select_count != 1) {
        FAIL("release should select 2 and close");
    }

    if (!open_at(&d, 4, 4, w, h)) {
        FAIL("reopen");
    }
    dropdown_mmove(&d, 0, 0, w, h, -10, 4, 0, 0);
    if (!dropdown_mup(&d) || d.open || select_count != 1) {
        FAIL("release outside should close without selecting");
    }

    if (!dropdown_close(&d) || d.open) {
        FAIL("close is idempotent");
    }
    return true;
}

bool test_dropdown_empty_and_leave(void) {
    reset_counts();
    DROPDOWN d = make_drop(0);
    dropdown_mmove(&d, 0, 0, 80, 20, 4, 4, 0, 0);
    if (dropdown_mdown(&d) || d.open) {
        FAIL("empty list should not open");
    }

    d.dropcount = 2;
    d.ondisplay = ondisplay;
    if (!d.mouseover) {
        FAIL("empty list still tracks collapsed hover");
    }
    if (dropdown_mmove(&d, 0, 0, 80, 20, 5, 5, 0, 0)) {
        FAIL("unchanged hover should not redraw");
    }
    if (!dropdown_mleave(&d) || d.mouseover) {
        FAIL("mleave");
    }
    if (dropdown_mleave(&d)) {
        FAIL("second mleave");
    }
    return true;
}

bool test_dropdown_over_bounds(void) {
    reset_counts();
    DROPDOWN d = make_drop(3);
    d.selected = 2;
    open_at(&d, 4, 4, 80, 20);

    /* Far above: over = my/h + selected - 1 can go negative and used to wrap uint16_t. */
    dropdown_mmove(&d, 0, 0, 80, 20, 4, -40, 0, 0);
    if (d.over >= d.dropcount) {
        FAIL("over wrapped or OOB, got %u", d.over);
    }

    dropdown_mmove(&d, 0, 0, 80, 20, 4, 1, 0, 0);
    if (d.over != 2) {
        FAIL("hover on closed-box row should be selected (2), got %u", d.over);
    }
    return true;
}

bool test_dropdown_list_helpers(void) {
    reset_counts();
    DROPDOWN d;
    memset(&d, 0, sizeof d);
    d.onselect  = on_select;
    d.ondisplay = dropdown_list_ondisplay;

    dropdown_list_add_hardcoded(NULL, "x", NULL);
    dropdown_list_add_hardcoded(&d, NULL, NULL);
    if (d.dropcount != 0) {
        FAIL("NULL add should be ignored");
    }

    char a[] = "alpha";
    char b[] = "beta";
    dropdown_list_add_hardcoded(&d, a, (void *)1);
    dropdown_list_add_hardcoded(&d, b, (void *)2);
    dropdown_list_add_localized(&d, STR_KEEP, (void *)3);
    if (d.dropcount != 3) {
        FAIL("expected 3 items, got %u", d.dropcount);
    }

    STRING *s0 = dropdown_list_ondisplay(0, &d);
    if (!s0 || s0->length != 5 || memcmp(s0->str, "alpha", 5) != 0) {
        FAIL("hardcoded display");
    }

    DROP_ELEMENT *e2 = &((DROP_ELEMENT *)d.userdata)[2];
    if (e2->handle != (void *)3) {
        FAIL("localized handle");
    }

    UTOX_I18N_STR ids[] = { STR_KEEP, STR_DELETE_MESSAGE };
    DROPDOWN simple;
    memset(&simple, 0, sizeof simple);
    simple.userdata  = ids;
    simple.dropcount = 2;
    STRING *loc      = simple_dropdown_ondisplay(0, &simple);
    if (!loc || loc->length == 0) {
        FAIL("simple_dropdown_ondisplay");
    }

    dropdown_list_clear(&d);
    if (d.dropcount != 0 || d.userdata || d.selected != 0) {
        FAIL("clear");
    }
    dropdown_list_clear(NULL);
    dropdown_list_add_localized(NULL, STR_KEEP, NULL);
    dropdown_list_ondisplay(0, &d); /* empty after clear */
    simple_dropdown_ondisplay(0, NULL);
    return true;
}

bool test_dropdown_null_onselect_and_draw(void) {
    reset_counts();
    DROPDOWN d = make_drop(2);
    d.onselect = NULL;
    open_at(&d, 4, 4, 80, 20);
    dropdown_mmove(&d, 0, 0, 80, 20, 4, 1, 0, 0);
    if (!dropdown_mup(&d) || d.open) {
        FAIL("NULL onselect should still close");
    }

    d = make_drop(2);
    d.style = AUXILIARY_STYLE;
    dropdown_draw(&d, 10, 10, 80, 20);

    open_at(&d, 4, 4, 80, 20);
    dropdown_draw(&d, 10, 10, 80, 20);
    dropdown_drawactive();

    settings.window_height = 10;
    dropdown_drawactive();
    settings.window_height = 480;

    dropdown_close(&d);
    dropdown_drawactive(); /* no active */

    d.dropcount = 0;
    dropdown_draw(&d, 0, 0, 80, 20);

    d.ondisplay = NULL;
    d.dropcount = 2;
    dropdown_draw(&d, 0, 0, 80, 20);
    return true;
}

int main(void) {
    int result = 0;
    RUN_TEST(test_dropdown_open_select_close)
    RUN_TEST(test_dropdown_empty_and_leave)
    RUN_TEST(test_dropdown_over_bounds)
    RUN_TEST(test_dropdown_list_helpers)
    RUN_TEST(test_dropdown_null_onselect_and_draw)
    return result;
}
