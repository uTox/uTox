#include "test.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../src/chatlog.c"
#include "../src/messages.c"
#include "../src/messages_chatlog.c"
#include "../src/text.c"

#include "../src/filesys.h"
#include "../src/messages.h"
#include "mock/mock_domain.h"

static FRIEND test_friend;
static GROUPCHAT test_group;
static FILE_TRANSFER incoming_ft[2];
static FILE_TRANSFER outgoing_ft[2];

FRIEND *get_friend(uint32_t friend_number) {
    if (friend_number != 0) {
        return NULL;
    }
    return &test_friend;
}

GROUPCHAT *get_group(uint32_t group_number) {
    if (group_number != 0) {
        return NULL;
    }
    return &test_group;
}

static void reset_messages(void) {
    mock_domain_reset();
    memset(&test_friend, 0, sizeof test_friend);
    memset(&test_group, 0, sizeof test_group);
    memset(incoming_ft, 0, sizeof incoming_ft);
    memset(outgoing_ft, 0, sizeof outgoing_ft);
    test_friend.number = 0;
    test_friend.name = "Alice";
    test_friend.name_length = 5;
    memcpy(test_friend.id_str, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
           TOX_PUBLIC_KEY_SIZE * 2);
    test_friend.ft_incoming = incoming_ft;
    test_friend.ft_outgoing = outgoing_ft;
    self.name_length = 4;
    memcpy(self.name, "Self", 4);
    settings.portable_mode = true;
    settings.logging_enabled = true;
    settings.use_long_time_msg = true;
    messages_friend.object = NULL;
    messages_group.object = NULL;
}

static void attach_scroll(MESSAGES *m, SCROLLABLE *scroll) {
    memset(scroll, 0, sizeof(*scroll));
    m->panel.content_scroll = scroll;
    m->panel.object = m;
    m->width = 400;
}

static NATIVE_IMAGE *dummy_image(void) {
    return (NATIVE_IMAGE *)calloc(1, 1);
}

static void draw_all(MESSAGES *m) {
    messages_updateheight(m, 400);
    messages_draw(&m->panel, 0, 0, 400, 300);
    settings.use_long_time_msg = false;
    messages_draw(&m->panel, 0, 0, 400, 300);
    settings.use_long_time_msg = true;
    messages_draw(&m->panel, 0, 0, 10, 300);
}

bool test_messages_add_types_and_clear(void) {
    reset_messages();
    MESSAGES m;
    memset(&m, 0, sizeof m);
    messages_init(&m, 0);
    SCROLLABLE scroll;
    attach_scroll(&m, &scroll);
    mock_sel_friend = &test_friend;

    if (message_add_type_text(&m, true, "hello", 5, false, false) == UINT32_MAX) {
        FAIL("add text");
    }
    if (message_add_type_text(&m, false, "reply", 5, false, false) == UINT32_MAX) {
        FAIL("add incoming text");
    }
    MESSAGES bad;
    memset(&bad, 0, sizeof bad);
    bad.id = 99;
    if (message_add_type_text(&bad, true, "x", 1, false, false) != UINT32_MAX) {
        FAIL("missing friend");
    }

    if (message_add_type_action(&m, true, "waves", 5, false, false) == UINT32_MAX) {
        FAIL("add action");
    }
    if (message_add_type_action(&m, false, "jumps", 5, false, false) == UINT32_MAX) {
        FAIL("add incoming action");
    }
    if (message_add_type_action(&bad, true, "x", 1, false, false) != UINT32_MAX) {
        FAIL("action missing friend");
    }
    if (message_add_type_notice(&m, "note", 4, false) == UINT32_MAX) {
        FAIL("add notice");
    }

    NATIVE_IMAGE *img = dummy_image();
    if (!message_add_type_image(&m, true, img, 8, 8, false)) {
        free(img);
        FAIL("add image");
    }
    NATIVE_IMAGE *wide = dummy_image();
    if (!message_add_type_image(&m, false, wide, 500, 40, false)) {
        free(wide);
        FAIL("add wide image");
    }
    m.data[m.number - 1]->via.img.zoom = 1;
    m.data[m.number - 1]->via.img.position = 0.5;
    if (message_add_type_image(&m, true, NULL, 8, 8, false) != 0) {
        FAIL("invalid image");
    }

    static const uint8_t ft_status[] = {
        FILE_TRANSFER_STATUS_NONE,      FILE_TRANSFER_STATUS_ACTIVE,     FILE_TRANSFER_STATUS_PAUSED_US,
        FILE_TRANSFER_STATUS_PAUSED_BOTH, FILE_TRANSFER_STATUS_PAUSED_THEM, FILE_TRANSFER_STATUS_BROKEN,
        FILE_TRANSFER_STATUS_COMPLETED, FILE_TRANSFER_STATUS_KILLED,
    };
    for (size_t i = 0; i < sizeof ft_status; i++) {
        MSG_HEADER *ft = message_add_type_file(&m, 0, true, false, ft_status[i], (const uint8_t *)"a.bin", 5, 100,
                                               i == 1 ? 150 : 10);
        if (!ft || !ft->via.ft.name || !ft->via.ft.path) {
            FAIL("add file status %u", ft_status[i]);
        }
        ft->via.ft.speed = (i == 1) ? 10 : 0;
    }
    MSG_HEADER *inline_ft = message_add_type_file(&m, 0, false, true, FILE_TRANSFER_STATUS_COMPLETED,
                                                  (const uint8_t *)"i.png", 5, 10, 10);
    if (!inline_ft || inline_ft->via.ft.path) {
        FAIL("inline file has no path");
    }
    MSG_HEADER *zero = message_add_type_file(&m, 0, true, false, FILE_TRANSFER_STATUS_ACTIVE, (const uint8_t *)"z", 1,
                                             0, 0);
    if (!zero) {
        FAIL("zero-size file");
    }

    if (m.number < 10) {
        FAIL("expected several messages, got %u", m.number);
    }

    test_friend.alias = "Bob";
    test_friend.alias_length = 3;
    draw_all(&m);
    test_friend.alias = NULL;
    test_friend.alias_length = 0;

    messages_clear_all(&m);
    if (m.number != 0 || m.data) {
        FAIL("clear all");
    }
    messages_init(&m, 0);
    messages_clear_all(&m);
    return true;
}

bool test_messages_log_receipt_queue(void) {
    reset_messages();
    MESSAGES m;
    memset(&m, 0, sizeof m);
    messages_init(&m, 0);
    SCROLLABLE scroll;
    attach_scroll(&m, &scroll);
    mock_sel_friend = &test_friend;

    if (message_add_type_text(&m, true, "logged", 6, true, true) == UINT32_MAX) {
        FAIL("log+send text");
    }
    if (mock_last_tox_msg != TOX_SEND_MESSAGE) {
        FAIL("send text posts tox");
    }
    if (message_add_type_action(&m, true, "acts", 4, true, true) == UINT32_MAX) {
        FAIL("log+send action");
    }
    if (message_add_type_notice(&m, "n", 1, true) == UINT32_MAX) {
        FAIL("log notice");
    }

    MSG_HEADER *sent = m.data[0];
    sent->receipt = 42;
    sent->receipt_time = 0;
    messages_clear_receipt(&m, 42);
    if (sent->receipt != (uint32_t)-1 || sent->receipt_time == 0) {
        FAIL("clear receipt");
    }
    messages_clear_receipt(&m, 99);

    sent->receipt = 1;
    sent->receipt_time = 0;
    sent->disk_offset = 0;
    messages_clear_receipt(&m, 1);

    sent->receipt = 7;
    sent->receipt_time = 0;
    messages_send_from_queue(&m, 0);
    if (mock_last_tox_msg != TOX_SEND_ACTION && mock_last_tox_msg != TOX_SEND_MESSAGE) {
        FAIL("resend queue");
    }

    if (!message_log_to_disk(&m, sent)) {
        FAIL("log text again");
    }
    settings.logging_enabled = false;
    if (message_log_to_disk(&m, sent)) {
        FAIL("logging disabled");
    }
    settings.logging_enabled = true;
    test_friend.skip_msg_logging = true;
    if (message_log_to_disk(&m, sent)) {
        FAIL("skip logging");
    }
    test_friend.skip_msg_logging = false;

    NATIVE_IMAGE *img = dummy_image();
    message_add_type_image(&m, true, img, 4, 4, false);
    if (message_log_to_disk(&m, m.data[m.number - 1])) {
        FAIL("image is not logged");
    }

    m.is_groupchat = true;
    if (message_log_to_disk(&m, sent)) {
        FAIL("no group logging");
    }
    m.is_groupchat = false;

    MESSAGES missing;
    memset(&missing, 0, sizeof missing);
    missing.id = 5;
    if (message_log_to_disk(&missing, sent)) {
        FAIL("log missing friend");
    }

    if (messages_read_from_log(9)) {
        FAIL("read log missing friend");
    }
    messages_clear_all(&m);
    mock_sel_friend = NULL;
    if (!messages_read_from_log(0)) {
        FAIL("read previously written log");
    }
    if (test_friend.msg.number == 0) {
        FAIL("read_from_log must load messages into friend.msg");
    }
    if (test_friend.msg.chatlog_skip == 0) {
        FAIL("read_from_log must set chatlog_skip");
    }
    if (!test_friend.msg.chatlog_exhausted) {
        FAIL("short log must be exhausted");
    }
    messages_clear_all(&test_friend.msg);
    return true;
}

bool test_messages_mouse_char_and_file(void) {
    reset_messages();
    MESSAGES m;
    memset(&m, 0, sizeof m);
    messages_init(&m, 0);
    SCROLLABLE scroll;
    attach_scroll(&m, &scroll);
    mock_sel_friend = &test_friend;
    messages_friend.object = &m;
    scroll.viewport_height = 300;

    message_add_type_text(&m, true, "http://example.com", 18, false, false);
    message_add_type_text(&m, false, "https://x.test more", 19, false, false);
    message_add_type_text(&m, true, "tox:abcdef", 10, false, false);
    NATIVE_IMAGE *wide = dummy_image();
    message_add_type_image(&m, true, wide, 500, 40, false);
    MSG_HEADER *ft_in = message_add_type_file(&m, 1u << 16, true, false, FILE_TRANSFER_STATUS_NONE,
                                              (const uint8_t *)"in.bin", 6, 50, 0);
    MSG_HEADER *ft_act = message_add_type_file(&m, 0, false, false, FILE_TRANSFER_STATUS_ACTIVE,
                                               (const uint8_t *)"up.bin", 6, 50, 10);
    MSG_HEADER *ft_pause = message_add_type_file(&m, 0, false, false, FILE_TRANSFER_STATUS_PAUSED_US,
                                                 (const uint8_t *)"p.bin", 5, 50, 10);
    MSG_HEADER *ft_done = message_add_type_file(&m, 0, false, false, FILE_TRANSFER_STATUS_COMPLETED,
                                                (const uint8_t *)"d.bin", 5, 50, 50);
    if (ft_done && ft_done->via.ft.path) {
        memcpy(ft_done->via.ft.path, "done.bin", 9);
    }
    MSG_HEADER *ft_inline = message_add_type_file(&m, 0, false, true, FILE_TRANSFER_STATUS_COMPLETED,
                                                  (const uint8_t *)"i.png", 5, 10, 10);
    (void)ft_in;
    (void)ft_act;
    (void)ft_pause;
    (void)ft_inline;
    messages_updateheight(&m, 400);
    messages_draw(&m.panel, 0, 0, 400, 800);

    messages_mmove(&m.panel, 0, 0, 400, 300, 20, 2, 0, 0);
    messages_mdown(&m.panel);
    messages_mup(&m.panel);
    int y = m.data[0]->height + 2;
    messages_mmove(&m.panel, 0, 0, 400, 300, 20, y, 0, 0);
    messages_mdown(&m.panel);
    messages_mup(&m.panel);
    y += m.data[1]->height;
    messages_mmove(&m.panel, 0, 0, 400, 300, 20, y, 0, 0);
    messages_mdown(&m.panel);
    messages_mup(&m.panel);
    messages_dclick(&m.panel, false);
    messages_dclick(&m.panel, true);
    messages_mright(&m.panel);
    messages_mup(&m.panel);

    messages_mmove(&m.panel, 0, 0, 400, 300, 390, 2, 0, 0);
    messages_dclick(&m.panel, false);

    messages_mmove(&m.panel, 0, 0, 400, 300, -1, 2, 0, 0);
    messages_mmove(&m.panel, 0, 0, 400, 300, 20, -1, 0, 0);

    uint32_t img_i = 3;
    m.cursor_over_msg = img_i;
    m.cursor_over_position = 1;
    messages_mdown(&m.panel);
    m.cursor_down_msg = img_i;
    messages_mmove(&m.panel, 0, 0, 400, 300, 20, 2, 20, 0);
    messages_dclick(&m.panel, false);

    m.cursor_over_msg = 4;
    m.cursor_over_position = 2;
    messages_mdown(&m.panel);
    m.cursor_over_msg = 5;
    m.cursor_over_position = 2;
    messages_mdown(&m.panel);
    m.cursor_over_msg = 6;
    m.cursor_over_position = 2;
    messages_mdown(&m.panel);
    m.cursor_over_msg = 5;
    m.cursor_over_position = 1;
    messages_mdown(&m.panel);
    m.cursor_over_msg = 7;
    m.cursor_over_position = 3;
    messages_mdown(&m.panel);
    m.cursor_over_msg = 8;
    m.cursor_over_position = 3;
    messages_mdown(&m.panel);
    m.cursor_over_msg = 4;
    m.cursor_over_position = 0;
    messages_mdown(&m.panel);

    m.sel_start_msg = 0;
    m.sel_end_msg = 1;
    m.sel_start_position = 0;
    m.sel_end_position = 3;
    m.cursor_over_msg = UINT32_MAX;
    messages_mdown(&m.panel);

    char sel[256];
    m.sel_start_msg = 0;
    m.sel_end_msg = 2;
    m.sel_start_position = 0;
    m.sel_end_position = 4;
    messages_selection(&m.panel, sel, sizeof sel, true);
    messages_selection(&m.panel, sel, sizeof sel, false);

    m.selecting_text = true;
    m.cursor_over_msg = 0;
    messages_mup(&m.panel);

    messages_char('a');
    messages_char(KEY_HOME);
    messages_char(KEY_END);
    messages_char(KEY_PAGEUP);
    messages_char(KEY_PAGEDOWN);

    mock_sel_friend = NULL;
    mock_sel_group = NULL;
    messages_char(KEY_HOME);

    scroll.content_height = 200;
    scroll.viewport_height = 300;
    messages_try_load_older_chatlog(&m, 300);
    scroll.content_height = 800;
    scroll.d = 0;
    messages_try_load_older_chatlog(&m, 300);
    messages_try_load_older_chatlog(&m, 0);
    m.chatlog_exhausted = true;
    messages_try_load_older_chatlog(&m, 300);

    messages_mleave(&m.panel);
    messages_mwheel(&m.panel, 300, 1.0, false);
    messages_mright(&m.panel);

    messages_clear_all(&m);
    messages_mup(&m.panel);
    return true;
}

bool test_messages_group_mouse_char(void) {
    reset_messages();
    MESSAGES m;
    memset(&m, 0, sizeof m);
    messages_init(&m, 0);
    SCROLLABLE scroll;
    attach_scroll(&m, &scroll);
    m.is_groupchat = true;
    mock_sel_group = &test_group;
    messages_group.object = &m;
    scroll.viewport_height = 300;

    MSG_HEADER *msg = calloc(1, sizeof(MSG_HEADER));
    msg->msg_type = MSG_TYPE_TEXT;
    msg->via.grp.length = 5;
    msg->via.grp.msg = calloc(1, 5);
    memcpy(msg->via.grp.msg, "hello", 5);
    msg->via.grp.author_length = 3;
    msg->via.grp.author = calloc(1, 3);
    memcpy(msg->via.grp.author, "Ann", 3);
    message_add_group(&m, msg);

    MSG_HEADER *act = calloc(1, sizeof(MSG_HEADER));
    act->msg_type = MSG_TYPE_ACTION_TEXT;
    act->via.grp.length = 5;
    act->via.grp.msg = calloc(1, 5);
    memcpy(act->via.grp.msg, "waves", 5);
    act->via.grp.author_length = 3;
    act->via.grp.author = calloc(1, 3);
    memcpy(act->via.grp.author, "Bob", 3);
    message_add_group(&m, act);

    messages_updateheight(&m, 400);
    messages_draw(&m.panel, 0, 0, 400, 300);
    messages_mmove(&m.panel, 0, 0, 400, 300, 20, 10, 0, 0);
    messages_mdown(&m.panel);
    m.selecting_text = true;
    m.cursor_down_msg = 0;
    m.cursor_down_position = 0;
    messages_mmove(&m.panel, 0, 0, 400, 300, 40, 10, 5, 0);
    messages_dclick(&m.panel, false);
    messages_dclick(&m.panel, true);
    messages_mright(&m.panel);
    messages_mwheel(&m.panel, 300, 1.0, false);
    messages_mup(&m.panel);
    messages_mleave(&m.panel);

    char sel[128];
    m.sel_start_msg = 0;
    m.sel_end_msg = 1;
    m.sel_start_position = 0;
    m.sel_end_position = 5;
    messages_selection(&m.panel, sel, sizeof sel, true);
    messages_selection(&m.panel, sel, sizeof sel, false);

    messages_char('a');
    messages_char(KEY_HOME);
    messages_char(KEY_END);
    messages_char(KEY_PAGEUP);
    messages_char(KEY_PAGEDOWN);

    messages_try_load_older_chatlog(&m, 300);
    m.is_groupchat = false;
    m.chatlog_exhausted = true;
    messages_try_load_older_chatlog(&m, 300);

    m.is_groupchat = true;
    NATIVE_IMAGE *gimg = dummy_image();
    MSG_HEADER *gimp = calloc(1, sizeof(MSG_HEADER));
    gimp->msg_type = MSG_TYPE_IMAGE;
    gimp->via.img.image = gimg;
    gimp->via.img.w = 8;
    gimp->via.img.h = 8;
    message_add_group(&m, gimp);
    MSG_HEADER *gft = message_add_type_file(&m, 0, true, false, FILE_TRANSFER_STATUS_ACTIVE,
                                            (const uint8_t *)"g.bin", 5, 10, 1);
    (void)gft;
    messages_updateheight(&m, 400);
    messages_draw(&m.panel, 0, 0, 400, 300);
    messages_mmove(&m.panel, 0, 0, 400, 300, 30, 20, 4, 0);
    messages_mdown(&m.panel);
    messages_mup(&m.panel);

    messages_clear_all(&m);
    return true;
}

bool test_messages_day_notice_and_null(void) {
    reset_messages();
    MESSAGES m;
    memset(&m, 0, sizeof m);
    messages_init(&m, 0);
    SCROLLABLE scroll;
    attach_scroll(&m, &scroll);

    time_t t1 = 100000;
    time_t t2 = t1 + 60 * 60 * 24 + 10;
    if (!messages_day_changed(t1, t2)) {
        FAIL("day should change");
    }
    if (messages_day_changed(t1, t1 + 10)) {
        FAIL("same day");
    }
    MSG_HEADER *day = messages_create_day_notice(t2);
    if (!day || day->msg_type != MSG_TYPE_NOTICE_DAY_CHANGE || !day->via.notice_day.msg) {
        message_free(day);
        FAIL("day notice");
    }
    if (message_add_group(&m, day) == UINT32_MAX) {
        FAIL("add day notice");
    }

    message_add_type_text(&m, false, "first", 5, false, false);
    message_add_type_text(&m, false, "second", 6, false, false);

    messages_updateheight(&m, 400);
    messages_draw(&m.panel, 0, 0, 400, 300);
    m.sel_start_msg = 0;
    m.sel_end_msg = 0;
    m.sel_start_position = 0;
    m.sel_end_position = 2;
    messages_draw(&m.panel, 0, 0, 400, 300);

    messages_clear_all(&m);
    return true;
}

bool test_messages_backlog_and_edges(void) {
    reset_messages();
    MESSAGES m;
    memset(&m, 0, sizeof m);
    messages_init(&m, 0);
    SCROLLABLE scroll;
    attach_scroll(&m, &scroll);
    mock_sel_friend = &test_friend;

    messages_updateheight(&m, 0);
    if (m.height != 0) {
        FAIL("zero width should skip height");
    }

    char empty_sel[8];
    if (messages_selection(&m.panel, empty_sel, sizeof empty_sel, true) != 0) {
        FAIL("empty selection");
    }

    for (uint32_t i = 0; i < 30; i++) {
        if (message_add_type_notice(&m, "n", 1, false) == UINT32_MAX) {
            FAIL("notice %u", i);
        }
    }

    m.sel_start_msg = 2;
    m.sel_end_msg = 4;
    m.cursor_down_msg = 3;
    m.cursor_over_msg = 5;
    m.sel_start_position = 1;
    m.sel_end_position = 1;
    m.cursor_down_position = 1;
    m.cursor_over_position = 1;

    while (m.number < UTOX_MAX_BACKLOG_MESSAGES) {
        if (message_add_type_notice(&m, "x", 1, false) == UINT32_MAX) {
            FAIL("fill backlog");
        }
    }
    uint32_t before = m.number;
    if (message_add_type_notice(&m, "y", 1, false) == UINT32_MAX) {
        FAIL("overflow add");
    }
    if (m.number != before) {
        FAIL("overflow should keep cap %u vs %u", before, m.number);
    }
    if (m.sel_start_msg != 1 || m.sel_end_msg != 3) {
        FAIL("overflow should rewind selection %u %u", m.sel_start_msg, m.sel_end_msg);
    }

    m.sel_start_msg = 0;
    m.sel_end_msg = 0;
    m.cursor_down_msg = 0;
    m.cursor_over_msg = 0;
    if (message_add_type_notice(&m, "z", 1, false) == UINT32_MAX) {
        FAIL("overflow at zero index");
    }
    if (m.sel_start_position != 0 || m.sel_end_position != 0) {
        FAIL("zero-index overflow clears positions");
    }

    m.data[0]->msg_type = MSG_TYPE_NULL;
    m.data[0]->height = 12;
    messages_draw(&m.panel, 0, -1000, 400, 50);
    m.cursor_over_msg = 0;
    messages_mmove(&m.panel, 0, 0, 400, 300, 20, 1, 0, 0);
    messages_mdown(&m.panel);
    messages_dclick(&m.panel, false);
    messages_mright(&m.panel);
    m.data[0]->msg_type = MSG_TYPE_NOTICE;

    char tiny[4];
    m.sel_start_msg = 0;
    m.sel_end_msg = 2;
    m.sel_start_position = 0;
    m.sel_end_position = 1;
    messages_selection(&m.panel, tiny, sizeof tiny, true);

    messages_clear_all(&m);
    return true;
}

static MSG_HEADER *add_group_text(MESSAGES *m, const char *author, const char *text) {
    MSG_HEADER *msg = calloc(1, sizeof(MSG_HEADER));
    msg->msg_type              = MSG_TYPE_TEXT;
    msg->via.grp.length        = (uint16_t)strlen(text);
    msg->via.grp.msg           = calloc(1, msg->via.grp.length);
    memcpy(msg->via.grp.msg, text, msg->via.grp.length);
    msg->via.grp.author_length = (uint16_t)strlen(author);
    msg->via.grp.author        = calloc(1, msg->via.grp.author_length);
    memcpy(msg->via.grp.author, author, msg->via.grp.author_length);
    message_add_group(m, msg);
    return msg;
}

bool test_messages_group_draw_visible(void) {
    reset_messages();
    MESSAGES m;
    memset(&m, 0, sizeof m);
    messages_init(&m, 0);
    SCROLLABLE scroll;
    attach_scroll(&m, &scroll);
    m.is_groupchat = true;
    mock_sel_group = &test_group;

    for (int i = 0; i < 8; i++) {
        add_group_text(&m, "Ann", "hello group message");
    }
    messages_updateheight(&m, 400);
    /* Start at MAIN_TOP so messages are not skipped as above the view. */
    messages_draw(&m.panel, 0, SCALE(MAIN_TOP), 400, 400);

    m.sel_start_msg      = 1;
    m.sel_end_msg        = 3;
    m.sel_start_position = 0;
    m.sel_end_position   = 5;
    messages_draw(&m.panel, 0, SCALE(MAIN_TOP), 400, 400);

    messages_clear_all(&m);
    return true;
}

bool test_messages_mmove_image_and_file_hitboxes(void) {
    reset_messages();
    MESSAGES m;
    memset(&m, 0, sizeof m);
    messages_init(&m, 0);
    SCROLLABLE scroll;
    attach_scroll(&m, &scroll);
    mock_sel_friend = &test_friend;

    message_add_type_text(&m, true, "pad", 3, false, false);
    NATIVE_IMAGE *wide = dummy_image();
    message_add_type_image(&m, true, wide, 500, 40, false);
    MSG_HEADER *ft = message_add_type_file(&m, 0, false, false, FILE_TRANSFER_STATUS_ACTIVE,
                                           (const uint8_t *)"up.bin", 6, 50, 10);
    (void)ft;
    messages_updateheight(&m, 400);

    int y_img = m.data[0]->height + 2;
    messages_mmove(&m.panel, 0, 0, 400, 300, SCALE(MESSAGES_X) + 10, y_img, 0, 0);
    if (m.cursor_over_msg != 1) {
        FAIL("mmove should land on image msg, got %u", m.cursor_over_msg);
    }

    m.cursor_down_msg = 1;
    m.data[1]->via.img.zoom = 1;
    m.data[1]->via.img.position = 0.0;
    messages_mmove(&m.panel, 0, 0, 400, 300, 20, y_img, -400, 0);
    if (m.data[1]->via.img.position < 0.999) {
        FAIL("image pan should clamp to 1.0, got %f", m.data[1]->via.img.position);
    }
    m.cursor_down_msg = UINT32_MAX;

    int y = 0;
    for (uint32_t i = 0; i < 2; i++) {
        y += m.data[i]->height;
    }
    y += 2;
    /* Approximate get_time_width() == SCALE(TIME_WIDTH_LONG) with use_long_time_msg. */
    int time_w   = SCALE(TIME_WIDTH_LONG);
    int mx_right = 400 - time_w - BM_FTB_WIDTH / 2 - SCROLL_WIDTH;
    messages_mmove(&m.panel, 0, 0, 400, 300, mx_right, y, 0, 0);
    if (m.cursor_over_msg != 2) {
        FAIL("mmove should land on file msg (y=%d h0=%u h1=%u h2=%u over=%u)", y, m.data[0]->height,
             m.data[1]->height, m.data[2]->height, m.cursor_over_msg);
    }
    if (m.cursor_over_position != 2 && m.cursor_over_position != 1 && m.cursor_over_position != 3) {
        FAIL("file hitbox should set over_position, got %u", m.cursor_over_position);
    }

    messages_clear_all(&m);
    return true;
}

bool test_messages_uri_click_https_tox(void) {
    reset_messages();
    MESSAGES m;
    memset(&m, 0, sizeof m);
    messages_init(&m, 0);
    SCROLLABLE scroll;
    attach_scroll(&m, &scroll);
    mock_sel_friend = &test_friend;

    message_add_type_text(&m, true, "https://example.org/x", 21, false, false);
    message_add_type_text(&m, false, "tox:abcdef012345", 16, false, false);
    messages_updateheight(&m, 400);

    mock_last_openurl[0] = 0;
    messages_mmove(&m.panel, 0, 0, 400, 300, SCALE(MESSAGES_X) + 4, 2, 0, 0);
    if (m.cursor_over_uri == UINT32_MAX) {
        FAIL("https URI should be detected under cursor");
    }
    messages_mdown(&m.panel);
    m.selecting_text = false;
    /* Re-hover without dragging so mup sees a clean URI click. */
    messages_mmove(&m.panel, 0, 0, 400, 300, SCALE(MESSAGES_X) + 4, 2, 0, 0);
    m.cursor_down_uri = m.cursor_over_uri;
    if (m.cursor_over_position > m.cursor_over_uri + m.urllen - 1) {
        m.cursor_over_position = m.cursor_over_uri;
    }
    messages_mup(&m.panel);
    if (strncmp(mock_last_openurl, "https://", 8) != 0) {
        FAIL("https click should openurl, got '%s' (uri=%u pos=%u len=%u sel=%d)", mock_last_openurl,
             m.cursor_over_uri, m.cursor_over_position, m.urllen, (int)m.selecting_text);
    }

    mock_last_openurl[0] = 0;
    int y = m.data[0]->height + 2;
    messages_mmove(&m.panel, 0, 0, 400, 300, SCALE(MESSAGES_X) + 4, y, 0, 0);
    if (m.cursor_over_uri == UINT32_MAX) {
        FAIL("tox: URI should be detected under cursor");
    }
    messages_mdown(&m.panel);
    m.selecting_text = false;
    messages_mmove(&m.panel, 0, 0, 400, 300, SCALE(MESSAGES_X) + 4, y, 0, 0);
    m.cursor_down_uri = m.cursor_over_uri;
    if (m.cursor_over_position > m.cursor_over_uri + m.urllen - 1) {
        m.cursor_over_position = m.cursor_over_uri;
    }
    messages_mup(&m.panel);
    if (strncmp(mock_last_openurl, "tox:", 4) != 0) {
        FAIL("tox: click should openurl, got '%s'", mock_last_openurl);
    }

    messages_clear_all(&m);
    return true;
}

bool test_messages_try_load_older_branches(void) {
    reset_messages();
    MESSAGES m;
    memset(&m, 0, sizeof m);
    messages_init(&m, 0);

    if (messages_try_load_older_chatlog(&m, 300)) {
        FAIL("no content_scroll should fail");
    }

    SCROLLABLE scroll;
    attach_scroll(&m, &scroll);
    m.id = 99;
    if (messages_try_load_older_chatlog(&m, 300)) {
        FAIL("missing friend should fail");
    }

    m.id = 0;
    mock_sel_friend = &test_friend;
    scroll.content_height = 800;
    scroll.viewport_height = 300;
    scroll.d = 0.9;
    if (messages_try_load_older_chatlog(&m, 300)) {
        FAIL("far from top should not load");
    }

    scroll.content_height = 200;
    scroll.viewport_height = 300;
    scroll.d = 0;
    messages_try_load_older_chatlog(&m, 300);

    messages_clear_all(&m);
    return true;
}

bool test_messages_receipt_updates_disk(void) {
    reset_messages();
    MESSAGES m;
    memset(&m, 0, sizeof m);
    messages_init(&m, 0);
    SCROLLABLE scroll;
    attach_scroll(&m, &scroll);
    mock_sel_friend = &test_friend;

    if (message_add_type_text(&m, true, "logged", 6, true, false) == UINT32_MAX) {
        FAIL("log text");
    }
    MSG_HEADER *sent = m.data[0];
    if (sent->disk_offset == 0 && m.number > 1) {
        /* day notice may be first; find logged text */
        for (uint32_t i = 0; i < m.number; i++) {
            if (m.data[i]->msg_type == MSG_TYPE_TEXT && m.data[i]->our_msg) {
                sent = m.data[i];
                break;
            }
        }
    }
    sent->receipt      = 42;
    sent->receipt_time = 0;
    messages_clear_receipt(&m, 42);
    if (sent->receipt != (uint32_t)-1 || sent->receipt_time == 0) {
        FAIL("clear receipt with disk_offset %zu", (size_t)sent->disk_offset);
    }

    messages_clear_all(&m);
    return true;
}

bool test_messages_char_scroll_clamps(void) {
    reset_messages();
    MESSAGES m;
    memset(&m, 0, sizeof m);
    messages_init(&m, 0);
    SCROLLABLE scroll;
    attach_scroll(&m, &scroll);
    messages_friend.object = &m;
    mock_sel_friend = &test_friend;
    scroll.viewport_height = 300;

    scroll.d = -0.5;
    messages_char(KEY_PAGEUP);
    if (scroll.d < 0.0) {
        FAIL("PAGEUP should clamp d >= 0");
    }

    scroll.d = 1.5;
    messages_char(KEY_PAGEDOWN);
    if (scroll.d > 1.0) {
        FAIL("PAGEDOWN should clamp d <= 1");
    }

    scroll.d = 0.5;
    messages_char(KEY_HOME);
    if (scroll.d != 0.0) {
        FAIL("HOME should set d=0");
    }
    messages_char(KEY_END);
    if (scroll.d != 1.0) {
        FAIL("END should set d=1");
    }

    messages_clear_all(&m);
    return true;
}

bool test_messages_init_clears_existing(void) {
    reset_messages();
    MESSAGES m;
    memset(&m, 0, sizeof m);
    messages_init(&m, 0);
    message_add_type_notice(&m, "n", 1, false);
    if (m.number == 0) {
        FAIL("need a message before re-init");
    }
    messages_init(&m, 3);
    if (m.number != 0 || m.id != 3) {
        FAIL("re-init should clear and set id");
    }
    messages_clear_all(&m);
    return true;
}

bool test_messages_reverse_selection_drag(void) {
    reset_messages();
    MESSAGES m;
    memset(&m, 0, sizeof m);
    messages_init(&m, 0);
    SCROLLABLE scroll;
    attach_scroll(&m, &scroll);
    mock_sel_friend = &test_friend;

    message_add_type_text(&m, true, "first message here", 18, false, false);
    message_add_type_text(&m, false, "second message here", 19, false, false);
    messages_updateheight(&m, 400);

    messages_mmove(&m.panel, 0, 0, 400, 300, SCALE(MESSAGES_X) + 40, 2, 0, 0);
    messages_mdown(&m.panel);
    int y2 = m.data[0]->height + 2;
    messages_mmove(&m.panel, 0, 0, 400, 300, SCALE(MESSAGES_X) + 10, y2, -5, 0);
    if (!m.selecting_text) {
        FAIL("drag should start selection");
    }
    /* Drag back above the down point (reverse msg order). */
    messages_mmove(&m.panel, 0, 0, 400, 300, SCALE(MESSAGES_X) + 5, 2, -10, 0);
    if (m.sel_start_msg > m.sel_end_msg) {
        FAIL("selection bounds should be ordered");
    }

    messages_clear_all(&m);
    return true;
}

bool test_messages_read_from_log_page(void) {
    reset_messages();
    char logpath[UTOX_FILE_NAME_LENGTH];
    snprintf(logpath, sizeof logpath, "./tox/%.*s.new.txt", TOX_PUBLIC_KEY_SIZE * 2, test_friend.id_str);
    remove(logpath);

    MESSAGES m;
    memset(&m, 0, sizeof m);
    messages_init(&m, 0);
    mock_sel_friend = NULL;

    for (int i = 0; i < 25; i++) {
        char buf[16];
        int n = snprintf(buf, sizeof buf, "msg-%02d", i);
        if (message_add_type_text(&m, true, buf, (uint16_t)n, true, false) == UINT32_MAX) {
            messages_clear_all(&m);
            FAIL("log line %d", i);
        }
    }
    messages_clear_all(&m);

    if (!messages_read_from_log(0)) {
        FAIL("read 25-line log");
    }
    if (test_friend.msg.chatlog_skip != UTOX_CHATLOG_PAGE_SIZE) {
        FAIL("skip should be page %u, got %u", UTOX_CHATLOG_PAGE_SIZE, test_friend.msg.chatlog_skip);
    }
    if (test_friend.msg.chatlog_exhausted) {
        FAIL("25 records is more than one page");
    }
    messages_clear_all(&test_friend.msg);
    remove(logpath);
    return true;
}

int main(void) {
    int result = 0;
    pthread_mutex_init(&messages_lock, NULL);
    settings.portable_mode = true;
    RUN_TEST(test_messages_add_types_and_clear);
    RUN_TEST(test_messages_log_receipt_queue);
    RUN_TEST(test_messages_read_from_log_page);
    RUN_TEST(test_messages_mouse_char_and_file);
    RUN_TEST(test_messages_group_mouse_char);
    RUN_TEST(test_messages_day_notice_and_null);
    RUN_TEST(test_messages_backlog_and_edges);
    RUN_TEST(test_messages_group_draw_visible);
    RUN_TEST(test_messages_mmove_image_and_file_hitboxes);
    RUN_TEST(test_messages_uri_click_https_tox);
    RUN_TEST(test_messages_try_load_older_branches);
    RUN_TEST(test_messages_receipt_updates_disk);
    RUN_TEST(test_messages_char_scroll_clamps);
    RUN_TEST(test_messages_init_clears_existing);
    RUN_TEST(test_messages_reverse_selection_drag);
    pthread_mutex_destroy(&messages_lock);
    return result;
}
