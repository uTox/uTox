#ifndef SETTINGS_H
#define SETTINGS_H

#include "debug.h"
#include "../langs/i18n_decls.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* House keeping for uTox save file. */
#define UTOX_SAVE_VERSION 4
#define DEFAULT_FPS 25

typedef struct utox_settings {
    uint8_t  save_version;

    uint32_t last_version;
    uint32_t utox_last_version;

    bool show_splash;

    // Tox level settings
    bool block_friend_requests;
    bool save_encryption;

    // User interface settings
    UTOX_LANG language;

    bool audio_filtering_enabled;
    bool push_to_talk;
    bool audio_preview;
    bool video_preview;
    bool no_typing_notifications;
    bool inline_video;
    bool use_long_time_msg;
    bool accept_inline_images;

    // UX Settings
    bool logging_enabled;
    bool close_to_tray;
    bool start_in_tray;
    bool auto_startup;
    bool use_mini_flist;
    bool filter;
    bool magic_flist_enabled;

    // Notifications / Alerts
    bool    audible_notifications_enabled;
    bool    status_notifications;
    uint8_t group_notifications;

    uint16_t audio_device_out;
    uint16_t audio_device_in;
    uint8_t  video_fps;

    LOG_LVL verbose;
    FILE *  debug_file;

    uint32_t theme;
    uint8_t  scale;

    // OS interface settings
    uint32_t window_x;
    uint32_t window_y;
    uint32_t window_height;
    uint32_t window_width;
    uint32_t window_baseline;
    bool     window_maximized;

    // Low level settings (network, profile, portable-mode)
    bool     portable_mode;
    bool     disableudp;
    bool     enableipv6;
    bool     proxyenable;
    bool     force_proxy;
    uint16_t proxy_port;
    uint8_t  proxy_ip[255]; /* coincides with TOX_MAX_HOSTNAME_LENGTH from toxcore */
} SETTINGS;

extern SETTINGS settings;

/*
 * Loads settings from disk
 */
void config_load(void);

/*
 * Writes settings to disk
 */
void config_save(void);

#endif
