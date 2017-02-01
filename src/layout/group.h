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
extern BUTTON button_group_audio;

#endif // LAYOUT_GROUP_H
