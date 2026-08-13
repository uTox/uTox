#include "test.h"
#include "mock/mock_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/native/clipboard.h"
#include "../src/native/keyboard.h"
#include "../src/native/os.h"
#include "../src/native/ui.h"
#include "../src/ui/contextmenu.h"
#include "../src/ui/edit.h"
#include "../src/ui/scrollable.h"

enum {
    TEST_EMOD_SHIFT = (1 << 0),
    TEST_EMOD_CTRL  = (1 << 2),
};

int mock_redraw_count;
int mock_copy_count;
int mock_paste_os_count;
int mock_setselection_count;
int mock_keyboard_count;
int mock_contextmenu_count;
int change_count;
int enter_count;
int tab_count;
int shifttab_count;
int losefocus_count;

void redraw(void) {
    mock_redraw_count++;
}

void copy(int value) {
    (void)value;
    mock_copy_count++;
}

void paste(void) {
    mock_paste_os_count++;
}

void setselection(char *data, uint16_t length) {
    (void)data;
    (void)length;
    mock_setselection_count++;
}

void edit_will_deactivate(void) {}

void showkeyboard(bool show) {
    (void)show;
    mock_keyboard_count++;
}

void contextmenu_new(uint8_t count, UTOX_I18N_STR *menu_string_ids, void (*onselect)(uint8_t)) {
    (void)count;
    (void)menu_string_ids;
    (void)onselect;
    mock_contextmenu_count++;
}

#include "../src/text.c"
#include "../src/ui/scrollable.c"
#include "../src/ui/text.c"
#include "../src/ui/edit.c"

static char edit_buf[256];
static SCROLLABLE edit_scroll;

static void on_change(EDIT *e) {
    (void)e;
    change_count++;
}

static void on_enter(EDIT *e) {
    (void)e;
    enter_count++;
}

static void on_tab(EDIT *e) {
    (void)e;
    tab_count++;
}

static void on_shifttab(EDIT *e) {
    (void)e;
    shifttab_count++;
}

static void on_lose(EDIT *e) {
    (void)e;
    losefocus_count++;
}

static void reset_edit_counts(void) {
    mock_ui_reset();
    mock_redraw_count      = 0;
    mock_copy_count        = 0;
    mock_paste_os_count    = 0;
    mock_setselection_count = 0;
    mock_keyboard_count    = 0;
    mock_contextmenu_count = 0;
    change_count           = 0;
    enter_count            = 0;
    tab_count              = 0;
    shifttab_count         = 0;
    losefocus_count        = 0;
    font_small_lineheight  = 12;
}

static EDIT make_edit(bool multiline) {
    EDIT e;
    memset(&e, 0, sizeof e);
    memset(edit_buf, 0, sizeof edit_buf);
    memset(&edit_scroll, 0, sizeof edit_scroll);
    e.data      = edit_buf;
    e.data_size = sizeof edit_buf;
    e.onchange  = on_change;
    if (multiline) {
        e.multiline = true;
        e.scroll    = &edit_scroll;
        e.width     = 100;
        e.height    = 48;
    }
    return e;
}

static void finish_edit(EDIT *e) {
    edit_resetfocus();
    if (!e || !e->history) {
        return;
    }
    for (uint16_t i = 0; i < e->history_length; i++) {
        free(e->history[i]);
    }
    free(e->history);
    e->history        = NULL;
    e->history_cur    = 0;
    e->history_length = 0;
}

bool test_edit_setstr_focus_cursor(void) {
    reset_edit_counts();
    EDIT e = make_edit(false);

    edit_setstr(NULL, "x", 1);
    edit_setstr(&e, NULL, 4);
    if (e.length != 0) {
        FAIL("NULL str should clear");
    }

    char hello[] = "hello";
    edit_setstr(&e, hello, 5);
    if (e.length != 5 || memcmp(e.data, "hello", 5) != 0 || change_count < 1) {
        FAIL("setstr");
    }

    char longstr[300];
    memset(longstr, 'a', sizeof longstr);
    edit_setstr(&e, longstr, sizeof longstr);
    if (e.length != e.data_size - 1) {
        FAIL("setstr should clamp to data_size-1");
    }

    edit_setstr(&e, hello, 5);
    if (edit_active() || edit_get_active()) {
        FAIL("setstr does not focus");
    }

    edit_setfocus(&e);
    if (!edit_active() || edit_get_active() != &e) {
        FAIL("setfocus");
    }
    if (edit_getcursorpos() != 0 || edit_copy(NULL, 0) != 5) {
        FAIL("setfocus selects all");
    }

    edit_setcursorpos(&e, 2);
    if (edit_getcursorpos() != 2 || edit_copy(NULL, 0) != 0) {
        FAIL("setcursorpos should collapse selection");
    }

    edit_setcursorpos(&e, 99);
    if (edit_getcursorpos() != 5) {
        FAIL("cursor past end clamps");
    }

    edit_setfocus(NULL);
    if (!edit_active()) {
        FAIL("NULL setfocus is a no-op when already focused");
    }

    edit_resetfocus();
    if (edit_active() || edit_copy(NULL, 0) != 0) {
        FAIL("resetfocus / copy without active");
    }

    finish_edit(&e);
    return true;
}

bool test_edit_type_backspace_arrows(void) {
    reset_edit_counts();
    EDIT e = make_edit(false);
    edit_setfocus(&e);
    edit_setcursorpos(&e, 0);

    edit_char('h', false, 0);
    edit_char('i', false, 0);
    if (e.length != 2 || memcmp(e.data, "hi", 2) != 0) {
        FAIL("type hi");
    }

    edit_char(KEY_BACK, true, 0);
    if (e.length != 1 || e.data[0] != 'h' || edit_getcursorpos() != 1) {
        FAIL("backspace");
    }

    edit_char('e', false, 0);
    edit_char('l', false, 0);
    edit_char('l', false, 0);
    edit_char('o', false, 0);
    /* hello */
    edit_char(KEY_LEFT, true, 0);
    if (edit_getcursorpos() != 4) {
        FAIL("left, pos %u", edit_getcursorpos());
    }
    edit_char(KEY_HOME, true, 0);
    if (edit_getcursorpos() != 0) {
        FAIL("home");
    }
    edit_char(KEY_END, true, 0);
    if (edit_getcursorpos() != 5) {
        FAIL("end");
    }

    edit_char(KEY_LEFT, true, TEST_EMOD_CTRL);
    if (edit_getcursorpos() != 0) {
        FAIL("ctrl-left to start of word");
    }
    edit_char(KEY_RIGHT, true, TEST_EMOD_CTRL);
    if (edit_getcursorpos() != 5) {
        FAIL("ctrl-right to end of word");
    }

    edit_setcursorpos(&e, 0);
    edit_char(KEY_DEL, true, 0);
    if (e.length != 4 || e.data[0] != 'e') {
        FAIL("del first char");
    }

    e.readonly = true;
    uint16_t len = e.length;
    edit_char('x', false, 0);
    edit_char(KEY_BACK, true, 0);
    edit_char(KEY_DEL, true, 0);
    if (e.length != len) {
        FAIL("readonly should ignore insert/delete");
    }

    finish_edit(&e);
    return true;
}

bool test_edit_selection_paste_undo(void) {
    reset_edit_counts();
    EDIT e = make_edit(false);
    char hello[] = "hello";
    edit_setstr(&e, hello, 5);
    edit_setfocus(&e);
    edit_setselectedrange(1, 3); /* ell */

    char sel[8] = { 0 };
    if (edit_copy(sel, sizeof sel) != 3 || memcmp(sel, "ell", 3) != 0) {
        FAIL("copy selection");
    }

    edit_char('X', false, 0);
    if (e.length != 3 || memcmp(e.data, "hXo", 3) != 0) {
        FAIL("typing replaces selection, got '%.*s'", e.length, e.data);
    }

    /* replace records delete then insert — two undos restore the original */
    edit_char('z', true, TEST_EMOD_CTRL);
    edit_char('z', true, TEST_EMOD_CTRL);
    if (e.length != 5 || memcmp(e.data, "hello", 5) != 0) {
        FAIL("undo restore, got '%.*s'", e.length, e.data);
    }

    edit_char('y', true, TEST_EMOD_CTRL);
    edit_char('y', true, TEST_EMOD_CTRL);
    if (e.length != 3 || memcmp(e.data, "hXo", 3) != 0) {
        FAIL("redo");
    }

    edit_setcursorpos(&e, 3);
    char extra[] = "YZ";
    edit_paste(extra, 2, false);
    if (e.length != 5 || memcmp(e.data, "hXoYZ", 5) != 0) {
        FAIL("paste at end");
    }

    edit_char('a', true, TEST_EMOD_CTRL);
    if (edit_copy(NULL, 0) != 5) {
        FAIL("ctrl-a selects all");
    }

    uint16_t mark_loc = 0, mark_len = 0;
    edit_setmark(1, 2);
    if (!edit_getmark(&mark_loc, &mark_len) || mark_loc != 1 || mark_len != 2) {
        FAIL("ime mark");
    }
    edit_setmark(0, 0);
    if (edit_getmark(NULL, NULL)) {
        FAIL("cleared mark is invalid");
    }

    finish_edit(&e);
    return true;
}

bool test_edit_enter_tab_focus_switch(void) {
    reset_edit_counts();
    EDIT a = make_edit(false);
    EDIT b;
    char buf_b[64];
    memset(&b, 0, sizeof b);
    memset(buf_b, 0, sizeof buf_b);
    b.data        = buf_b;
    b.data_size   = sizeof buf_b;
    a.onenter     = on_enter;
    a.ontab       = on_tab;
    a.onshifttab  = on_shifttab;
    a.onlosefocus = on_lose;

    edit_setfocus(&a);
    edit_char(KEY_RETURN, true, 0);
    if (enter_count != 1) {
        FAIL("enter");
    }
    edit_char(KEY_TAB, true, 0);
    if (tab_count != 1) {
        FAIL("tab");
    }
    edit_char(KEY_TAB, true, TEST_EMOD_SHIFT);
    if (shifttab_count != 1) {
        FAIL("shift-tab");
    }

    edit_setfocus(&b);
    if (losefocus_count != 1 || edit_get_active() != &b) {
        FAIL("switch focus");
    }

    edit_char('x', false, 0);
    if (edit_active() && edit_get_active() == &a) {
        FAIL("inactive edit must not receive chars");
    }

    finish_edit(&a);
    finish_edit(&b);
    return true;
}

bool test_edit_multiline_and_mouse(void) {
    reset_edit_counts();
    EDIT e = make_edit(true);
    edit_setfocus(&e);
    edit_setcursorpos(&e, 0);

    edit_char('a', false, 0);
    edit_char('\n', false, 0);
    edit_char('b', false, 0);
    if (e.length != 3 || e.data[1] != '\n') {
        FAIL("newline insert");
    }

    edit_char(KEY_UP, true, 0);
    if (edit_getcursorpos() > 1) {
        FAIL("up should leave second line, pos %u", edit_getcursorpos());
    }
    edit_char(KEY_DOWN, true, 0);
    if (edit_getcursorpos() < 1) {
        FAIL("down should reach second line");
    }

    edit_char(KEY_PAGEUP, true, 0);
    if (edit_scroll.d != 0.0) {
        FAIL("pageup");
    }
    edit_char(KEY_PAGEDOWN, true, 0);
    if (edit_scroll.d != 1.0) {
        FAIL("pagedown");
    }

    edit_mmove(&e, 0, 0, 80, 24, 10, 8, 0, 0);
    if (!e.mouseover) {
        FAIL("mmove hover");
    }
    if (!edit_mdown(&e) || mock_keyboard_count < 1) {
        FAIL("mdown focuses and shows keyboard");
    }
    edit_mup(&e);
    if (!edit_mleave(&e) || e.mouseover) {
        FAIL("mleave");
    }

    e.mouseover = true;
    if (!edit_mright(&e) || mock_contextmenu_count != 1) {
        FAIL("mright opens stub menu");
    }

    edit_mwheel(&e, 24, 1.0, true);
    edit_dclick(&e, false);
    edit_press();

    mock_draw_fill_count = 0;
    edit_draw(&e, 0, 0, 80, 40);
    if (mock_draw_fill_count < 1) {
        FAIL("edit_draw");
    }

    finish_edit(&e);

    EDIT plain = make_edit(false);
    maybe_i18nal_string_set_plain(&plain.empty_str, "hint", 4);
    mock_draw_text_count = 0;
    edit_draw(&plain, 0, 0, 80, 24);
    if (mock_draw_text_count < 1) {
        FAIL("empty hint");
    }

    finish_edit(&plain);
    return true;
}

int main(void) {
    int result = 0;
    RUN_TEST(test_edit_setstr_focus_cursor)
    RUN_TEST(test_edit_type_backspace_arrows)
    RUN_TEST(test_edit_selection_paste_undo)
    RUN_TEST(test_edit_enter_tab_focus_switch)
    RUN_TEST(test_edit_multiline_and_mouse)
    return result;
}
