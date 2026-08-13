#include "test.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/groups.c"
#include "../src/text.c"

#include "mock/mock_domain.h"

pthread_mutex_t messages_lock = PTHREAD_MUTEX_INITIALIZER;

static uint32_t group_msg_count;
static uint8_t last_group_msg_type;

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

static void reset_groups(void) {
    mock_domain_reset();
    raze_groups();
    self.groups_list_size = 0;
    self.groups_list_count = 0;
    group_msg_count = 0;
    last_group_msg_type = 0;
    settings.group_notifications = GNOTIFY_ALWAYS;
    memcpy(self.name, "Me", 2);
    self.name_length = 2;
}

bool test_group_create_peers_messages(void) {
    reset_groups();
    if (get_group(0)) {
        FAIL("empty group list");
    }

    GROUPCHAT *g = group_create(0, false, "Room");
    if (!g || get_group(0) != g) {
        FAIL("create group");
    }
    if (g->name_length != 4 || memcmp(g->name, "Room", 4) != 0) {
        FAIL("group name");
    }
    if (!g->msg.is_groupchat || g->number != 0) {
        FAIL("group msg panel");
    }

    GROUPCHAT *g1 = group_create(1, true, NULL);
    if (!g1 || strncmp(g1->name, "Groupchat #1", 12) != 0 || !g1->av_group) {
        FAIL("default av group name");
    }
    /* group_create may realloc the contiguous group array; refresh like flist does. */
    g = get_group(0);
    if (!g) {
        FAIL("group 0 after grow");
    }

    group_peer_add(g, 0, true, 0x112233);
    group_peer_add(g, 1, false, 0x445566);
    if (g->peer_count != 2 || !g->peer[0] || !g->peer[1]) {
        FAIL("peer add");
    }

    g->our_peer_number = 0;
    uint32_t n = group_add_message(g, 0, (const uint8_t *)"hi", 2, MSG_TYPE_TEXT);
    if (n == UINT32_MAX || group_msg_count == 0) {
        FAIL("add message");
    }

    if (group_add_message(g, 99, (const uint8_t *)"x", 1, MSG_TYPE_TEXT) != UINT32_MAX) {
        FAIL("missing peer");
    }
    if (group_add_message(g, 5, (const uint8_t *)"x", 1, MSG_TYPE_TEXT) != UINT32_MAX) {
        FAIL("missing peer slot");
    }
    if (group_add_message(g, UTOX_MAX_GROUP_PEERS, (const uint8_t *)"x", 1, MSG_TYPE_TEXT) != UINT32_MAX) {
        FAIL("peer id at cap");
    }
    if (group_add_message(g, UTOX_MAX_GROUP_PEERS + 1, (const uint8_t *)"x", 1, MSG_TYPE_TEXT)
        != UINT32_MAX) {
        FAIL("peer id above cap");
    }

    g->our_peer_number = 0;
    uint32_t before_act = group_msg_count;
    if (group_add_message(g, 0, (const uint8_t *)"dances", 6, MSG_TYPE_ACTION_TEXT) == UINT32_MAX
        || group_msg_count <= before_act || last_group_msg_type != MSG_TYPE_ACTION_TEXT) {
        FAIL("action message from our peer");
    }

    group_peer_name_change(g, 1, (const uint8_t *)"Ann", 3);
    if (!g->peer[1] || g->peer[1]->name_length != 3) {
        FAIL("first name is join");
    }
    if (last_group_msg_type != MSG_TYPE_NOTICE) {
        FAIL("join notice");
    }

    group_peer_name_change(g, 1, (const uint8_t *)"Anne", 4);
    if (g->peer[1]->name_length != 4 || memcmp(g->peer[1]->name, "Anne", 4) != 0) {
        FAIL("rename");
    }

    uint32_t before = group_msg_count;
    group_peer_del(g, 1);
    if (group_msg_count <= before || g->peer[1] || g->peer_count != 1) {
        FAIL("peer del");
    }
    group_peer_del(g, 1);

    group_notify_msg(g, "hello", 5);
    g->notify = GNOTIFY_NEVER;
    group_notify_msg(g, "hello", 5);
    g->notify = GNOTIFY_HIGHLIGHTS;
    group_notify_msg(g, "no mention", 10);
    group_notify_msg(g, "hey Me there", 12);
    mock_sel_group = g;
    g->notify = GNOTIFY_ALWAYS;
    group_notify_msg(g, "sel", 3);

    group_peer_add(g1, 0, true, 0x1);
    if (!g1->peer[0]) {
        FAIL("av group peer");
    }

    init_groups((Tox *)(uintptr_t)1);

    raze_groups();
    return true;
}

bool test_group_reset_and_raze(void) {
    reset_groups();
    GROUPCHAT *g = group_create(0, false, "G");
    group_peer_add(g, 0, true, 1);
    group_peer_add(g, 1, false, 2);
    group_reset_peerlist(g);
    if (g->peer || g->peer_count != 0) {
        FAIL("reset should clear peer list");
    }
    group_peer_add(g, 0, true, 3);
    group_peer_add(g, 2, false, 4);
    if (g->peer_count != 2 || !g->peer[0] || g->peer[1] || !g->peer[2]) {
        FAIL("peer hole");
    }
    group_reset_peerlist(g);
    if (g->peer || g->peer_count != 0) {
        FAIL("reset should free sparse peer ids");
    }
    group_peer_add(g, 0, true, 3);
    if (!g->peer || !g->peer[0]) {
        FAIL("peer add after reset");
    }

    group_peer_name_change(g, 0, (const uint8_t *)"A", 1);
    g->edit_history_length = 1;
    g->edit_history = calloc(1, sizeof(void *));
    g->edit_history[0] = calloc(1, sizeof(EDIT_CHANGE));
    g->msg.number = 1;
    g->msg.data = calloc(1, sizeof(MSG_HEADER *));
    g->msg.data[0] = calloc(1, sizeof(MSG_HEADER));
    g->msg.data[0]->via.grp.author = calloc(1, 2);
    g->msg.data[0]->via.grp.msg = calloc(1, 2);

    GROUPCHAT empty = { 0 };
    group_peer_name_change(&empty, 0, (const uint8_t *)"x", 1);

    mock_tox_conference_count = 1;
    memcpy(mock_tox_conference_title, "Title", 6);
    raze_groups();
    init_groups((Tox *)(uintptr_t)1);
    GROUPCHAT *loaded = get_group(0);
    if (!loaded || loaded->name_length != 5 || memcmp(loaded->name, "Title", 5) != 0) {
        FAIL("init_groups should load conference title");
    }

    mock_tox_conference_title[0] = 0;
    raze_groups();
    init_groups((Tox *)(uintptr_t)1);
    loaded = get_group(0);
    if (!loaded || strncmp(loaded->name, "Groupchat #0", 12) != 0) {
        FAIL("init_groups empty title");
    }

    raze_groups();
    GROUPCHAT *sparse = group_create(5, false, "Sparse");
    if (!sparse || get_group(5) != sparse) {
        FAIL("create group 5");
    }
    if (self.groups_list_size < 6) {
        FAIL("group list size should cover the highest index");
    }

    raze_groups();
    group_create(0, false, "A");
    group_create(1, false, "B");
    raze_groups();
    if (self.groups_list_count != 0 || self.groups_list_size != 0) {
        FAIL("raze should clear count and size");
    }
    if (get_group(0) || get_group(1) || get_group(5)) {
        FAIL("groups should be gone after raze");
    }
    return true;
}

bool test_group_peer_del_null_list(void) {
    reset_groups();
    GROUPCHAT *g = group_create(0, false, "G");
    group_reset_peerlist(g);
    if (g->peer) {
        FAIL("peer list should be NULL after reset");
    }
    /* peer_id >= UTOX_MAX_GROUP_PEERS makes group_add_message return before
     * dereferencing g->peer, so the NULL-list branch in group_peer_del is reachable. */
    group_peer_del(g, UTOX_MAX_GROUP_PEERS);
    if (g->peer) {
        FAIL("NULL peer list should stay NULL");
    }
    raze_groups();
    return true;
}

bool test_group_reinit_does_not_double_count(void) {
    reset_groups();
    GROUPCHAT *g = group_create(0, false, "Room");
    if (!g) {
        FAIL("create");
    }
    uint32_t counted = self.groups_list_count;
    group_init(g, 0, false, "Room");
    if (self.groups_list_count != counted) {
        FAIL("re-init bumped groups_list_count");
    }
    raze_groups();
    return true;
}

bool test_group_title_size_error(void) {
    reset_groups();
    mock_tox_conference_count     = 1;
    mock_tox_conference_title[0]  = 'T';
    mock_tox_conference_title[1]  = 0;
    mock_tox_conference_title_err = 1; /* any non-OK */

    init_groups((Tox *)(uintptr_t)1);
    GROUPCHAT *loaded = get_group(0);
    if (!loaded || strncmp(loaded->name, "Groupchat #0", 12) != 0) {
        FAIL("title size error should fall back to default name");
    }

    mock_tox_conference_title_err = 0;
    raze_groups();
    return true;
}

int main(void) {
    int result = 0;
    RUN_TEST(test_group_create_peers_messages);
    RUN_TEST(test_group_reset_and_raze);
    RUN_TEST(test_group_peer_del_null_list);
    RUN_TEST(test_group_reinit_does_not_double_count);
    RUN_TEST(test_group_title_size_error);
    raze_groups();
    return result;
}
