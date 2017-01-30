#ifndef LAYOUT_FRIEND_H
#define LAYOUT_FRIEND_H

#include "../ui/button.h"
#include "../ui/panel.h"

extern SCROLLABLE scrollbar_friend;

extern PANEL messages_friend;


// Local Tree
extern PANEL panel_friend,
             panel_add_friend,
             panel_friend_chat,
             panel_friend_video,
             panel_friend_settings,
             panel_friend_request;

// Top Bar
extern BUTTON button_call_decline,
              button_call_audio,
              button_call_video;
// Bottom Bar
extern BUTTON button_send_file,
              button_send_screenshot,
              button_chat_send;

// Friend Requests
extern BUTTON button_send_friend_request,
              button_accept_friend;

// Friend Settings
extern BUTTON button_export_chatlog;


#endif // LAYOUT_FRIEND_H
