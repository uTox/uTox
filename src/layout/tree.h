#ifndef UI_TREE_H
#define UI_TREE_H

typedef struct panel PANEL;

/* uTox panel draw hierarchy. */
extern PANEL panel_notify_generic;

/* TODO remove this and expose it differently */
extern PANEL panel_root, panel_side_bar, panel_self, panel_quick_buttons, panel_flist, panel_flist_list,
    panel_lower_buttons, panel_main, panel_chat, panel_group, panel_group_chat, panel_group_video, panel_group_settings,
    panel_friend, panel_friend_chat, panel_friend_video, panel_friend_settings, panel_friend_request, panel_overhead,
    panel_splash_page, panel_profile_password, panel_add_friend,
    messages_friend,
    messages_group,
    panel_settings_master,
        panel_settings_subheader,
        panel_settings_profile,
        panel_profile_password_settings,
        panel_settings_devices,
        panel_settings_ui,
        panel_settings_av,
        panel_settings_notifications,
        panel_settings_adv,
        panel_nospam_settings;

#endif // UI_TREE_H
