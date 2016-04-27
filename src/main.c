#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "main.h"
#include <getopt.h>

/* The utox_ functions contained in src/main.c are wrappers for the platform native_ functions
 * if you need to localize them to a specific platform, move them from here, to each
 * src/<platform>/main.x and change from utox_ to native_ */
_Bool utox_save_data_tox(uint8_t *data, size_t length){
    uint8_t name[] = "tox_save.tox";
    return native_save_data(name, strlen((const char*)name), data, length, 0);
}

_Bool utox_save_data_utox(UTOX_SAVE *data, size_t length){
    uint8_t name[] = "utox_save";
    return native_save_data(name, strlen((const char*)name), (const uint8_t*)data, length, 0);
}

_Bool utox_save_data_log(uint32_t friend_number, uint8_t *data, size_t length) {
    FRIEND *f = &friend[friend_number];
    uint8_t hex[TOX_PUBLIC_KEY_SIZE * 2];
    uint8_t name[TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".new.txt")];
    cid_to_string(hex, f->cid);
    snprintf((char*)name, TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".new.txt"), "%.*s.new.txt", TOX_PUBLIC_KEY_SIZE * 2, (char*)hex);

    return native_save_data(name, strlen((const char*)name), (const uint8_t*)data, length, 1);
}

uint8_t *utox_load_data_tox(size_t *size){
    uint8_t name[][20] = { "tox_save.tox",
                           "tox_save.tox.atomic",
                           "tox_save.tmp",
                           "tox_save"
    };

    uint8_t *data;

    for (int i = 0; i < 4; i++) {
        data = native_load_data(name[i], strlen((const char*)name[i]), size);
        if (data) {
            return data;
        } else {
            debug("NATIVE:\tUnable to load %s\n", name[i]);
        }
    }
    return NULL;
}

UTOX_SAVE *utox_load_data_utox(void){
    uint8_t name[] = "utox_save";
    return (UTOX_SAVE*)native_load_data(name, strlen((const char*)name), NULL);
}

/* Shared function between all four platforms */
void parse_args(int argc, char *argv[], bool *theme_was_set_on_argv, int8_t *should_launch_at_startup, int8_t *set_show_window, bool *no_updater) {
    // set default options
    theme = THEME_DEFAULT; // global declaration
    utox_portable = false; // global declaration
    *theme_was_set_on_argv = false;
    *should_launch_at_startup = 0;
    *set_show_window = 0;
    *no_updater = false;

    static struct option long_options[] = {
        {"theme",      required_argument,   NULL,  't'},
        {"portable",   no_argument,         NULL,  'p'},
        {"set",        required_argument,   NULL,  's'},
        {"unset",      required_argument,   NULL,  'u'},
        {"no-updater", no_argument,         NULL,  'n'},
        {"version",    no_argument,         NULL,  'v'},
        {"help",       no_argument,         NULL,  'h'},
        {0, 0, 0, 0}
    };

    int opt, long_index =0;
    while ((opt = getopt_long(argc, argv,"t:ps:u:nvh", long_options, &long_index )) != -1) {
        // loop through each option; ":" after each option means an argument is required
        switch (opt) {
            case 't':
                if (!strcmp(optarg, "default")) {
                    theme = THEME_DEFAULT;
                } else if (!strcmp(optarg, "dark")) {
                    theme = THEME_DARK;
                } else if (!strcmp(optarg, "light")) {
                    theme = THEME_LIGHT;
                } else if (!strcmp(optarg, "highcontrast")) {
                    theme = THEME_HIGHCONTRAST;
                } else if (!strcmp(optarg, "zenburn")) {
                    theme = THEME_ZENBURN;
                } else {
                    debug("Please specify correct theme (please check user manual for list of correct values).\n");
                    exit(EXIT_FAILURE);
                }
                *theme_was_set_on_argv = 1;
                break;

            case 'p':
                debug("Launching uTox in portable mode: All data will be saved to the tox folder in the current working directory\n");
                utox_portable = 1;
                break;

            case 's':
                if (!strcmp(optarg, "start-on-boot")) {
                    *should_launch_at_startup = 1;
                } else if (!strcmp(optarg, "show-window")) {
                    *set_show_window = 1;
                } else if (!strcmp(optarg, "hide-window")) {
                    *set_show_window = -1;
                } else {
                    debug("Please specify a correct set option (please check user manual for list of correct values).\n");
                    exit(EXIT_FAILURE);
                }
                break;

            case 'u':
                if (!strcmp(optarg, "start-on-boot")) {
                    *should_launch_at_startup = -1;
                } else {
                    debug("Please specify a correct unset option (please check user manual for list of correct values).\n");
                    exit(EXIT_FAILURE);
                }
                break;

            case 'n':
                *no_updater = 1;
                break;

            case 'v':
                debug("uTox version: %s\n", VERSION);
                exit(EXIT_SUCCESS);
                break;

            case 'h':
                debug("µTox - Lightweight Tox client version %s.\n\n", VERSION);
                debug("The following options are available:\n");
                debug("  -t --theme=<theme-name>  Specify a UI theme, where <theme-name> can be one of default, dark, light, highcontrast, zenburn.\n");
                debug("  -p --portable            Launch in portable mode: All data will be saved to the tox folder in the current working directory.\n");
                debug("  -s --set=<option>        Set an option: start-on-boot, show-window, hide-window.\n");
                debug("  -u --unset=<option>      Unset an option: start-on-boot.\n");
                debug("  -n --no-updater          Disable the updater.\n");
                debug("  -v --version             Print the version and exit.\n");
                debug("  -h --help                Shows this help text.\n");
                exit(EXIT_SUCCESS);
                break;

            case '?':
                debug("Invalid option: %c!\n", (char) optopt);
                break;
        }
    }
}

/** Change source of main.c if windows or android
 *  else default to xlib
 **/
#if defined __WIN32__
  //#include "windows/main.c"
#elif defined __ANDROID__
  #include "android/main.c"
#elif defined__OBJC__
  // #include "cocoa/main.m"
#else
  // #include "xlib/main.c"
#endif
