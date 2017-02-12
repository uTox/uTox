#ifndef SETTINGS_H
#define SETTINGS_H

typedef struct utox_save UTOX_SAVE;

#include "debug.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct utox_settings {
    // uTox versions settings
    uint32_t curr_version;
    uint32_t last_version;
    bool     show_splash;

    // Low level settings (network, profile, portable-mode)
    bool portable_mode;

    bool save_encryption;

    bool auto_update;
    bool update_to_develop;
    bool send_version;

    bool force_proxy;
    bool enable_udp;
    bool enable_ipv6;

    bool block_friend_requests;

    bool use_proxy;
    uint16_t proxy_port;

    // User interface settings
    bool close_to_tray;
    bool logging_enabled;
    bool audiofilter_enabled;
    bool start_in_tray;
    bool start_with_system;
    bool push_to_talk;
    bool audio_preview;
    bool video_preview;
    bool send_typing_status;
    bool use_mini_flist;
    bool inline_video;
    bool use_long_time_msg;
    bool accept_inline_images;

    // Notifications / Alerts
    bool    ringtone_enabled;
    bool    status_notifications;
    uint8_t group_notifications;

    LOG_LVL verbose;
    FILE *debug_file;

    uint32_t theme;

    // OS interface settings
    uint32_t window_height;
    uint32_t window_width;
    uint32_t window_baseline;

    bool    window_maximized;
} SETTINGS;

extern SETTINGS settings;

/*
 * Loads the config file and returns a settings struct
 */
UTOX_SAVE *config_load(void);

/*
 * Writes save_in to the disk
 */
void config_save(UTOX_SAVE *save_in);

#endif
