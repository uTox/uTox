#include "settings.h"

#include "debug.h"
#include "flist.h"
#include "groups.h"
#include "tox.h"

// TODO do we want to include the UI headers here?
// Or would it be better to supply a callback after settings are loaded?
#include "ui/edit.h"
#include "ui/switch.h"
#include "ui/dropdown.h"

#include "layout/settings.h"

#include "native/filesys.h"
#include "native/keyboard.h"

// UTOX_VERSION_NUMBER, MAIN_HEIGHT, MAIN_WIDTH, all save things..
#include "main.h"

#include <stdlib.h>
#include <string.h>
#include <minIni.h>

#define MATCH(x, y) (strcasecmp(x, y) == 0)
#define BOOL_TO_STR(b) b ? "true" : "false"
#define STR_TO_BOOL(s) (strcasecmp(s, "true") == 0)
#define NAMEOF(s) strchr((const char *)(#s), '>') == NULL \
    ? #s : (strchr((const char *)(#s), '>') + 1)

static const char *config_file_name = "utox_save.ini";

/**
 * Config section names.
 */
typedef enum {
    GENERAL_SECTION,
    INTERFACE_SECTION,
    AV_SECTION,
    NOTIFICATIONS_SECTION,
    ADVANCED_SECTION,
    UNKNOWN_SECTION
} CONFIG_SECTION;

static const char *config_sections[UNKNOWN_SECTION + 1] = {
    "general",
    "interface",
    "av",
    "notifications",
    "advanced",
    NULL
};

SETTINGS settings = {
    .last_version      = UTOX_VERSION_NUMBER,
    .utox_last_version = UTOX_VERSION_NUMBER,

    .show_splash = false,

    // Tox level settings
    .block_friend_requests  = false,
    .save_encryption        = true,

    // User interface settings
    .language               = LANG_EN,

    .audio_filtering_enabled = true,
    .push_to_talk            = false,
    .audio_preview           = false,
    .video_preview           = false,
    .no_typing_notifications = true,
    .use_long_time_msg       = true,
    .accept_inline_images    = true,

    // UX Settings
    .logging_enabled     = true,
    .close_to_tray       = false,
    .start_in_tray       = false,
    .auto_startup        = false,
    .use_mini_flist      = false,
    .magic_flist_enabled = false,

    // Notifications / Alerts
    .audible_notifications_enabled = true,
    .status_notifications          = true,
    .group_notifications           = GNOTIFY_ON,

    .audio_device_out = 0,
    .audio_device_in  = 0,
    .video_fps        = DEFAULT_FPS,

    .verbose    = LOG_LVL_ERROR,
    .debug_file = NULL,

    .theme = UINT32_MAX,

    // OS interface settings
    .window_x         = 0,
    .window_y         = 0,
    .window_height    = MAIN_HEIGHT,
    .window_width     = MAIN_WIDTH,
    .window_baseline  = 0,
    .window_maximized = false,

    // Low level settings (network, profile, portable-mode)
    .disableudp     = false,
    .enableipv6     = true,
    .proxyenable    = false,
    .force_proxy    = false,
    .proxy_port     = 0,
};

static void write_config_value_int(const char *filename, const char *section,
                                   const char *key, const long value) {
    if (ini_putl(section, key, value, filename) != 1) {
        LOG_ERR("Settings", "Unable to save config value: %lu.", value);
    }
}
#define WRITE_CONFIG_VALUE_INT(SECTION, VALUE) \
    write_config_value_int(config_path, config_sections[SECTION], \
        NAMEOF(VALUE), VALUE)

static void write_config_value_str(const char *filename, const char *section,
                                   const char *key, const char *value) {
    if (ini_puts(section, key, value, filename) != 1) {
        LOG_ERR("Settings", "Unable to save config value: %s.", value);
    }
}
#define WRITE_CONFIG_VALUE_STR(SECTION, VALUE) \
    write_config_value_str(config_path, config_sections[SECTION], \
        NAMEOF(VALUE), (const char *)VALUE)

static void write_config_value_bool(const char *filename, const char *section,
                                    const char *key, const bool value) {
    if (ini_puts(section, key, BOOL_TO_STR(value), filename) != 1) {
        LOG_ERR("Settings", "Unable to save config value: %s.", value);
    }
}
#define WRITE_CONFIG_VALUE_BOOL(SECTION, VALUE) \
    write_config_value_bool(config_path, config_sections[SECTION], \
        NAMEOF(VALUE), VALUE)

static CONFIG_SECTION get_section(const char *section) {
    if (MATCH(config_sections[GENERAL_SECTION], section)) {
        return GENERAL_SECTION;
    } else if (MATCH(config_sections[INTERFACE_SECTION], section)) {
        return INTERFACE_SECTION;
    } else if (MATCH(config_sections[AV_SECTION], section)) {
        return AV_SECTION;
    } else if (MATCH(config_sections[NOTIFICATIONS_SECTION], section)) {
        return NOTIFICATIONS_SECTION;
    } else if (MATCH(config_sections[ADVANCED_SECTION], section)) {
        return ADVANCED_SECTION;
    } else {
        return UNKNOWN_SECTION;
    }
}

static void parse_general_section(SETTINGS *config, const char *key,
                                  const char *value) {
    if (MATCH(NAMEOF(config->save_version), key)) {
        config->save_version = atoi(value);
    } else if (MATCH(NAMEOF(config->utox_last_version), key)) {
        config->utox_last_version = atoi(value);
    }
}

static void parse_interface_section(SETTINGS *config, const char *key,
                                    const char *value) {
    if (MATCH(NAMEOF(config->language), key)) {
        config->language = atoi(value);
    } else if (MATCH(NAMEOF(config->window_x), key)) {
        config->window_x = atoi(value);
    } else if (MATCH(NAMEOF(config->window_y), key)) {
        config->window_y = atoi(value);
    } else if (MATCH(NAMEOF(config->window_width), key)) {
        config->window_width = atoi(value);
    } else if (MATCH(NAMEOF(config->window_height), key)) {
        config->window_height = atoi(value);
    } else if (MATCH(NAMEOF(config->theme), key)) {
        // Allow users to override theme on the cmdline.
        if (config->theme == UINT32_MAX) {
            config->theme = atoi(value);
        }
    } else if (MATCH(NAMEOF(config->scale), key)) {
        config->scale = atoi(value);
    } else if (MATCH(NAMEOF(config->logging_enabled), key)) {
        config->logging_enabled = STR_TO_BOOL(value);
    } else if (MATCH(NAMEOF(config->close_to_tray), key)) {
        config->close_to_tray = STR_TO_BOOL(value);
    } else if (MATCH(NAMEOF(config->start_in_tray), key)) {
        config->start_in_tray = STR_TO_BOOL(value);
    } else if (MATCH(NAMEOF(config->auto_startup), key)) {
        config->auto_startup = STR_TO_BOOL(value);
    } else if (MATCH(NAMEOF(config->use_mini_flist), key)) {
        config->use_mini_flist = STR_TO_BOOL(value);
    } else if (MATCH(NAMEOF(config->filter), key)) {
        config->filter = STR_TO_BOOL(value);
    } else if (MATCH(NAMEOF(config->magic_flist_enabled), key)) {
        config->magic_flist_enabled = STR_TO_BOOL(value);
    } else if (MATCH(NAMEOF(config->use_long_time_msg), key)) {
        config->use_long_time_msg = STR_TO_BOOL(value);
    }
}

static void parse_av_section(SETTINGS *config, const char *key,
                             const char *value) {
    if (MATCH(NAMEOF(config->push_to_talk), key)) {
        config->push_to_talk = STR_TO_BOOL(value);
    } else if (MATCH(NAMEOF(config->audio_filtering_enabled), key)) {
        config->audio_filtering_enabled = STR_TO_BOOL(value);
    } else if (MATCH(NAMEOF(config->audio_device_in), key)) {
        config->audio_device_in = atoi(value);
    } else if (MATCH(NAMEOF(config->audio_device_out), key)) {
        config->audio_device_out = atoi(value);
    } else if (MATCH(NAMEOF(config->video_fps), key)) {
        char *temp;
        uint16_t value_fps = strtol((char *)value, &temp, 0);

        if (*temp == '\0' && value_fps >= 1 && value_fps <= UINT8_MAX) {
            settings.video_fps = value_fps;
            return;
        }

        LOG_WARN("Settings",
            "Fps value (%s) is invalid. It must be integer in range of [1,%u].",
            value, UINT8_MAX);
    }
}

static void parse_notifications_section(SETTINGS *config, const char *key,
                                        const char *value) {
    if (MATCH(NAMEOF(config->audible_notifications_enabled), key)) {
        config->audible_notifications_enabled = STR_TO_BOOL(value);
    } else if (MATCH(NAMEOF(config->status_notifications), key)) {
        config->status_notifications = STR_TO_BOOL(value);
    } else if (MATCH(NAMEOF(config->no_typing_notifications), key)) {
        config->no_typing_notifications = STR_TO_BOOL(value);
    } else if (MATCH(NAMEOF(config->group_notifications), key)) {
        config->group_notifications = atoi(value);
    }
}

static void parse_advanced_section(SETTINGS *config, const char *key,
                                   const char *value) {
    if (MATCH(NAMEOF(config->enableipv6), key)) {
        config->enableipv6 = STR_TO_BOOL(value);
    } else if (MATCH(NAMEOF(config->disableudp), key)) {
        config->disableudp = STR_TO_BOOL(value);
    } else if (MATCH(NAMEOF(config->proxyenable), key)) {
        config->proxyenable = STR_TO_BOOL(value);
    } else if (MATCH(NAMEOF(config->proxy_port), key)) {
        config->proxy_port = atoi(value);
    } else if (MATCH(NAMEOF(config->proxy_ip), key)) {
        strcpy((char *)config->proxy_ip, value);
    } else if (MATCH(NAMEOF(config->force_proxy), key)) {
        config->force_proxy = STR_TO_BOOL(value);
    } else if (MATCH(NAMEOF(config->block_friend_requests), key)) {
        config->block_friend_requests = STR_TO_BOOL(value);
    }
}

static int config_parser(const char *section, const char *key,
                         const char *value, void *config_v) {
    SETTINGS *config = (SETTINGS *)config_v;

    switch (get_section(section)) {
        case GENERAL_SECTION: {
            parse_general_section(config, key, value);
            break;
        }
        case INTERFACE_SECTION: {
            parse_interface_section(config, key, value);
            break;
        }
        case AV_SECTION: {
            parse_av_section(config, key, value);
            break;
        }
        case NOTIFICATIONS_SECTION: {
            parse_notifications_section(config, key, value);
            break;
        }
        case ADVANCED_SECTION: {
            parse_advanced_section(config, key, value);
            break;
        }
        case UNKNOWN_SECTION: {
            LOG_NOTE("Settings", "Unknown section in config file: %s", section);
            break;
        }
    }

    return 1;
}

static bool utox_load_config(void) {
    char *config_path = utox_get_filepath(config_file_name);

    if (!config_path) {
        LOG_ERR("Settings", "Unable to get %s path.", config_file_name);
        return false;
    }

    if (!ini_browse(config_parser, &settings, config_path)) {
        LOG_ERR("Settings", "Unable to parse %s.", config_file_name);
        free(config_path);
        return false;
    }

    free(config_path);

    return true;
}

static bool create_config_folder(char *config_path) {
    char *last_slash = strrchr(config_path, '/');
    if (!last_slash) {
        last_slash = strrchr(config_path, '\\');
    }

    char *save_folder = strdup(config_path);
    save_folder[last_slash - config_path + 1] = '\0';

    if (!native_create_dir((uint8_t *)save_folder)) {
        LOG_ERR("Settings", "Failed to create save folder %s.", save_folder);
        free(save_folder);
        return false;
    }

    free(save_folder);
    return true;
}

static bool utox_save_config(void) {
    char *config_path = utox_get_filepath(config_file_name);

    if (!config_path) {
        LOG_ERR("Settings", "Unable to get %s path.", config_file_name);
        return false;
    }

    if (!create_config_folder(config_path)) {
        free(config_path);
        return false;
    }

    SETTINGS *config = &settings;

    // general
    WRITE_CONFIG_VALUE_INT(GENERAL_SECTION, config->save_version);
    WRITE_CONFIG_VALUE_INT(GENERAL_SECTION, config->utox_last_version);

    // interface
    WRITE_CONFIG_VALUE_INT(INTERFACE_SECTION, config->language);
    WRITE_CONFIG_VALUE_INT(INTERFACE_SECTION, config->window_x);
    WRITE_CONFIG_VALUE_INT(INTERFACE_SECTION, config->window_y);
    WRITE_CONFIG_VALUE_INT(INTERFACE_SECTION, config->window_width);
    WRITE_CONFIG_VALUE_INT(INTERFACE_SECTION, config->window_height);
    WRITE_CONFIG_VALUE_INT(INTERFACE_SECTION, config->theme);
    WRITE_CONFIG_VALUE_INT(INTERFACE_SECTION, config->scale);
    WRITE_CONFIG_VALUE_BOOL(INTERFACE_SECTION, config->logging_enabled);
    WRITE_CONFIG_VALUE_BOOL(INTERFACE_SECTION, config->close_to_tray);
    WRITE_CONFIG_VALUE_BOOL(INTERFACE_SECTION, config->start_in_tray);
    WRITE_CONFIG_VALUE_BOOL(INTERFACE_SECTION, config->auto_startup);
    WRITE_CONFIG_VALUE_BOOL(INTERFACE_SECTION, config->use_mini_flist);
    WRITE_CONFIG_VALUE_BOOL(INTERFACE_SECTION, config->filter);
    WRITE_CONFIG_VALUE_BOOL(INTERFACE_SECTION, config->magic_flist_enabled);
    WRITE_CONFIG_VALUE_BOOL(INTERFACE_SECTION, config->use_long_time_msg);

    // av
    WRITE_CONFIG_VALUE_BOOL(AV_SECTION, config->push_to_talk);
    WRITE_CONFIG_VALUE_BOOL(AV_SECTION, config->audio_filtering_enabled);
    WRITE_CONFIG_VALUE_INT(AV_SECTION, config->audio_device_in);
    WRITE_CONFIG_VALUE_INT(AV_SECTION, config->audio_device_out);
    WRITE_CONFIG_VALUE_INT(AV_SECTION, config->video_fps);
    // TODO: video_input_device

    // notifications
    WRITE_CONFIG_VALUE_BOOL(NOTIFICATIONS_SECTION, config->audible_notifications_enabled);
    WRITE_CONFIG_VALUE_BOOL(NOTIFICATIONS_SECTION, config->status_notifications);
    WRITE_CONFIG_VALUE_BOOL(NOTIFICATIONS_SECTION, config->no_typing_notifications);
    WRITE_CONFIG_VALUE_INT(NOTIFICATIONS_SECTION, config->group_notifications);

    // advanced
    WRITE_CONFIG_VALUE_BOOL(ADVANCED_SECTION, config->enableipv6);
    WRITE_CONFIG_VALUE_BOOL(ADVANCED_SECTION, config->disableudp);
    WRITE_CONFIG_VALUE_BOOL(ADVANCED_SECTION, config->proxyenable);
    WRITE_CONFIG_VALUE_INT(ADVANCED_SECTION, config->proxy_port);
    WRITE_CONFIG_VALUE_STR(ADVANCED_SECTION, config->proxy_ip);
    WRITE_CONFIG_VALUE_BOOL(ADVANCED_SECTION, config->force_proxy);
    WRITE_CONFIG_VALUE_BOOL(ADVANCED_SECTION, config->block_friend_requests);

    free(config_path);

    return true;
}

static void init_default_settings(void) {
    settings.enableipv6  = true;
    settings.disableudp  = false;
    settings.proxyenable = false;
    settings.force_proxy = false;

    // Allow users to override theme on the cmdline.
    if (settings.theme == UINT32_MAX) {
        settings.theme = 0;
    }

    settings.audio_filtering_enabled       = true;
    settings.audible_notifications_enabled = true;
}

void config_load(void) {
    bool config_loaded = utox_load_config();
    if (!config_loaded) {
        LOG_ERR("Settings", "Unable to load uTox settings. Use defaults.");
        init_default_settings();
    }

    /* UX Settings */
    dropdown_language.selected = dropdown_language.over = settings.language;
    if (settings.window_width < MAIN_WIDTH) {
        settings.window_width = MAIN_WIDTH;
    }
    if (settings.window_height < MAIN_HEIGHT) {
        settings.window_height = MAIN_HEIGHT;
    }

    dropdown_theme.selected = dropdown_theme.over = settings.theme;

    if (settings.scale > 30) {
        settings.scale = 30;
    } else if (settings.scale < 5) {
        settings.scale = 10;
    }
    dropdown_dpi.selected = dropdown_dpi.over = settings.scale - 5;

    switch_save_chat_history.switch_on = settings.logging_enabled;
    switch_close_to_tray.switch_on = settings.close_to_tray;
    switch_start_in_tray.switch_on = settings.start_in_tray;
    switch_auto_startup.switch_on = settings.auto_startup;
    switch_mini_contacts.switch_on = settings.use_mini_flist;
    flist_set_filter(settings.filter);
    switch_magic_sidebar.switch_on = settings.magic_flist_enabled;

    /* Network settings */
    switch_ipv6.switch_on = settings.enableipv6;
    switch_udp.switch_on = !settings.disableudp;
    switch_udp.panel.disabled = settings.force_proxy;
    switch_proxy.switch_on = settings.proxyenable != 0;
    switch_proxy_force.switch_on = settings.force_proxy;
    switch_proxy_force.panel.disabled = settings.proxyenable == 0;

    /* AV */
    switch_push_to_talk.switch_on = settings.push_to_talk;
    switch_audio_filtering.switch_on = settings.audio_filtering_enabled;
    if (settings.video_fps == 0) {
        settings.video_fps = DEFAULT_FPS;
    }
    snprintf((char *)edit_video_fps.data, edit_video_fps.data_size, "%u",
        settings.video_fps);
    edit_video_fps.length = strnlen((char *)edit_video_fps.data,
        edit_video_fps.data_size - 1);

    /* Notifications */
    switch_audible_notifications.switch_on
        = settings.audible_notifications_enabled;

    switch_status_notifications.switch_on = settings.status_notifications;
    switch_typing_notes.switch_on = !settings.no_typing_notifications;

    settings.group_notifications = dropdown_global_group_notifications.selected
        = dropdown_global_group_notifications.over
        = settings.group_notifications;

    /* Advanced */
    edit_proxy_ip.length = strnlen((char *)settings.proxy_ip, sizeof(settings.proxy_ip));
    strncpy((char *)edit_proxy_ip.data, (char *)settings.proxy_ip, edit_proxy_ip.length);

    if (settings.proxy_port) {
        snprintf((char *)edit_proxy_port.data, edit_proxy_port.data_size, "%u",
            settings.proxy_port);
        edit_proxy_port.length = strnlen((char *)edit_proxy_port.data,
            edit_proxy_port.data_size - 1);
    }

    ui_set_scale(settings.scale);

    if (settings.push_to_talk) {
        init_ptt();
    }

    if (!config_loaded) {
        config_save();
    }
}

void config_save(void) {
    settings.save_version = UTOX_SAVE_VERSION;
    settings.filter = !!flist_get_filter();
    settings.audio_device_in = dropdown_audio_in.selected;
    settings.audio_device_out = dropdown_audio_out.selected;

    LOG_NOTE("uTox", "Saving uTox settings.");

    if (!utox_save_config()) {
        LOG_ERR("uTox", "Unable to save uTox settings.");
    }
}
