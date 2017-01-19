// Needs to be defined before main.h
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "main.h"

#include "flist.h"
#include "friend.h"
#include "groups.h"
#include "logging_native.h"
#include "theme.h"
#include "updater.h"

#include "ui/dropdowns.h"
#include "ui/edits.h"
#include "ui/switches.h"

#include <getopt.h>

SETTINGS settings = {
    .curr_version = UTOX_VERSION_NUMBER,
    // .last_version                // included here to match the full struct
    .show_splash = false,

    // Low level settings (network, profile, portable-mode)
    .use_proxy      = false,
    .force_proxy    = false,
    .enable_udp     = true,
    .enable_ipv6    = true,
    .save_encryption = true,
    .block_friend_requests = false,
    // .portable_mode               // included here to match the full struct

    .save_encryption    = true,

    .auto_update        = false,
    .update_to_develop  = false,
    .send_version       = false,

    .force_proxy        = false,
    .enable_udp         = true,
    .enable_ipv6        = true,

    .use_proxy          = false,
    .proxy_port         = 0,

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
    .group_notifications    = GNOTIFY_ALWAYS,

    .verbose = 1,

    // .theme                       // included here to match the full struct
    // OS interface settings
    .window_height        = MAIN_HEIGHT,
    .window_width         = MAIN_WIDTH,
    .window_baseline      = 0,

    .window_maximized     = 0,
};

/* The utox_ functions contained in src/main.c are wrappers for the platform native_ functions
 * if you need to localize them to a specific platform, move them from here, to each
 * src/<platform>/main.x and change from utox_ to native_ */
bool utox_data_save_tox(uint8_t *data, size_t length) {
    FILE *  fp     = native_get_file((uint8_t *)"tox_save.tox", NULL, UTOX_FILE_OPTS_WRITE);
    if (fp == NULL) {
        debug("Can not open tox_save.tox to write to it.\n");
        return true;
    }

    fwrite(data, length, 1, fp);
    flush_file(fp);
    fclose(fp);

    return false;
}

uint8_t *utox_data_load_tox(size_t *size) {
    uint8_t name[][20] = { "tox_save.tox", "tox_save.tox.atomic", "tox_save.tmp", "tox_save" };

    uint8_t *data;
    FILE *   fp;
    size_t   length = 0;

    for (int i = 0; i < 4; i++) {
        fp = native_get_file(name[i], &length, UTOX_FILE_OPTS_READ);
        if (fp == NULL) {
            continue;
        }
        data = calloc(length + 1, 1);
        if (data == NULL) {
            debug("Could not allocate memory for tox save.\n");
            fclose(fp);
            return NULL; // quit were out of memory, calloc will fail again
        }
        if (fread(data, 1, length, fp) != length) {
            debug("Could not read: %s.\n", name[i]);
            fclose(fp);
            free(data);
            return NULL; // return because if this file exits we don't want to fall back to an old version, we need the
                         // user to decide
        }
        fclose(fp);
        *size = length;
        return data;
    }
    return NULL;
}

bool utox_data_save_utox(UTOX_SAVE *data, size_t size) {
    FILE *fp = native_get_file((uint8_t *)"utox_save", NULL, UTOX_FILE_OPTS_WRITE);

    if (fp == NULL) {
        return false;
    }

    fwrite(data, size, 1, fp);
    flush_file(fp);
    fclose(fp);

    return true;
}

UTOX_SAVE *utox_data_load_utox(void) {
    size_t size = 0;
    FILE *fp = native_get_file((uint8_t *)"utox_save", &size, UTOX_FILE_OPTS_READ);

    if (fp == NULL) {
        return NULL;
    }

    UTOX_SAVE *save = calloc(size + 1, 1);
    if (save == NULL) {
        fclose(fp);
        return NULL;
    }

    if (fread(save, 1, size, fp) != size) {
        debug("Could not read save file\n");
        fclose(fp);
        free(save);
        return NULL;
    }
    fclose(fp);
    return save;
}

bool utox_data_save_ftinfo(char hex[TOX_PUBLIC_KEY_SIZE * 2], uint8_t *data, size_t length) {
    uint8_t name[TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".ftinfo")];
    snprintf((char *)name, sizeof(name), "%.*s.ftinfo", TOX_PUBLIC_KEY_SIZE * 2, hex);

    FILE *fp = native_get_file((uint8_t *)name, NULL, UTOX_FILE_OPTS_WRITE);

    if (fp == NULL) {
        return false;
    }

    fwrite(data, length, 1, fp);
    flush_file(fp);
    fclose(fp);

    return true;
}

UTOX_SAVE *config_load(void) {
    UTOX_SAVE *save;
    save = utox_data_load_utox();

    if (!save) {
        debug_notice("unable to load utox_save data\n");
        /* Create and set defaults */
        save              = calloc(1, sizeof(UTOX_SAVE));
        save->enableipv6  = 1;
        save->disableudp  = 0;
        save->proxyenable = 0;

        save->audio_filtering_enabled       = 1;
        save->audible_notifications_enabled = 1;
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

    dropdown_dpi.selected = dropdown_dpi.over = save->scale - 5;
    dropdown_proxy.selected = dropdown_proxy.over = save->proxyenable <= 2 ? save->proxyenable : 2;

    switch_ipv6.switch_on               = save->enableipv6;
    switch_udp.switch_on                = !save->disableudp;
    switch_save_chat_history.switch_on  = save->logging_enabled;
    switch_mini_contacts.switch_on      = save->use_mini_flist;
    switch_auto_startup.switch_on       = save->auto_startup;
    switch_auto_update.switch_on        = save->auto_update;

    switch_close_to_tray.switch_on = save->close_to_tray;
    switch_start_in_tray.switch_on = save->start_in_tray;

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

    if (strlen((char *)save->proxy_ip) <= 256){
        strcpy((char *)proxy_address, (char *)save->proxy_ip);
    }

    edit_proxy_ip.length = strlen((char *)save->proxy_ip);

    strcpy((char *)edit_proxy_ip.data, (char *)save->proxy_ip);

    if (save->proxy_port) {
        edit_proxy_port.length =
            snprintf((char *)edit_proxy_port.data, edit_proxy_port.maxlength + 1, "%u", save->proxy_port);
        if (edit_proxy_port.length >= edit_proxy_port.maxlength + 1) {
            edit_proxy_port.length = edit_proxy_port.maxlength;
        }
    }

    settings.logging_enabled     = save->logging_enabled;
    settings.close_to_tray       = save->close_to_tray;
    settings.start_in_tray       = save->start_in_tray;
    settings.start_with_system   = save->auto_startup;
    settings.ringtone_enabled    = save->audible_notifications_enabled;
    settings.audiofilter_enabled = save->audio_filtering_enabled;
    settings.use_mini_flist      = save->use_mini_flist;

    settings.send_typing_status   = !save->no_typing_notifications;
    settings.group_notifications  = save->group_notifications;
    settings.status_notifications = save->status_notifications;

    settings.window_width  = save->window_width;
    settings.window_height = save->window_height;

    settings.last_version = save->utox_last_version;

    loaded_audio_out_device = save->audio_device_out;
    loaded_audio_in_device  = save->audio_device_in;

    settings.auto_update            = save->auto_update;
    switch_auto_update.switch_on    = save->auto_update;
    settings.update_to_develop      = save->update_to_develop;
    settings.send_version           = save->send_version;


    if (save->push_to_talk) {
        init_ptt();
    }

    return save;
}

void config_save(UTOX_SAVE *save_in) {
    UTOX_SAVE *save = calloc(1, sizeof(UTOX_SAVE) + 256);

    /* Copy the data from the in data to protect the calloc */
    save->window_x      = save_in->window_x;
    save->window_y      = save_in->window_y;
    save->window_width  = save_in->window_width;
    save->window_height = save_in->window_height;

    save->save_version                  = UTOX_SAVE_VERSION;
    save->scale                         = ui_scale - 1;
    save->proxyenable                   = dropdown_proxy.selected;
    save->logging_enabled               = settings.logging_enabled;
    save->close_to_tray                 = settings.close_to_tray;
    save->start_in_tray                 = settings.start_in_tray;
    save->auto_startup                  = settings.start_with_system;
    save->audible_notifications_enabled = settings.ringtone_enabled;
    save->audio_filtering_enabled       = settings.audiofilter_enabled;
    save->push_to_talk                  = settings.push_to_talk;
    save->use_mini_flist                = settings.use_mini_flist;

    save->disableudp              = !settings.enable_udp;
    save->enableipv6              = settings.enable_ipv6;
    save->no_typing_notifications = !settings.send_typing_status;

    save->filter     = flist_get_filter();
    save->proxy_port = settings.proxy_port;

    save->audio_device_in  = dropdown_audio_in.selected;
    save->audio_device_out = dropdown_audio_out.selected;
    save->theme            = settings.theme;

    save->utox_last_version    = settings.curr_version;
    save->group_notifications  = settings.group_notifications;
    save->status_notifications = settings.status_notifications;

    save->auto_update           = settings.auto_update;
    save->update_to_develop     = settings.update_to_develop;
    save->send_version          = settings.send_version;

    memcpy(save->proxy_ip, proxy_address, 256); /* Magic number inside toxcore */

    debug_notice("uTox:\tWriting uTox Save\n");
    utox_data_save_utox(save, sizeof(*save) + 256); /* Magic number inside toxcore */
    free(save);
}

uint8_t *utox_data_load_custom_theme(size_t *out) {
    FILE *fp = native_get_file((uint8_t *)"utox_theme.ini", out, UTOX_FILE_OPTS_READ);

    if (fp == NULL) {
        return NULL;
    }

    uint8_t *data = calloc(*out + 1, 1);
    if (data == NULL) {
        fclose(fp);
        return NULL;
    }

    if (fread(data, 1, *out, fp) != *out) {
        debug_error("Theme:\tCould not read custom theme from file\n");
        fclose(fp);
        free(data);
        return NULL;
    }
    fclose(fp);

    return data;
}

/* Shared function between all four platforms */
void parse_args(int argc, char *argv[],
                bool *skip_updater,
                bool *from_updater,
                bool *theme_was_set_on_argv,
                int8_t *should_launch_at_startup,
                int8_t *set_show_window
                ) {
    // set default options
    settings.theme = THEME_DEFAULT;
    settings.portable_mode = false;
    if (skip_updater) {
        *skip_updater = false;
    }
    if (from_updater) {
        *from_updater = false;
    }
    if (theme_was_set_on_argv) {
        *theme_was_set_on_argv = false;
    }
    if (should_launch_at_startup) {
        *should_launch_at_startup = 0;
    }
    if (set_show_window) {
        *set_show_window = 0;
    }

    static struct option long_options[] = {
        { "theme", required_argument, NULL, 't' },  { "portable", no_argument, NULL, 'p' },
        { "set", required_argument, NULL, 's' },    { "unset", required_argument, NULL, 'u' },
        { "skip-updater", no_argument, NULL, 'N' }, { "signal-updater", no_argument, NULL, 'S' },
        { "version", no_argument, NULL, 0 },
        { "silent", no_argument, NULL, 1 },        { "verbose", no_argument, NULL, 'v' },
        { "help", no_argument, NULL, 'h' },        { 0, 0, 0, 0 }
    };

    int opt, long_index = 0;
    while ((opt = getopt_long(argc, argv, "t:ps:u:nvh", long_options, &long_index)) != -1) {
        // loop through each option; ":" after each option means an argument is required
        switch (opt) {
            case 't': {
                if (!strcmp(optarg, "default")) {
                    settings.theme = THEME_DEFAULT;
                } else if (!strcmp(optarg, "dark")) {
                    settings.theme = THEME_DARK;
                } else if (!strcmp(optarg, "light")) {
                    settings.theme = THEME_LIGHT;
                } else if (!strcmp(optarg, "highcontrast")) {
                    settings.theme = THEME_HIGHCONTRAST;
                } else if (!strcmp(optarg, "zenburn")) {
                    settings.theme = THEME_ZENBURN;
                } else if (!strcmp(optarg, "solarized-light")) {
                    settings.theme = THEME_SOLARIZED_LIGHT;
                } else if (!strcmp(optarg, "solarized-dark")) {
                    settings.theme = THEME_SOLARIZED_DARK;
                } else {
                    debug_error(
                        "Please specify correct theme (please check user manual for list of correct values).\n");
                    exit(EXIT_FAILURE);
                }
                *theme_was_set_on_argv = 1;
                break;
            }

            case 'p': {
                debug("Launching uTox in portable mode: All data will be saved to the tox folder in the current "
                      "working directory\n");
                settings.portable_mode = 1;
                break;
            }

            case 's': {
                if (!strcmp(optarg, "start-on-boot")) {
                    *should_launch_at_startup = 1;
                } else if (!strcmp(optarg, "show-window")) {
                    *set_show_window = 1;
                } else if (!strcmp(optarg, "hide-window")) {
                    *set_show_window = -1;
                } else {
                    debug_error(
                        "Please specify a correct set option (please check user manual for list of correct values).\n");
                    exit(EXIT_FAILURE);
                }
                break;
            }

            case 'u': {
                if (!strcmp(optarg, "start-on-boot")) {
                    *should_launch_at_startup = -1;
                } else {
                    debug_error("Please specify a correct unset option (please check user manual for list of correct "
                                "values).\n");
                    exit(EXIT_FAILURE);
                }
                break;
            }

            case 'N': {
                if (skip_updater) {
                    *skip_updater = true;
                }
                break;
            }
            case 'S': {
                if (from_updater) {
                    *from_updater = true;
                }
                break;
            }

            case 0: {
                debug_error("uTox version: %s\n", VERSION);
                #ifdef GIT_VERSION
                debug_error("git version %s\n", GIT_VERSION);
                #endif
                exit(EXIT_SUCCESS);
                break;
            }

            case 1: {
                settings.verbose = 0;
                break;
            }

            case 'v': {
                settings.verbose++;
                break;
            }

            case 'h': {
                debug_error("µTox - Lightweight Tox client version %s.\n\n", VERSION);
                debug_error("The following options are available:\n");
                debug_error("  -t --theme=<theme-name>  Specify a UI theme, where <theme-name> can be one of default, "
                            "dark, light, highcontrast, zenburn.\n");
                debug_error("  -p --portable            Launch in portable mode: All data will be saved to the tox "
                            "folder in the current working directory.\n");
                debug_error("  -s --set=<option>        Set an option: start-on-boot, show-window, hide-window.\n");
                debug_error("  -u --unset=<option>      Unset an option: start-on-boot.\n");
                debug_error("  -n --no-updater          Disable the updater.\n");
                debug_error("  -v --verbose             Increase the amount of output, use -v multiple times to get "
                            "full debug output.\n");
                debug_error("  -h --help                Shows this help text.\n");
                debug_error("  --version                Print the version and exit.\n");
                debug_error("  --silent                 Set the verbosity level to 0, disable all debugging output.\n");
                exit(EXIT_SUCCESS);
                break;
            }

            case '?': debug("Invalid option: %c!\n", (char)optopt); break;
        }
    }
}

void utox_init(void) {
    /* Called by the native main for every platform after loading utox setting, before showing/drawing any windows. */
    if (settings.curr_version != settings.last_version) {
        settings.show_splash = 1;
    }

    if (settings.auto_update) {
        updater_check();
    }
}

/** Android is still a bit legacy, so we just include it all here. */
#if defined __ANDROID__
#include "android/main.c"
#endif
