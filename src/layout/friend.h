#ifndef LAYOUT_FRIEND_H
#define LAYOUT_FRIEND_H


typedef struct scrollable SCROLLABLE;
extern SCROLLABLE scrollbar_friend;

typedef struct panel PANEL;
extern PANEL messages_friend;

extern PANEL panel_friend,
             panel_add_friend,
             panel_friend_chat,
             panel_friend_video,
             panel_friend_settings,
             panel_friend_request;


typedef struct button BUTTON;
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

typedef struct uiswitch UISWITCH;
extern UISWITCH switch_friend_autoaccept_ft;


typedef struct edit EDIT;
extern EDIT edit_add_new_friend_id,
            edit_add_new_friend_msg,

            edit_chat_msg_friend,
            edit_friend_pubkey,
            edit_friend_alias;

#endif // LAYOUT_FRIEND_H
