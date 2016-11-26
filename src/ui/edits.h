#ifndef UI_EDITS_H
#define UI_EDITS_H

#include "edit.h"

extern EDIT edit_name, edit_toxid, edit_friend_pubkey, edit_status, edit_add_id, edit_add_msg, edit_msg, edit_msg_group,
    edit_search, edit_proxy_ip, edit_proxy_port, edit_profile_password, edit_friend_alias, edit_group_topic, edit_nospam,

    edit_add_new_device_to_self;

/* This function sends the message to the friend/group it's also called by the send button. */
void edit_msg_onenter(EDIT *edit);

#endif
