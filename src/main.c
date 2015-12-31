#include "main.h"

/* Shared function between all four platforms */

void parseArgs(int argc, char *argv[], _Bool *theme_was_set_on_argv, int32_t *launch_at_startup, int32_t *set_show_window, _Bool *no_updater) {
    // set default options
    theme = THEME_DEFAULT; // global declaration
    utox_portable = 0; // global declaration
    *theme_was_set_on_argv = 0;
    *launch_at_startup = 0;
    *set_show_window = 0;
    *no_updater = 0;
    
    int opt;
    while ((opt = getopt(argc, argv, "t:ps:u:nvh")) != -1 ) { // loop through each option; ":" after each option means an argument is required
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
                    debug("Please specify correct theme (please check user manual for list of correct values).");
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
                    *launch_at_startup = 1;
                } else if (!strcmp(optarg, "show-window")) {
                    *set_show_window = 1;
                } else if (!strcmp(optarg, "hide-window")) {
                    *set_show_window = -1;
                } else {
                    debug("Please specify a correct set option (please check user manual for list of correct values).");
                    exit(EXIT_FAILURE);
                }
                break;
                
            case 'u':
                if (!strcmp(optarg, "start-on-boot")) {
                    *launch_at_startup = -1;
                } else {
                    debug("Please specify a correct unset option (please check user manual for list of correct values).");
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
                printf("µTox - Lightweight Tox client version %s.\n\n", VERSION);
                printf("The following options are available:\n\n");
                printf("  -t --theme <theme-name>  Specify a UI theme, where <theme-name> can be one of default, dark, light, highcontrast, zenburn.\n");
                printf("  -p --portable            Launch in portable mode: All data will be saved to the tox folder in the current working directory.\n");
                printf("  -s --set <option>        Set an option: start-on-boot, show-window, hide-window.\n");
                printf("  -u --unset <option>      Unset an option: start-on-boot.\n");
                printf("  -n --no-updater          Disable the updater.\n");
                printf("  -v --version             Print the version and exit.\n");
                printf("  -h --help                Shows this help text.\n\n");
                exit(EXIT_SUCCESS);
                break;
                
            case '?':
                printf("Invalid option: %c!\n", (char) optopt);
                break;
        }
    }
}

/** Change source of main.c if windows or android
 *  else default to xlib
 **/
#ifdef __WIN32__
//#include "windows/main.c"
#else
#ifdef __ANDROID__
#include "android/main.c"
#else
#ifdef __OBJC__
// #include "cocoa/main.m"
#else
// #include "xlib/main.c"
#endif
#endif
#endif
