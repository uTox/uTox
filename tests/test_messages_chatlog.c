#include "test.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../src/macros.h"
#include "../src/messages.h"
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
    (void)m;
    (void)width;
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
        m.data[m.number++] = pad;
        m.extra--;
    }

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

int main(void) {
    int result = 0;
    RUN_TEST(test_load_chatlog_skip_windows)
    RUN_TEST(test_initial_page_and_chatlog_skip)
    RUN_TEST(test_load_older_prepend)
    RUN_TEST(test_short_log_exhausted)
    RUN_TEST(test_prepend_cap_drops_newest)
    RUN_TEST(test_short_messages_keep_paging)
    return result;
}
