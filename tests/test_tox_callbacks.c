#include "test.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/chatlog.c"
#include "../src/friend.c"
#include "../src/groups.c"
#include "../src/text.c"
#include "../src/tox_callbacks.c"

#include "mock/mock_domain.h"

#define TOX_DUMMY ((Tox *)(uintptr_t)1)

pthread_mutex_t messages_lock = PTHREAD_MUTEX_INITIALIZER;

static uint32_t text_count;
static uint32_t action_count;
static uint32_t notice_count;
static uint32_t image_count;
static uint32_t init_count;
static uint32_t clear_all_count;
static uint32_t read_log_count;
static uint32_t group_msg_count;
static uint8_t last_group_msg_type;
static uint32_t last_receipt;
static uint32_t ft_online_count;
static uint32_t ft_offline_count;
static uint32_t avatar_online_count;
static uint32_t av_disconnect_count;
static uint32_t last_ft_fid;
static char last_text[128];
static uint16_t last_text_len;

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

uint32_t message_add_type_text(MESSAGES *m, bool auth, const char *msgtxt, uint16_t length, bool log, bool send) {
    (void)m;
    (void)auth;
    (void)log;
    (void)send;
    text_count++;
    last_text_len = length;
    last_text[0]  = 0;
    if (msgtxt && length) {
        size_t n = length < sizeof last_text ? length : sizeof last_text - 1;
        memcpy(last_text, msgtxt, n);
        last_text[n] = 0;
    }
    return text_count;
}

uint32_t message_add_type_action(MESSAGES *m, bool auth, const char *msgtxt, uint16_t length, bool log, bool send) {
    (void)m;
    (void)auth;
    (void)msgtxt;
    (void)length;
    (void)log;
    (void)send;
    action_count++;
    return action_count;
}

void messages_clear_receipt(MESSAGES *m, uint32_t receipt_number) {
    (void)m;
    last_receipt = receipt_number;
}

uint32_t message_add_group(MESSAGES *m, MSG_HEADER *msg) {
    (void)m;
    if (!msg) {
        return UINT32_MAX;
    }
    last_group_msg_type = msg->msg_type;
    group_msg_count++;
    free(msg->via.grp.author);
    free(msg->via.grp.msg);
    free(msg);
    return group_msg_count;
}

void message_free(MSG_HEADER *msg) {
    if (!msg) {
        return;
    }
    free(msg->via.grp.author);
    free(msg->via.grp.msg);
    free(msg);
}

void ft_friend_online(Tox *tox, uint32_t friend_number) {
    (void)tox;
    ft_online_count++;
    last_ft_fid = friend_number;
}

void ft_friend_offline(Tox *tox, uint32_t friend_number) {
    (void)tox;
    ft_offline_count++;
    last_ft_fid = friend_number;
}

bool avatar_on_friend_online(Tox *tox, uint32_t friend_number) {
    (void)tox;
    avatar_online_count++;
    last_ft_fid = friend_number;
    return true;
}

void utox_av_local_disconnect(ToxAV *av, int32_t friend_number) {
    (void)av;
    av_disconnect_count++;
    last_ft_fid = (uint32_t)friend_number;
}

/* UI thread owns FRIEND_NAME / FRIEND_STATUS_MESSAGE / GROUP_TOPIC payloads. */
static void clear_utox_posted(void) {
    if (mock_last_utox_msg == FRIEND_NAME || mock_last_utox_msg == FRIEND_STATUS_MESSAGE
        || mock_last_utox_msg == GROUP_TOPIC) {
        free(mock_last_utox_data);
    }
    mock_last_utox_data = NULL;
    mock_last_utox_msg  = 0;
    mock_last_utox_p1   = 0;
    mock_last_utox_p2   = 0;
}

static void reset_cb(void) {
    mock_domain_reset();
    free_friends();
    raze_groups();
    text_count = 0;
    action_count = 0;
    notice_count = 0;
    image_count = 0;
    init_count = 0;
    clear_all_count = 0;
    read_log_count = 0;
    group_msg_count = 0;
    last_group_msg_type = 0;
    last_receipt = 0;
    ft_online_count = 0;
    ft_offline_count = 0;
    avatar_online_count = 0;
    av_disconnect_count = 0;
    last_ft_fid = 0;
    last_text[0] = 0;
    last_text_len = 0;
    settings.portable_mode = true;
    settings.block_friend_requests = false;
    settings.status_notifications = true;
    settings.group_notifications = GNOTIFY_ALWAYS;
    self.name_length = 2;
    memcpy(self.name, "Me", 2);
    utox_set_callbacks_friends(TOX_DUMMY);
    utox_set_callbacks_groups(TOX_DUMMY);
}

static FRIEND *make_friend(uint32_t n) {
    utox_friend_init(TOX_DUMMY, n);
    return get_friend(n);
}

bool test_callbacks_are_registered(void) {
    reset_cb();
    if (!mock_cb_friend_request || !mock_cb_friend_message || !mock_cb_friend_name
        || !mock_cb_friend_status_message || !mock_cb_friend_status || !mock_cb_friend_typing
        || !mock_cb_friend_read_receipt || !mock_cb_friend_connection_status) {
        FAIL("friend callbacks");
    }
    if (!mock_cb_conference_invite || !mock_cb_conference_message || !mock_cb_conference_peer_name
        || !mock_cb_conference_title || !mock_cb_conference_peer_list_changed
        || !mock_cb_conference_connected) {
        FAIL("group callbacks");
    }
    return true;
}

bool test_friend_request_callback(void) {
    reset_cb();
    uint8_t pk[TOX_PUBLIC_KEY_SIZE];
    memset(pk, 0xAB, sizeof pk);

    mock_cb_friend_request(TOX_DUMMY, pk, (const uint8_t *)"hi", 2, NULL);
    if (mock_last_utox_msg != FRIEND_INCOMING_REQUEST) {
        FAIL("incoming request posted");
    }
    FREQUEST *r = get_frequest(mock_last_utox_p1);
    if (!r || r->length != 2 || memcmp(r->msg, "hi", 2) != 0) {
        FAIL("stored request message");
    }
    if (memcmp(r->bin_id, pk, TOX_PUBLIC_KEY_SIZE) != 0) {
        FAIL("stored public key");
    }
    if (mock_last_audio_msg != UTOXAUDIO_PLAY_NOTIFICATION
        || mock_last_audio_param1 != NOTIFY_TONE_FRIEND_REQUEST) {
        FAIL("request tone");
    }

    settings.block_friend_requests = true;
    clear_utox_posted();
    mock_cb_friend_request(TOX_DUMMY, pk, (const uint8_t *)"no", 2, NULL);
    if (mock_last_utox_msg != 0) {
        FAIL("blocked request should not post");
    }

    settings.block_friend_requests = false;
    clear_utox_posted();
    mock_cb_friend_request(TOX_DUMMY, NULL, (const uint8_t *)"x", 1, NULL);
    if (mock_last_utox_msg != 0 || get_frequest(1)) {
        FAIL("NULL id should not store a request");
    }

    mock_cb_friend_request(TOX_DUMMY, pk, NULL, 8, NULL);
    r = get_frequest(mock_last_utox_p1);
    if (mock_last_utox_msg != FRIEND_INCOMING_REQUEST || !r || r->length != 0) {
        FAIL("NULL message treated as empty");
    }
    return true;
}

bool test_friend_message_and_meta(void) {
    reset_cb();
    FRIEND *f = make_friend(0);
    if (!f) {
        FAIL("make friend");
    }

    mock_cb_friend_message(TOX_DUMMY, 0, TOX_MESSAGE_TYPE_NORMAL, (const uint8_t *)"hello", 5, NULL);
    if (text_count != 1 || last_text_len != 5 || memcmp(last_text, "hello", 5) != 0) {
        FAIL("normal message");
    }
    if (mock_last_utox_msg != FRIEND_MESSAGE) {
        FAIL("notify posts FRIEND_MESSAGE");
    }

    mock_cb_friend_message(TOX_DUMMY, 0, TOX_MESSAGE_TYPE_ACTION, (const uint8_t *)"waves", 5, NULL);
    if (action_count != 1) {
        FAIL("action message");
    }

    uint32_t texts = text_count;
    mock_cb_friend_message(TOX_DUMMY, 0, (TOX_MESSAGE_TYPE)99, (const uint8_t *)"x", 1, NULL);
    if (text_count != texts) {
        FAIL("unsupported type should not add text");
    }

    mock_cb_friend_message(TOX_DUMMY, 99, TOX_MESSAGE_TYPE_NORMAL, (const uint8_t *)"nope", 4, NULL);
    if (text_count != texts) {
        FAIL("missing friend");
    }

    mock_cb_friend_message(TOX_DUMMY, 0, TOX_MESSAGE_TYPE_NORMAL, NULL, 4, NULL);
    if (text_count != texts + 1 || last_text_len != 0) {
        FAIL("NULL message body");
    }

    mock_cb_friend_name(TOX_DUMMY, 0, (const uint8_t *)"Bob", 3, NULL);
    if (mock_last_utox_msg != FRIEND_NAME || mock_last_utox_p1 != 0 || mock_last_utox_p2 != 3) {
        FAIL("name change");
    }
    if (!mock_last_utox_data || memcmp(mock_last_utox_data, "Bob", 3) != 0) {
        FAIL("name payload");
    }

    mock_cb_friend_name(TOX_DUMMY, 0, NULL, 9, NULL);
    if (mock_last_utox_msg != FRIEND_NAME || mock_last_utox_p2 != 0 || mock_last_utox_data) {
        FAIL("NULL name is empty");
    }

    mock_cb_friend_status_message(TOX_DUMMY, 0, (const uint8_t *)"brb", 3, NULL);
    if (mock_last_utox_msg != FRIEND_STATUS_MESSAGE || mock_last_utox_p2 != 3
        || !mock_last_utox_data || memcmp(mock_last_utox_data, "brb", 3) != 0) {
        FAIL("status message");
    }

    mock_cb_friend_status(TOX_DUMMY, 0, TOX_USER_STATUS_BUSY, NULL);
    if (mock_last_utox_msg != FRIEND_STATE || mock_last_utox_p2 != TOX_USER_STATUS_BUSY) {
        FAIL("user status");
    }

    mock_cb_friend_typing(TOX_DUMMY, 0, true, NULL);
    if (mock_last_utox_msg != FRIEND_TYPING || mock_last_utox_p2 != 1) {
        FAIL("typing");
    }
    return true;
}

bool test_friend_receipt_and_connection(void) {
    reset_cb();
    FRIEND *f = make_friend(0);
    if (!f) {
        FAIL("make friend");
    }

    mock_cb_friend_read_receipt(TOX_DUMMY, 0, 42, NULL);
    if (last_receipt != 42) {
        FAIL("receipt");
    }
    mock_cb_friend_read_receipt(TOX_DUMMY, 9, 1, NULL);
    if (last_receipt != 42) {
        FAIL("missing friend receipt");
    }

    f->online = false;
    mock_cb_friend_connection_status(TOX_DUMMY, 0, TOX_CONNECTION_UDP, NULL);
    if (ft_online_count != 1 || avatar_online_count != 1 || last_ft_fid != 0) {
        FAIL("coming online");
    }
    if (mock_last_utox_msg != FRIEND_ONLINE || mock_last_utox_p2 != 1) {
        FAIL("FRIEND_ONLINE true");
    }

    f->online          = true;
    f->call_state_self = 1;
    mock_cb_friend_connection_status(TOX_DUMMY, 0, TOX_CONNECTION_NONE, NULL);
    if (ft_offline_count != 1 || av_disconnect_count != 1) {
        FAIL("going offline hangs up FT and AV");
    }
    if (mock_last_utox_msg != FRIEND_ONLINE || mock_last_utox_p2 != 0) {
        FAIL("FRIEND_ONLINE false");
    }

    uint32_t on = ft_online_count;
    mock_cb_friend_connection_status(TOX_DUMMY, 99, TOX_CONNECTION_UDP, NULL);
    if (ft_online_count != on) {
        FAIL("missing friend connection");
    }
    return true;
}

bool test_group_invite_and_topic(void) {
    reset_cb();
    uint8_t cookie[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };

    mock_tox_conference_join_fail = true;
    clear_utox_posted();
    mock_cb_conference_invite(TOX_DUMMY, 0, TOX_CONFERENCE_TYPE_TEXT, cookie, sizeof cookie, NULL);
    if (mock_last_utox_msg != 0 || get_group(0)) {
        FAIL("failed join should not add a group");
    }

    mock_tox_conference_join_fail   = false;
    mock_tox_conference_join_result = 0;
    mock_cb_conference_invite(TOX_DUMMY, 0, TOX_CONFERENCE_TYPE_TEXT, cookie, sizeof cookie, NULL);
    GROUPCHAT *g = get_group(0);
    if (!g || mock_last_utox_msg != GROUP_ADD || mock_last_utox_p1 != 0) {
        FAIL("text invite creates group");
    }
    if (g->av_group) {
        FAIL("text group is not AV");
    }
    uint32_t counted = self.groups_list_count;
    mock_cb_conference_invite(TOX_DUMMY, 0, TOX_CONFERENCE_TYPE_TEXT, cookie, sizeof cookie, NULL);
    if (self.groups_list_count != counted) {
        FAIL("re-invite must not double-count");
    }

    mock_toxav_join_result = 1;
    mock_cb_conference_invite(TOX_DUMMY, 0, TOX_CONFERENCE_TYPE_AV, cookie, sizeof cookie, NULL);
    GROUPCHAT *av = get_group(1);
    if (!av || !av->av_group || mock_last_utox_msg != GROUP_ADD || mock_last_utox_p1 != 1) {
        FAIL("AV invite");
    }

    mock_toxav_join_result = -1;
    clear_utox_posted();
    mock_cb_conference_invite(TOX_DUMMY, 0, TOX_CONFERENCE_TYPE_AV, cookie, sizeof cookie, NULL);
    if (mock_last_utox_msg != 0) {
        FAIL("failed AV join");
    }

    clear_utox_posted();
    mock_cb_conference_invite(TOX_DUMMY, 0, (TOX_CONFERENCE_TYPE)99, cookie, sizeof cookie, NULL);
    if (mock_last_utox_msg != 0) {
        FAIL("unknown conference type");
    }

    mock_cb_conference_title(TOX_DUMMY, 0, 0, (const uint8_t *)"Topic", 5, NULL);
    if (mock_last_utox_msg != GROUP_TOPIC || mock_last_utox_p1 != 0 || mock_last_utox_p2 != 5) {
        FAIL("topic");
    }
    if (!mock_last_utox_data || memcmp(mock_last_utox_data, "Topic", 5) != 0) {
        FAIL("topic payload");
    }

    clear_utox_posted();
    mock_cb_conference_title(TOX_DUMMY, 0, 0, NULL, 4, NULL);
    if (mock_last_utox_msg != 0) {
        FAIL("NULL topic");
    }
    mock_cb_conference_title(TOX_DUMMY, 0, 0, (const uint8_t *)"", 0, NULL);
    if (mock_last_utox_msg != 0) {
        FAIL("empty topic");
    }
    return true;
}

bool test_group_message_peers_connected(void) {
    reset_cb();
    GROUPCHAT *g = group_create(0, false, "Room");
    if (!g) {
        FAIL("create group");
    }
    group_peer_add(g, 0, true, 0x112233);

    mock_cb_conference_message(TOX_DUMMY, 0, 0, TOX_MESSAGE_TYPE_NORMAL, (const uint8_t *)"hi", 2, NULL);
    if (group_msg_count == 0 || last_group_msg_type != MSG_TYPE_TEXT) {
        FAIL("group text");
    }
    if (mock_last_utox_msg != GROUP_MESSAGE) {
        FAIL("GROUP_MESSAGE");
    }

    mock_cb_conference_message(TOX_DUMMY, 0, 0, TOX_MESSAGE_TYPE_ACTION, (const uint8_t *)"dances", 6, NULL);
    if (last_group_msg_type != MSG_TYPE_ACTION_TEXT) {
        FAIL("group action");
    }

    uint32_t before = group_msg_count;
    clear_utox_posted();
    mock_cb_conference_message(TOX_DUMMY, 9, 0, TOX_MESSAGE_TYPE_NORMAL, (const uint8_t *)"x", 1, NULL);
    if (group_msg_count != before || mock_last_utox_msg != 0) {
        FAIL("missing group message must not post");
    }

    mock_cb_conference_peer_name(TOX_DUMMY, 0, 0, (const uint8_t *)"Ann", 3, NULL);
    if (!g->peer[0] || g->peer[0]->name_length != 3 || memcmp(g->peer[0]->name, "Ann", 3) != 0) {
        FAIL("peer name");
    }
    if (mock_last_utox_msg != GROUP_PEER_NAME) {
        FAIL("GROUP_PEER_NAME");
    }

    clear_utox_posted();
    mock_cb_conference_peer_name(TOX_DUMMY, 0, 5, (const uint8_t *)"X", 1, NULL);
    if (mock_last_utox_msg != 0) {
        FAIL("missing peer name");
    }
    group_reset_peerlist(g);
    mock_cb_conference_peer_name(TOX_DUMMY, 0, 0, (const uint8_t *)"X", 1, NULL);
    if (mock_last_utox_msg != 0) {
        FAIL("NULL peer list name");
    }

    mock_tox_conference_peer_count_n = 2;
    memset(mock_tox_conference_peer_name, 'A', TOX_MAX_NAME_LENGTH);
    mock_tox_conference_peer_name_len         = TOX_MAX_NAME_LENGTH;
    mock_tox_conference_peer_name_size_report = TOX_MAX_NAME_LENGTH + 40;
    mock_cb_conference_peer_list_changed(TOX_DUMMY, 0, NULL);
    g = get_group(0);
    if (!g || g->peer_count != 2 || !g->peer[0] || !g->peer[1] || g->peer[2]) {
        FAIL("peer list rebuild");
    }
    if (g->peer[0]->name_length != TOX_MAX_NAME_LENGTH || g->peer[0]->name[0] != 'A') {
        FAIL("oversized peer name capped");
    }
    if (mock_last_utox_msg != GROUP_PEER_CHANGE) {
        FAIL("GROUP_PEER_CHANGE");
    }
    /* Second rebuild used to calloc(number_peers) and overflow group_reset_peerlist. */
    mock_cb_conference_peer_list_changed(TOX_DUMMY, 0, NULL);
    if (get_group(0)->peer_count != 2) {
        FAIL("second peer list rebuild");
    }

    clear_utox_posted();
    mock_cb_conference_peer_list_changed(TOX_DUMMY, 9, NULL);
    if (mock_last_utox_msg != 0) {
        FAIL("missing group peer list");
    }

    mock_cb_conference_connected(TOX_DUMMY, 0, NULL);
    if (!get_group(0)->connected) {
        FAIL("connected");
    }
    mock_cb_conference_connected(TOX_DUMMY, 9, NULL);
    return true;
}

int main(void) {
    int result = 0;
    setvbuf(stdout, NULL, _IONBF, 0);
    settings.portable_mode = true;
    RUN_TEST(test_callbacks_are_registered);
    RUN_TEST(test_friend_request_callback);
    RUN_TEST(test_friend_message_and_meta);
    RUN_TEST(test_friend_receipt_and_connection);
    RUN_TEST(test_group_invite_and_topic);
    RUN_TEST(test_group_message_peers_connected);
    free_friends();
    raze_groups();
    return result;
}
