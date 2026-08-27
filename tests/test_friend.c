#include "test.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../src/chatlog.c"
#include "../src/friend.c"
#include "../src/text.c"

#include "../src/avatar.h"
#include "../src/av/audio.h"
#include "mock/mock_domain.h"

pthread_mutex_t messages_lock = PTHREAD_MUTEX_INITIALIZER;

static uint32_t notice_count;
static uint32_t image_count;
static uint32_t init_count;
static uint32_t clear_all_count;
static uint32_t read_log_count;

void messages_init(MESSAGES *m, uint32_t friend_number) {
    if (m) {
        memset(m, 0, sizeof(*m));
        m->id = friend_number;
    }
    init_count++;
}

bool messages_read_from_log(uint32_t friend_number) {
    (void)friend_number;
    read_log_count++;
    return false;
}

void messages_clear_all(MESSAGES *m) {
    (void)m;
    clear_all_count++;
}

uint32_t message_add_type_notice(MESSAGES *m, const char *msgtxt, uint16_t length, bool log) {
    (void)m;
    (void)msgtxt;
    (void)length;
    (void)log;
    notice_count++;
    return 0;
}

uint32_t message_add_type_image(MESSAGES *m, bool auth, NATIVE_IMAGE *img, uint16_t width, uint16_t height, bool log) {
    (void)m;
    (void)auth;
    (void)img;
    (void)width;
    (void)height;
    (void)log;
    image_count++;
    return 0;
}

void message_free(MSG_HEADER *msg) {
    free(msg);
}

static void friend_slots_cleanup(void) {
    free_friends();
}

static void reset_friend_test(void) {
    mock_domain_reset();
    friend_slots_cleanup();
    notice_count = 0;
    image_count = 0;
    init_count = 0;
    clear_all_count = 0;
    read_log_count = 0;
    settings.portable_mode = true;
    settings.status_notifications = true;
    settings.logging_enabled = true;
    self.name_length = 4;
    memcpy(self.name, "Self", 4);
}

static FRIEND *make_friend(uint32_t n) {
    utox_friend_init((Tox *)(uintptr_t)1, n);
    return get_friend(n);
}

bool test_string_to_id_and_cid(void) {
    reset_friend_test();
    uint8_t dest[TOX_ADDRESS_SIZE];
    char hex[TOX_ADDRESS_SIZE * 2 + 1];
    memset(hex, 'a', TOX_ADDRESS_SIZE * 2);
    hex[TOX_ADDRESS_SIZE * 2] = 0;
    if (!string_to_id(dest, hex)) {
        FAIL("lowercase hex id");
    }
    if (dest[0] != 0xaa) {
        FAIL("parsed nibble");
    }

    memset(hex, 'F', TOX_ADDRESS_SIZE * 2);
    if (!string_to_id(dest, hex) || dest[0] != 0xFF) {
        FAIL("uppercase hex id");
    }

    memset(hex, '0', TOX_ADDRESS_SIZE * 2);
    if (!string_to_id(dest, hex) || dest[0] != 0) {
        FAIL("zero hex id");
    }

    hex[1] = 'x';
    if (string_to_id(dest, hex)) {
        FAIL("invalid hex should fail");
    }

    hex[0] = 'x';
    hex[1] = '0';
    if (string_to_id(dest, hex)) {
        FAIL("invalid high nibble");
    }

    const char *mixed = "0123456789ABCDEFabcdef0123456789ABCDEFabcdef0123456789ABCDEFabcdef0123456789ABCDEFabcdef";
    if (strlen(mixed) < TOX_ADDRESS_SIZE * 2 || !string_to_id(dest, (char *)mixed)) {
        FAIL("mixed hex alphabet");
    }

    char out[TOX_PUBLIC_KEY_SIZE * 2 + 1];
    uint8_t pk[TOX_PUBLIC_KEY_SIZE];
    memset(pk, 0xBC, TOX_PUBLIC_KEY_SIZE);
    cid_to_string(out, pk);
    out[TOX_PUBLIC_KEY_SIZE * 2] = 0;
    if (out[0] != 'B' || out[1] != 'C') {
        FAIL("cid_to_string");
    }
    return true;
}

bool test_get_friend_bounds_and_requests(void) {
    reset_friend_test();
    if (get_friend(0) || get_frequest(0)) {
        FAIL("empty lists");
    }

    uint8_t id[TOX_ADDRESS_SIZE];
    memset(id, 0x22, sizeof id);
    uint16_t a = friend_request_new(id, (const uint8_t *)"hi", 2);
    uint16_t b = friend_request_new(id, (const uint8_t *)"yo", 2);
    if (a == UINT16_MAX || b == UINT16_MAX) {
        FAIL("request alloc");
    }
    FREQUEST *ra = get_frequest(a);
    if (!ra || ra->length != 2 || memcmp(ra->msg, "hi", 2) != 0) {
        FAIL("request a");
    }
    friend_request_free(b);
    if (get_frequest(b)) {
        FAIL("freed last request should be gone");
    }
    if (!get_frequest(a)) {
        FAIL("first request remains after freeing last");
    }

    uint16_t c = friend_request_new(id, (const uint8_t *)"cc", 2);
    if (c == UINT16_MAX) {
        FAIL("third request");
    }
    friend_request_free(a);
    if (get_frequest(a)) {
        FAIL("freed middle request should be gone");
    }
    FREQUEST *rc = get_frequest(c);
    if (!rc || rc->length != 2 || memcmp(rc->msg, "cc", 2) != 0) {
        FAIL("later request survives middle free");
    }

    friend_request_free(c);
    if (get_frequest(a) || get_frequest(b) || get_frequest(c) || get_frequest(0)) {
        FAIL("requests should be gone after freeing all");
    }
    friend_request_free(99);
    return true;
}

bool test_friend_init_alias_lookup_online(void) {
    reset_friend_test();
    FRIEND *f = make_friend(0);
    if (!f || f->number != 0 || !f->name || f->name_length != 5 || memcmp(f->name, "Alice", 5) != 0) {
        FAIL("init name");
    }
    if (!f->online || !f->status_message || f->status_length != 2) {
        FAIL("init status/online");
    }
    if (init_count == 0 || read_log_count == 0) {
        FAIL("messages_init / read log");
    }

    friend_set_alias(f, (uint8_t *)"Bob", 3);
    if (!f->alias || f->alias_length != 3 || memcmp(f->alias, "Bob", 3) != 0) {
        FAIL("set alias");
    }
    friend_set_alias(f, (uint8_t *)"x", 0);
    if (f->alias || f->alias_length) {
        FAIL("clear alias via length 0");
    }
    friend_set_alias(f, NULL, 4);
    if (f->alias) {
        FAIL("alias length without pointer");
    }
    if (find_friend_by_name((uint8_t *)"Alice") != f) {
        FAIL("find by name");
    }
    if (find_friend_by_name((uint8_t *)"Al")) {
        FAIL("prefix should not match name");
    }
    friend_set_alias(f, (uint8_t *)"Zed", 3);
    if (find_friend_by_name((uint8_t *)"Zed") != f) {
        FAIL("find by alias");
    }
    if (find_friend_by_name((uint8_t *)"Ze")) {
        FAIL("prefix should not match alias");
    }
    friend_setname(f, (uint8_t *)"ZedName", 7);
    if (find_friend_by_name((uint8_t *)"ZedName") != f) {
        FAIL("find by renamed name");
    }
    if (find_friend_by_name((uint8_t *)"Nope")) {
        FAIL("missing name");
    }
    if (find_friend_by_name(NULL)) {
        FAIL("NULL name");
    }
    if (get_friend_by_id(f->id_str) != f) {
        FAIL("get by id");
    }
    if (get_friend_by_id("0000000000000000000000000000000000000000000000000000000000000000")) {
        FAIL("unknown id");
    }

    if (!friend_set_online(f, false) || f->online || f->typing) {
        FAIL("go offline");
    }
    if (friend_set_online(f, false)) {
        FAIL("unchanged online");
    }
    friend_set_typing(f, 1);
    if (!f->typing) {
        FAIL("typing");
    }
    if (!friend_set_online(f, true) || !f->online) {
        FAIL("go online");
    }

    friend_setname(f, (uint8_t *)"Carol", 5);
    if (notice_count == 0 || f->name_length != 5 || memcmp(f->name, "Carol", 5) != 0) {
        FAIL("rename should notice");
    }
    uint32_t after_rename = notice_count;
    friend_setname(f, (uint8_t *)"Carol", 5);
    if (notice_count != after_rename) {
        FAIL("same name should not notice");
    }

    mock_sel_item_type = ITEM_FRIEND;
    mock_sel_friend = NULL;
    friend_set_alias(f, NULL, 0);
    friend_setname(f, (uint8_t *)"Dana", 4);

    mock_sel_friend = f;
    friend_setname(f, (uint8_t *)"", 0);
    if (f->name_length != TOX_PUBLIC_KEY_SIZE * 2) {
        FAIL("empty name becomes pubkey hex");
    }

    friend_history_clear(NULL);
    friend_history_clear(f);
    if (clear_all_count == 0) {
        FAIL("history clear");
    }

    friend_slots_cleanup();
    return true;
}

bool test_friend_metadata_and_add(void) {
    reset_friend_test();
    FRIEND *f = make_friend(0);
    friend_set_alias(f, (uint8_t *)"Meta", 4);
    f->ft_autoaccept = true;
    utox_write_metadata(f);

    friend_set_alias(f, NULL, 0);
    friend_free(f);

    FRIEND *f2 = make_friend(0);
    if (!f2->alias || f2->alias_length != 4 || memcmp(f2->alias, "Meta", 4) != 0) {
        FAIL("metadata alias reload");
    }
    if (!f2->ft_autoaccept) {
        FAIL("metadata ft_autoaccept");
    }

    char dest[UTOX_FILE_NAME_LENGTH];
    snprintf(dest, sizeof dest, "%.*s.fmetadata", TOX_PUBLIC_KEY_SIZE * 2, f2->id_str);

    f2->skip_msg_logging = true;
    f2->ft_autoaccept = false;
    friend_set_alias(f2, NULL, 0);
    utox_write_metadata(f2);
    friend_free(f2);
    FRIEND *f_skip = make_friend(0);
    if (f_skip->alias) {
        FAIL("metadata without alias");
    }
    if (!f_skip->skip_msg_logging) {
        FAIL("skip_msg_logging should reload from metadata");
    }
    if (f_skip->ft_autoaccept) {
        FAIL("ft_autoaccept false should reload from metadata");
    }
    friend_set_alias(f_skip, NULL, 0);
    friend_free(f_skip);

    FRIEND_META_DATA bad_ver = { 0 };
    bad_ver.version = 1;
    FILE *ver = utox_get_file(dest, NULL, UTOX_FILE_OPTS_WRITE);
    if (ver) {
        fwrite(&bad_ver, sizeof bad_ver, 1, ver);
        fclose(ver);
    }
    FRIEND *f_ver = make_friend(0);
    if (f_ver->alias) {
        FAIL("unsupported metadata version");
    }
    friend_set_alias(f_ver, NULL, 0);
    friend_free(f_ver);

    FILE *bad = utox_get_file(dest, NULL, UTOX_FILE_OPTS_WRITE);
    if (bad) {
        fwrite("xx", 1, 2, bad);
        fclose(bad);
    }
    FRIEND *f3 = make_friend(0);
    if (f3->alias) {
        FAIL("short metadata should not set alias");
    }

    addfriend_status = ADDF_NONE;
    friend_add("", 0, "msg", 3);
    if (addfriend_status != ADDF_NONAME) {
        FAIL("empty name");
    }
    friend_add("   ", 3, "msg", 3);
    if (addfriend_status != ADDF_NONAME) {
        FAIL("spaces only");
    }
    friend_add("not-an-id", 9, "msg", 3);
    if (addfriend_status != ADDF_BADNAME) {
        FAIL("bad name");
    }

    char addr[TOX_ADDRESS_SIZE * 2 + 1];
    memset(addr, 'a', TOX_ADDRESS_SIZE * 2);
    addr[TOX_ADDRESS_SIZE * 2] = 0;
    friend_add(addr, TOX_ADDRESS_SIZE * 2, "hello", 5);
    if (mock_last_tox_msg != TOX_FRIEND_NEW) {
        FAIL("full tox id add");
    }

    char pk[TOX_PUBLIC_KEY_SIZE * 2 + 4];
    memset(pk, ' ', sizeof pk);
    memset(pk + 1, 'b', TOX_PUBLIC_KEY_SIZE * 2);
    pk[TOX_PUBLIC_KEY_SIZE * 2 + 2] = ' ';
    pk[TOX_PUBLIC_KEY_SIZE * 2 + 3] = 0;
    friend_add(pk, (uint16_t)strlen(pk), "x", 1);
    if (addfriend_status != ADDF_NOFREQUESTSENT || mock_last_tox_msg != TOX_FRIEND_NEW_NO_REQ) {
        FAIL("pubkey add with spaces");
    }

    uint8_t raw[TOX_ADDRESS_SIZE];
    memset(raw, 1, sizeof raw);
    friend_addid(raw, "yo", 2);
    if (mock_last_tox_msg != TOX_FRIEND_NEW) {
        FAIL("friend_addid");
    }

    friend_slots_cleanup();
    return true;
}

bool test_friend_notify_image_list(void) {
    reset_friend_test();
    FRIEND *f = make_friend(0);
    mock_sel_friend = NULL;
    have_focus = false;
    friend_notify_msg(f, "hello", 5);
    if (!f->unread_msg || mock_last_audio_msg != UTOXAUDIO_PLAY_NOTIFICATION) {
        FAIL("notify msg");
    }

    mock_sel_friend = f;
    have_focus = true;
    f->unread_msg = false;
    friend_notify_msg(f, "quiet", 5);
    if (f->unread_msg) {
        FAIL("selected+focused should not mark unread");
    }

    f->online = true;
    friend_notify_status(f, (const uint8_t *)"bye", 3, "offline");
    f->online = false;
    friend_notify_status(f, (const uint8_t *)"hi", 2, "online");
    settings.status_notifications = false;
    friend_notify_status(f, (const uint8_t *)"x", 1, "online");

    uint8_t dummy_native;
    uint8_t png = 0;
    friend_sendimage(f, (NATIVE_IMAGE *)&dummy_native, 10, 10, &png, 1);
    if (image_count == 0) {
        FAIL("send image");
    }
    friend_recvimage(f, NULL, 1, 1);
    friend_recvimage(f, (NATIVE_IMAGE *)&dummy_native, 2, 2);
    if (image_count < 2) {
        FAIL("recv image");
    }

    f->edit_history_length = 1;
    f->edit_history = calloc(1, sizeof(void *));
    f->edit_history[0] = calloc(1, sizeof(EDIT_CHANGE));
    f->call_state_self = 1;

    FRIEND *f1 = make_friend(1);
    if (!f1 || get_friend(1) != f1) {
        FAIL("second friend slot");
    }
    if (self.friend_list_count < 2) {
        FAIL("two friends before free_friends");
    }
    free_friends();
    if (self.friend_list_count != 0 || self.friend_list_size != 0) {
        FAIL("free_friends should clear count and size");
    }
    if (get_friend(0) || get_friend(1)) {
        FAIL("friends should be gone after free_friends");
    }

    mock_tox_friend_count = 1;
    utox_friend_list_init((Tox *)(uintptr_t)1);
    if (!get_friend(0) || self.friend_list_count == 0) {
        FAIL("list init");
    }
    friend_slots_cleanup();
    return true;
}

bool test_friend_has_avatar(void) {
    reset_friend_test();
    FRIEND *fp = NULL;
    if (friend_has_avatar(fp)) {
        FAIL("NULL friend is not an avatar");
    }
    if (!friend_has_avatar(fp)) {
        /* desired: negation must not crash and must be true */
    } else {
        FAIL("!friend_has_avatar(NULL)");
    }

    FRIEND bare = { 0 };
    fp = &bare;
    if (!friend_has_avatar(fp)) {
        /* no avatar */
    } else {
        FAIL("!friend_has_avatar with NULL avatar");
    }
    if (friend_has_avatar(fp)) {
        FAIL("NULL avatar pointer");
    }

    AVATAR none = { 0 };
    bare.avatar = &none;
    if (friend_has_avatar(fp)) {
        FAIL("NONE format is not set");
    }
    if (!friend_has_avatar(fp)) {
        /* no avatar */
    } else {
        FAIL("!friend_has_avatar NONE");
    }

    none.format = UTOX_AVATAR_FORMAT_PNG;
    if (!friend_has_avatar(fp)) {
        FAIL("PNG format should count as set");
    }
    return true;
}

bool test_friend_sparse_make_and_free(void) {
    reset_friend_test();
    FRIEND *f0 = make_friend(0);
    FRIEND *f5 = make_friend(5);
    if (!f0 || !f5) {
        FAIL("create friends 0 and 5");
    }
    FRIEND *hole = get_friend(1);
    if (!hole || hole->name || hole->alias || hole->avatar || hole->status_message) {
        FAIL("slots opened by a sparse create should be zeroed");
    }
    if (self.friend_list_size < 6 || self.friend_list_count != 2) {
        FAIL("list size should cover index 5");
    }
    free_friends();
    if (self.friend_list_count != 0 || self.friend_list_size != 0) {
        FAIL("sparse free_friends should clear count and size");
    }
    if (get_friend(0) || get_friend(5)) {
        FAIL("sparse friends should be gone");
    }
    return true;
}

bool test_friend_add_pubkey_parses_only_32_bytes(void) {
    reset_friend_test();

    char bad[TOX_PUBLIC_KEY_SIZE * 2 + 1];
    memset(bad, 'z', TOX_PUBLIC_KEY_SIZE * 2);
    bad[TOX_PUBLIC_KEY_SIZE * 2] = 0;
    addfriend_status = ADDF_NONE;
    mock_last_tox_msg = 0;
    friend_add(bad, (uint16_t)(TOX_PUBLIC_KEY_SIZE * 2), "x", 1);
    if (addfriend_status != ADDF_BADNAME || mock_last_tox_msg != 0) {
        FAIL("invalid pubkey hex should be BADNAME and send nothing");
    }

    char pk[TOX_PUBLIC_KEY_SIZE * 2 + 1];
    memset(pk, 'a', TOX_PUBLIC_KEY_SIZE * 2);
    pk[TOX_PUBLIC_KEY_SIZE * 2] = 0;
    addfriend_status = ADDF_NONE;
    friend_add(pk, (uint16_t)(TOX_PUBLIC_KEY_SIZE * 2), "x", 1);
    if (addfriend_status != ADDF_NOFREQUESTSENT || mock_last_tox_msg != TOX_FRIEND_NEW_NO_REQ) {
        FAIL("valid 64-hex pubkey");
    }
    if (mock_last_tox_blob_len != TOX_PUBLIC_KEY_SIZE) {
        FAIL("posted key length %zu", mock_last_tox_blob_len);
    }
    for (size_t i = 0; i < TOX_PUBLIC_KEY_SIZE; i++) {
        if (mock_last_tox_blob[i] != 0xaa) {
            FAIL("posted key byte %zu", i);
        }
    }
    return true;
}

bool test_friend_request_reuses_hole(void) {
    reset_friend_test();
    uint8_t id[TOX_ADDRESS_SIZE];
    memset(id, 0x33, sizeof id);

    uint16_t a = friend_request_new(id, (const uint8_t *)"a", 1);
    uint16_t b = friend_request_new(id, (const uint8_t *)"b", 1);
    uint16_t c = friend_request_new(id, (const uint8_t *)"c", 1);
    if (a == UINT16_MAX || b == UINT16_MAX || c == UINT16_MAX) {
        FAIL("alloc three requests");
    }

    friend_request_free(b);
    if (get_frequest(b)) {
        FAIL("middle hole should be empty");
    }

    uint16_t d = friend_request_new(id, (const uint8_t *)"d", 1);
    if (d != b) {
        FAIL("new request should reuse hole at %u, got %u", b, d);
    }
    FREQUEST *rd = get_frequest(d);
    if (!rd || rd->length != 1 || rd->msg[0] != 'd') {
        FAIL("reused slot content");
    }
    if (!get_frequest(a) || !get_frequest(c)) {
        FAIL("neighbors should remain");
    }

    friend_request_free(a);
    friend_request_free(d);
    friend_request_free(c);
    return true;
}

bool test_friend_free_with_messages(void) {
    reset_friend_test();
    FRIEND *f = make_friend(0);
    if (!f) {
        FAIL("make friend");
    }

    f->msg.data    = calloc(2, sizeof(MSG_HEADER *));
    f->msg.number  = 2;
    f->msg.data[0] = calloc(1, sizeof(MSG_HEADER));
    f->msg.data[1] = calloc(1, sizeof(MSG_HEADER));
    if (!f->msg.data[0] || !f->msg.data[1]) {
        FAIL("alloc msg headers");
    }
    f->msg.data[0]->msg_type = MSG_TYPE_TEXT;
    f->msg.data[1]->msg_type = MSG_TYPE_NOTICE;

    friend_free(f);
    if (self.friend_list_count != 0) {
        FAIL("friend_free should decrement count");
    }

    free_friends();
    return true;
}

bool test_friend_notify_status_tones(void) {
    reset_friend_test();
    FRIEND *f = make_friend(0);
    if (!f) {
        FAIL("make friend");
    }

    f->online = true;
    mock_last_audio_msg    = 0;
    mock_last_audio_param1 = 0;
    friend_notify_status(f, (const uint8_t *)"bye", 3, "offline");
    if (mock_last_audio_msg != UTOXAUDIO_PLAY_NOTIFICATION) {
        FAIL("expected notification audio msg, got %u", mock_last_audio_msg);
    }
    if (mock_last_audio_param1 != NOTIFY_TONE_FRIEND_OFFLINE) {
        FAIL("online friend notify should play offline tone, got %u", mock_last_audio_param1);
    }

    f->online = false;
    mock_last_audio_param1 = 0;
    friend_notify_status(f, (const uint8_t *)"hi", 2, "online");
    if (mock_last_audio_param1 != NOTIFY_TONE_FRIEND_ONLINE) {
        FAIL("offline friend notify should play online tone, got %u", mock_last_audio_param1);
    }

    friend_slots_cleanup();
    return true;
}

bool test_friend_sparse_lookup_by_id(void) {
    reset_friend_test();
    FRIEND *f0 = make_friend(0);
    FRIEND *f5 = make_friend(5);
    if (!f0 || !f5) {
        FAIL("create sparse friends");
    }

    char id5[TOX_PUBLIC_KEY_SIZE * 2 + 1];
    memcpy(id5, f5->id_str, TOX_PUBLIC_KEY_SIZE * 2);
    id5[TOX_PUBLIC_KEY_SIZE * 2] = 0;

    friend_set_alias(f5, (uint8_t *)"SparseFive", 10);

    /* Product iterates friend_list_count indices, not friend_list_size.
     * With friends at 0 and 5, count==2 so lookups never reach index 5. */
    FRIEND *by_id = get_friend_by_id(id5);
    if (by_id != f5) {
        FAIL("get_friend_by_id should find sparse friend 5 (got %p expected %p)", (void *)by_id,
             (void *)f5);
    }
    FRIEND *by_alias = find_friend_by_name((uint8_t *)"SparseFive");
    if (by_alias != f5) {
        FAIL("find_friend_by_name should find sparse friend 5 alias");
    }

    friend_slots_cleanup();
    return true;
}

bool test_friend_metadata_write_open_fail(void) {
    reset_friend_test();
    FRIEND *f = make_friend(0);
    if (!f) {
        FAIL("make friend");
    }
    friend_set_alias(f, (uint8_t *)"X", 1);

    /* Block ./tox as a directory by replacing it with a regular file. */
    rename("./tox", "./tox_bak_meta_test");
    FILE *tox_file = fopen("./tox", "wb");
    if (!tox_file) {
        rename("./tox_bak_meta_test", "./tox");
        FAIL("could not create blocking file ./tox");
    }
    fwrite("x", 1, 1, tox_file);
    fclose(tox_file);

    utox_write_metadata(f); /* should log and return without crashing */

    remove("./tox");
    rename("./tox_bak_meta_test", "./tox");

    friend_slots_cleanup();
    return true;
}

int main(void) {
    int result = 0;
    setvbuf(stdout, NULL, _IONBF, 0);
    settings.portable_mode = true;
    RUN_TEST(test_string_to_id_and_cid);
    RUN_TEST(test_get_friend_bounds_and_requests);
    RUN_TEST(test_friend_init_alias_lookup_online);
    RUN_TEST(test_friend_metadata_and_add);
    RUN_TEST(test_friend_notify_image_list);
    RUN_TEST(test_friend_has_avatar);
    RUN_TEST(test_friend_sparse_make_and_free);
    RUN_TEST(test_friend_add_pubkey_parses_only_32_bytes);
    RUN_TEST(test_friend_request_reuses_hole);
    RUN_TEST(test_friend_free_with_messages);
    RUN_TEST(test_friend_notify_status_tones);
    RUN_TEST(test_friend_sparse_lookup_by_id);
    RUN_TEST(test_friend_metadata_write_open_fail);
    friend_slots_cleanup();
    return result;
}
