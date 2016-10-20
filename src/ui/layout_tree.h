// layout_tree.h
//
// uTox user interface layout.

#ifndef LAYOUT_TREE_H
#define LAYOUT_TREE_H

#include "buttons.h"
#include "draw_helpers.h"
#include "draw_helpers.h"
#include "dropdowns.h"
#include "edits.h"
#include "switches.h"

// clang-format off
// Scrollbar or friend list
SCROLLABLE scrollbar_flist = {
    .panel = { .type = PANEL_SCROLLABLE, },
    .color = C_SCROLL,
    .x     = 2,
    .left  = 1,
    .small = 1,
},

// Scrollbar in chat window
scrollbar_friend = {
    .panel = { .type = PANEL_SCROLLABLE, },
    .color = C_SCROLL,
},

scrollbar_group = {
    .panel = { .type = PANEL_SCROLLABLE, },
    .color = C_SCROLL,
},

scrollbar_settings = {
    .panel = { .type = PANEL_SCROLLABLE, },
    .color = C_SCROLL,
};

PANEL messages_friend = {
    .type = PANEL_MESSAGES,
    .content_scroll = &scrollbar_friend,
},

messages_group = {
    .type = PANEL_MESSAGES,
    .content_scroll = &scrollbar_group,
};

// clang-format off
/* Root panel, hold all the other panels */

PANEL panel_root = {
    .type = PANEL_NONE,
    .drawfunc = draw_background,
    .disabled = 0,
    .child = (PANEL*[]) { /* Clang warns about this being a temporary array which will be destoryed
                           * at the end of the expression. But because this has always worked in the
                           * past, and because of this comment  http://stackoverflow.com/questions/31212114/clang-complains-pointer-is-initialized-by-a-temporary-array#comment50455257_31212154
                           * I've chosen to ignore this warning. If you're feeling pedantic you can
                           * define and name each array seperatly and change the PANEL struct. */
        &panel_side_bar, &panel_main,
        NULL
    }
},

/* Left side bar, holds the user, the roster, and the setting buttons */
panel_side_bar = {
    .type = PANEL_NONE,
    .disabled = 0,
    .child = (PANEL*[]) {
        &panel_self,
        &panel_quick_buttons,
        &panel_flist,
        NULL
    }
},
    /* The user badge and buttons */
    panel_self = {
        .type     = PANEL_NONE,
        .disabled = 0,
        .drawfunc = draw_user_badge,
        .child    = (PANEL*[]) {
            (PANEL*)&button_avatar, (PANEL*)&button_name,       (PANEL*)&button_usr_state,
                                   (PANEL*)&button_status_msg,
            NULL
        }
    },
    /* Left sided toggles */
    panel_quick_buttons = {
        .type     = PANEL_NONE,
        .disabled = 0,
        .child    = (PANEL*[]) {
            (PANEL*)&button_filter_friends, /* Top of roster */

            (PANEL*)&edit_search,           /* Bottom of roster*/
            (PANEL*)&button_settings,
            (PANEL*)&button_add_new_contact,
            NULL
        }
    },
    /* The friends and group was called list */
    panel_flist = {
        .type     = PANEL_NONE,
        .disabled = 0,
        .child    = (PANEL*[]) {
            // TODO rename these
            &panel_flist_list,
            (PANEL*)&scrollbar_flist,
            NULL
        }
    },
        panel_flist_list = {
            .type           = PANEL_LIST,
            .content_scroll = &scrollbar_flist,
        },

/* Main panel, holds the overhead/settings, or the friend/group containers */
panel_main = {
    .type = PANEL_NONE,
    .disabled = 0,
    .child = (PANEL*[]) {
        &panel_chat,
        &panel_overhead,
        NULL
    }
},
    /* Chat panel, friend or group, depending on what's selected */
    panel_chat = {
        .type = PANEL_NONE,
        .disabled = 1,
        .child = (PANEL*[]) {
            &panel_group,
            &panel_friend,
            &panel_friend_request,
            NULL
        }
    },
        panel_group = {
            .type = PANEL_NONE,
            .disabled = 1,
            .child = (PANEL*[]) {
                &panel_group_chat,
                &panel_group_video,
                &panel_group_settings,
                NULL
            }
        },
            panel_group_chat = {
                .type = PANEL_NONE,
                .disabled = 0,
                .drawfunc = draw_group,
                .child = (PANEL*[]) {
                    (PANEL*)&scrollbar_group,
                    (PANEL*)&edit_msg_group, // this needs to be one of the first, to get events before the others
                    (PANEL*)&messages_group,
                    (PANEL*)&button_group_audio,
                    (PANEL*)&button_chat_send,
                    NULL
                }
            },
            panel_group_video = {
                .type = PANEL_NONE,
                .disabled = 1,
                .child = (PANEL*[]) {
                    NULL
                }
            },
            panel_group_settings = {
                .type = PANEL_NONE,
                .disabled = 1,
                .drawfunc = draw_group_settings,
                .child = (PANEL*[]) {
                    (PANEL*)&edit_group_topic,
                    (PANEL*)&dropdown_notify_groupchats,
                    NULL
                }
            },
        panel_friend = {
            .type = PANEL_NONE,
            .disabled = 1,
            .child = (PANEL*[]) {
                &panel_friend_chat,
                &panel_friend_video,
                &panel_friend_settings,
                NULL
            }
        },
            panel_friend_chat = {
                .type = PANEL_NONE,
                .disabled = 0,
                .drawfunc = draw_friend,
                .child = (PANEL*[]) {
                    (PANEL*)&scrollbar_friend,
                    (PANEL*)&edit_msg, // this needs to be one of the first, to get events before the others
                    (PANEL*)&messages_friend,
                    (PANEL*)&button_call_decline, (PANEL*)&button_call_audio, (PANEL*)&button_call_video,
                    (PANEL*)&button_send_file, (PANEL*)&button_send_screenshot, (PANEL*)&button_chat_send,
                    NULL
                }
            },
            panel_friend_video = {
                .type = PANEL_INLINE_VIDEO,
                .disabled = 1,
                .child = (PANEL*[]) {
                    NULL
                }
            },
            panel_friend_settings = {
                .type = PANEL_NONE,
                .disabled = 1,
                .drawfunc = draw_friend_settings,
                .child = (PANEL*[]) {
                    (PANEL*)&edit_friend_pubkey,
                    (PANEL*)&edit_friend_alias,
                    (PANEL*)&dropdown_friend_autoaccept_ft,
                    (PANEL*)&button_export_chatlog,
                    NULL
                }
            },
        panel_friend_request = {
            .type = PANEL_NONE,
            .disabled = 1,
            .drawfunc = draw_friend_request,
            .child = (PANEL*[]) {
                (PANEL*)&button_accept_friend,
                NULL
            }
        },
    /* Settings master panel, holds the lower level settings */
    panel_overhead = {
        .type = PANEL_NONE,
        .disabled = 0,
        .child = (PANEL*[]) {
            &panel_splash_page,
            &panel_profile_password,
            &panel_add_friend,
            &panel_settings_master,
            NULL
        }
    },
        panel_splash_page = {
            .type = PANEL_NONE,
            .disabled = 1,
            .drawfunc = draw_splash_page,
            .content_scroll = &scrollbar_settings,
            .child = (PANEL*[]) {
                NULL,
            }
        },

        panel_profile_password = {
            .type = PANEL_NONE,
            .disabled = 0,
            .drawfunc = draw_profile_password,
            .child = (PANEL*[]) {
                (PANEL*)&edit_profile_password,
                NULL
            }
        },
        panel_add_friend = {
            .type = PANEL_NONE,
            .disabled = 1,
            .drawfunc = draw_add_friend,
            .child = (PANEL*[]) {
                (PANEL*)&button_send_friend_request,
                (PANEL*)&edit_add_id, (PANEL*)&edit_add_msg,
                NULL
            }
        },
        panel_settings_master = {
            .type = PANEL_NONE,
            .disabled = 1,
            .drawfunc = draw_settings_header,
            .child = (PANEL*[]) {
                &panel_settings_subheader,
                NULL
            }
        },
            panel_settings_subheader = {
                .type = PANEL_NONE,
                .disabled = 0,
                .drawfunc = draw_settings_sub_header,
                .child = (PANEL*[]) {
                    (PANEL*)&button_settings_sub_profile,
                    (PANEL*)&button_settings_sub_devices,
                    (PANEL*)&button_settings_sub_net,
                    (PANEL*)&button_settings_sub_ui,
                    (PANEL*)&button_settings_sub_av,
                    (PANEL*)&scrollbar_settings,
                    &panel_settings_profile,
                    &panel_settings_devices,
                    &panel_settings_net,
                    &panel_settings_ui,
                    &panel_settings_av,
                    NULL
                }
            },

            /* Panel to draw settings page */
            panel_settings_profile = {
                .type = PANEL_NONE,
                .disabled = 0,
                .drawfunc = draw_settings_text_profile,
                .content_scroll = &scrollbar_settings,
                .child = (PANEL*[]) {
                    (PANEL*)&edit_name,
                    (PANEL*)&edit_status,
                    // Text: Tox ID
                    (PANEL*)&edit_toxid,
                    (PANEL*)&button_copyid,
                    (PANEL*)&dropdown_language,
                    (PANEL*)&button_show_password_settings,
                    &panel_profile_password_settings,
                    NULL
                }
            },

                panel_profile_password_settings = {
                    .type     = PANEL_NONE,
                    .disabled = 1,
                    .drawfunc = draw_settings_text_password,
                    .child = (PANEL*[]) {
                        (PANEL*)&edit_profile_password,
                        (PANEL*)&button_lock_uTox,
                        NULL
                    }
                },

            /* Panel to draw settings page */
            panel_settings_devices = {
                .type = PANEL_NONE,
                .disabled = 1,
                .drawfunc = draw_settings_text_devices,
                .content_scroll = &scrollbar_settings,
                .child = NULL,
            },

            panel_settings_net = {
                .type = PANEL_NONE,
                /* Disabled by default, enabled by network button */
                .disabled = 1,
                .drawfunc = draw_settings_text_network,
                .content_scroll = &scrollbar_settings,
                .child = (PANEL*[]) {
                    (PANEL*)&edit_proxy_ip,
                    (PANEL*)&edit_proxy_port,
                    (PANEL*)&dropdown_proxy,
                    (PANEL*)&switch_ipv6,
                    (PANEL*)&switch_udp,
                    NULL
                }
            },

            panel_settings_ui = {
                .type = PANEL_NONE,
                .drawfunc = draw_settings_text_ui,
                .disabled = 1,
                .content_scroll = &scrollbar_settings,
                .child = (PANEL*[]) {
                    (PANEL*)&dropdown_dpi,
                    (PANEL*)&dropdown_theme,
                    (PANEL*)&switch_logging,
                    (PANEL*)&switch_close_to_tray,
                    (PANEL*)&switch_start_in_tray,
                    (PANEL*)&switch_auto_startup,
                    (PANEL*)&switch_typing_notes,
                    (PANEL*)&switch_mini_contacts,
                    NULL
                }
            },

            panel_settings_av = {
                .type = PANEL_NONE,
                .disabled = 1,
                .drawfunc = draw_settings_text_av,
                .content_scroll = &scrollbar_settings,
                .child = (PANEL*[]) {
                    (PANEL*)&button_callpreview,
                    (PANEL*)&switch_push_to_talk,
                    (PANEL*)&button_videopreview,
                    (PANEL*)&dropdown_audio_in,
                    (PANEL*)&dropdown_audio_out,
                    (PANEL*)&dropdown_video,
                    (PANEL*)&switch_audible_notifications,
                    (PANEL*)&switch_audio_filtering,
                    (PANEL*)&dropdown_global_group_notifications,
                    (PANEL*)&switch_status_notifications,
                    NULL
                }
            };
// clang-format on

#endif