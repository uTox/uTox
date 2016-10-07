#ifndef UI_DROPDOWN_H
#define UI_DROPDOWN_H

//Simple static dropdowns.
extern DROPDOWN dropdown_dpi,
                dropdown_language,
                dropdown_proxy,
                dropdown_ipv6,
                dropdown_udp,
                dropdown_theme,
                dropdown_auto_startup,
                dropdown_push_to_talk,
                dropdown_typing_notes,
                dropdown_mini_roster,
                dropdown_friend_autoaccept_ft,
                dropdown_notify_groupchats,
                dropdown_global_group_notifications;

//List-based dropdowns. list_dropdown_* functions are applicable.
extern DROPDOWN dropdown_audio_in,
                dropdown_audio_out,
                dropdown_video;

#endif
