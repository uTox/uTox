#ifndef LAYOUT_GROUP_H
#define LAYOUT_GROUP_H

typedef struct scrollable SCROLLABLE;
extern SCROLLABLE scrollbar_group;

typedef struct panel PANEL;
extern PANEL panel_group,
                panel_group_chat,
                panel_group_video,
                panel_group_settings,
                messages_group;

typedef struct button BUTTON;
extern BUTTON button_group_audio,
              button_chat_send_group;

typedef struct dropdown DROPDOWN;
extern DROPDOWN dropdown_notify_groupchats;

typedef struct edit EDIT;
extern EDIT edit_chat_msg_group,
            edit_group_topic;

#endif // LAYOUT_GROUP_H
