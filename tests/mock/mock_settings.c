#include "branding.h"
#include "settings.h"

#include "../test.h"


SETTINGS settings = {
    .utox_last_version = UTOX_VERSION_NUMBER,
    // .last_version                // included here to match the full struct
    .show_splash = false,

    // Low level settings (network, profile, portable-mode)
    .disableudp     = false,
    .enableipv6     = true,

    .proxyenable    = false,
    .force_proxy    = false,
    .proxy_port     = 0,

    // Tox level settings
    .block_friend_requests  = false,
    .save_encryption        = true,

    // testing always in portable mode to not touch any real tox profile!
    .portable_mode = true,


    // User interface settings
    .close_to_tray           = false,
    .logging_enabled         = true,
    .audio_filtering_enabled = true,
    .start_in_tray           = false,
    .auto_startup            = false,
    .push_to_talk            = false,
    .audio_preview           = false,
    .video_preview           = false,
    .no_typing_notifications = true,
    .use_mini_flist          = false,
    // .inline_video                // included here to match the full struct
    .use_long_time_msg       = true,
    .accept_inline_images    = true,

    // Notifications / Alerts
    .audible_notifications_enabled = true,
    .status_notifications   = true,
    .group_notifications    = 0,

    .verbose = LOG_LVL_ERROR,
    .debug_file = NULL,

    // .theme                       // included here to match the full struct
    // OS interface settings
    .window_height        = 480,
    .window_width         = 640,
    .window_baseline      = 0,

    .window_maximized     = 0,
};

void config_load(void) {
    FAIL_FATAL("called a mocked function, this should not happen: %s", __FUNCTION__);
}

void config_save(void) {
    FAIL_FATAL("called a mocked function, this should not happen: %s", __FUNCTION__);
}
