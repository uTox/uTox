#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <getopt.h>

#include "main.h"

#include "friend.h"
#include "groups.h"
#include "theme.h"
#include "util.h"

SETTINGS settings = {
    .curr_version = UTOX_VERSION_NUMBER,
    // .last_version                // included here to match the full struct
    .show_splash = false,

    .use_proxy      = false,
    .force_proxy    = false,
    .enable_udp     = true,
    .enable_ipv6    = true,
    .use_encryption = true,
    // .portable_mode               // included here to match the full struct

    .proxy_port = 0,

    .close_to_tray          = false,
    .logging_enabled        = true,
    .ringtone_enabled       = true,
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
    .status_notifications   = true,

    .group_notifications    = GNOTIFY_ALWAYS,

    .verbose = 1,

    .window_height        = 600,
    .window_width         = 800,
    .window_baseline      = 0,
    .window_maximized     = 0,
};

/* The utox_ functions contained in src/main.c are wrappers for the platform native_ functions
 * if you need to localize them to a specific platform, move them from here, to each
 * src/<platform>/main.x and change from utox_ to native_ */
bool utox_data_save_tox(uint8_t *data, size_t length) {
    FILE *  fp     = native_get_file("tox_save.tox", NULL, UTOX_FILE_OPTS_WRITE);
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
        fp = native_get_file((char *)name[i], &length, UTOX_FILE_OPTS_READ);
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
    FILE *fp = native_get_file("utox_save", NULL, UTOX_FILE_OPTS_WRITE);

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
    FILE *fp = native_get_file("utox_save", &size, UTOX_FILE_OPTS_READ);

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
    char name[TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".ftinfo")];
    snprintf(name, sizeof(name), "%.*s.ftinfo", TOX_PUBLIC_KEY_SIZE * 2, hex);

    FILE *fp = native_get_file(name, NULL, UTOX_FILE_OPTS_WRITE);

    if (fp == NULL) {
        return false;
    }

    fwrite(data, length, 1, fp);
    flush_file(fp);
    fclose(fp);

    return true;
}

uint8_t *utox_data_load_custom_theme(size_t *out) {
    FILE *fp = native_get_file("utox_theme.ini", out, UTOX_FILE_OPTS_READ);
    uint8_t *data;

    if (fp == NULL) {
        return NULL;
    }

    data = calloc(*out + 1, 1);
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

/*******************************************************************
 **  Chatlog Section                                              **
 *******************************************************************/

static FILE* chatlog_get_file(char hex[TOX_PUBLIC_KEY_SIZE * 2], bool append) {
    char name[TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".new.txt")];
    snprintf(name, sizeof(name), "%.*s.new.txt", TOX_PUBLIC_KEY_SIZE * 2, hex);

    if (append) {
        return native_get_file(name, NULL, UTOX_FILE_OPTS_READ   | UTOX_FILE_OPTS_WRITE |
                                           UTOX_FILE_OPTS_APPEND | UTOX_FILE_OPTS_MKDIR);
    } else {
        return native_get_file(name, NULL, UTOX_FILE_OPTS_READ);
    }
}

size_t utox_save_chatlog(char hex[TOX_PUBLIC_KEY_SIZE * 2], uint8_t *data, size_t length) {
    FILE *fp = chatlog_get_file(hex, true);

    if (fp == NULL) {
        debug("uTox:\tError getting a file handle for this chatlog!\n");
        return 0;
    }

    off_t offset = ftello(fp);
    fwrite(data, length, 1, fp);
    flush_file(fp);
    fclose(fp);

    return offset;
}

static size_t utox_count_chatlog(char hex[TOX_PUBLIC_KEY_SIZE * 2]) {
    FILE *file = chatlog_get_file(hex, false);

    if (!file) {
        return 0;
    }

    LOG_FILE_MSG_HEADER header;
    size_t              records_count = 0;

    while (1 == fread(&header, sizeof(header), 1, file)) {
        fseeko(file, header.author_length + header.msg_length + 1, SEEK_CUR);
        records_count++;
    }


    if (ferror(file) || !feof(file)) {
        /* TODO: consider removing or truncating the log file.
         * If !feof() this means that the file has an incomplete record,
         * which would prevent it from loading forever, even though
         * new records will keep being appended as usual. */
        debug_error("Log read trying to count history for friend %.*s\n", TOX_PUBLIC_KEY_SIZE * 2, hex);
        fclose(file);
        return 0;
    }

    fclose(file);
    return records_count;
}

/* TODO create fxn that will try to recover a corrupt chat history.
 *
 * In the majority of bug reports the corrupt message is often the first, so in
 * theory we should be able to trim the start of the chatlog up to and including
 * the first \n char. We may have to do so multiple times, but once we find the
 * first valid message everything else should "work" */
uint8_t **utox_load_chatlog(char hex[TOX_PUBLIC_KEY_SIZE * 2], size_t *size, uint32_t count, uint32_t skip) {
    /* Becasue every platform is different, we have to ask them to open the file for us.
     * However once we have it, every platform does the same thing, this should prevent issues
     * from occuring on a single platform. */
    LOG_FILE_MSG_HEADER header;

    size_t records_count = utox_count_chatlog(hex);
    if (skip >= records_count) {
        debug_error("Native log read:\tError, skipped all records\n");
        return NULL;
    }

    FILE *file = chatlog_get_file(hex, false);
    if (!file) {
        debug("History:\tUnable to access file provided.\n");
        return NULL;
    }

    if (count > (records_count - skip)) {
        count = records_count - skip;
    }

    uint8_t **data = calloc(1, sizeof(*data) * count + 1);

    size_t start_at     = records_count - count - skip;
    size_t actual_count = 0;

    size_t file_offset = 0;

    while (1 == fread(&header, sizeof(header), 1, file)) {
        if (start_at) {
            fseeko(file, header.author_length, SEEK_CUR); /* Skip the recorded author */
            fseeko(file, header.msg_length, SEEK_CUR);    /* Skip the message */
            fseeko(file, 1, SEEK_CUR);                    /* Skip the newline char */
            start_at--;
            file_offset = ftello(file);
            continue;
        }

        if (count) {
            /* we have to skip the author name for now, it's left here for group chats support in the future */
            fseeko(file, header.author_length, SEEK_CUR);
            if (header.msg_length > 1 << 16) {
                debug_error("Can't malloc that much, you'll probably have to move or delete, your history for this"
                            " peer.\n\t\tFriend number %.*s, count %u, actual_count %lu, start at %lu, error size %lu",
                            TOX_PUBLIC_KEY_SIZE * 2, hex, count, actual_count, start_at, header.msg_length);
                if (size) {
                    *size = 0;
                }
                return NULL;
            }
            MSG_TEXT *msg = calloc(1, sizeof(MSG_TEXT) + header.msg_length);
            msg->our_msg  = header.author;

            /* TEMP Fix to recover logs from v0.8.* */
            if (header.log_version == 0) {
                msg->receipt_time = 1;
            } else {
                msg->receipt_time = header.receipt;
            }

            msg->length        = header.msg_length;
            msg->time          = header.time;
            msg->msg_type      = header.msg_type;
            msg->disk_offset   = file_offset;
            msg->author_length = header.author_length;

            if (1 != fread(msg->msg, msg->length, 1, file)) {
                debug("Native log read:\tError,reading this record... stopping\n");
                break;
            }
            msg->length = utf8_validate((uint8_t *)msg->msg, msg->length);
            *data++     = (void *)msg;
            count--;
            actual_count++;
            fseeko(file, 1, SEEK_CUR); /* seek an extra \n char */
            file_offset = ftello(file);
        }
    }

    fclose(file);

    if (size) {
        *size = actual_count;
    }
    return data - actual_count;
}

bool utox_update_chatlog(char hex[TOX_PUBLIC_KEY_SIZE * 2], size_t offset, uint8_t *data, size_t length) {
    FILE *file = chatlog_get_file(hex, true);

    if (!file) {
        debug_error("History:\tUnable to access file provided.\n");
        return 0;
    }

    if (fseeko(file, offset, SEEK_SET)) {
        debug_error("History:\tUnable to seek to position %lu in file provided.", offset);
        return 0;
    }

    fwrite(data, length, 1, file);
    fflush(file);
    fclose(file);

    return 1;
}

bool utox_remove_friend_chatlog(char hex[TOX_PUBLIC_KEY_SIZE * 2]) {
    char name[TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".new.txt")];

    snprintf(name, sizeof(name), "%.*s.new.txt", TOX_PUBLIC_KEY_SIZE * 2, hex);

    return utox_remove_file((uint8_t*)name, sizeof(name));
}

void utox_export_chatlog_init(uint32_t friend_number) {
    native_export_chatlog_init(friend_number);
}

void utox_export_chatlog(char hex[TOX_PUBLIC_KEY_SIZE * 2], FILE *dest_file) {
    if (!dest_file) {
        return;
    }

    FILE *file = chatlog_get_file(hex, false);

    LOG_FILE_MSG_HEADER header;
    while (1 == fread(&header, sizeof(header), 1, file)) {
        char c;
        /* Write Author */
        fwrite("<", 1, 1, dest_file);
        for (int i = 0; i < header.author_length; ++i) {
            c = fgetc(file);
            if (c != EOF) {
                fputc(c, dest_file);
            } else {
                continue;
            }
        }
        fwrite(">", 1, 1, dest_file);

        /* Write text */
        fwrite(" ", 1, 1, dest_file);
        for (int i = 0; i < header.msg_length; ++i) {
            c = fgetc(file);
            if (c != EOF) {
                fputc(c, dest_file);
            } else {
                continue;
            }
        }
        c = fgetc(file); /* the newline char */
        fputc(c, dest_file);
    }

    fclose(file);
    fclose(dest_file);
}

bool utox_remove_file(const uint8_t *full_name, size_t length) {
    return native_remove_file(full_name, length);
}

/* Shared function between all four platforms */
void parse_args(int argc, char *argv[], bool *theme_was_set_on_argv, int8_t *should_launch_at_startup,
                int8_t *set_show_window, bool *no_updater) {
    // set default options
    settings.theme            = THEME_DEFAULT;
    settings.portable_mode    = false;
    *theme_was_set_on_argv    = false;
    *should_launch_at_startup = 0;
    *set_show_window          = 0;
    *no_updater               = false;

    static struct option long_options[] = {
        { "theme", required_argument, NULL, 't' }, { "portable", no_argument, NULL, 'p' },
        { "set", required_argument, NULL, 's' },   { "unset", required_argument, NULL, 'u' },
        { "no-updater", no_argument, NULL, 'n' },  { "version", no_argument, NULL, 0 },
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

            case 'n': {
                *no_updater = 1;
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
}

/** Android is still a bit legacy, so we just include it all here. */
#if defined __ANDROID__
#include "android/main.c"
#endif
