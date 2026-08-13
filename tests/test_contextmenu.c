#include "test.h"
#include "mock/mock_ui.h"

#include <stdio.h>
#include <string.h>

#include "../src/ui/contextmenu.h"

#include "../src/ui/contextmenu.c"

static int select_count;
static uint8_t last_selected;

static void on_select(uint8_t i) {
    select_count++;
    last_selected = i;
}

static STRING *ondisplay(uint8_t i, const CONTEXTMENU *cm) {
    static STRING s;
    static char buf[16];
    (void)cm;
    snprintf(buf, sizeof buf, "item-%u", (unsigned)i);
    s.str    = buf;
    s.length = (uint16_t)strlen(buf);
    return &s;
}

static void reset_menu(void) {
    mock_ui_reset();
    select_count   = 0;
    last_selected  = 0xff;
    mouse.x        = 100;
    mouse.y        = 100;
    settings.window_width  = 640;
    settings.window_height = 480;
}

static void open_menu(uint8_t n) {
    contextmenu_new_ex(n, NULL, on_select, ondisplay);
}

bool test_contextmenu_select_and_outside(void) {
    reset_menu();
    const int row = SCALE(24);

    if (contextmenu_mdown() || contextmenu_mup() || contextmenu_mleave() || contextmenu_mmove(0, 0, 0, 0)) {
        FAIL("closed menu should ignore mouse");
    }

    open_menu(3);
    if (!contextmenu_mmove(104, 100 + 4, 0, 0)) {
        FAIL("hover first row should redraw");
    }
    if (!contextmenu_mdown()) {
        FAIL("mdown on row");
    }
    if (!contextmenu_mup() || select_count != 1 || last_selected != 0) {
        FAIL("mup should select 0, got %u count %d", last_selected, select_count);
    }

    if (contextmenu_mup()) {
        FAIL("closed after select");
    }

    open_menu(3);
    contextmenu_mmove(104, 100 + row + 4, 0, 0);
    if (!contextmenu_mdown()) {
        FAIL("mdown row 1");
    }
    if (!contextmenu_mup() || last_selected != 1 || select_count != 2) {
        FAIL("should select 1");
    }

    open_menu(3);
    if (!contextmenu_mmove(104, 100 + row * 2 + 4, 0, 0)) {
        FAIL("hover last row");
    }
    if (!contextmenu_mmove(0, 0, 0, 0)) {
        FAIL("leave via mmove should clear over");
    }
    if (!contextmenu_mdown()) {
        FAIL("mdown outside closes");
    }
    if (contextmenu_mup() || select_count != 2) {
        FAIL("outside close must not select");
    }
    return true;
}

bool test_contextmenu_mup_without_mdown(void) {
    reset_menu();
    open_menu(2);

    /* over and down both start at 0xFF; a release must not fire onselect(255). */
    if (contextmenu_mup() || select_count != 0) {
        FAIL("mup before mdown must not select");
    }

    contextmenu_mmove(104, 100 + 4, 0, 0);
    if (contextmenu_mup() || select_count != 0) {
        FAIL("mup with over set but down unset must not select");
    }

    if (!contextmenu_mleave()) {
        FAIL("mleave clears hover");
    }
    if (contextmenu_mleave()) {
        FAIL("second mleave");
    }
    return true;
}

bool test_contextmenu_null_callbacks_and_draw(void) {
    reset_menu();
    contextmenu_new_ex(2, NULL, NULL, ondisplay);
    contextmenu_mmove(104, 100 + 4, 0, 0);
    if (!contextmenu_mdown()) {
        FAIL("mdown");
    }
    if (!contextmenu_mup()) {
        FAIL("NULL onselect should still close");
    }

    reset_menu();
    contextmenu_new_ex(2, NULL, on_select, NULL);
    mock_draw_fill_count  = 0;
    mock_draw_frame_count = 0;
    mock_draw_text_count  = 0;
    contextmenu_draw();
    if (mock_draw_fill_count < 1 || mock_draw_frame_count < 1) {
        FAIL("open menu still paints chrome without ondisplay");
    }
    if (mock_draw_text_count != 0) {
        FAIL("NULL ondisplay should skip labels");
    }

    contextmenu_draw(); /* still open */
    if (!contextmenu_mdown()) {
        FAIL("close empty-label menu");
    }

    mock_draw_fill_count = 0;
    contextmenu_draw();
    if (mock_draw_fill_count != 0) {
        FAIL("closed menu should not paint");
    }

    contextmenu_new_ex(0, NULL, on_select, ondisplay);
    contextmenu_mmove(104, 104, 0, 0);
    if (!contextmenu_mdown()) {
        FAIL("empty menu mdown closes");
    }

    UTOX_I18N_STR ids[] = { STR_CUT, STR_COPY };
    contextmenu_new(2, ids, on_select);
    contextmenu_mmove(104, 100 + 4, 0, 0);
    contextmenu_mdown();
    if (!contextmenu_mup() || last_selected != 0) {
        FAIL("localized new should select");
    }
    return true;
}

bool test_contextmenu_draw_no_spurious_hover(void) {
    reset_menu();
    open_menu(3);
    mock_draw_fill_count = 0;
    contextmenu_draw();
    /* background + frame; no hover highlight while over == 0xFF */
    if (mock_draw_fill_count != 1) {
        FAIL("no-hover draw should fill once, got %d", mock_draw_fill_count);
    }

    contextmenu_mmove(104, 100 + 4, 0, 0);
    mock_draw_fill_count = 0;
    contextmenu_draw();
    if (mock_draw_fill_count != 2) {
        FAIL("hovered row should fill bg + highlight, got %d", mock_draw_fill_count);
    }

    ui_scale = 0.0; /* CONTEXT_HEIGHT == 0 */
    if (contextmenu_mmove(104, 104, 0, 0)) {
        FAIL("zero row height must not divide by zero");
    }
    ui_scale = 10.0;
    contextmenu_mmove(0, 0, 0, 0);
    contextmenu_mdown();
    return true;
}

int main(void) {
    int result = 0;
    RUN_TEST(test_contextmenu_select_and_outside)
    RUN_TEST(test_contextmenu_mup_without_mdown)
    RUN_TEST(test_contextmenu_null_callbacks_and_draw)
    RUN_TEST(test_contextmenu_draw_no_spurious_hover)
    return result;
}
