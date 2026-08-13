#include "test.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../src/macros.h"
#include "../src/messages.h"
#include "../src/ui/scrollable.h"
#include "../src/chatlog.c"
#include "../src/text.c"
#include "../src/messages_chatlog.c"

#define MOCK_FRIEND_ID "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
#define CHATLOG_TEST_PAGE 5

pthread_mutex_t messages_lock = PTHREAD_MUTEX_INITIALIZER;

void native_export_chatlog_init(uint32_t friend_number) {
    (void)friend_number;
}

void messages_updateheight(MESSAGES *m, int width) {
    (void)width;
    if (!m) {
        return;
    }
    int h = 0;
    for (uint32_t i = 0; i < m->number; i++) {
        if (!m->data[i]) {
            continue;
        }
        if (m->data[i]->height == 0) {
            m->data[i]->height = 20;
        }
        h += m->data[i]->height;
    }
    m->height = h;
}

void message_free(MSG_HEADER *msg) {
    if (!msg) {
        return;
    }
    switch (msg->msg_type) {
        case MSG_TYPE_TEXT:
        case MSG_TYPE_ACTION_TEXT:
        case MSG_TYPE_NOTICE:
            free(msg->via.txt.msg);
            break;
        case MSG_TYPE_NOTICE_DAY_CHANGE:
            free(msg->via.notice_day.msg);
            break;
        default:
            break;
    }
    free(msg);
}

static void free_message_list(MSG_HEADER **list, size_t n) {
    if (!list) {
        return;
    }
    for (size_t i = 0; i < n; i++) {
        message_free(list[i]);
    }
    free(list);
}

static void messages_clear_test(MESSAGES *m) {
    if (!m || !m->data) {
        return;
    }
    for (uint32_t i = 0; i < m->number; i++) {
        message_free(m->data[i]);
    }
    free(m->data);
    memset(m, 0, sizeof(*m));
}

static uint8_t *create_numbered_message(unsigned index, size_t *length) {
    LOG_FILE_MSG_HEADER header;
    memset(&header, 0, sizeof(header));

    char *author = strdup("tox user");
    size_t author_length = 9;

    char msg_buf[64];
    int written = snprintf(msg_buf, sizeof(msg_buf), "msg-%03u", index);
    if (written < 0) {
        FAIL_FATAL("snprintf failed");
    }
    size_t msg_length = (size_t)written;
    char *msg = strdup(msg_buf);

    header.log_version   = LOGFILE_SAVE_VERSION;
    header.time          = (time_t)(1000 + (int)index);
    header.author_length = author_length;
    header.msg_length    = msg_length;
    header.author        = 1;
    header.receipt       = 1;
    header.msg_type      = MSG_TYPE_TEXT;

    *length = sizeof(header) + msg_length + author_length + 1;

    uint8_t *data = calloc(1, *length);
    if (!data) {
        FAIL_FATAL("Can't calloc for chat logging data. size: %zu", *length);
    }
    memcpy(data, &header, sizeof(header));
    memcpy(data + sizeof(header), author, author_length);
    memcpy(data + sizeof(header) + author_length, msg, msg_length);
    strcpy2(data + *length - 1, "\n");

    free(author);
    free(msg);
    return data;
}

static void unlink_test_log(const char *id) {
    char path[UTOX_FILE_NAME_LENGTH];
    snprintf(path, sizeof(path), "./tox/%.*s.new.txt", TOX_PUBLIC_KEY_SIZE * 2, id);
    remove(path);
    utox_remove_friend_chatlog((char *)id);
}

static bool write_numbered_log(const char *id, unsigned count) {
    unlink_test_log(id);
    size_t expected_offset = 0;
    for (unsigned i = 0; i < count; i++) {
        size_t length = 0;
        uint8_t *data = create_numbered_message(i, &length);
        size_t offset = utox_save_chatlog((char *)id, data, length);
        if (i == 0) {
            if (offset != 0) {
                free(data);
                FAIL("first chatlog offset should be 0, got %zu", offset);
            }
        } else if (offset != expected_offset) {
            free(data);
            FAIL("chatlog offset mismatch at %u: got %zu expected %zu", i, offset, expected_offset);
        }
        expected_offset = offset + length;
        free(data);
    }

    size_t records = utox_count_chatlog((char *)id);
    if (records != count) {
        FAIL("utox_count_chatlog expected %u got %zu", count, records);
    }
    return true;
}

static bool msg_text_is(MSG_HEADER *msg, const char *expected) {
    if (!msg || msg->msg_type != MSG_TYPE_TEXT) {
        return false;
    }
    if (msg->via.txt.length != strlen(expected)) {
        return false;
    }
    return memcmp(msg->via.txt.msg, expected, msg->via.txt.length) == 0;
}

bool test_load_chatlog_skip_windows(void) {
    const char *id = MOCK_FRIEND_ID;
    const unsigned total = 12;
    const unsigned page  = CHATLOG_TEST_PAGE;

    if (!write_numbered_log(id, total)) {
        return false;
    }

    size_t n = 0;
    MSG_HEADER **newest = utox_load_chatlog((char *)id, &n, page, 0);
    if (!newest || n != page) {
        free_message_list(newest, n);
        FAIL("expected newest page of %u, got %zu", page, n);
    }
    if (!msg_text_is(newest[0], "msg-007") || !msg_text_is(newest[page - 1], "msg-011")) {
        free_message_list(newest, n);
        FAIL("newest page content mismatch");
    }
    free_message_list(newest, n);

    MSG_HEADER **older = utox_load_chatlog((char *)id, &n, page, page);
    if (!older || n != page) {
        free_message_list(older, n);
        FAIL("expected older page of %u, got %zu", page, n);
    }
    if (!msg_text_is(older[0], "msg-002") || !msg_text_is(older[page - 1], "msg-006")) {
        free_message_list(older, n);
        FAIL("older page content mismatch");
    }
    free_message_list(older, n);

    MSG_HEADER **oldest = utox_load_chatlog((char *)id, &n, page, page * 2);
    if (!oldest || n != 2) {
        free_message_list(oldest, n);
        FAIL("expected remaining 2 oldest messages, got %zu", n);
    }
    if (!msg_text_is(oldest[0], "msg-000") || !msg_text_is(oldest[1], "msg-001")) {
        free_message_list(oldest, n);
        FAIL("oldest page content mismatch");
    }
    free_message_list(oldest, n);

    unlink_test_log(id);
    return true;
}

bool test_initial_page_and_chatlog_skip(void) {
    const char *id = MOCK_FRIEND_ID;
    const unsigned total = 12;
    const unsigned page  = CHATLOG_TEST_PAGE;

    if (!write_numbered_log(id, total)) {
        return false;
    }

    size_t n = 0;
    MSG_HEADER **data = utox_load_chatlog((char *)id, &n, page, 0);
    if (!data || n != page) {
        free_message_list(data, n);
        FAIL("initial page load failed");
    }

    MESSAGES m;
    memset(&m, 0, sizeof(m));
    m.data  = calloc(page + 10, sizeof(MSG_HEADER *));
    m.extra = 10;
    for (size_t i = 0; i < n; i++) {
        m.data[m.number++] = data[i];
    }
    free(data);

    m.chatlog_skip      = (uint32_t)n;
    m.chatlog_exhausted = (n < page) || (m.chatlog_skip >= utox_count_chatlog((char *)id));

    if (m.chatlog_skip != page) {
        messages_clear_test(&m);
        FAIL("chatlog_skip should be %u, got %u", page, m.chatlog_skip);
    }
    if (m.chatlog_exhausted) {
        messages_clear_test(&m);
        FAIL("chatlog should not be exhausted after first page of larger log");
    }

    messages_clear_test(&m);
    unlink_test_log(id);
    return true;
}

bool test_load_older_prepend(void) {
    const char *id = MOCK_FRIEND_ID;
    const unsigned total = 12;

    if (!write_numbered_log(id, total)) {
        return false;
    }

    size_t n = 0;
    MSG_HEADER **data = utox_load_chatlog((char *)id, &n, CHATLOG_TEST_PAGE, 0);
    if (!data || n != CHATLOG_TEST_PAGE) {
        free_message_list(data, n);
        FAIL("setup newest page failed");
    }

    MESSAGES m;
    memset(&m, 0, sizeof(m));
    m.data              = calloc(64, sizeof(MSG_HEADER *));
    m.extra             = 64;
    m.chatlog_skip      = (uint32_t)n;
    m.chatlog_exhausted = false;
    for (size_t i = 0; i < n; i++) {
        m.data[m.number++] = data[i];
    }
    free(data);

    if (!messages_load_older_chatlog(&m, id)) {
        messages_clear_test(&m);
        FAIL("messages_load_older_chatlog returned false");
    }

    if (m.number < CHATLOG_TEST_PAGE + 2) {
        messages_clear_test(&m);
        FAIL("expected prepended older messages, number=%u", m.number);
    }
    if (!msg_text_is(m.data[0], "msg-000")) {
        messages_clear_test(&m);
        FAIL("oldest message should be at index 0 after prepend");
    }
    if (!m.chatlog_exhausted) {
        messages_clear_test(&m);
        FAIL("chatlog should be exhausted after loading all older pages");
    }
    if (messages_load_older_chatlog(&m, id)) {
        messages_clear_test(&m);
        FAIL("second load_older should fail when exhausted");
    }

    messages_clear_test(&m);
    unlink_test_log(id);
    return true;
}

bool test_prepend_cap_drops_newest(void) {
    const char *id = MOCK_FRIEND_ID;
    const unsigned total = UTOX_MAX_BACKLOG_MESSAGES + 30;

    if (!write_numbered_log(id, total)) {
        return false;
    }

    size_t n = 0;
    MSG_HEADER **data = utox_load_chatlog((char *)id, &n, UTOX_CHATLOG_PAGE_SIZE, 0);
    if (!data || n == 0) {
        free_message_list(data, n);
        FAIL("cap-test initial load failed");
    }

    MESSAGES m;
    memset(&m, 0, sizeof(m));
    m.data              = calloc(UTOX_MAX_BACKLOG_MESSAGES + 32, sizeof(MSG_HEADER *));
    m.extra             = 32;
    m.chatlog_skip      = (uint32_t)n;
    m.chatlog_exhausted = false;
    for (size_t i = 0; i < n; i++) {
        m.data[m.number++] = data[i];
    }
    free(data);

    while (m.number < UTOX_MAX_BACKLOG_MESSAGES - 5) {
        MSG_HEADER *pad = calloc(1, sizeof(MSG_HEADER));
        pad->msg_type   = MSG_TYPE_TEXT;
        pad->via.txt.msg = strdup("pad");
        pad->via.txt.length = 3;
        pad->time = 1;
        pad->height = 20;
        m.data[m.number++] = pad;
        m.extra--;
    }

    m.width          = 200;
    m.height         = (int)m.number * 20;
    m.sel_start_msg  = m.number - 1;
    m.sel_end_msg    = m.number - 1;
    m.cursor_down_msg = m.number - 1;
    m.cursor_over_msg = m.number - 1;

    const uint32_t before_skip = m.chatlog_skip;
    if (!messages_load_older_chatlog(&m, id)) {
        messages_clear_test(&m);
        FAIL("load_older at near-cap failed");
    }
    if (m.number > UTOX_MAX_BACKLOG_MESSAGES) {
        messages_clear_test(&m);
        FAIL("number %u exceeds cap %u", m.number, UTOX_MAX_BACKLOG_MESSAGES);
    }
    if (m.chatlog_skip <= before_skip) {
        messages_clear_test(&m);
        FAIL("chatlog_skip should advance after load_older");
    }
    if (m.number == 0) {
        messages_clear_test(&m);
        FAIL("message list empty after capped prepend");
    }

    messages_clear_test(&m);
    unlink_test_log(id);
    return true;
}

bool test_short_log_exhausted(void) {
    const char *id = MOCK_FRIEND_ID;
    if (!write_numbered_log(id, 3)) {
        return false;
    }

    size_t n = 0;
    MSG_HEADER **data = utox_load_chatlog((char *)id, &n, UTOX_CHATLOG_PAGE_SIZE, 0);
    if (!data || n != 3) {
        free_message_list(data, n);
        FAIL("short log load failed");
    }

    MESSAGES m;
    memset(&m, 0, sizeof(m));
    m.data              = calloc(16, sizeof(MSG_HEADER *));
    m.extra             = 16;
    m.chatlog_skip      = (uint32_t)n;
    m.chatlog_exhausted = (n < UTOX_CHATLOG_PAGE_SIZE);
    for (size_t i = 0; i < n; i++) {
        m.data[m.number++] = data[i];
    }
    free(data);

    if (!m.chatlog_exhausted) {
        messages_clear_test(&m);
        FAIL("short log should mark chatlog exhausted");
    }
    if (messages_load_older_chatlog(&m, id)) {
        messages_clear_test(&m);
        FAIL("load_older on exhausted short log should fail");
    }

    messages_clear_test(&m);
    unlink_test_log(id);
    return true;
}

bool test_short_messages_keep_paging(void) {
    const char *id = MOCK_FRIEND_ID;
    const unsigned total = 50;

    if (!write_numbered_log(id, total)) {
        return false;
    }

    size_t n = 0;
    MSG_HEADER **data = utox_load_chatlog((char *)id, &n, CHATLOG_TEST_PAGE, 0);
    if (!data || n != CHATLOG_TEST_PAGE) {
        free_message_list(data, n);
        FAIL("short-message paging setup failed");
    }

    MESSAGES m;
    memset(&m, 0, sizeof(m));
    m.data              = calloc(64, sizeof(MSG_HEADER *));
    m.extra             = 64;
    m.chatlog_skip      = (uint32_t)n;
    m.chatlog_exhausted = false;
    for (size_t i = 0; i < n; i++) {
        m.data[m.number++] = data[i];
    }
    free(data);

    const uint32_t skip0 = m.chatlog_skip;
    if (!messages_load_older_chatlog(&m, id)) {
        messages_clear_test(&m);
        FAIL("older page should load when more records remain on disk");
    }
    if (m.chatlog_skip <= skip0) {
        messages_clear_test(&m);
        FAIL("chatlog_skip should advance, was %u now %u", skip0, m.chatlog_skip);
    }
    if (m.chatlog_exhausted) {
        messages_clear_test(&m);
        FAIL("chatlog should not be exhausted after one older page of %u", total);
    }

    if (!messages_load_older_chatlog(&m, id)) {
        messages_clear_test(&m);
        FAIL("second older page should load for short-message chatlog");
    }
    if (m.chatlog_skip >= total) {
        messages_clear_test(&m);
        FAIL("chatlog_skip %u should stay below total %u", m.chatlog_skip, total);
    }

    messages_clear_test(&m);
    unlink_test_log(id);
    return true;
}

static MSG_HEADER *make_text_msg(time_t t, const char *text) {
    MSG_HEADER *msg = calloc(1, sizeof(MSG_HEADER));
    if (!msg) {
        FAIL_FATAL("calloc MSG_HEADER");
    }
    msg->time     = t;
    msg->msg_type = MSG_TYPE_TEXT;
    msg->via.txt.msg = strdup(text);
    msg->via.txt.length = strlen(text);
    return msg;
}

bool test_prepend_day_notice(void) {
    MESSAGES m;
    memset(&m, 0, sizeof(m));
    m.data  = calloc(8, sizeof(MSG_HEADER *));
    m.extra = 8;

    /* Existing newest message on day 2. */
    time_t day2 = 1700000000;
    m.data[0] = make_text_msg(day2, "day2");
    m.number  = 1;

    /* Prepend older batch spanning day0 -> day1. */
    time_t day0 = day2 - 3 * 24 * 60 * 60;
    time_t day1 = day2 - 1 * 24 * 60 * 60;
    MSG_HEADER *batch[2];
    batch[0] = make_text_msg(day0, "day0");
    batch[1] = make_text_msg(day1, "day1");

    size_t out = messages_prepend(&m, batch, 2);
    if (out < 3) {
        messages_clear_test(&m);
        FAIL("expected day notices inserted, prepended=%zu", out);
    }

    bool saw_notice = false;
    for (uint32_t i = 0; i < m.number; i++) {
        if (m.data[i] && m.data[i]->msg_type == MSG_TYPE_NOTICE_DAY_CHANGE) {
            saw_notice = true;
            break;
        }
    }
    if (!saw_notice) {
        messages_clear_test(&m);
        FAIL("expected MSG_TYPE_NOTICE_DAY_CHANGE after cross-day prepend");
    }

    messages_clear_test(&m);
    return true;
}

bool test_exhausted_idempotent(void) {
    const char *id = MOCK_FRIEND_ID;
    if (!write_numbered_log(id, 3)) {
        return false;
    }

    size_t n = 0;
    MSG_HEADER **data = utox_load_chatlog((char *)id, &n, UTOX_CHATLOG_PAGE_SIZE, 0);
    if (!data || n != 3) {
        free_message_list(data, n);
        FAIL("short log load failed");
    }

    MESSAGES m;
    memset(&m, 0, sizeof(m));
    m.data              = calloc(16, sizeof(MSG_HEADER *));
    m.extra             = 16;
    m.chatlog_skip      = (uint32_t)n;
    m.chatlog_exhausted = (n < UTOX_CHATLOG_PAGE_SIZE);
    for (size_t i = 0; i < n; i++) {
        m.data[m.number++] = data[i];
    }
    free(data);

    if (!m.chatlog_exhausted) {
        messages_clear_test(&m);
        unlink_test_log(id);
        FAIL("short log should mark chatlog exhausted");
    }

    if (messages_load_older_chatlog(&m, id)) {
        messages_clear_test(&m);
        unlink_test_log(id);
        FAIL("load_older on exhausted should fail");
    }
    if (messages_load_older_chatlog(&m, id)) {
        messages_clear_test(&m);
        unlink_test_log(id);
        FAIL("second load_older on exhausted should still fail");
    }

    messages_clear_test(&m);
    unlink_test_log(id);
    return true;
}

static uint8_t *create_message_flags(unsigned index, int author, int receipt, size_t *length) {
    LOG_FILE_MSG_HEADER header;
    memset(&header, 0, sizeof(header));

    char *author_name = strdup("tox user");
    size_t author_length = 9;

    char msg_buf[64];
    int written = snprintf(msg_buf, sizeof(msg_buf), "u-%03u", index);
    if (written < 0) {
        FAIL_FATAL("snprintf failed");
    }
    size_t msg_length = (size_t)written;
    char *msg = strdup(msg_buf);

    header.log_version   = LOGFILE_SAVE_VERSION;
    header.time          = (time_t)(1000 + (int)index);
    header.author_length = author_length;
    header.msg_length    = msg_length;
    header.author        = author ? 1 : 0;
    header.receipt       = receipt ? 1 : 0;
    header.msg_type      = MSG_TYPE_TEXT;

    *length = sizeof(header) + msg_length + author_length + 1;
    uint8_t *data = calloc(1, *length);
    if (!data) {
        FAIL_FATAL("calloc chatlog record");
    }
    memcpy(data, &header, sizeof(header));
    memcpy(data + sizeof(header), author_name, author_length);
    memcpy(data + sizeof(header) + author_length, msg, msg_length);
    strcpy2(data + *length - 1, "\n");
    free(author_name);
    free(msg);
    return data;
}

bool test_unsent_initial_window(void) {
    const char *id = MOCK_FRIEND_ID;
    unlink_test_log(id);

    const unsigned unsent_n = UTOX_CHATLOG_PAGE_SIZE + 5;
    const unsigned sent_n   = 10;
    for (unsigned i = 0; i < unsent_n; i++) {
        size_t length = 0;
        uint8_t *data = create_message_flags(i, 1, 0, &length);
        utox_save_chatlog((char *)id, data, length);
        free(data);
    }
    for (unsigned i = 0; i < sent_n; i++) {
        size_t length = 0;
        uint8_t *data = create_message_flags(unsent_n + i, 1, 1, &length);
        utox_save_chatlog((char *)id, data, length);
        free(data);
    }

    size_t unsent = utox_count_unsent_chatlog((char *)id);
    if (unsent != unsent_n) {
        unlink_test_log(id);
        FAIL("unsent count expected %u got %zu", unsent_n, unsent);
    }

    const uint32_t load_count = (unsent > UTOX_CHATLOG_PAGE_SIZE)
                                    ? (uint32_t)(unsent + UTOX_CHATLOG_PAGE_SIZE)
                                    : UTOX_CHATLOG_PAGE_SIZE;

    size_t n = 0;
    MSG_HEADER **data = utox_load_chatlog((char *)id, &n, load_count, 0);
    if (!data || n < unsent_n) {
        free_message_list(data, n);
        unlink_test_log(id);
        FAIL("initial window must cover all unsent (%u), got %zu (load_count=%u)",
             unsent_n, n, load_count);
    }

    bool found_oldest_unsent = false;
    for (size_t i = 0; i < n; i++) {
        if (msg_text_is(data[i], "u-000")) {
            found_oldest_unsent = true;
            break;
        }
    }
    if (!found_oldest_unsent) {
        free_message_list(data, n);
        unlink_test_log(id);
        FAIL("oldest unsent u-000 missing from expanded initial window");
    }

    free_message_list(data, n);
    unlink_test_log(id);
    return true;
}

bool test_prepend_guards_and_day_changed(void) {
    unlink_test_log(MOCK_FRIEND_ID);

    if (messages_day_changed(0, 1000)) {
        FAIL("last==0 is not a day change");
    }
    time_t t = 1700000000;
    if (messages_day_changed(t, t)) {
        FAIL("same timestamp is not a day change");
    }
    if (!messages_day_changed(t, t + 3 * 24 * 60 * 60)) {
        FAIL("later day should be a day change");
    }
    if (!messages_day_changed(t, t + 40 * 24 * 60 * 60)) {
        FAIL("later month should be a day change");
    }
    if (!messages_day_changed(t, t + 400 * 24 * 60 * 60)) {
        FAIL("later year should be a day change");
    }
    if (!messages_day_changed(t, t + 24 * 60 * 60)) {
        FAIL("next calendar day should be a day change");
    }
    if (messages_day_changed(t + 40 * 24 * 60 * 60, t)) {
        FAIL("earlier month is not a day change");
    }
    if (messages_day_changed(t + 400 * 24 * 60 * 60, t)) {
        FAIL("earlier year is not a day change");
    }

    MSG_HEADER *notice = messages_create_day_notice(t + 86400);
    if (!notice || notice->msg_type != MSG_TYPE_NOTICE_DAY_CHANGE || !notice->via.notice_day.length) {
        message_free(notice);
        FAIL("messages_create_day_notice");
    }
    message_free(notice);

    MESSAGES empty;
    memset(&empty, 0, sizeof(empty));
    if (messages_prepend(NULL, NULL, 1) != 0 || messages_prepend(&empty, NULL, 1) != 0) {
        FAIL("prepend should reject NULL");
    }

    MESSAGES m;
    memset(&m, 0, sizeof(m));
    MSG_HEADER *empty_batch[1] = { NULL };
    if (messages_prepend(&m, empty_batch, 1) != 0) {
        FAIL("prepend of only NULL entries should return 0");
    }

    if (messages_load_older_chatlog(NULL, MOCK_FRIEND_ID)
        || messages_load_older_chatlog(&m, NULL)) {
        FAIL("load_older should reject NULL");
    }
    m.is_groupchat = true;
    if (messages_load_older_chatlog(&m, MOCK_FRIEND_ID)) {
        FAIL("load_older should skip group chats");
    }
    m.is_groupchat = false;
    m.chatlog_skip = 0;
    if (messages_load_older_chatlog(&m, MOCK_FRIEND_ID)) {
        FAIL("load_older on missing log should fail");
    }
    if (!m.chatlog_exhausted) {
        FAIL("missing log should mark exhausted");
    }

    m.chatlog_exhausted = false;
    m.chatlog_loading   = true;
    if (messages_load_older_chatlog(&m, MOCK_FRIEND_ID)) {
        FAIL("load_older should refuse while already loading");
    }
    m.chatlog_loading = false;
    return true;
}

bool test_prepend_adjusts_scroll(void) {
    SCROLLABLE scroll;
    memset(&scroll, 0, sizeof(scroll));
    scroll.viewport_height = 40;
    scroll.d               = 0.0;

    MESSAGES m;
    memset(&m, 0, sizeof(m));
    m.data                 = calloc(8, sizeof(MSG_HEADER *));
    m.extra                = 8;
    m.width                = 200;
    m.panel.content_scroll = &scroll;
    m.data[0]              = make_text_msg(1700000100, "visible");
    m.data[0]->height      = 20;
    m.number               = 1;
    m.height               = 20;

    MSG_HEADER *batch[1];
    batch[0]         = make_text_msg(1700000000, "older");
    batch[0]->height = 20;

    if (messages_prepend(&m, batch, 1) == 0) {
        messages_clear_test(&m);
        FAIL("prepend with scroll should succeed");
    }
    if (scroll.content_height != m.height || m.height < 40) {
        messages_clear_test(&m);
        FAIL("scroll content_height not updated (%d vs height %d)", scroll.content_height, m.height);
    }

    /* Content taller than viewport: keep relative position. */
    scroll.viewport_height = 30;
    scroll.d               = 0.5;
    m.height               = 80;
    for (uint32_t i = 0; i < m.number; i++) {
        m.data[i]->height = 40;
    }

    MSG_HEADER *batch2[1];
    batch2[0]         = make_text_msg(1699990000, "even-older");
    batch2[0]->height = 40;
    if (messages_prepend(&m, batch2, 1) == 0) {
        messages_clear_test(&m);
        FAIL("second prepend failed");
    }
    if (scroll.d < 0.0 || scroll.d > 1.0) {
        messages_clear_test(&m);
        FAIL("scroll.d out of range %g", scroll.d);
    }

    /* Near the top: reset to the inserted height. */
    scroll.viewport_height = 100;
    scroll.d               = 0.0;
    m.height               = 200;
    for (uint32_t i = 0; i < m.number; i++) {
        if (m.data[i]) {
            m.data[i]->height = 50;
        }
    }
    MSG_HEADER *batch3[1];
    batch3[0]         = make_text_msg(1699900000, "near-top");
    batch3[0]->height = 20;
    if (messages_prepend(&m, batch3, 1) == 0) {
        messages_clear_test(&m);
        FAIL("near-top prepend failed");
    }

    /* Viewport unset: use the height-delta / height branch. */
    scroll.viewport_height = 0;
    MSG_HEADER *batch4[1];
    batch4[0]         = make_text_msg(1699800000, "no-viewport");
    batch4[0]->height = 20;
    if (messages_prepend(&m, batch4, 1) == 0) {
        messages_clear_test(&m);
        FAIL("viewport<=0 prepend failed");
    }

    /* UINT32_MAX indices stay put; real indices bump. */
    m.sel_start_msg    = UINT32_MAX;
    m.sel_end_msg      = 0;
    m.cursor_down_msg  = UINT32_MAX;
    m.cursor_over_msg  = 1;
    MSG_HEADER *batch5[1];
    batch5[0]         = make_text_msg(1699700000, "idx");
    batch5[0]->height = 20;
    size_t added = messages_prepend(&m, batch5, 1);
    if (added == 0) {
        messages_clear_test(&m);
        FAIL("index bump prepend failed");
    }
    if (m.sel_start_msg != UINT32_MAX || m.cursor_down_msg != UINT32_MAX) {
        messages_clear_test(&m);
        FAIL("UINT32_MAX indices should not bump");
    }
    if (m.sel_end_msg < 1 || m.cursor_over_msg < 2) {
        messages_clear_test(&m);
        FAIL("finite indices should bump");
    }

    /* Existing slot 0 is NULL: skip the trailing day-notice. */
    message_free(m.data[0]);
    m.data[0] = NULL;
    MSG_HEADER *batch6[1];
    batch6[0]         = make_text_msg(1699600000, "null-head");
    batch6[0]->height = 20;
    if (messages_prepend(&m, batch6, 1) == 0) {
        messages_clear_test(&m);
        FAIL("NULL head prepend failed");
    }

    messages_clear_test(&m);
    return true;
}

bool test_prepend_refuses_over_cap(void) {
    const size_t n = UTOX_MAX_BACKLOG_MESSAGES;
    MSG_HEADER **batch = calloc(n, sizeof(*batch));
    if (!batch) {
        FAIL_FATAL("calloc over-cap batch");
    }
    const time_t t0 = 1700000000;
    const time_t t1 = t0 + 3 * 24 * 60 * 60;
    for (size_t i = 0; i < n; i++) {
        /* Last message on a later day inserts a notice → out = n+1 > cap. */
        batch[i] = make_text_msg(i + 1 == n ? t1 : t0, "x");
    }

    MESSAGES m;
    memset(&m, 0, sizeof(m));
    if (messages_prepend(&m, batch, n) != 0) {
        messages_clear_test(&m);
        for (size_t i = 0; i < n; i++) {
            message_free(batch[i]);
        }
        free(batch);
        FAIL("batch larger than cap should be refused");
    }

    for (size_t i = 0; i < n; i++) {
        message_free(batch[i]);
    }
    free(batch);
    return true;
}

bool test_drop_last_clears_indices(void) {
    MESSAGES m;
    memset(&m, 0, sizeof(m));
    m.data            = calloc(4, sizeof(MSG_HEADER *));
    m.extra           = 4;
    const time_t t     = 1700000000;
    m.data[0]         = make_text_msg(t, "only");
    m.number          = 1;
    m.width           = 0;
    m.sel_start_msg   = 0;
    m.sel_end_msg     = 0;
    m.cursor_down_msg = 0;
    m.cursor_over_msg = 0;

    const size_t n = UTOX_MAX_BACKLOG_MESSAGES;
    MSG_HEADER **batch = calloc(n, sizeof(*batch));
    if (!batch) {
        messages_clear_test(&m);
        FAIL_FATAL("calloc drop-last batch");
    }
    for (size_t i = 0; i < n; i++) {
        batch[i] = make_text_msg(t, "p");
    }

    if (messages_prepend(&m, batch, n) == 0) {
        for (size_t i = 0; i < n; i++) {
            message_free(batch[i]);
        }
        free(batch);
        messages_clear_test(&m);
        FAIL("prepend of a full page should drop the lone newest");
    }
    free(batch);
    if (m.sel_start_msg != UINT32_MAX || m.sel_end_msg != UINT32_MAX
        || m.cursor_down_msg != UINT32_MAX || m.cursor_over_msg != UINT32_MAX) {
        messages_clear_test(&m);
        FAIL("dropping the last message should clear selection indices");
    }

    messages_clear_test(&m);
    return true;
}

bool test_load_older_empty_batch(void) {
    const char *id = MOCK_FRIEND_ID;
    unlink_test_log(id);
    native_create_dir((uint8_t *)"./tox/");

    char path[UTOX_FILE_NAME_LENGTH];
    snprintf(path, sizeof(path), "./tox/%.*s.new.txt", TOX_PUBLIC_KEY_SIZE * 2, id);
    FILE *fp = fopen(path, "wb");
    if (!fp) {
        FAIL("open truncated-only log");
    }
    LOG_FILE_MSG_HEADER hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.log_version   = LOGFILE_SAVE_VERSION;
    hdr.time          = 1700000000;
    hdr.author_length = 0;
    hdr.msg_length    = 40;
    hdr.author        = 1;
    hdr.receipt       = 1;
    hdr.msg_type      = MSG_TYPE_TEXT;
    fwrite(&hdr, sizeof(hdr), 1, fp);
    fwrite("xx", 1, 2, fp);
    fclose(fp);

    MESSAGES m;
    memset(&m, 0, sizeof(m));
    m.data  = calloc(8, sizeof(MSG_HEADER *));
    m.extra = 8;
    if (messages_load_older_chatlog(&m, id)) {
        messages_clear_test(&m);
        unlink_test_log(id);
        FAIL("truncated-only log should yield empty batch");
    }
    if (!m.chatlog_exhausted) {
        messages_clear_test(&m);
        unlink_test_log(id);
        FAIL("empty batch should mark exhausted");
    }

    messages_clear_test(&m);
    unlink_test_log(id);
    return true;
}

bool test_adjust_scroll_near_top_after_prepend(void) {
    SCROLLABLE scroll;
    memset(&scroll, 0, sizeof(scroll));
    scroll.viewport_height = 90;
    scroll.d               = 0.0;

    MESSAGES m;
    memset(&m, 0, sizeof(m));
    m.data                 = calloc(8, sizeof(MSG_HEADER *));
    m.extra                = 8;
    m.width                = 200;
    m.panel.content_scroll = &scroll;
    m.data[0]              = make_text_msg(1700000100, "visible");
    m.data[0]->height      = 100;
    m.number               = 1;
    m.height               = 100;

    MSG_HEADER *batch[1];
    batch[0]         = make_text_msg(1700000000, "older");
    batch[0]->height = 80;

    if (messages_prepend(&m, batch, 1) == 0) {
        messages_clear_test(&m);
        FAIL("prepend near top should succeed");
    }
    if (scroll.d < 0.0 || scroll.d > 1.0) {
        messages_clear_test(&m);
        FAIL("scroll.d should clamp to [0,1], got %f", scroll.d);
    }

    messages_clear_test(&m);
    return true;
}

int main(void) {
    int result = 0;
    RUN_TEST(test_load_chatlog_skip_windows)
    RUN_TEST(test_initial_page_and_chatlog_skip)
    RUN_TEST(test_load_older_prepend)
    RUN_TEST(test_short_log_exhausted)
    RUN_TEST(test_prepend_cap_drops_newest)
    RUN_TEST(test_short_messages_keep_paging)
    RUN_TEST(test_prepend_day_notice)
    RUN_TEST(test_exhausted_idempotent)
    RUN_TEST(test_unsent_initial_window)
    RUN_TEST(test_prepend_guards_and_day_changed)
    RUN_TEST(test_prepend_adjusts_scroll)
    RUN_TEST(test_prepend_refuses_over_cap)
    RUN_TEST(test_drop_last_clears_indices)
    RUN_TEST(test_load_older_empty_batch)
    RUN_TEST(test_adjust_scroll_near_top_after_prepend)
    return result;
}
