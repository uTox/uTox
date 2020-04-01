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

#include "main.h" // UTOX_VERSION_NUMBER, MAIN_HEIGHT, MAIN_WIDTH, all save things..

#include <stdlib.h>
#include <string.h>
#include <minIni.h>

#define MATCH(x, y) (strcasecmp(x, y) == 0)
#define BOOL_TO_STR(b) b ? "true" : "false"
#define STR_TO_BOOL(s) (strcasecmp(s, "true") == 0)
#define NAMEOF(s) strchr((const char *)(#s), '>') == NULL ? #s : (strchr((const char *)(#s), '>') + 1)

uint16_t loaded_audio_out_device = 0;
uint16_t loaded_audio_in_device  = 0;

static const char *config_file_name     = "utox_save.ini";
static const char *config_file_name_old = "utox_save";

static const uint16_t proxy_address_size = 256; // Magic number inside Toxcore.

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
    // .last_version                // included here to match the full struct
    .curr_version = UTOX_VERSION_NUMBER,
    .next_version = UTOX_VERSION_NUMBER,

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

    // .portable_mode               // included here to match the full struct

    // User interface settings
    .language               = LANG_EN,
    .audiofilter_enabled    = true,
    .push_to_talk           = false,
    .audio_preview          = false,
    .video_preview          = false,
    .send_typing_status     = false,
    // .inline_video                // included here to match the full struct
    .use_long_time_msg      = true,
    .accept_inline_images   = true,

    // UX Settings
    .logging_enabled        = true,
    .close_to_tray          = false,
    .start_in_tray          = false,
    .start_with_system      = false,
    .use_mini_flist         = false,
    .magic_flist_enabled    = false,

    .video_fps              = DEFAULT_FPS,

    // Notifications / Alerts
    .ringtone_enabled       = true,
    .status_notifications   = true,
    .group_notifications    = GNOTIFY_ALWAYS,

    .verbose = LOG_LVL_ERROR,
    .debug_file = NULL,

    .theme                = UINT32_MAX,
    // OS interface settings
    .window_x             = 0,
    .window_y             = 0,
    .window_height        = MAIN_HEIGHT,
    .window_width         = MAIN_WIDTH,
    .window_baseline      = 0,

    .window_maximized     = 0,
};

static void write_config_value_int(const char *filename, const char *section, const char *key, const long value) {
    if (ini_putl(section, key, value, filename) != 1) {
        LOG_ERR("Settings", "Unable to save config value: %lu.", value);
    }
}

static void write_config_value_str(const char *filename, const char *section, const char *key, const char *value) {
    if (ini_puts(section, key, value, filename) != 1) {
        LOG_ERR("Settings", "Unable to save config value: %s.", value);
    }
}

static void write_config_value_bool(const char *filename, const char *section, const char *key, const bool value) {
    if (ini_puts(section, key, BOOL_TO_STR(value), filename) != 1) {
        LOG_ERR("Settings", "Unable to save config value: %s.", value);
    }
}

static CONFIG_SECTION get_section(const char* section) {
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

static void parse_general_section(UTOX_SAVE *config, const char* key, const char* value) {
    if (MATCH(NAMEOF(config->save_version), key)) {
        config->save_version = atoi(value);
    } else if (MATCH(NAMEOF(config->utox_last_version), key)) {
        config->utox_last_version = atoi(value);
    } else if (MATCH(NAMEOF(config->send_version), key)) {
        config->send_version = STR_TO_BOOL(value);
    } else if (MATCH(NAMEOF(config->update_to_develop), key)) {
        config->update_to_develop = STR_TO_BOOL(value);
    }
}

static void parse_interface_section(UTOX_SAVE *config, const char* key, const char* value) {
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
        config->theme = atoi(value);
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

static void parse_av_section(UTOX_SAVE *config, const char* key, const char* value) {
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

        LOG_WARN("Settings", "Fps value (%s) is invalid. It must be integer in range of [1,%u].",
             value, UINT8_MAX);
    }
}

static void parse_notifications_section(UTOX_SAVE *config, const char* key, const char* value) {
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

static void parse_advanced_section(UTOX_SAVE *config, const char* key, const char* value) {
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
    } else if (MATCH(NAMEOF(config->auto_update), key)) {
        config->auto_update = STR_TO_BOOL(value);
    }
}

static int config_parser(const char* section, const char* key, const char* value, void* config_v) {
    UTOX_SAVE *config = (UTOX_SAVE*) config_v;

    switch(get_section(section)) {
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

static UTOX_SAVE *utox_load_config(void) {
    UTOX_SAVE *save = calloc(1, sizeof(UTOX_SAVE) + proxy_address_size + 1);
    if (!save) {
        LOG_ERR("Settings", "Unable to calloc for UTOX_SAVE.");
        return NULL;
    }

    char *config_path = utox_get_filepath(config_file_name);

    if (!config_path) {
        LOG_ERR("Settings", "Unable to get %s path.", config_file_name);
        free(save);
        return NULL;
    }

    if (!ini_browse(config_parser, save, config_path)) {
        LOG_ERR("Settings", "Unable to parse %s.", config_file_name);
        free(config_path);
        free(save);
        return NULL;
    }

    free(config_path);

    return save;
}

static bool utox_save_config(UTOX_SAVE *config) {
    char *config_path = utox_get_filepath(config_file_name);

    if (!config_path) {
        LOG_ERR("Settings", "Unable to get %s path.", config_file_name);
        return false;
    }

    // general
    write_config_value_int(config_path, config_sections[GENERAL_SECTION], NAMEOF(config->save_version), config->save_version);
    write_config_value_int(config_path, config_sections[GENERAL_SECTION], NAMEOF(config->utox_last_version), config->utox_last_version);
    write_config_value_bool(config_path, config_sections[GENERAL_SECTION], NAMEOF(config->send_version), config->send_version);
    write_config_value_bool(config_path, config_sections[GENERAL_SECTION], NAMEOF(config->update_to_develop), config->update_to_develop);

    // interface
    write_config_value_int(config_path, config_sections[INTERFACE_SECTION], NAMEOF(config->language), config->language);
    write_config_value_int(config_path, config_sections[INTERFACE_SECTION], NAMEOF(config->window_x), config->window_x);
    write_config_value_int(config_path, config_sections[INTERFACE_SECTION], NAMEOF(config->window_y), config->window_y);
    write_config_value_int(config_path, config_sections[INTERFACE_SECTION], NAMEOF(config->window_width), config->window_width);
    write_config_value_int(config_path, config_sections[INTERFACE_SECTION], NAMEOF(config->window_height), config->window_height);
    write_config_value_int(config_path, config_sections[INTERFACE_SECTION], NAMEOF(config->theme), config->theme);
    write_config_value_int(config_path, config_sections[INTERFACE_SECTION], NAMEOF(config->scale), config->scale);
    write_config_value_bool(config_path, config_sections[INTERFACE_SECTION], NAMEOF(config->logging_enabled), config->logging_enabled);
    write_config_value_bool(config_path, config_sections[INTERFACE_SECTION], NAMEOF(config->close_to_tray), config->close_to_tray);
    write_config_value_bool(config_path, config_sections[INTERFACE_SECTION], NAMEOF(config->start_in_tray), config->start_in_tray);
    write_config_value_bool(config_path, config_sections[INTERFACE_SECTION], NAMEOF(config->auto_startup), config->auto_startup);
    write_config_value_bool(config_path, config_sections[INTERFACE_SECTION], NAMEOF(config->use_mini_flist), config->use_mini_flist);
    write_config_value_bool(config_path, config_sections[INTERFACE_SECTION], NAMEOF(config->filter), config->filter);
    write_config_value_bool(config_path, config_sections[INTERFACE_SECTION], NAMEOF(config->magic_flist_enabled), config->magic_flist_enabled);
    write_config_value_bool(config_path, config_sections[INTERFACE_SECTION], NAMEOF(config->use_long_time_msg), config->use_long_time_msg);

    // av
    write_config_value_bool(config_path, config_sections[AV_SECTION], NAMEOF(config->push_to_talk), config->push_to_talk);
    write_config_value_bool(config_path, config_sections[AV_SECTION], NAMEOF(config->audio_filtering_enabled), config->audio_filtering_enabled);
    write_config_value_int(config_path, config_sections[AV_SECTION], NAMEOF(config->audio_device_in), config->audio_device_in);
    write_config_value_int(config_path, config_sections[AV_SECTION], NAMEOF(config->audio_device_out), config->audio_device_out);
    write_config_value_int(config_path, config_sections[AV_SECTION], NAMEOF(config->video_fps), config->video_fps);
    // TODO: video_input_device

    // notifications
    write_config_value_bool(config_path, config_sections[NOTIFICATIONS_SECTION], NAMEOF(config->audible_notifications_enabled), config->audible_notifications_enabled);
    write_config_value_bool(config_path, config_sections[NOTIFICATIONS_SECTION], NAMEOF(config->status_notifications), config->status_notifications);
    write_config_value_bool(config_path, config_sections[NOTIFICATIONS_SECTION], NAMEOF(config->no_typing_notifications), config->no_typing_notifications);
    write_config_value_int(config_path, config_sections[NOTIFICATIONS_SECTION], NAMEOF(config->group_notifications), config->group_notifications);

    // advanced
    write_config_value_bool(config_path, config_sections[ADVANCED_SECTION], NAMEOF(config->enableipv6), config->enableipv6);
    write_config_value_bool(config_path, config_sections[ADVANCED_SECTION], NAMEOF(config->disableudp), config->disableudp);
    write_config_value_bool(config_path, config_sections[ADVANCED_SECTION], NAMEOF(config->proxyenable), config->proxyenable);
    write_config_value_int(config_path, config_sections[ADVANCED_SECTION], NAMEOF(config->proxy_port), config->proxy_port);
    write_config_value_str(config_path, config_sections[ADVANCED_SECTION], NAMEOF(config->proxy_ip), (const char *)config->proxy_ip);
    write_config_value_bool(config_path, config_sections[ADVANCED_SECTION], NAMEOF(config->force_proxy), config->force_proxy);
    write_config_value_bool(config_path, config_sections[ADVANCED_SECTION], NAMEOF(config->auto_update), config->auto_update);
    // TODO: block_friend_requests

    free(config_path);

    return true;
}

static UTOX_SAVE *init_default_settings(void) {
    UTOX_SAVE *save = calloc(1, sizeof(UTOX_SAVE));
    if (!save) {
        LOG_FATAL_ERR(EXIT_MALLOC, "Settings", "Unable to malloc for default settings.");
    }

    save->enableipv6  = true;
    save->disableudp  = false;
    save->proxyenable = false;
    save->force_proxy = false;

    save->audio_filtering_enabled       = true;
    save->audible_notifications_enabled = true;

    return save;
}

// TODO refactor to match same order in main.h
UTOX_SAVE *config_load(void) {
    UTOX_SAVE *save = utox_load_config();

    // TODO: Remove this in ~0.18.0 release
    if (!save) {
        LOG_NOTE("Settings", "Unable to load uTox settings from %s. Trying old %s.", config_file_name, config_file_name_old);
        save = utox_data_load_utox();
    }

    if (!save) {
        LOG_ERR("Settings", "Unable to load uTox settings. Use defaults.");
        save = init_default_settings();
    }

    if (save->scale > 30) {
        save->scale = 30;
    } else if (save->scale < 5) {
        save->scale = 10;
    }

    if (save->window_width < MAIN_WIDTH) {
        save->window_width = MAIN_WIDTH;
    }
    if (save->window_height < MAIN_HEIGHT) {
        save->window_height = MAIN_HEIGHT;
    }

    /* UX Settings */

    dropdown_language.selected = dropdown_language.over = settings.language = save->language;

    dropdown_dpi.selected = dropdown_dpi.over = save->scale - 5;

    switch_save_chat_history.switch_on = save->logging_enabled;
    switch_close_to_tray.switch_on     = save->close_to_tray;
    switch_start_in_tray.switch_on     = save->start_in_tray;
    switch_mini_contacts.switch_on     = save->use_mini_flist;
    switch_magic_sidebar.switch_on     = save->magic_flist_enabled;

    switch_ipv6.switch_on             = save->enableipv6;
    switch_udp.switch_on              = !save->disableudp;
    switch_udp.panel.disabled         = save->force_proxy;
    switch_proxy.switch_on            = save->proxyenable;
    switch_proxy_force.switch_on      = save->force_proxy;
    switch_proxy_force.panel.disabled = !save->proxyenable;

    switch_auto_startup.switch_on = save->auto_startup;

    settings.group_notifications = dropdown_global_group_notifications.selected =
        dropdown_global_group_notifications.over = save->group_notifications;

    switch_audible_notifications.switch_on = save->audible_notifications_enabled;
    switch_audio_filtering.switch_on       = save->audio_filtering_enabled;
    switch_push_to_talk.switch_on          = save->push_to_talk;
    switch_status_notifications.switch_on  = save->status_notifications;

    dropdown_theme.selected = dropdown_theme.over = save->theme;

    switch_typing_notes.switch_on = !save->no_typing_notifications;

    flist_set_filter(save->filter); /* roster list filtering */

    /* Network settings */
    settings.enable_ipv6 = save->enableipv6;
    settings.enable_udp  = !save->disableudp;
    settings.use_proxy   = !!save->proxyenable;
    settings.proxy_port  = save->proxy_port;
    settings.force_proxy = save->force_proxy;

    if (strlen((char *)save->proxy_ip) <= proxy_address_size){
        strcpy((char *)proxy_address, (char *)save->proxy_ip);
    }

    edit_proxy_ip.length = strlen((char *)save->proxy_ip);

    strcpy((char *)edit_proxy_ip.data, (char *)save->proxy_ip);

    if (save->proxy_port) {
        snprintf((char *)edit_proxy_port.data, edit_proxy_port.data_size,
                 "%u", save->proxy_port);
        edit_proxy_port.length = strnlen((char *)edit_proxy_port.data,
                                         edit_proxy_port.data_size - 1);
    }

    /* UX settings */
    settings.logging_enabled      = save->logging_enabled;
    settings.close_to_tray        = save->close_to_tray;
    settings.start_in_tray        = save->start_in_tray;
    settings.start_with_system    = save->auto_startup;
    settings.use_mini_flist       = save->use_mini_flist;
    settings.magic_flist_enabled  = save->magic_flist_enabled;
    settings.use_long_time_msg    = save->use_long_time_msg;

    settings.ringtone_enabled     = save->audible_notifications_enabled;
    settings.audiofilter_enabled  = save->audio_filtering_enabled;

    settings.send_typing_status   = !save->no_typing_notifications;
    settings.status_notifications = save->status_notifications;

    settings.window_x             = save->window_x;
    settings.window_y             = save->window_y;
    settings.window_width         = save->window_width;
    settings.window_height        = save->window_height;

    settings.last_version         = save->utox_last_version;

    loaded_audio_out_device       = save->audio_device_out;
    loaded_audio_in_device        = save->audio_device_in;

    settings.video_fps = save->video_fps != 0 ? save->video_fps : DEFAULT_FPS;

    snprintf((char *)edit_video_fps.data, edit_video_fps.data_size,
             "%u", settings.video_fps);
    edit_video_fps.length = strnlen((char *)edit_video_fps.data,
                                    edit_video_fps.data_size - 1);

    // TODO: Don't clobber (and start saving) commandline flags.

    // Allow users to override theme on the cmdline.
    if (settings.theme == UINT32_MAX) {
        settings.theme = save->theme;
    }

    ui_set_scale(save->scale);

    if (save->push_to_talk) {
        init_ptt();
    }

    return save;
}

// TODO refactor to match order in main.h
void config_save(UTOX_SAVE *save_in) {
    UTOX_SAVE *save = calloc(1, sizeof(UTOX_SAVE) + proxy_address_size);
    if (!save) {
        LOG_ERR("Settings", "Could not allocate memory to save settings");
        return;
    }

    /* Copy the data from the in data to protect the calloc */
    save->window_x                      = save_in->window_x;
    save->window_y                      = save_in->window_y;
    save->window_width                  = save_in->window_width;
    save->window_height                 = save_in->window_height;

    save->save_version                  = UTOX_SAVE_VERSION;
    save->scale                         = ui_scale;
    save->proxyenable                   = switch_proxy.switch_on;
    save->audible_notifications_enabled = settings.ringtone_enabled;
    save->audio_filtering_enabled       = settings.audiofilter_enabled;
    save->push_to_talk                  = settings.push_to_talk;

    /* UX Settings */
    save->logging_enabled               = settings.logging_enabled;
    save->close_to_tray                 = settings.close_to_tray;
    save->start_in_tray                 = settings.start_in_tray;
    save->auto_startup                  = settings.start_with_system;
    save->use_mini_flist                = settings.use_mini_flist;
    save->magic_flist_enabled           = settings.magic_flist_enabled;
    save->use_long_time_msg             = settings.use_long_time_msg;
    save->video_fps                     = settings.video_fps;

    save->disableudp                    = !settings.enable_udp;
    save->enableipv6                    = settings.enable_ipv6;
    save->no_typing_notifications       = !settings.send_typing_status;

    save->filter                        = flist_get_filter();
    save->proxy_port                    = settings.proxy_port;
    save->force_proxy                   = settings.force_proxy;

    save->audio_device_in               = dropdown_audio_in.selected;
    save->audio_device_out              = dropdown_audio_out.selected;
    save->theme                         = settings.theme;

    save->utox_last_version             = settings.curr_version;
    save->group_notifications           = settings.group_notifications;
    save->status_notifications          = settings.status_notifications;

    save->language                      = settings.language;

    memcpy(save->proxy_ip, proxy_address, proxy_address_size);

    LOG_NOTE("uTox", "Saving uTox settings.");

    if (!utox_save_config(save)) {
        LOG_ERR("uTox", "Unable to save uTox settings.");
    }

    free(save);
}

// TODO: Remove this in ~0.18.0 release
UTOX_SAVE *utox_data_load_utox(void) {
    size_t size = 0;
    FILE *fp = utox_get_file(config_file_name_old, &size, UTOX_FILE_OPTS_READ);

    if (!fp) {
        LOG_ERR("Settings", "Unable to open %s.", config_file_name_old);
        return NULL;
    }

    UTOX_SAVE *save = calloc(1, size + 1);
    if (!save) {
        LOG_ERR("Settings", "Unable to malloc for %s.", config_file_name_old);
        fclose(fp);
        return NULL;
    }

    if (fread(save, size, 1, fp) != 1) {
        LOG_ERR("Settings", "Could not read save file %s", config_file_name_old);
        fclose(fp);
        free(save);
        return NULL;
    }

    fclose(fp);
    return save;
}
