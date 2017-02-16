// Needs to be defined before main.h
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "main.h"

#include "debug.h"
#include "flist.h"
#include "friend.h"
#include "groups.h"
#include "main_native.h"
#include "settings.h"
#include "theme.h"
#include "updater.h"

#include <getopt.h>

/* The utox_ functions contained in src/main.c are wrappers for the platform native_ functions
 * if you need to localize them to a specific platform, move them from here, to each
 * src/<platform>/main.x and change from utox_ to native_
 */

bool utox_data_save_tox(uint8_t *data, size_t length) {
    FILE *fp= native_get_file((uint8_t *)"tox_save.tox", NULL, UTOX_FILE_OPTS_WRITE);
    if (fp == NULL) {
        LOG_ERR(__FILE__, "Can not open tox_save.tox to write to it.");
        return true;
    }

    if (fwrite(data, length, 1, fp) != 1) {
        LOG_ERR(__FILE__, "Unable to write Tox save to file.");
        return true;
    }

    flush_file(fp);
    fclose(fp);

    return false;
}

uint8_t *utox_data_load_tox(size_t *size) {
    const uint8_t name[][20] = { "tox_save.tox", "tox_save.tox.atomic", "tox_save.tmp", "tox_save" };

    for (uint8_t i = 0; i < 4; i++) {
        size_t length = 0;

        const FILE *fp = native_get_file(name[i], &length, UTOX_FILE_OPTS_READ);
        if (fp == NULL) {
            continue;
        }

        uint8_t *data = calloc(1, length + 1);

        if (data == NULL) {
            LOG_ERR(__FILE__, "Could not allocate memory for tox save.");
            fclose(fp);
            // Quit. We're out of memory, calloc will fail again.
            return NULL;
        }

        if (fread(data, length, 1, fp) != 1) {
            LOG_ERR(__FILE__, "Could not read: %s.", name[i]);
            fclose(fp);
            free(data);
            // Return NULL, because if a Tox save exits we don't want to fall
            // back to an old version, we need the user to decide what to do.
            return NULL;
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

    if (fwrite(data, size, 1, fp) != 1) {
        LOG_ERR(__FILE__, "Unable to write uTox settings to file.");
        return false;
    }

    flush_file(fp);
    fclose(fp);

    return true;
}

UTOX_SAVE *utox_data_load_utox(void) {
    size_t size = 0;
    const FILE *fp = native_get_file((uint8_t *)"utox_save", &size, UTOX_FILE_OPTS_READ);

    if (fp == NULL) {
        return NULL;
    }

    const UTOX_SAVE *save = calloc(1, size + 1);
    if (save == NULL) {
        fclose(fp);
        return NULL;
    }

    if (fread(save, size, 1, fp) != 1) {
        LOG_ERR(__FILE__, "Could not read save file");

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

    if (fwrite(data, length, 1, fp) != 1) {
        LOG_ERR(__FILE__, "Unable to write ftinfo to file.");
        fclose(fp);
        return false;
    }

    fclose(fp);

    return true;
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
        { "help", no_argument, NULL, 'h' },        { "debug", required_argument, NULL, 2 },
        { 0, 0, 0, 0 }
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
                    LOG_NORM("Please specify correct theme (please check user manual for list of correct values).\n");
                    exit(EXIT_FAILURE);
                }
                *theme_was_set_on_argv = 1;
                break;
            }

            case 'p': {
                LOG_INFO("uTox", "Launching uTox in portable mode: All data will be saved to the tox folder in the current "
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
                    LOG_NORM("Please specify a correct set option (please check user manual for list of correct values).\n");
                    exit(EXIT_FAILURE);
                }
                break;
            }

            case 'u': {
                if (!strcmp(optarg, "start-on-boot")) {
                    *should_launch_at_startup = -1;
                } else {
                    LOG_ERR("", "Please specify a correct unset option (please check user manual for list of correct "
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
                LOG_NORM("uTox version: %s\n", VERSION);
                #ifdef GIT_VERSION
                LOG_NORM("git version %s\n", GIT_VERSION);
                #endif
                exit(EXIT_SUCCESS);
                break;
            }

            case 1: {
                settings.verbose = LOG_LVL_FATAL;
                break;
            }

            case 'v': {
                settings.verbose++;
                break;
            }

            case 2: {
                settings.debug_file = fopen(optarg, "a+");
                if (!settings.debug_file) {
                    settings.debug_file = stdout;
                    LOG_NORM("Could not open %s. Logging to stdout.\n", optarg);
                }
                break;
            }

            case 'h': {
                LOG_NORM("µTox - Lightweight Tox client version %s.\n\n", VERSION);
                LOG_NORM("The following options are available:\n");
                LOG_NORM("  -t --theme=<theme-name>  Specify a UI theme, where <theme-name> can be one of default, "
                            "dark, light, highcontrast, zenburn.\n");
                LOG_NORM("  -p --portable            Launch in portable mode: All data will be saved to the tox "
                            "folder in the current working directory.\n");
                LOG_NORM("  -s --set=<option>        Set an option: start-on-boot, show-window, hide-window.\n");
                LOG_NORM("  -u --unset=<option>      Unset an option: start-on-boot.\n");
                LOG_NORM("  -n --no-updater          Disable the updater.\n");
                LOG_NORM("  -v --verbose             Increase the amount of output, use -v multiple times to get "
                            "full debug output.\n");
                LOG_NORM("  -h --help                Shows this help text.\n");
                LOG_NORM("  --version                Print the version and exit.\n");
                LOG_NORM("  --silent                 Set the verbosity level to 0, disable all debugging output.\n");
                LOG_NORM("  --debug                  Set a file for utox to log errors to.\n");
                exit(EXIT_SUCCESS);
                break;
            }

            case '?': LOG_TRACE(__FILE__, ", (char)optopt" ); break;
        }
    }
}

void utox_init(void) {
    /* Called by the native main for every platform after loading utox setting, before showing/drawing any windows. */
    if (settings.curr_version != settings.last_version) {
        settings.show_splash = true;
    }

    // TODO(grayhatter)
    // if (settings.auto_update) {
    //     updater_check();
    // }

    settings.debug_file = stdout;
}

void utox_raze(void) {
    if (settings.debug_file != stdout) {
        fclose(settings.debug_file);
    }
}

/** Android is still a bit legacy, so we just include it all here. */
#if defined __ANDROID__
#include "android/main.c"
#endif
