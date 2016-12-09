#include "../main.h"

#ifdef __OBJC__
#include "../cocoa/main.h"
#else
#include "../xlib/main.h"
#endif

// TODO: DRY. This function exists in both posix/filesys.c and in android/main.c
static void mode_from_file_opts(UTOX_FILE_OPTS opts, char *mode) {
    if (opts & UTOX_FILE_OPTS_READ) {
        mode[0] = 'r';
    }

    if (opts & UTOX_FILE_OPTS_APPEND) {
        mode[0] = 'a';
    } else if (opts & UTOX_FILE_OPTS_WRITE) {
        mode[0] = 'w';
    }

    mode[1] = 'b';

    if ((opts & (UTOX_FILE_OPTS_WRITE | UTOX_FILE_OPTS_APPEND)) && (opts & UTOX_FILE_OPTS_READ)) {
        mode[2] = '+';
    }

    mode[3] = '\0';

    return;
}

FILE *native_get_file(const char *name, size_t *size, UTOX_FILE_OPTS opts) {
    char path[UTOX_FILE_NAME_LENGTH] = { 0 };

    if (settings.portable_mode) {
        snprintf(path, UTOX_FILE_NAME_LENGTH, "./tox/");
    } else {
        snprintf(path, UTOX_FILE_NAME_LENGTH, "%s/.config/tox/", getenv("HOME"));
    }

    if (opts > UTOX_FILE_OPTS_DELETE) {
        debug_error("NATIVE:\tDon't call native_get_file with UTOX_FILE_OPTS_DELETE in combination with other options.\n");
        return NULL;
    }

    if (opts & UTOX_FILE_OPTS_READ || opts & UTOX_FILE_OPTS_MKDIR) {
        if (!native_create_dir(path)) {
            return NULL;
        }
    }

    if (strlen(path) + strlen(name) >= UTOX_FILE_NAME_LENGTH) {
        debug("NATIVE:\tLoad directory name too long\n");
        return NULL;
    } else {
        snprintf(path + strlen(path), UTOX_FILE_NAME_LENGTH - strlen(path), "%s", name);
    }

    if (opts == UTOX_FILE_OPTS_DELETE) {
        remove(path);
        return NULL;
    }

    char mode[4] = { 0 };
    mode_from_file_opts(opts, mode);

    FILE *fp = fopen(path, mode);

    if (fp == NULL) {
        debug_error("NATIVE:\tCould not open %s\n", path);
        return NULL;
    }

    if (size != NULL) {
        fseek(fp, 0, SEEK_END);
        *size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
    }

    return fp;
}
