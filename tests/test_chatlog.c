#include "test.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../src/macros.h"
#include "../src/messages.h"
#include "../src/chatlog.c"
#include "../src/text.c"

#define MOCK_FRIEND_ID "6460FF76319AF777A999ABA2024D5D0AEB202360688ECBABFE56C9403B872D2F"

void message_free(MSG_HEADER *msg) {
    if (!msg) {
        return;
    }
    free(msg->via.txt.msg);
    free(msg);
}

void native_export_chatlog_init(uint32_t friend_number) {
    (void)friend_number;
    char *name = strdup("chatlog_export.txt");
    FILE *file = fopen(name, "wb");
    if (file) {
        /* utox_export_chatlog fclose()s dest_file. */
        utox_export_chatlog(MOCK_FRIEND_ID, file);
    } else {
        FAIL_FATAL("unable to open file for writing: %s", name);
    }
    free(name);
}

static void unlink_log(const char *id) {
    char path[UTOX_FILE_NAME_LENGTH];
    snprintf(path, sizeof(path), "./tox/%.*s.new.txt", TOX_PUBLIC_KEY_SIZE * 2, id);
    remove(path);
    utox_remove_friend_chatlog((char *)id);
}

static bool buf_contains(const char *buf, size_t n, const char *needle) {
    const size_t len = strlen(needle);
    if (len == 0 || len > n) {
        return false;
    }
    for (size_t i = 0; i + len <= n; i++) {
        if (memcmp(buf + i, needle, len) == 0) {
            return true;
        }
    }
    return false;
}

static void free_loaded(MSG_HEADER **list, size_t n) {
    if (!list) {
        return;
    }
    for (size_t i = 0; i < n; i++) {
        if (!list[i]) {
            continue;
        }
        free(list[i]->via.txt.msg);
        free(list[i]);
    }
    free(list);
}

uint8_t *create_mock_message_ex(size_t *length, int author, int receipt, const char *text,
                                uint8_t msg_type, time_t t) {
    LOG_FILE_MSG_HEADER header;
    memset(&header, 0, sizeof(header));

    char *author_name = strdup("tox user");
    size_t author_length = 9;
    size_t msg_length = strlen(text);

    header.log_version   = LOGFILE_SAVE_VERSION;
    header.time          = t;
    header.author_length = author_length;
    header.msg_length    = msg_length;
    header.author        = author ? 1 : 0;
    header.receipt       = receipt ? 1 : 0;
    header.msg_type      = msg_type;

    *length = sizeof(header) + msg_length + author_length + 1;

    uint8_t *data = calloc(1, *length);
    if (!data) {
        FAIL_FATAL("Can't calloc for chat logging data. size: %zu", *length);
    }
    memcpy(data, &header, sizeof(header));
    memcpy(data + sizeof(header), author_name, author_length);
    memcpy(data + sizeof(header) + author_length, text, msg_length);
    strcpy2(data + *length - 1, "\n");

    free(author_name);
    return data;
}

uint8_t *create_mock_message(size_t *length, int author, int receipt, const char *text) {
    return create_mock_message_ex(length, author, receipt, text, MSG_TYPE_TEXT, time(NULL));
}

bool test_write_chatlog(void) {
    char id_str[TOX_PUBLIC_KEY_SIZE * 2] = MOCK_FRIEND_ID;
    unlink_log(id_str);

    size_t length1;
    uint8_t *data1 = create_mock_message(&length1, 1, 1, "This is a test message.");

    uint64_t disk_offset1 = utox_save_chatlog(id_str, data1, length1);
    LOG("disk offset 1: %lu", (unsigned long)disk_offset1);
    assert(disk_offset1 == 0);

    size_t length2;
    uint8_t *data2 = create_mock_message(&length2, 1, 1, "This is a test message.");

    uint64_t disk_offset2 = utox_save_chatlog(id_str, data2, length2);
    LOG("disk offset 2: %lu", (unsigned long)disk_offset2);
    assert(disk_offset2 == length1);

    free(data1);
    free(data2);
    unlink_log(id_str);
    return true;
}

bool test_read_chatlog(void) {
    char id_str[TOX_PUBLIC_KEY_SIZE * 2] = MOCK_FRIEND_ID;
    unlink_log(id_str);

    size_t length1;
    uint8_t *data1 = create_mock_message(&length1, 1, 1, "alpha");
    size_t off1 = utox_save_chatlog(id_str, data1, length1);

    size_t length2;
    uint8_t *data2 = create_mock_message(&length2, 0, 1, "beta");
    size_t off2 = utox_save_chatlog(id_str, data2, length2);

    if (utox_count_chatlog(id_str) != 2) {
        free(data1);
        free(data2);
        unlink_log(id_str);
        FAIL("count should be 2");
    }

    size_t n = 0;
    MSG_HEADER **msgs = utox_load_chatlog(id_str, &n, 10, 0);
    if (!msgs || n != 2) {
        free_loaded(msgs, n);
        free(data1);
        free(data2);
        unlink_log(id_str);
        FAIL("load expected 2 got %zu", n);
    }

    if (msgs[0]->disk_offset != off1 || msgs[1]->disk_offset != off2) {
        free_loaded(msgs, n);
        free(data1);
        free(data2);
        unlink_log(id_str);
        FAIL("disk_offset mismatch");
    }

    if (msgs[0]->via.txt.length != 5 || memcmp(msgs[0]->via.txt.msg, "alpha", 5) != 0) {
        free_loaded(msgs, n);
        free(data1);
        free(data2);
        unlink_log(id_str);
        FAIL("first message text mismatch");
    }
    if (msgs[1]->via.txt.length != 4 || memcmp(msgs[1]->via.txt.msg, "beta", 4) != 0) {
        free_loaded(msgs, n);
        free(data1);
        free(data2);
        unlink_log(id_str);
        FAIL("second message text mismatch");
    }

    free_loaded(msgs, n);
    free(data1);
    free(data2);
    unlink_log(id_str);
    return true;
}

bool test_count_unsent(void) {
    char id_str[TOX_PUBLIC_KEY_SIZE * 2] = MOCK_FRIEND_ID;
    unlink_log(id_str);

    size_t len;
    uint8_t *a = create_mock_message(&len, 1, 0, "unsent-a");
    utox_save_chatlog(id_str, a, len);
    free(a);

    uint8_t *b = create_mock_message(&len, 1, 0, "unsent-b");
    utox_save_chatlog(id_str, b, len);
    free(b);

    uint8_t *c = create_mock_message(&len, 1, 1, "sent");
    utox_save_chatlog(id_str, c, len);
    free(c);

    uint8_t *d = create_mock_message(&len, 0, 0, "incoming");
    utox_save_chatlog(id_str, d, len);
    free(d);

    if (utox_count_chatlog(id_str) != 4) {
        unlink_log(id_str);
        FAIL("total count");
    }
    if (utox_count_unsent_chatlog(id_str) != 2) {
        unlink_log(id_str);
        FAIL("unsent count expected 2");
    }

    unlink_log(id_str);
    return true;
}

bool test_update_chatlog_receipt(void) {
    char id_str[TOX_PUBLIC_KEY_SIZE * 2] = MOCK_FRIEND_ID;
    unlink_log(id_str);

    size_t len;
    uint8_t *data = create_mock_message(&len, 1, 0, "pending");
    size_t offset = utox_save_chatlog(id_str, data, len);

    if (utox_count_unsent_chatlog(id_str) != 1) {
        free(data);
        unlink_log(id_str);
        FAIL("should be unsent before update");
    }

    LOG_FILE_MSG_HEADER *hdr = (LOG_FILE_MSG_HEADER *)data;
    hdr->receipt = 1;
    if (!utox_update_chatlog(id_str, offset, data, sizeof(*hdr))) {
        free(data);
        unlink_log(id_str);
        FAIL("utox_update_chatlog failed");
    }

    if (utox_count_unsent_chatlog(id_str) != 0) {
        free(data);
        unlink_log(id_str);
        FAIL("should be sent after receipt update");
    }

    free(data);
    unlink_log(id_str);
    return true;
}

bool test_corrupt_tail(void) {
    char id_str[TOX_PUBLIC_KEY_SIZE * 2] = MOCK_FRIEND_ID;
    unlink_log(id_str);

    size_t len;
    uint8_t *data = create_mock_message(&len, 1, 1, "ok");
    utox_save_chatlog(id_str, data, len);
    free(data);

    /* Append bytes shorter than a header ? not a complete record. */
    char path[UTOX_FILE_NAME_LENGTH];
    snprintf(path, sizeof(path), "./tox/%.*s.new.txt", TOX_PUBLIC_KEY_SIZE * 2, id_str);
    FILE *fp = fopen(path, "ab");
    if (!fp) {
        unlink_log(id_str);
        FAIL("open for corrupt append");
    }
    uint8_t junk[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    fwrite(junk, 1, sizeof(junk), fp);
    fclose(fp);

    /* Incomplete trailing junk is ignored; complete records still count. */
    if (utox_count_chatlog(id_str) != 1) {
        unlink_log(id_str);
        FAIL("expected 1 complete record despite corrupt tail");
    }

    size_t n = 0;
    MSG_HEADER **msgs = utox_load_chatlog(id_str, &n, 10, 0);
    if (!msgs || n != 1) {
        free_loaded(msgs, n);
        unlink_log(id_str);
        FAIL("load should return the intact message, got %zu", n);
    }
    free_loaded(msgs, n);
    unlink_log(id_str);
    return true;
}

bool test_empty_log_and_skip_past_end(void) {
    char id_str[TOX_PUBLIC_KEY_SIZE * 2] = MOCK_FRIEND_ID;
    unlink_log(id_str);

    if (utox_count_chatlog(id_str) != 0 || utox_count_unsent_chatlog(id_str) != 0) {
        FAIL("missing log should count as 0");
    }

    size_t n = 99;
    MSG_HEADER **msgs = utox_load_chatlog(id_str, &n, 10, 0);
    if (msgs || n != 99) {
        free_loaded(msgs, n);
        FAIL("empty log should return NULL and leave size unset");
    }

    size_t len;
    uint8_t *data = create_mock_message(&len, 1, 1, "only");
    utox_save_chatlog(id_str, data, len);
    free(data);

    n = 0;
    msgs = utox_load_chatlog(id_str, &n, 10, 5);
    if (msgs) {
        free_loaded(msgs, n);
        unlink_log(id_str);
        FAIL("skip past end should return NULL");
    }

    unlink_log(id_str);
    return true;
}

bool test_oversized_and_truncated_records(void) {
    char id_str[TOX_PUBLIC_KEY_SIZE * 2] = MOCK_FRIEND_ID;
    unlink_log(id_str);

    size_t len;
    uint8_t *ok = create_mock_message(&len, 1, 1, "keep-me");
    utox_save_chatlog(id_str, ok, len);
    free(ok);

    char path[UTOX_FILE_NAME_LENGTH];
    snprintf(path, sizeof(path), "./tox/%.*s.new.txt", TOX_PUBLIC_KEY_SIZE * 2, id_str);
    FILE *fp = fopen(path, "ab");
    if (!fp) {
        unlink_log(id_str);
        FAIL("open log to append oversized header");
    }

    LOG_FILE_MSG_HEADER huge;
    memset(&huge, 0, sizeof(huge));
    huge.log_version   = LOGFILE_SAVE_VERSION;
    huge.time          = time(NULL);
    huge.author_length = 1;
    huge.msg_length    = (1u << 16) + 8;
    huge.author        = 1;
    huge.receipt       = 1;
    huge.msg_type      = MSG_TYPE_TEXT;
    fwrite(&huge, sizeof(huge), 1, fp);
    fputc('x', fp);
    fclose(fp);

    size_t n = 7;
    MSG_HEADER **msgs = utox_load_chatlog(id_str, &n, 10, 0);
    if (msgs || n != 0) {
        free_loaded(msgs, n);
        unlink_log(id_str);
        FAIL("oversized record should abort load, size=%zu", n);
    }
    if (utox_load_chatlog(id_str, NULL, 10, 0)) {
        unlink_log(id_str);
        FAIL("oversized load with size=NULL should still return NULL");
    }

    unlink_log(id_str);
    ok = create_mock_message(&len, 1, 1, "first");
    utox_save_chatlog(id_str, ok, len);
    free(ok);

    fp = fopen(path, "ab");
    if (!fp) {
        unlink_log(id_str);
        FAIL("open log to append truncated body");
    }
    LOG_FILE_MSG_HEADER trunc;
    memset(&trunc, 0, sizeof(trunc));
    trunc.log_version   = LOGFILE_SAVE_VERSION;
    trunc.time          = time(NULL);
    trunc.author_length = 1;
    trunc.msg_length    = 40;
    trunc.author        = 1;
    trunc.receipt       = 1;
    trunc.msg_type      = MSG_TYPE_TEXT;
    fwrite(&trunc, sizeof(trunc), 1, fp);
    fputc('y', fp);
    fwrite("no", 1, 2, fp);
    fclose(fp);

    n = 0;
    msgs = utox_load_chatlog(id_str, &n, 10, 0);
    if (!msgs || n != 1) {
        free_loaded(msgs, n);
        unlink_log(id_str);
        FAIL("truncated body should keep the intact record, got %zu", n);
    }
    free_loaded(msgs, n);
    unlink_log(id_str);
    return true;
}

bool test_export_chatlog(void) {
    char id_str[TOX_PUBLIC_KEY_SIZE * 2] = MOCK_FRIEND_ID;
    unlink_log(id_str);

    utox_export_chatlog(id_str, NULL);

    time_t day1 = 1700000000;
    time_t day2 = day1 + 3 * 24 * 60 * 60;
    size_t len;
    uint8_t *a = create_mock_message_ex(&len, 1, 1, "hello-day1", MSG_TYPE_TEXT, day1);
    utox_save_chatlog(id_str, a, len);
    free(a);
    uint8_t *b = create_mock_message_ex(&len, 0, 1, "notice-me", MSG_TYPE_NOTICE, day2);
    utox_save_chatlog(id_str, b, len);
    free(b);
    uint8_t *c = create_mock_message_ex(&len, 1, 1, "hello-day2", MSG_TYPE_TEXT, day2);
    utox_save_chatlog(id_str, c, len);
    free(c);

    const char *out_path = "./tox/chatlog_export_test.txt";
    remove(out_path);
    FILE *out = fopen(out_path, "wb");
    if (!out) {
        unlink_log(id_str);
        FAIL("open export dest");
    }
    utox_export_chatlog(id_str, out);

    FILE *in = fopen(out_path, "rb");
    if (!in) {
        unlink_log(id_str);
        FAIL("reopen export dest");
    }
    char buf[1024] = { 0 };
    size_t got = fread(buf, 1, sizeof(buf) - 1, in);
    fclose(in);
    remove(out_path);

    /* Author field is stored with a trailing NUL, so the export can contain
     * embedded zeros — search the raw bytes, not via strstr. */
    if (got == 0 || !buf_contains(buf, got, "hello-day1") || !buf_contains(buf, got, "hello-day2")) {
        unlink_log(id_str);
        FAIL("export missing message text (%zu bytes)", got);
    }
    if (!buf_contains(buf, got, "Day has changed")) {
        unlink_log(id_str);
        FAIL("export should insert day-change line");
    }
    if (!buf_contains(buf, got, "notice-me")) {
        unlink_log(id_str);
        FAIL("export should include notice body");
    }

    utox_export_chatlog_init(0);
    remove("chatlog_export.txt");
    unlink_log(id_str);
    return true;
}

bool test_export_calendar_and_eof(void) {
    char id_str[TOX_PUBLIC_KEY_SIZE * 2] = MOCK_FRIEND_ID;
    unlink_log(id_str);

    time_t jan = 1704067200; /* 2024-01-01 UTC-ish */
    time_t feb = jan + 40 * 24 * 60 * 60;
    time_t feb2 = feb + 24 * 60 * 60;
    size_t len;
    uint8_t *a = create_mock_message_ex(&len, 1, 1, "jan", MSG_TYPE_TEXT, jan);
    utox_save_chatlog(id_str, a, len);
    free(a);
    uint8_t *b = create_mock_message_ex(&len, 1, 1, "feb", MSG_TYPE_TEXT, feb);
    utox_save_chatlog(id_str, b, len);
    free(b);
    uint8_t *c = create_mock_message_ex(&len, 1, 1, "feb2", MSG_TYPE_TEXT, feb2);
    utox_save_chatlog(id_str, c, len);
    free(c);

    const char *out_path = "./tox/chatlog_export_cal.txt";
    remove(out_path);
    FILE *out = fopen(out_path, "wb");
    if (!out) {
        unlink_log(id_str);
        FAIL("open calendar export");
    }
    utox_export_chatlog(id_str, out);
    FILE *in = fopen(out_path, "rb");
    if (!in) {
        unlink_log(id_str);
        FAIL("reopen calendar export");
    }
    char buf[2048] = { 0 };
    size_t got = fread(buf, 1, sizeof(buf) - 1, in);
    fclose(in);
    remove(out_path);
    if (!buf_contains(buf, got, "jan") || !buf_contains(buf, got, "feb2")) {
        unlink_log(id_str);
        FAIL("calendar export missing messages");
    }

    /* Truncated author so export hits EOF in fgetc loops. */
    unlink_log(id_str);
    char path[UTOX_FILE_NAME_LENGTH];
    snprintf(path, sizeof(path), "./tox/%.*s.new.txt", TOX_PUBLIC_KEY_SIZE * 2, id_str);
    native_create_dir((uint8_t *)"./tox/");
    FILE *fp = fopen(path, "wb");
    if (!fp) {
        FAIL("open truncated-author log");
    }
    LOG_FILE_MSG_HEADER hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.log_version   = LOGFILE_SAVE_VERSION;
    hdr.time          = jan;
    hdr.author_length = 20;
    hdr.msg_length    = 4;
    hdr.author        = 1;
    hdr.receipt       = 1;
    hdr.msg_type      = MSG_TYPE_TEXT;
    fwrite(&hdr, sizeof(hdr), 1, fp);
    fwrite("ab", 1, 2, fp);
    fclose(fp);

    out = fopen(out_path, "wb");
    if (!out) {
        unlink_log(id_str);
        FAIL("open eof export");
    }
    utox_export_chatlog(id_str, out);
    remove(out_path);

    size_t n = 0;
    MSG_HEADER **msgs = utox_load_chatlog(id_str, NULL, 10, 0);
    free_loaded(msgs, n);

    if (utox_update_chatlog(id_str, (size_t)-1, (uint8_t *)&hdr, sizeof(hdr))) {
        unlink_log(id_str);
        FAIL("fseek to (size_t)-1 should fail");
    }

    unlink_log(id_str);

    /* Directory in place of the log: open for write/count/update must fail. */
    snprintf(path, sizeof(path), "./tox/%.*s.new.txt", TOX_PUBLIC_KEY_SIZE * 2, id_str);
    native_create_dir((uint8_t *)path);
    if (utox_save_chatlog(id_str, (uint8_t *)&hdr, sizeof(hdr)) != 0) {
        rmdir(path);
        FAIL("save over a directory should fail");
    }
    if (utox_count_chatlog(id_str) != 0 || utox_count_unsent_chatlog(id_str) != 0) {
        rmdir(path);
        FAIL("count over a directory should be 0");
    }
    if (utox_update_chatlog(id_str, 0, (uint8_t *)&hdr, sizeof(hdr))) {
        rmdir(path);
        FAIL("update over a directory should fail");
    }
    rmdir(path);
    return true;
}

int main(void) {
    int result = 0;
    RUN_TEST(test_write_chatlog)
    RUN_TEST(test_read_chatlog)
    RUN_TEST(test_count_unsent)
    RUN_TEST(test_update_chatlog_receipt)
    RUN_TEST(test_corrupt_tail)
    RUN_TEST(test_empty_log_and_skip_past_end)
    RUN_TEST(test_oversized_and_truncated_records)
    RUN_TEST(test_export_chatlog)
    RUN_TEST(test_export_calendar_and_eof)
    return result;
}
