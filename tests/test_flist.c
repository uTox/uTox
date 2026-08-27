#include "test.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/flist.c"
#include "../src/text.c"

#include "../src/friend.h"
#include "../src/groups.h"
#include "../src/layout/friend.h"
#include "../src/tox.h"
#include "mock/mock_domain.h"

uint8_t addfriend_status;

#define SLOT 8

static FRIEND friends[SLOT];
static bool friend_live[SLOT];
static GROUPCHAT groups[SLOT];
static bool group_live[SLOT];
static FREQUEST freq[SLOT];
static bool freq_live[SLOT];
static uint32_t text_added;
static bool list_live;

FRIEND *get_friend(uint32_t friend_number) {
    if (friend_number >= SLOT || !friend_live[friend_number]) {
        return NULL;
    }
    return &friends[friend_number];
}

GROUPCHAT *get_group(uint32_t group_number) {
    if (group_number >= SLOT || !group_live[group_number]) {
        return NULL;
    }
    return &groups[group_number];
}

FREQUEST *get_frequest(uint16_t frequest_number) {
    if (frequest_number >= SLOT || !freq_live[frequest_number]) {
        return NULL;
    }
    return &freq[frequest_number];
}

void friend_free(FRIEND *f) {
    if (!f) {
        return;
    }
    free(f->name);
    f->name = NULL;
    free(f->alias);
    f->alias = NULL;
    free(f->typed);
    f->typed = NULL;
    f->typed_length = 0;
    if (f->number < SLOT) {
        friend_live[f->number] = false;
    }
}

void group_free(GROUPCHAT *g) {
    if (!g) {
        return;
    }
    if (g->number < SLOT) {
        group_live[g->number] = false;
    }
}

void friend_request_free(uint16_t number) {
    if (number >= SLOT) {
        return;
    }
    free(freq[number].msg);
    freq[number].msg = NULL;
    freq_live[number] = false;
}

FRIEND *get_friend_by_id(const char *id_str) {
    if (!id_str) {
        return NULL;
    }
    for (uint32_t i = 0; i < SLOT; i++) {
        if (friend_live[i] && strncmp(friends[i].id_str, id_str, TOX_PUBLIC_KEY_SIZE * 2) == 0) {
            return &friends[i];
        }
    }
    return NULL;
}

uint32_t message_add_type_text(MESSAGES *m, bool auth, const char *msgtxt, uint16_t length, bool log, bool send) {
    (void)m;
    (void)auth;
    (void)msgtxt;
    (void)length;
    (void)log;
    (void)send;
    text_added++;
    return text_added;
}

void friend_history_clear(FRIEND *f) {
    (void)f;
}

void messages_updateheight(MESSAGES *m, int width) {
    if (m) {
        m->width = width;
    }
}

static char *dupstr(const char *s) {
    size_t n = strlen(s);
    char *d = malloc(n + 1);
    if (!d) {
        return NULL;
    }
    memcpy(d, s, n + 1);
    return d;
}

static FRIEND *live_friend(uint32_t n, const char *name, bool online) {
    if (n >= SLOT) {
        return NULL;
    }
    memset(&friends[n], 0, sizeof friends[n]);
    friends[n].number = (uint8_t)n;
    friends[n].name = dupstr(name);
    friends[n].name_length = strlen(name);
    friends[n].online = online;
    memset(friends[n].id_bin, 0, TOX_PUBLIC_KEY_SIZE);
    friends[n].id_bin[0] = (uint8_t)(n + 1);
    memset(friends[n].id_str, 'A' + (char)n, TOX_PUBLIC_KEY_SIZE * 2);
    friend_live[n] = true;
    if (self.friend_list_size < n + 1) {
        self.friend_list_size = n + 1;
    }
    self.friend_list_count++;
    return &friends[n];
}

static GROUPCHAT *live_group(uint32_t n, const char *name) {
    if (n >= SLOT) {
        return NULL;
    }
    memset(&groups[n], 0, sizeof groups[n]);
    groups[n].number = (uint16_t)n;
    snprintf(groups[n].name, sizeof groups[n].name, "%s", name);
    groups[n].name_length = (uint16_t)strlen(name);
    group_live[n] = true;
    if (self.groups_list_size < n + 1) {
        self.groups_list_size = n + 1;
    }
    self.groups_list_count++;
    return &groups[n];
}

static FREQUEST *live_req(uint16_t n, const char *msg) {
    if (n >= SLOT) {
        return NULL;
    }
    memset(&freq[n], 0, sizeof freq[n]);
    freq[n].number = n;
    freq[n].msg = dupstr(msg);
    freq[n].length = strlen(msg);
    freq_live[n] = true;
    return &freq[n];
}

static void teardown_list(void) {
    if (list_live) {
        flist_freeall();
        list_live = false;
    }
    for (uint32_t i = 0; i < SLOT; i++) {
        free(friends[i].name);
        friends[i].name = NULL;
        free(friends[i].alias);
        friends[i].alias = NULL;
        free(friends[i].typed);
        friends[i].typed = NULL;
        friends[i].typed_length = 0;
        friend_live[i] = false;
        group_live[i] = false;
        if (freq_live[i]) {
            free(freq[i].msg);
            freq[i].msg = NULL;
            freq_live[i] = false;
        }
    }
    memset(&self, 0, sizeof self);
    text_added = 0;
}

static void reset_flist(void) {
    mock_domain_reset();
    teardown_list();
    settings.use_mini_flist = false;
    settings.inline_video = true;
    panel_profile_password.disabled = true;
}

static void start_list(void) {
    if (list_live) {
        flist_freeall();
        list_live = false;
    }
    flist_start();
    list_live = true;
}

/* showncount is static; probe by selecting until the selection stops moving.
 * Opening a friend clears unread_msg, so this is not valid while the online
 * filter is hiding unread-only rows. */
static uint32_t shown_len(void) {
    flist_selectchat(0);
    ITEM_TYPE t0 = flist_get_sel_item_type();
    FRIEND *f0 = flist_get_sel_friend();
    GROUPCHAT *g0 = flist_get_sel_group();
    uint32_t n = 1;
    for (; n < 32; n++) {
        flist_selectchat(n);
        if (flist_get_sel_item_type() == t0 && flist_get_sel_friend() == f0 && flist_get_sel_group() == g0) {
            return n;
        }
        t0 = flist_get_sel_item_type();
        f0 = flist_get_sel_friend();
        g0 = flist_get_sel_group();
    }
    return n;
}

bool test_start_empty_and_create_item(void) {
    reset_flist();
    start_list();
    if (flist_get_filter() != 0) {
        FAIL("default filter off");
    }
    if (shown_len() != 1) {
        FAIL("empty list is just group-create, got %u", shown_len());
    }
    flist_selectchat(0);
    if (flist_get_sel_item_type() != ITEM_GROUP_CREATE) {
        FAIL("only create-group row");
    }
    if (flist_get_sel_friend() || flist_get_sel_group() || flist_get_sel_frequest()) {
        FAIL("no sel friend/group/request");
    }
    return true;
}

bool test_add_select_search_filter(void) {
    reset_flist();
    start_list();
    FRIEND *alice = live_friend(0, "Alice", true);
    FRIEND *bob = live_friend(1, "Bob", false);
    bob->alias = dupstr("Bobby");
    bob->alias_length = 5;
    GROUPCHAT *room = live_group(0, "Room");
    flist_add_friend(alice, "hi", 2);
    flist_add_friend(bob, NULL, 0);
    flist_add_group(room);
    if (text_added != 1) {
        FAIL("welcome text only for alice");
    }
    /* friends, group, create */
    if (shown_len() != 4) {
        FAIL("shown 4 got %u", shown_len());
    }

    flist_selectchat(0);
    if (flist_get_sel_item_type() != ITEM_FRIEND || flist_get_sel_friend() != alice) {
        FAIL("select first friend");
    }
    flist_selectchat(2);
    if (flist_get_sel_group() != room) {
        FAIL("select group");
    }

    flist_search("ali");
    /* matching friend + group + create (search applies to friends only) */
    if (shown_len() != 3) {
        FAIL("search ali shown %u", shown_len());
    }
    flist_selectchat(0);
    if (flist_get_sel_friend() != alice) {
        FAIL("search keeps alice");
    }

    flist_search("bobby");
    flist_selectchat(0);
    if (flist_get_sel_friend() != bob) {
        FAIL("search alias");
    }

    flist_search("AAAAAAAA");
    flist_selectchat(0);
    if (flist_get_sel_friend() != alice) {
        FAIL("search by id_str");
    }

    flist_search(NULL);
    flist_set_filter(1);
    /* online alice + group + create; bob offline hidden unless selected */
    flist_selectchat(0);
    if (flist_get_sel_friend() != alice) {
        FAIL("filter shows alice");
    }
    flist_selectchat(1);
    if (flist_get_sel_friend() == bob) {
        FAIL("offline bob hidden");
    }
    if (flist_get_sel_group() != room) {
        FAIL("group still shown under online filter");
    }

    bob->unread_msg = true;
    flist_update_shown_list();
    /* Do not probe with shown_len(): selecting a friend clears unread_msg. */
    flist_selectchat(1);
    if (flist_get_sel_friend() != bob) {
        FAIL("unread offline still shown");
    }
    flist_set_filter(0);
    return true;
}

bool test_tabs_and_last(void) {
    reset_flist();
    start_list();
    FRIEND *a = live_friend(0, "A", true);
    FRIEND *b = live_friend(1, "B", true);
    flist_add_friend(a, NULL, 0);
    flist_add_friend(b, NULL, 0);
    flist_first_tab();
    if (flist_get_sel_friend() != a) {
        FAIL("first tab");
    }
    flist_next_tab();
    if (flist_get_sel_friend() != b) {
        FAIL("next tab");
    }
    flist_previous_tab();
    if (flist_get_sel_friend() != a) {
        FAIL("prev tab");
    }
    flist_last_tab();
    if (flist_get_sel_friend() != b) {
        FAIL("last tab skips create");
    }
    return true;
}

bool test_delete_and_frequest_accept(void) {
    reset_flist();
    start_list();
    FRIEND *a = live_friend(0, "A", true);
    FRIEND *b = live_friend(1, "B", true);
    flist_add_friend(a, NULL, 0);
    flist_add_friend(b, NULL, 0);
    flist_selectchat(0);
    flist_delete_sitem();
    /* Friend lifetime is owned by the tox thread (FRIEND_REMOVE); flist only drops the row. */
    if (shown_len() != 2) {
        FAIL("after delete shown %u", shown_len());
    }
    flist_selectchat(0);
    if (flist_get_sel_friend() != b) {
        FAIL("remaining friend");
    }
    /* delete last remaining friend, then the create row is left */
    flist_delete_sitem();
    if (shown_len() != 1 || flist_get_sel_item_type() != ITEM_GROUP_CREATE) {
        FAIL("list empty except create");
    }

    FREQUEST *r = live_req(0, "hello");
    flist_add_frequest(r);
    FRIEND *c = live_friend(2, "C", true);
    flist_selectchat(0);
    flist_add_friend_accepted(c, r);
    flist_selectchat(0);
    if (flist_get_sel_friend() != c) {
        FAIL("accepted request becomes friend");
    }

    GROUPCHAT *room = live_group(0, "Room");
    flist_add_group(room);
    flist_selectchat(1);
    if (flist_get_sel_group() != room) {
        FAIL("select group to delete");
    }
    flist_delete_sitem();
    if (flist_get_sel_group()) {
        FAIL("group row removed");
    }
    if (mock_last_tox_msg != TOX_GROUP_PART) {
        FAIL("delete group posts TOX_GROUP_PART");
    }

    FREQUEST *r2 = live_req(1, "later");
    flist_add_frequest(r2);
    flist_selectchat(1);
    if (flist_get_sel_frequest() != r2) {
        FAIL("select request to delete");
    }
    flist_delete_sitem();
    if (flist_get_sel_frequest()) {
        FAIL("request row removed");
    }
    return true;
}

bool test_search_null_friend_and_match_helper(void) {
    reset_flist();
    if (friend_matches_search_string(NULL, "x")) {
        FAIL("NULL friend does not match");
    }
    FRIEND *a = live_friend(0, "Alice", true);
    if (!friend_matches_search_string(a, NULL) || !friend_matches_search_string(a, "ICE")) {
        FAIL("case-insensitive name");
    }
    start_list();
    flist_add_friend(a, NULL, 0);
    /* a shown item pointing at a missing friend must not crash the filter */
    friend_live[0] = false;
    flist_update_shown_list();
    if (shown_len() != 1) {
        FAIL("missing friend dropped from shown, got %u", shown_len());
    }
    return true;
}

bool test_start_sparse_and_restart(void) {
    reset_flist();
    live_friend(0, "Zero", true);
    live_friend(2, "Two", true);
    self.friend_list_count = 2;
    self.friend_list_size = 3;
    start_list();
    if (shown_len() != 3) {
        FAIL("sparse start should list two friends + create, got %u", shown_len());
    }
    flist_selectchat(0);
    if (!flist_get_sel_friend() || strcmp(flist_get_sel_friend()->name, "Zero") != 0) {
        FAIL("first packed friend");
    }
    flist_selectchat(1);
    if (!flist_get_sel_friend() || strcmp(flist_get_sel_friend()->name, "Two") != 0) {
        FAIL("skipped hole, packed friend 2");
    }
    /* Rebuild roster arrays only; contacts stay owned by friend.c. */
    flist_start();
    if (shown_len() != 3) {
        FAIL("second start replaces list");
    }
    return true;
}

bool test_select_last_and_uri_use_shown_index(void) {
    reset_flist();
    start_list();
    flist_select_last();
    if (flist_get_sel_item_type() != ITEM_SETTINGS && flist_get_sel_item_type() != ITEM_GROUP_CREATE
        && flist_get_sel_friend()) {
        FAIL("select_last on tiny list must not invent a friend");
    }

    FRIEND *low = live_friend(0, "Low", true);
    FRIEND *high = live_friend(5, "High", true);
    flist_add_friend(low, NULL, 0);
    flist_add_friend(high, NULL, 0);
    char uri[4 + TOX_ADDRESS_SIZE * 2 + 1];
    memcpy(uri, "tox:", 4);
    memset(uri + 4, 'A' + 5, TOX_PUBLIC_KEY_SIZE * 2);
    memset(uri + 4 + TOX_PUBLIC_KEY_SIZE * 2, '0', TOX_ADDRESS_SIZE * 2 - TOX_PUBLIC_KEY_SIZE * 2);
    uri[4 + TOX_ADDRESS_SIZE * 2] = 0;
    if (!try_open_tox_uri(uri)) {
        FAIL("uri parse");
    }
    if (flist_get_sel_friend() != high) {
        FAIL("uri must select friend 5, not shown_list[5]");
    }
    if (try_open_tox_uri("not-a-uri") || try_open_tox_uri("tox:short")) {
        FAIL("bad uri must fail");
    }

    char unknown[4 + TOX_ADDRESS_SIZE * 2 + 1];
    memcpy(unknown, "tox:", 4);
    memset(unknown + 4, '9', TOX_ADDRESS_SIZE * 2);
    unknown[4 + TOX_ADDRESS_SIZE * 2] = 0;
    if (!try_open_tox_uri(unknown)) {
        FAIL("unknown friend uri still succeeds");
    }
    if (flist_get_sel_item_type() != ITEM_ADD) {
        FAIL("unknown uri opens add-friend");
    }
    if (edit_add_new_friend_id.length != TOX_ADDRESS_SIZE * 2
        || memcmp(edit_add_new_friend_id.data, unknown + 4, TOX_ADDRESS_SIZE * 2) != 0) {
        FAIL("add-friend id filled from uri");
    }

    tox_thread_init = UTOX_TOX_THREAD_INIT_NONE;
    flist_selectchat(0);
    if (!try_open_tox_uri(unknown)) {
        FAIL("unknown uri with tox not ready");
    }
    if (flist_get_sel_item_type() == ITEM_ADD) {
        FAIL("must not switch to add-friend before tox is up");
    }
    tox_thread_init = UTOX_TOX_THREAD_INIT_SUCCESS;
    return true;
}

bool test_null_add_and_open_untyped(void) {
    reset_flist();
    start_list();
    flist_add_friend(NULL, NULL, 0);
    flist_add_group(NULL);
    flist_add_frequest(NULL);
    if (shown_len() != 1) {
        FAIL("null adds ignored");
    }
    FRIEND *a = live_friend(0, "A", true);
    a->typed = NULL;
    a->typed_length = 0;
    flist_add_friend(a, NULL, 0);
    flist_selectchat(0);
    if (flist_get_sel_friend() != a) {
        FAIL("open friend with no typed buffer");
    }
    return true;
}

bool test_dump_reload_and_pages(void) {
    reset_flist();
    start_list();
    FRIEND *alice = live_friend(0, "Alice", true);
    FRIEND *bob = live_friend(1, "Bob", true);
    flist_add_friend(alice, NULL, 0);
    flist_add_friend(bob, NULL, 0);
    flist_selectchat(1);
    if (flist_get_sel_friend() != bob) {
        FAIL("select bob before dump");
    }

    flist_dump_contacts();
    self.friend_list_count = 0;
    alice = live_friend(0, "Alice", true);
    bob = live_friend(1, "Bob", true);
    flist_reload_contacts();
    if (flist_get_sel_friend() != bob) {
        FAIL("reload restores selected friend by id_bin");
    }

    flist_selectsettings();
    if (flist_get_sel_item_type() != ITEM_SETTINGS || flist_get_sel_friend()) {
        FAIL("select settings");
    }
    flist_selectaddfriend();
    if (flist_get_sel_item_type() != ITEM_ADD) {
        FAIL("select add friend");
    }
    flist_reselect_current();
    if (flist_get_sel_item_type() != ITEM_ADD) {
        FAIL("reselect keeps add friend");
    }
    flist_selectswap();
    if (flist_get_sel_item_type() == ITEM_FRIEND) {
        FAIL("selectswap leaves friend chat");
    }

    settings.use_mini_flist = true;
    flist_update_shown_list();
    settings.use_mini_flist = false;
    flist_update_shown_list();

    /* Missing friend while selected: dump must not leak the saved key. */
    flist_selectchat(0);
    friend_live[0] = false;
    flist_dump_contacts();
    friend_live[0] = true;
    friend_live[1] = true;
    flist_reload_contacts();
    if (flist_get_sel_item_type() == ITEM_FRIEND && flist_get_sel_friend() == alice) {
        FAIL("dump with missing friend must not restore alice");
    }
    return true;
}

int main(void) {
    int result = 0;
    setvbuf(stdout, NULL, _IONBF, 0);
    RUN_TEST(test_start_empty_and_create_item);
    RUN_TEST(test_add_select_search_filter);
    RUN_TEST(test_tabs_and_last);
    RUN_TEST(test_delete_and_frequest_accept);
    RUN_TEST(test_search_null_friend_and_match_helper);
    RUN_TEST(test_start_sparse_and_restart);
    RUN_TEST(test_select_last_and_uri_use_shown_index);
    RUN_TEST(test_null_add_and_open_untyped);
    RUN_TEST(test_dump_reload_and_pages);
    teardown_list();
    return result;
}
