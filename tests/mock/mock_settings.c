#include "branding.h"
#include "settings.h"

#include "../test.h"


SETTINGS settings = {
    .curr_version = UTOX_VERSION_NUMBER,
    // .last_version                // included here to match the full struct
    .show_splash = false,

    // Low level settings (network, profile, portable-mode)
    .enable_udp     = true,
    .enable_ipv6    = true,

    .use_proxy      = false,
    .force_proxy    = false,
    .proxy_port     = 0,

    // Tox level settings
    .block_friend_requests  = false,
    .save_encryption        = true,

    // uTox internals
    .auto_update        = false,
    .update_to_develop  = false,
    .send_version       = false,

    // testing always in portable mode to not touch any real tox profile!
    .portable_mode = true,


    // User interface settings
    .close_to_tray          = false,
    .logging_enabled        = true,
    .audiofilter_enabled    = true,
    .start_in_tray          = false,
    .start_with_system      = false,
    .push_to_talk           = false,
    .audio_preview          = false,
    .video_preview          = false,
    .send_typing_status     = false,
    .use_mini_flist         = false,
    // .inline_video                // included here to match the full struct
    .use_long_time_msg      = true,
    .accept_inline_images   = true,

    // Notifications / Alerts
    .ringtone_enabled       = true,
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

/*
 * Loads the config file and returns a settings struct
 */
UTOX_SAVE *config_load(void) {
    FAIL_FATAL("called a mocked function, this should not happen: %s", __FUNCTION__);
}

/*
 * Writes save_in to the disk
 */
void config_save(UTOX_SAVE *save_in) {
    FAIL_FATAL("called a mocked function, this should not happen: %s", __FUNCTION__);
}

/**
 * Saves the settings for uTox
 *
 * Returns a bool indicating if it succeeded or not
 */
bool utox_data_save_utox(UTOX_SAVE *data, size_t length) {
    FAIL_FATAL("called a mocked function, this should not happen: %s", __FUNCTION__);
}

/**
 * Loads uTox settings
 *
 * Returns a memory pointer of *size, the caller needs to free this
 * Returns NULL on failure
 */
UTOX_SAVE *utox_data_load_utox(void) {
    FAIL_FATAL("called a mocked function, this should not happen: %s", __FUNCTION__);
}
