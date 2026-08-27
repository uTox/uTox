#include "mock_domain.h"

#include "../../src/friend.h"
#include "../../src/groups.h"

#include <stdint.h>

void flist_update_shown_list(void) {}
void flist_selectaddfriend(void) {}
void flist_selectchat(int index) {
    (void)index;
}
void flist_add_friend(FRIEND *f, const char *msg, const int msg_length) {
    (void)f;
    (void)msg;
    (void)msg_length;
}
void flist_add_group(GROUPCHAT *g) {
    (void)g;
}

FRIEND *flist_get_sel_friend(void) {
    return mock_sel_friend;
}
GROUPCHAT *flist_get_sel_group(void) {
    return mock_sel_group;
}
ITEM_TYPE flist_get_sel_item_type(void) {
    return mock_sel_item_type;
}
