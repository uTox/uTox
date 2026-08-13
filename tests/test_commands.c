#include "test.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/commands.c"
#include "../src/command_funcs.c"
#include "../src/text.c"

#include "mock/mock_domain.h"

static FRIEND cmd_friend;
static GROUPCHAT cmd_group;
static char cmd_alias[64];
static uint16_t cmd_alias_len;
static bool cmd_meta_written;
static FRIEND *cmd_found;

void friend_set_alias(FRIEND *f, uint8_t *alias, uint16_t length) {
    (void)f;
    if (length == 0 || !alias) {
        cmd_alias[0] = 0;
        cmd_alias_len = 0;
        return;
    }
    if (length >= sizeof cmd_alias) {
        length = sizeof cmd_alias - 1;
    }
    memcpy(cmd_alias, alias, length);
    cmd_alias[length] = 0;
    cmd_alias_len = length;
}

void utox_write_metadata(FRIEND *f) {
    (void)f;
    cmd_meta_written = true;
}

FRIEND *find_friend_by_name(uint8_t *name) {
    (void)name;
    return cmd_found;
}

bool string_to_id(uint8_t *dest, char *src) {
    if (!dest || !src) {
        return false;
    }
    if (*src == 'z' || *src == 'Z') {
        return false;
    }
    memset(dest, 0x11, TOX_ADDRESS_SIZE);
    return true;
}

static void reset_cmd(void) {
    mock_domain_reset();
    memset(&cmd_friend, 0, sizeof cmd_friend);
    memset(&cmd_group, 0, sizeof cmd_group);
    cmd_friend.number = 3;
    cmd_friend.online = true;
    cmd_group.number = 1;
    cmd_alias[0] = 0;
    cmd_alias_len = 0;
    cmd_meta_written = false;
    cmd_found = &cmd_friend;
    mock_sel_friend = &cmd_friend;
    mock_sel_group = &cmd_group;
}

bool test_run_command_untrusted_and_plain(void) {
    reset_cmd();
    char buf[] = "/alias bob";
    char *cmd = NULL;
    char *arg = NULL;
    if (utox_run_command(buf, (uint16_t)strlen(buf), &cmd, &arg, 0) != 0) {
        FAIL("untrusted commands must be rejected");
    }

    char plain[] = "hello there";
    cmd = (char *)0x1;
    arg = NULL;
    uint16_t n = utox_run_command(plain, (uint16_t)strlen(plain), &cmd, &arg, 1);
    if (n != 0 || arg != plain) {
        FAIL("plain text is not a command");
    }
    return true;
}

bool test_run_command_alias_invite_topic_file_device(void) {
    reset_cmd();
    char *cmd = NULL;
    char *arg = NULL;

    char alias[] = "/alias    Bob";
    uint16_t n = utox_run_command(alias, (uint16_t)strlen(alias), &cmd, &arg, 1);
    if (n != (uint16_t)-1) {
        FAIL("successful alias should return -1, got %u", n);
    }
    if (cmd_alias_len != 3 || memcmp(cmd_alias, "Bob", 3) != 0 || !cmd_meta_written) {
        FAIL("slash_alias should set alias and write metadata");
    }

    char alias_none[] = "/alias ";
    cmd = NULL;
    arg = NULL;
    cmd_meta_written = false;
    n = utox_run_command(alias_none, (uint16_t)strlen(alias_none), &cmd, &arg, 1);
    if (n != (uint16_t)-1 || cmd_alias_len != 0 || !cmd_meta_written) {
        FAIL("alias with empty arg should clear alias");
    }

    char alias_again[] = "/alias Bob";
    cmd = NULL;
    arg = NULL;
    cmd_meta_written = false;
    n = utox_run_command(alias_again, (uint16_t)strlen(alias_again), &cmd, &arg, 1);
    if (n != (uint16_t)-1 || cmd_alias_len != 3) {
        FAIL("alias should set again");
    }

    char alias_nospace[] = "/alias";
    cmd = NULL;
    arg = NULL;
    cmd_meta_written = false;
    n = utox_run_command(alias_nospace, (uint16_t)strlen(alias_nospace), &cmd, &arg, 1);
    if (n != (uint16_t)-1 || cmd_alias_len != 0 || !cmd_meta_written) {
        FAIL("/alias without space should clear alias");
    }

    mock_sel_friend = NULL;
    mock_sel_group = &cmd_group;
    char alias_nogroup[] = "/alias Eve";
    cmd = NULL;
    arg = NULL;
    n = utox_run_command(alias_nogroup, (uint16_t)strlen(alias_nogroup), &cmd, &arg, 1);
    if (n == (uint16_t)-1) {
        FAIL("alias without a selected friend should fail");
    }

    mock_sel_friend = &cmd_friend;
    mock_sel_group = NULL;
    char invite_nogroup[] = "/invite Alice";
    cmd = NULL;
    arg = NULL;
    mock_last_tox_msg = 0;
    n = utox_run_command(invite_nogroup, (uint16_t)strlen(invite_nogroup), &cmd, &arg, 1);
    if (n == (uint16_t)-1) {
        FAIL("invite without a selected group should fail");
    }

    mock_sel_group = &cmd_group;
    char invite[] = "/invite Alice";
    n = utox_run_command(invite, (uint16_t)strlen(invite), &cmd, &arg, 1);
    if (n != (uint16_t)-1 || mock_last_tox_msg != TOX_GROUP_SEND_INVITE || mock_last_tox_p1 != cmd_group.number) {
        FAIL("invite online friend into selected group");
    }

    cmd_friend.online = false;
    char invite_off[] = "/invite Alice";
    mock_last_tox_msg = 0;
    n = utox_run_command(invite_off, (uint16_t)strlen(invite_off), &cmd, &arg, 1);
    if (n == (uint16_t)-1) {
        FAIL("offline invite should fail");
    }

    cmd_found = NULL;
    char invite_miss[] = "/invite Nobody";
    n = utox_run_command(invite_miss, (uint16_t)strlen(invite_miss), &cmd, &arg, 1);
    if (n == (uint16_t)-1) {
        FAIL("missing friend invite should fail");
    }
    cmd_found = &cmd_friend;
    cmd_friend.online = true;

    mock_sel_group = NULL;
    char topic_nogroup[] = "/topic hello topic";
    mock_last_tox_msg = 0;
    n = utox_run_command(topic_nogroup, (uint16_t)strlen(topic_nogroup), &cmd, &arg, 1);
    if (n == (uint16_t)-1) {
        FAIL("topic without a selected group should fail");
    }

    mock_sel_group = &cmd_group;
    char topic[] = "/topic hello topic";
    n = utox_run_command(topic, (uint16_t)strlen(topic), &cmd, &arg, 1);
    if (n != (uint16_t)-1 || mock_last_tox_msg != TOX_GROUP_SET_TOPIC || mock_last_tox_p1 != cmd_group.number) {
        FAIL("topic command");
    }

    mock_sel_friend = NULL;
    char send_nofriend[] = "/sendfile /tmp/x";
    mock_last_tox_msg = 0;
    n = utox_run_command(send_nofriend, (uint16_t)strlen(send_nofriend), &cmd, &arg, 1);
    if (n == (uint16_t)-1) {
        FAIL("sendfile without a selected friend should fail");
    }

    mock_sel_friend = &cmd_friend;
    char send[] = "/sendfile /tmp/x";
    n = utox_run_command(send, (uint16_t)strlen(send), &cmd, &arg, 1);
    if (n != (uint16_t)-1 || mock_last_tox_msg != TOX_FILE_SEND_NEW_SLASH) {
        FAIL("sendfile command");
    }

    char send_empty[] = "/sendfile ";
    cmd = NULL;
    arg = NULL;
    mock_last_tox_msg = 0;
    n = utox_run_command(send_empty, (uint16_t)strlen(send_empty), &cmd, &arg, 1);
    if (n == (uint16_t)-1) {
        FAIL("sendfile without path should fail");
    }

    char invite_noarg[] = "/invite ";
    cmd = NULL;
    arg = NULL;
    cmd_found = NULL;
    n = utox_run_command(invite_noarg, (uint16_t)strlen(invite_noarg), &cmd, &arg, 1);
    if (n == (uint16_t)-1) {
        FAIL("invite without friend should fail");
    }
    cmd_found = &cmd_friend;

    mock_sel_friend = NULL;
    char device_nofriend[] = "/d 0000000000000000000000000000000000000000000000000000000000000000000000000000";
    mock_last_tox_msg = 0;
    n = utox_run_command(device_nofriend, (uint16_t)strlen(device_nofriend), &cmd, &arg, 1);
    if (n == (uint16_t)-1) {
        FAIL("device without a selected friend should fail");
    }

    mock_sel_friend = &cmd_friend;
    char device_noarg[] = "/d";
    cmd = NULL;
    arg = NULL;
    n = utox_run_command(device_noarg, (uint16_t)strlen(device_noarg), &cmd, &arg, 1);
    if (n == (uint16_t)-1) {
        FAIL("device without id should fail");
    }

    char device[] = "/d 0000000000000000000000000000000000000000000000000000000000000000000000000000";
    n = utox_run_command(device, (uint16_t)strlen(device), &cmd, &arg, 1);
    if (n != (uint16_t)-1 || mock_last_tox_msg != TOX_FRIEND_NEW_DEVICE) {
        FAIL("device command");
    }

    char device_bad[] = "/d zzzz";
    mock_last_tox_msg = 0;
    n = utox_run_command(device_bad, (uint16_t)strlen(device_bad), &cmd, &arg, 1);
    if (n == (uint16_t)-1 || mock_last_tox_msg != 0) {
        FAIL("device with invalid id should fail");
    }

    char unknown[] = "/nope arg";
    n = utox_run_command(unknown, (uint16_t)strlen(unknown), &cmd, &arg, 1);
    if (n == (uint16_t)-1) {
        FAIL("unknown command should not report success");
    }
    return true;
}

bool test_do_tox_url(void) {
    reset_cmd();
    uint8_t url[] = "tox:ABCDEF0123/ignored?message=Hi+there&skip=1";
    do_tox_url(url, (int)strlen((char *)url));
    if (edit_add_new_friend_id.length == 0) {
        FAIL("id should be parsed from tox url");
    }
    if (edit_add_new_friend_msg.length == 0) {
        FAIL("message= should fill friend request edit");
    }

    tox_thread_init = UTOX_TOX_THREAD_INIT_NONE;
    g_select_add_friend_later = 0;
    uint8_t early[] = "tox:abc";
    do_tox_url(early, (int)strlen((char *)early));
    if (!g_select_add_friend_later) {
        FAIL("URL before tox thread should be deferred");
    }

    tox_thread_init = UTOX_TOX_THREAD_INIT_SUCCESS;
    uint8_t extras[] = "tox:user@host.example/ignored?skip=1&message=Hi+there";
    do_tox_url(extras, (int)strlen((char *)extras));
    if (edit_add_new_friend_id.length == 0 || edit_add_new_friend_msg.length == 0) {
        FAIL("tox url should keep id chars and decode message=");
    }

    uint16_t id_len = edit_add_new_friend_id.length;
    uint16_t msg_len = edit_add_new_friend_msg.length;
    char id_copy[256];
    char msg_copy[256];
    memcpy(id_copy, edit_add_new_friend_id.data, id_len);
    memcpy(msg_copy, edit_add_new_friend_msg.data, msg_len);

    uint8_t not_tox[] = "https://example";
    do_tox_url(not_tox, (int)strlen((char *)not_tox));
    if (edit_add_new_friend_id.length != id_len || memcmp(edit_add_new_friend_id.data, id_copy, id_len) != 0) {
        FAIL("non-tox url should not change add-friend fields");
    }

    uint8_t bad[] = "tox:abc%zz";
    do_tox_url(bad, (int)strlen((char *)bad));
    if (edit_add_new_friend_id.length != id_len || memcmp(edit_add_new_friend_id.data, id_copy, id_len) != 0) {
        FAIL("invalid tox url should not keep a partial id");
    }
    if (edit_add_new_friend_msg.length != msg_len || memcmp(edit_add_new_friend_msg.data, msg_copy, msg_len) != 0) {
        FAIL("invalid tox url should not change message");
    }

    uint8_t long_url[8 + 300];
    memcpy(long_url, "tox:", 4);
    memset(long_url + 4, 'A', 300);
    long_url[304] = 0;
    do_tox_url(long_url, 304);
    if (edit_add_new_friend_id.length != id_len || memcmp(edit_add_new_friend_id.data, id_copy, id_len) != 0) {
        FAIL("overlong tox id should not commit");
    }

    uint16_t saved_id_size = edit_add_new_friend_id.data_size;
    uint16_t saved_msg_size = edit_add_new_friend_msg.data_size;
    edit_add_new_friend_id.data_size = 0;
    uint8_t shortu[] = "tox:abc";
    do_tox_url(shortu, (int)strlen((char *)shortu));
    if (edit_add_new_friend_id.length != id_len) {
        FAIL("empty id buffer should abort parse");
    }
    edit_add_new_friend_id.data_size = saved_id_size;
    edit_add_new_friend_msg.data_size = saved_msg_size;
    return true;
}

bool test_slash_helpers_direct(void) {
    reset_cmd();
    if (slash_send_file(&cmd_friend, NULL, 0)) {
        FAIL("NULL filepath");
    }
    if (!slash_send_file(&cmd_friend, "file.bin", 8)) {
        FAIL("send file");
    }
    if (!slash_topic(&cmd_group, "t", 1)) {
        FAIL("topic");
    }
    if (slash_device(&cmd_friend, "zzzz", 4)) {
        FAIL("device helper rejects bad id");
    }
    if (slash_device(&cmd_friend, NULL, 0)) {
        FAIL("device helper NULL arg");
    }
    if (slash_invite(&cmd_group, NULL, 0)) {
        FAIL("invite helper NULL arg");
    }
    if (slash_topic(&cmd_group, NULL, 0) || slash_topic(&cmd_group, "t", -1)) {
        FAIL("topic helper guards");
    }
    return true;
}

int main(void) {
    int result = 0;
    RUN_TEST(test_run_command_untrusted_and_plain);
    RUN_TEST(test_run_command_alias_invite_topic_file_device);
    RUN_TEST(test_do_tox_url);
    RUN_TEST(test_slash_helpers_direct);
    return result;
}
