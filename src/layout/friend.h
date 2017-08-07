#ifndef LAYOUT_FRIEND_H
#define LAYOUT_FRIEND_H

#include "../typedefs.h"

extern SCROLLABLE scrollbar_friend;

extern PANEL messages_friend;

extern PANEL panel_friend,
             panel_add_friend,
             panel_friend_chat,
             panel_friend_video,
             panel_friend_settings,
             panel_friend_request,
             panel_friend_confirm_deletion;

// Top Bar
extern BUTTON button_call_decline,
              button_call_audio,
              button_call_video;
// Bottom Bar
extern BUTTON button_send_file,
              button_send_screenshot,
              button_chat_send_friend;

// Friend Requests
extern BUTTON button_send_friend_request,
              button_accept_friend;

// Friend Settings
extern BUTTON button_export_chatlog;

// Friend Deletion model
extern BUTTON button_confirm_deletion,
              button_deny_deletion;

extern UISWITCH switch_friend_autoaccept_ft;


extern EDIT edit_add_new_friend_id,
            edit_add_new_friend_msg,

            edit_chat_msg_friend,
            edit_friend_pubkey,
            edit_friend_alias;

#endif // LAYOUT_FRIEND_H
