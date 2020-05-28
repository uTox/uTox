#ifndef LAYOUT_SETTINGS_H
#define LAYOUT_SETTINGS_H

typedef struct scrollable SCROLLABLE;
extern SCROLLABLE scrollbar_settings;

typedef struct panel PANEL;
extern PANEL panel_settings_master,
             panel_settings_subheader,
             panel_settings_profile,
             panel_profile_password_settings,
             panel_settings_devices,
             panel_settings_ui,
             panel_settings_av,
             panel_settings_notifications,
             panel_settings_adv,
             panel_nospam_settings;

extern PANEL panel_profile_password;

typedef struct button BUTTON;
extern BUTTON   button_settings,
                button_settings_sub_profile,
                button_settings_sub_devices,
                button_settings_sub_net,
                button_settings_sub_ui,
                button_settings_sub_av,
                button_settings_sub_adv,
                button_settings_sub_notifications;

extern BUTTON   button_add_new_device_to_self;

extern BUTTON   button_callpreview,
                button_videopreview,
                button_copyid,
                button_show_qr,
                button_qr,
                button_lock_uTox,
                button_show_password_settings,
                button_change_nospam,
                button_revert_nospam,
                button_show_nospam;

typedef struct uiswitch UISWITCH;
extern UISWITCH /* User Interface Tab */
                switch_save_chat_history,
                switch_close_to_tray,
                switch_start_in_tray,
                switch_auto_startup,
                switch_mini_contacts,
                switch_magic_sidebar,
                /* AV Tab */
                switch_push_to_talk,
                switch_audio_filtering,
                /* Notifications Tab */
                switch_audible_notifications,
                switch_status_notifications,
                switch_typing_notes,
                /* Advanced Tab */
                switch_ipv6,
                switch_udp,
                switch_proxy,
                switch_proxy_force,
                switch_block_friend_requests;

typedef struct dropdown DROPDOWN;
extern DROPDOWN /* Profile */
                dropdown_language,
                /* User interface */
                dropdown_theme,
                dropdown_dpi,
                /* AV */
                dropdown_audio_in,
                dropdown_audio_out,
                dropdown_video,
                /* Notifications */
                dropdown_global_group_notifications;

typedef struct edit EDIT;
extern EDIT /* Profile */
            edit_name,
            edit_status_msg,
            edit_toxid,
            /* Advanced */
            edit_proxy_ip,
            edit_proxy_port,
            edit_profile_password,
            edit_nospam,
            /* Video */
            edit_video_fps,
            /* MDevice */
            edit_add_new_device_to_self;

void reset_settings_controls(void);

#endif // LAYOUT_SETTINGS_H
