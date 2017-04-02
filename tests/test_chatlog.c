#include "test.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../src/macros.h"
#include "../src/chatlog.c"
#include "../src/text.c"

#define MOCK_FRIEND_ID "6460FF76319AF777A999ABA2024D5D0AEB202360688ECBABFE56C9403B872D2F"

void native_export_chatlog_init(uint32_t friend_number) {
    char* name = strdup("chatlog_export.txt");
    FILE *file = fopen(name, "wb");
    if (file) {
        utox_export_chatlog(MOCK_FRIEND_ID, file);
    } else {
        FAIL_FATAL("unable to open file for writing: %s", name);
    }
    free(name);
}

bool test_write_chatlog();
bool test_read_chatlog();

int main() {
    int result = 0;
    RUN_TEST(test_write_chatlog)
    RUN_TEST(test_read_chatlog)

    return result;
}

uint8_t* create_mock_message(size_t *length) {
    LOG_FILE_MSG_HEADER header;
    memset(&header, 0, sizeof(header));

    char* author = strdup("tox user");
    size_t author_length = 9;

    size_t msg_length = strlen("This is a test message.");
    char* msg = strdup("This is a test message.");

    header.log_version   = LOGFILE_SAVE_VERSION;
    header.time          = time(NULL);
    header.author_length = author_length;
    header.msg_length    = msg_length;
    header.author        = 1; // we are the message author
    header.receipt       = 1;
    header.msg_type      = 1; // MSG_TYPE_TEXT;

    *length = sizeof(header) + msg_length + author_length + 1; /* extra \n char  */

    uint8_t *data = calloc(1, *length);
    if (!data) {
        FAIL_FATAL("Can't calloc for chat logging data. size: %lu", *length);
    }
    memcpy(data, &header, sizeof(header));
    memcpy(data + sizeof(header), author, author_length);
    memcpy(data + sizeof(header) + author_length, msg, msg_length);
    strcpy2(data + *length - 1, "\n");

    free(author);
    free(msg);

    return data;
}

/**
 * // TODO is there an automated way to track test coverage in C?
 * @covers utox_save_chatlog()
 */
bool test_write_chatlog() {
    char id_str[TOX_PUBLIC_KEY_SIZE * 2] = MOCK_FRIEND_ID;

    size_t length1;
    uint8_t *data1 = create_mock_message(&length1);

    uint64_t disk_offset1 = utox_save_chatlog(id_str, data1, length1);
    LOG("disk offset 1: %lu", disk_offset1);
    assert(disk_offset1 == 0);

    size_t length2;
    uint8_t *data2 = create_mock_message(&length2);

    uint64_t disk_offset2 = utox_save_chatlog(id_str, data2, length2);
    LOG("disk offset 2: %lu", disk_offset2);
    assert(disk_offset2 == length1);

    free(data1);
    free(data2);

    return true;
}

bool test_read_chatlog() {
    LOG_INFO("test", "testing...");
//    FAIL("not good!");

    // TODO implement

    return true;
}
