#include "test.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/file_transfers.c"
#include "../src/text.c"

#include "../src/filesys.h"
#include "mock/mock_domain.h"

#define TOX_DUMMY ((Tox *)(uintptr_t)1)

static FRIEND test_friend;
static AVATAR test_avatar;

static uint32_t incoming_fileno(uint32_t index) {
    return (index + 1) << 16;
}

FRIEND *get_friend(uint32_t friend_number) {
    if (friend_number != 0) {
        return NULL;
    }
    return &test_friend;
}

static void hex64(char *out, char fill) {
    memset(out, fill, TOX_PUBLIC_KEY_SIZE * 2);
    out[TOX_PUBLIC_KEY_SIZE * 2] = 0;
}

static void cancel_open(void) {
    Tox *tox = TOX_DUMMY;
    for (uint16_t i = 0; i < test_friend.ft_outgoing_size; i++) {
        if (test_friend.ft_outgoing && test_friend.ft_outgoing[i].in_use) {
            ft_local_control(tox, 0, i, TOX_FILE_CONTROL_CANCEL);
        }
    }
    for (uint16_t i = 0; i < test_friend.ft_incoming_size; i++) {
        if (test_friend.ft_incoming && test_friend.ft_incoming[i].in_use) {
            ft_local_control(tox, 0, incoming_fileno(i), TOX_FILE_CONTROL_CANCEL);
        }
    }
    free(test_friend.ft_outgoing);
    free(test_friend.ft_incoming);
    test_friend.ft_outgoing = NULL;
    test_friend.ft_incoming = NULL;
    test_friend.ft_outgoing_size = 0;
    test_friend.ft_incoming_size = 0;
    test_friend.ft_outgoing_active_count = 0;
    test_friend.ft_incoming_active_count = 0;
}

static void reset_ft(void) {
    cancel_open();
    mock_domain_reset();
    memset(&test_friend, 0, sizeof test_friend);
    memset(&test_avatar, 0, sizeof test_avatar);
    test_friend.number = 0;
    hex64(test_friend.id_str, 'A');
    test_friend.avatar = &test_avatar;
    settings.portable_mode = true;
    settings.accept_inline_images = true;
    free(self.png_data);
    self.png_data = NULL;
    self.png_size = 0;
    utox_set_callbacks_file_transfer(TOX_DUMMY);
}

static void incoming_ftinfo_name(char *out, size_t out_sz) {
    char hex[TOX_HASH_LENGTH * 2 + 1];
    memset(hex, 0, sizeof hex);
    to_hex(hex, mock_incoming_file_id, TOX_HASH_LENGTH);
    snprintf(out, out_sz, "%.*s.ftinfo", TOX_HASH_LENGTH * 2, hex);
}

static bool write_resume_blob(const char *name, FILE_TRANSFER *saved) {
    FILE *fp = utox_get_file(name, NULL, UTOX_FILE_OPTS_WRITE | UTOX_FILE_OPTS_MKDIR);
    if (!fp) {
        return false;
    }
    bool ok = fwrite(saved, sizeof(FILE_TRANSFER), 1, fp) == 1;
    fclose(fp);
    return ok;
}

static FILE *make_payload(const char *path, const char *contents) {
    FILE *fp = fopen(path, "w+b");
    if (!fp) {
        return NULL;
    }
    if (fwrite(contents, 1, strlen(contents), fp) != strlen(contents)) {
        fclose(fp);
        return NULL;
    }
    fflush(fp);
    return fp;
}

bool test_unknown_transfer_and_ui_data(void) {
    reset_ft();
    MSG_HEADER *ui = (MSG_HEADER *)(uintptr_t)0x11;

    if (ft_set_ui_data(0, 0, ui)) {
        FAIL("ui_data on missing transfer");
    }
    ft_local_control(TOX_DUMMY, 0, 0, TOX_FILE_CONTROL_RESUME);

    uint8_t blob[] = { 1, 2, 3, 4 };
    uint32_t n = ft_send_data(TOX_DUMMY, 0, blob, sizeof blob, (uint8_t *)"x.bin", 5);
    if (n == UINT32_MAX) {
        FAIL("send data");
    }
    if (!ft_set_ui_data(0, n, ui)) {
        FAIL("set ui_data");
    }
    if (!test_friend.ft_outgoing[n].ui_data || test_friend.ft_outgoing[n].ui_data != ui) {
        FAIL("ui_data pointer");
    }
    if (ft_set_ui_data(99, n, ui)) {
        FAIL("ui_data unknown friend");
    }

    cancel_open();
    return true;
}

bool test_send_data_slot_status_and_limit(void) {
    reset_ft();
    uint8_t blob[8];
    memset(blob, 0x22, sizeof blob);

    uint32_t n = ft_send_data(TOX_DUMMY, 0, blob, sizeof blob, (uint8_t *)"inline.bin", 10);
    if (n != 0) {
        FAIL("first file number %u", n);
    }
    FILE_TRANSFER *ft = &test_friend.ft_outgoing[0];
    if (!ft->in_use || !ft->in_memory || !ft->inline_img || ft->incoming) {
        FAIL("outgoing in-memory flags");
    }
    if (ft->status != FILE_TRANSFER_STATUS_PAUSED_THEM) {
        FAIL("new send starts paused by them");
    }
    if (ft->target_size != sizeof blob || ft->via.memory != blob) {
        FAIL("payload pointer/size");
    }
    if (test_friend.ft_outgoing_active_count != 1) {
        FAIL("active count");
    }
    if (ft_send_data(TOX_DUMMY, 99, blob, sizeof blob, (uint8_t *)"x", 1) != UINT32_MAX) {
        FAIL("send to missing friend");
    }
    if (ft_send_data(NULL, 0, blob, sizeof blob, (uint8_t *)"x", 1) != UINT32_MAX) {
        FAIL("send without tox");
    }

    /* Fill remaining slots up to the documented cap. */
    uint32_t sent = 1;
    while (sent < MAX_FILE_TRANSFERS) {
        uint32_t id = ft_send_data(TOX_DUMMY, 0, blob, sizeof blob, (uint8_t *)"n", 1);
        if (id == UINT32_MAX) {
            FAIL("send %u of %u", sent, MAX_FILE_TRANSFERS);
        }
        sent++;
    }
    if (test_friend.ft_outgoing_active_count != MAX_FILE_TRANSFERS) {
        FAIL("active at cap %u", test_friend.ft_outgoing_active_count);
    }
    if (ft_send_data(TOX_DUMMY, 0, blob, sizeof blob, (uint8_t *)"over", 4) != UINT32_MAX) {
        FAIL("33rd in-memory send must fail");
    }

    cancel_open();
    return true;
}

bool test_send_file_basename(void) {
    reset_ft();
    FILE *fp = make_payload("./tox/ft_plain.txt", "hello");
    if (!fp) {
        FAIL("open payload");
    }

    uint8_t noslash[] = "hello.txt";
    uint32_t n = ft_send_file(TOX_DUMMY, 0, fp, noslash, strlen((char *)noslash), NULL);
    if (n == UINT32_MAX) {
        fclose(fp);
        FAIL("send file without directory");
    }
    FILE_TRANSFER *ft = &test_friend.ft_outgoing[n];
    if (!ft->name || ft->name_length != 9 || memcmp(ft->name, "hello.txt", 9) != 0) {
        FAIL("basename without slash: '%.*s' len %u", (int)ft->name_length, ft->name, (unsigned)ft->name_length);
    }
    ft_local_control(TOX_DUMMY, 0, n, TOX_FILE_CONTROL_CANCEL);

    FILE *fp2 = make_payload("./tox/ft_dir.txt", "world");
    if (!fp2) {
        FAIL("open payload 2");
    }
    uint8_t withslash[] = "./tox/ft_dir.txt";
    n = ft_send_file(TOX_DUMMY, 0, fp2, withslash, strlen((char *)withslash), NULL);
    if (n == UINT32_MAX) {
        fclose(fp2);
        FAIL("send file with slash");
    }
    ft = &test_friend.ft_outgoing[n];
    if (!ft->name || ft->name_length != 10 || memcmp(ft->name, "ft_dir.txt", 10) != 0) {
        FAIL("basename with slash: '%.*s'", (int)ft->name_length, ft->name);
    }

    cancel_open();
    remove("./tox/ft_plain.txt");
    remove("./tox/ft_dir.txt");
    return true;
}

bool test_send_avatar_and_max(void) {
    reset_ft();
    uint8_t png[16];
    memset(png, 0x89, sizeof png);
    self.png_data = malloc(sizeof png);
    memcpy(self.png_data, png, sizeof png);
    self.png_size = sizeof png;

    if (ft_send_avatar(TOX_DUMMY, 99) != UINT32_MAX) {
        FAIL("avatar to missing friend");
    }
    uint32_t n = ft_send_avatar(TOX_DUMMY, 0);
    if (n == UINT32_MAX) {
        FAIL("send avatar");
    }
    FILE_TRANSFER *ft = &test_friend.ft_outgoing[n];
    if (!ft->avatar || ft->incoming || ft->status != FILE_TRANSFER_STATUS_PAUSED_THEM) {
        FAIL("avatar flags");
    }
    if (mock_last_file_kind != TOX_FILE_KIND_AVATAR) {
        FAIL("tox kind avatar");
    }

    /* Remaining 31 data sends + this avatar = 32. Next avatar must fail. */
    uint8_t blob[4] = { 1, 2, 3, 4 };
    for (uint32_t i = 1; i < MAX_FILE_TRANSFERS; i++) {
        if (ft_send_data(TOX_DUMMY, 0, blob, sizeof blob, (uint8_t *)"n", 1) == UINT32_MAX) {
            FAIL("fill to cap");
        }
    }
    if (ft_send_avatar(TOX_DUMMY, 0) != UINT32_MAX) {
        FAIL("avatar past MAX_FILE_TRANSFERS");
    }
    FILE *overflow = make_payload("./tox/ft_over.bin", "x");
    if (!overflow) {
        FAIL("overflow payload");
    }
    if (ft_send_file(TOX_DUMMY, 0, overflow, (uint8_t *)"ft_over.bin", 11, NULL) != UINT32_MAX) {
        fclose(overflow);
        FAIL("file past MAX_FILE_TRANSFERS");
    }
    fclose(overflow);
    remove("./tox/ft_over.bin");

    cancel_open();
    return true;
}

bool test_local_and_remote_status_machine(void) {
    reset_ft();
    uint8_t blob[4] = { 9, 9, 9, 9 };
    uint32_t n = ft_send_data(TOX_DUMMY, 0, blob, sizeof blob, (uint8_t *)"s.bin", 5);
    FILE_TRANSFER *ft = &test_friend.ft_outgoing[n];

    if (!mock_cb_file_recv_control) {
        FAIL("control callback not registered");
    }

    /* Them resume: paused-them -> active */
    mock_cb_file_recv_control(TOX_DUMMY, 0, n, TOX_FILE_CONTROL_RESUME, NULL);
    if (ft->status != FILE_TRANSFER_STATUS_ACTIVE) {
        FAIL("remote resume -> active, got %u", ft->status);
    }

    ft_local_control(TOX_DUMMY, 0, n, TOX_FILE_CONTROL_PAUSE);
    if (ft->status != FILE_TRANSFER_STATUS_PAUSED_US) {
        FAIL("local pause -> paused us, got %u", ft->status);
    }

    mock_cb_file_recv_control(TOX_DUMMY, 0, n, TOX_FILE_CONTROL_PAUSE, NULL);
    if (ft->status != FILE_TRANSFER_STATUS_PAUSED_BOTH) {
        FAIL("remote pause while we paused -> both, got %u", ft->status);
    }

    ft_local_control(TOX_DUMMY, 0, n, TOX_FILE_CONTROL_RESUME);
    if (ft->status != FILE_TRANSFER_STATUS_PAUSED_THEM) {
        FAIL("local resume from both -> paused them, got %u", ft->status);
    }

    mock_cb_file_recv_control(TOX_DUMMY, 0, n, TOX_FILE_CONTROL_RESUME, NULL);
    if (ft->status != FILE_TRANSFER_STATUS_ACTIVE) {
        FAIL("remote resume again -> active");
    }

    ft_local_control(TOX_DUMMY, 0, n, TOX_FILE_CONTROL_CANCEL);
    if (ft->in_use) {
        FAIL("cancel must decon slot");
    }
    if (test_friend.ft_outgoing_active_count != 0) {
        FAIL("active count after cancel");
    }
    return true;
}

bool test_outgoing_chunks(void) {
    reset_ft();
    uint8_t blob[8];
    memset(blob, 0xAB, sizeof blob);
    uint32_t n = ft_send_data(TOX_DUMMY, 0, blob, sizeof blob, (uint8_t *)"c.bin", 5);
    if (!mock_cb_file_chunk_request) {
        FAIL("chunk request callback");
    }

    mock_cb_file_recv_control(TOX_DUMMY, 0, n, TOX_FILE_CONTROL_RESUME, NULL);
    mock_cb_file_chunk_request(TOX_DUMMY, 0, n, 0, 4, NULL);
    if (mock_file_send_chunk_count != 1 || mock_last_chunk_length != 4) {
        FAIL("first chunk");
    }
    mock_cb_file_chunk_request(TOX_DUMMY, 0, n, 4, 4, NULL);
    mock_cb_file_chunk_request(TOX_DUMMY, 0, n, 8, 0, NULL);
    if (test_friend.ft_outgoing[n].in_use) {
        FAIL("complete should decon");
    }
    return true;
}

bool test_incoming_file_write(void) {
    reset_ft();
    const uint8_t name[] = "photo.png";
    if (!mock_cb_file_recv) {
        FAIL("recv callback");
    }
    mock_cb_file_recv(TOX_DUMMY, 0, incoming_fileno(0), TOX_FILE_KIND_DATA, 5, name, sizeof name - 1, NULL);

    FILE_TRANSFER *ft = &test_friend.ft_incoming[0];
    if (!ft->in_use || !ft->incoming || ft->target_size != 5) {
        FAIL("incoming request flags");
    }
    if (mock_last_utox_msg != FILE_INCOMING_NEW) {
        FAIL("FILE_INCOMING_NEW posted");
    }

    if (!utox_file_start_write(0, incoming_fileno(0), "./tox/photo.png")) {
        FAIL("start write");
    }
    ft_local_control(TOX_DUMMY, 0, incoming_fileno(0), TOX_FILE_CONTROL_RESUME);
    if (ft->status != FILE_TRANSFER_STATUS_ACTIVE) {
        FAIL("resume incoming -> active %u", ft->status);
    }

    const uint8_t chunk[] = { 'h', 'e', 'l', 'l', 'o' };
    mock_cb_file_recv_chunk(TOX_DUMMY, 0, incoming_fileno(0), 0, chunk, sizeof chunk, NULL);
    mock_cb_file_recv_chunk(TOX_DUMMY, 0, incoming_fileno(0), 5, NULL, 0, NULL);
    if (test_friend.ft_incoming[0].in_use) {
        FAIL("incoming complete should decon");
    }

    FILE *out = fopen("./tox/photo.png", "rb");
    if (!out) {
        FAIL("written file missing");
    }
    char buf[8] = { 0 };
    size_t got = fread(buf, 1, sizeof buf, out);
    fclose(out);
    remove("./tox/photo.png");
    if (got != 5 || memcmp(buf, "hello", 5) != 0) {
        FAIL("file contents");
    }
    return true;
}

bool test_incoming_unknown_friend_does_not_crash(void) {
    reset_ft();
    const uint8_t name[] = "a.bin";
    mock_cb_file_recv(TOX_DUMMY, 99, incoming_fileno(0), TOX_FILE_KIND_DATA, 10, name, sizeof name - 1, NULL);
    if (test_friend.ft_incoming_active_count != 0) {
        FAIL("no slot for unknown friend");
    }
    if (mock_file_control_count == 0 || mock_last_file_control != TOX_FILE_CONTROL_CANCEL) {
        FAIL("unknown friend must cancel the offer");
    }
    return true;
}

bool test_incoming_avatar_null_and_too_large(void) {
    reset_ft();
    test_friend.avatar = NULL;
    mock_cb_file_recv(TOX_DUMMY, 0, incoming_fileno(0), TOX_FILE_KIND_AVATAR, 32, NULL, 0, NULL);
    if (!test_friend.ft_incoming || !test_friend.ft_incoming[0].in_use || !test_friend.ft_incoming[0].avatar) {
        FAIL("avatar accepted when friend has no avatar yet");
    }
    cancel_open();

    reset_ft();
    mock_cb_file_recv(TOX_DUMMY, 0, incoming_fileno(0), TOX_FILE_KIND_AVATAR, UTOX_AVATAR_MAX_DATA_LENGTH + 1, NULL, 0,
                      NULL);
    if (test_friend.ft_incoming_active_count != 0) {
        FAIL("oversized avatar must be rejected");
    }

    mock_cb_file_recv(TOX_DUMMY, 0, incoming_fileno(0), TOX_FILE_KIND_AVATAR, 0, NULL, 0, NULL);
    if (mock_last_utox_msg != FRIEND_AVATAR_UNSET) {
        FAIL("zero-size avatar unsets");
    }
    return true;
}

bool test_incoming_oob_ui_data(void) {
    reset_ft();
    const uint8_t name[] = "a.bin";
    mock_cb_file_recv(TOX_DUMMY, 0, incoming_fileno(0), TOX_FILE_KIND_DATA, 4, name, sizeof name - 1, NULL);
    if (test_friend.ft_incoming_size != 1) {
        FAIL("one incoming slot");
    }

    MSG_HEADER *ui = (MSG_HEADER *)(uintptr_t)0x22;
    /* Index 1 is not allocated; must not write past the array. */
    if (ft_set_ui_data(0, incoming_fileno(1), ui)) {
        FAIL("ui_data on unallocated incoming index");
    }
    if (ft_set_ui_data(0, incoming_fileno(0), ui) == false) {
        FAIL("ui_data on real incoming");
    }
    cancel_open();
    return true;
}

bool test_sparse_outgoing_zeroed_and_offline(void) {
    reset_ft();
    mock_tox_file_send_next = 2;
    uint8_t blob[4] = { 1, 2, 3, 4 };
    uint32_t n = ft_send_data(TOX_DUMMY, 0, blob, sizeof blob, (uint8_t *)"s.bin", 5);
    if (n != 2) {
        FAIL("expected tox file number 2, got %u", n);
    }
    if (test_friend.ft_outgoing_size != 3) {
        FAIL("array grown to 3, size %u", test_friend.ft_outgoing_size);
    }
    if (test_friend.ft_outgoing[0].in_use || test_friend.ft_outgoing[1].in_use) {
        FAIL("hole slots must be zeroed");
    }
    if (!test_friend.ft_outgoing[2].in_use) {
        FAIL("slot 2 in use");
    }

    ft_friend_offline(TOX_DUMMY, 0);
    if (test_friend.ft_outgoing[2].in_use) {
        FAIL("offline breaks active outgoing");
    }
    ft_friend_offline(TOX_DUMMY, 99);
    return true;
}

bool test_incoming_inline_png(void) {
    reset_ft();
    const uint8_t name[] = "utox-inline.png";
    uint8_t png[] = { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x01 };
    mock_cb_file_recv(TOX_DUMMY, 0, incoming_fileno(0), TOX_FILE_KIND_DATA, sizeof png, name, sizeof name - 1, NULL);

    FILE_TRANSFER *ft = &test_friend.ft_incoming[0];
    if (!ft->inline_img || !ft->in_memory || !ft->via.memory) {
        FAIL("inline accepted");
    }

    mock_cb_file_recv_chunk(TOX_DUMMY, 0, incoming_fileno(0), 0, png, sizeof png, NULL);
    mock_cb_file_recv_chunk(TOX_DUMMY, 0, incoming_fileno(0), sizeof png, NULL, 0, NULL);
    if (test_friend.ft_incoming[0].in_use) {
        FAIL("inline complete decon");
    }
    return true;
}

bool test_friend_online_without_resume_files(void) {
    reset_ft();
    ft_friend_online(TOX_DUMMY, 0);
    ft_friend_online(TOX_DUMMY, 99);
    return true;
}

bool test_incoming_resume_from_disk(void) {
    reset_ft();
    FILE *payload = make_payload("./tox/resume.bin", "hello!!!");
    if (!payload) {
        FAIL("create resume payload");
    }
    fclose(payload);

    FILE_TRANSFER saved;
    memset(&saved, 0, sizeof saved);
    saved.in_use = true;
    saved.incoming = true;
    saved.resumeable = true;
    saved.current_size = 3;
    saved.target_size = 8;
    memcpy(saved.data_hash, mock_incoming_file_id, TOX_HASH_LENGTH);
    snprintf((char *)saved.path, sizeof saved.path, "%s", "./tox/resume.bin");

    char info[UTOX_FILE_NAME_LENGTH];
    incoming_ftinfo_name(info, sizeof info);
    if (!write_resume_blob(info, &saved)) {
        FAIL("write .ftinfo");
    }

    const uint8_t name[] = "resume.bin";
    mock_cb_file_recv(TOX_DUMMY, 0, incoming_fileno(0), TOX_FILE_KIND_DATA, 8, name, sizeof name - 1, NULL);

    FILE_TRANSFER *ft = &test_friend.ft_incoming[0];
    if (!ft->in_use || !ft->via.file || ft->current_size != 3) {
        FAIL("resume restored offset %zu file %p", ft->current_size, (void *)ft->via.file);
    }
    if (mock_last_utox_msg != FILE_SEND_NEW && mock_last_utox_msg != FILE_STATUS_UPDATE) {
        FAIL("resumed incoming should notify UI, got %u", mock_last_utox_msg);
    }

    const uint8_t rest[] = { 'l', 'o', '!', '!', '!' };
    mock_cb_file_recv_chunk(TOX_DUMMY, 0, incoming_fileno(0), 3, rest, sizeof rest, NULL);
    mock_cb_file_recv_chunk(TOX_DUMMY, 0, incoming_fileno(0), 8, NULL, 0, NULL);
    if (test_friend.ft_incoming[0].in_use) {
        FAIL("resumed incoming complete should decon");
    }

    FILE *out = fopen("./tox/resume.bin", "rb");
    if (!out) {
        FAIL("resume payload missing");
    }
    char buf[16] = { 0 };
    size_t got = fread(buf, 1, sizeof buf, out);
    fclose(out);
    remove("./tox/resume.bin");
    utox_get_file(info, NULL, UTOX_FILE_OPTS_DELETE);
    if (got < 8 || memcmp(buf, "hello!!!", 8) != 0) {
        FAIL("resumed contents");
    }
    return true;
}

bool test_incoming_resume_size_mismatch(void) {
    reset_ft();
    char info[UTOX_FILE_NAME_LENGTH];
    incoming_ftinfo_name(info, sizeof info);
    FILE *fp = utox_get_file(info, NULL, UTOX_FILE_OPTS_WRITE | UTOX_FILE_OPTS_MKDIR);
    if (!fp) {
        FAIL("write short .ftinfo");
    }
    fwrite("xx", 1, 2, fp);
    fclose(fp);

    const uint8_t name[] = "x.bin";
    mock_cb_file_recv(TOX_DUMMY, 0, incoming_fileno(0), TOX_FILE_KIND_DATA, 4, name, sizeof name - 1, NULL);
    if (mock_last_utox_msg != FILE_INCOMING_NEW) {
        FAIL("bad .ftinfo falls back to new incoming");
    }
    cancel_open();
    utox_get_file(info, NULL, UTOX_FILE_OPTS_DELETE);
    return true;
}

bool test_start_write_unwritable(void) {
    reset_ft();
    const uint8_t name[] = "a.bin";
    mock_cb_file_recv(TOX_DUMMY, 0, incoming_fileno(0), TOX_FILE_KIND_DATA, 4, name, sizeof name - 1, NULL);

    if (utox_file_start_write(0, incoming_fileno(0), NULL)) {
        FAIL("NULL path must fail");
    }
    if (!utox_file_start_write(0, incoming_fileno(0), "./tox")) {
        /* directory is not a writable file; transfer is broken/decon'd */
        if (test_friend.ft_incoming[0].in_use) {
            FAIL("failed start_write must break the transfer");
        }
        return true;
    }
    FAIL("start_write to a directory must fail");
}

bool test_incoming_chunk_speed(void) {
    reset_ft();
    mock_time_set(1);
    const uint8_t name[] = "s.bin";
    mock_cb_file_recv(TOX_DUMMY, 0, incoming_fileno(0), TOX_FILE_KIND_DATA, 10, name, sizeof name - 1, NULL);
    if (!utox_file_start_write(0, incoming_fileno(0), "./tox/speed.bin")) {
        FAIL("start write speed");
    }
    ft_local_control(TOX_DUMMY, 0, incoming_fileno(0), TOX_FILE_CONTROL_RESUME);

    const uint8_t a[] = { 1, 2, 3, 4, 5 };
    const uint8_t b[] = { 6, 7, 8, 9, 10 };
    mock_cb_file_recv_chunk(TOX_DUMMY, 0, incoming_fileno(0), 0, a, sizeof a, NULL);
    mock_time_set(1ull + 1000ull * 1000ull * 100ull);
    mock_cb_file_recv_chunk(TOX_DUMMY, 0, incoming_fileno(0), 5, b, sizeof b, NULL);

    FILE_TRANSFER *ft = &test_friend.ft_incoming[0];
    if (ft->last_check_time != 1ull + 1000ull * 1000ull * 100ull) {
        FAIL("calculate_speed must run after elapsed ticks, t=%llu", (unsigned long long)ft->last_check_time);
    }
    mock_cb_file_recv_chunk(TOX_DUMMY, 0, incoming_fileno(0), 10, NULL, 0, NULL);
    remove("./tox/speed.bin");
    return true;
}

bool test_incoming_inline_not_png(void) {
    reset_ft();
    const uint8_t name[] = "utox-inline.png";
    uint8_t junk[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    mock_cb_file_recv(TOX_DUMMY, 0, incoming_fileno(0), TOX_FILE_KIND_DATA, sizeof junk, name, sizeof name - 1, NULL);
    mock_cb_file_recv_chunk(TOX_DUMMY, 0, incoming_fileno(0), 0, junk, sizeof junk, NULL);
    if (test_friend.ft_incoming && test_friend.ft_incoming[0].in_use) {
        FAIL("non-png inline must cancel");
    }
    if (mock_last_file_control != TOX_FILE_CONTROL_CANCEL) {
        FAIL("non-png inline cancels");
    }
    return true;
}

bool test_friend_online_outgoing_resume(void) {
    reset_ft();
    FILE *payload = make_payload("./tox/out.bin", "abcd");
    if (!payload) {
        FAIL("outgoing resume payload");
    }
    fclose(payload);

    FILE_TRANSFER saved;
    memset(&saved, 0, sizeof saved);
    saved.in_use = true;
    saved.resumeable = true;
    saved.incoming = false;
    snprintf((char *)saved.path, sizeof saved.path, "%s", "./tox/out.bin");

    char info[UTOX_FILE_NAME_LENGTH];
    snprintf(info, sizeof info, "%.*s%02i.ftoutfo", TOX_PUBLIC_KEY_SIZE * 2, test_friend.id_str, 0);
    if (!write_resume_blob(info, &saved)) {
        FAIL("write .ftoutfo");
    }

    ft_friend_online(TOX_DUMMY, 0);
    if (test_friend.ft_outgoing_active_count == 0 || !test_friend.ft_outgoing || !test_friend.ft_outgoing[0].in_use) {
        FAIL("online restores outgoing file");
    }
    cancel_open();
    remove("./tox/out.bin");
    utox_get_file(info, NULL, UTOX_FILE_OPTS_DELETE);
    return true;
}

int main(void) {
    int result = 0;
    setvbuf(stdout, NULL, _IONBF, 0);
    settings.portable_mode = true;
    native_create_dir((uint8_t *)"./tox");
    RUN_TEST(test_unknown_transfer_and_ui_data);
    RUN_TEST(test_send_data_slot_status_and_limit);
    RUN_TEST(test_send_file_basename);
    RUN_TEST(test_send_avatar_and_max);
    RUN_TEST(test_local_and_remote_status_machine);
    RUN_TEST(test_outgoing_chunks);
    RUN_TEST(test_incoming_file_write);
    RUN_TEST(test_incoming_unknown_friend_does_not_crash);
    RUN_TEST(test_incoming_avatar_null_and_too_large);
    RUN_TEST(test_incoming_oob_ui_data);
    RUN_TEST(test_sparse_outgoing_zeroed_and_offline);
    RUN_TEST(test_incoming_inline_png);
    RUN_TEST(test_friend_online_without_resume_files);
    RUN_TEST(test_incoming_resume_from_disk);
    RUN_TEST(test_incoming_resume_size_mismatch);
    RUN_TEST(test_start_write_unwritable);
    RUN_TEST(test_incoming_chunk_speed);
    RUN_TEST(test_incoming_inline_not_png);
    RUN_TEST(test_friend_online_outgoing_resume);
    cancel_open();
    free(self.png_data);
    self.png_data = NULL;
    return result;
}
