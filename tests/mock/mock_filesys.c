#include "../../src/filesys.h"
#include "../../src/native/filesys.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

/*
 * Chatlog unit tests pull filesys through this mock.
 * On Windows/MinGW do not compile posix/filesys.c (mkdir(2) / O_BINARY);
 * provide only the natives chatlog actually uses. Real UNIX builds keep posix.
 */
#ifdef _WIN32

#include <direct.h>
#include <sys/stat.h>

#include "../../src/debug.h"
#include "../../src/settings.h"

bool native_create_dir(const uint8_t *filepath) {
    char path[UTOX_FILE_NAME_LENGTH] = { 0 };
    snprintf(path, sizeof path, "%s", (const char *)filepath);
    for (size_t i = 0; path[i]; ++i) {
        if (path[i] == '/') {
            path[i] = '\\';
        }
    }
    size_t n = strlen(path);
    while (n > 1 && path[n - 1] == '\\') {
        path[--n] = 0;
    }

    const int status = _mkdir(path);
    if (status == 0) {
        return true;
    }
    /* _mkdir also sets EEXIST when a regular file already occupies the path. */
    if (errno == EEXIST) {
        struct stat st;
        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
            return true;
        }
    }
    LOG_WARN("Filesys", "Unable to create directory %s. Error: %d", filepath, errno);
    return false;
}

static void opts_to_sysmode(UTOX_FILE_OPTS opts, char *mode) {
    if (opts & UTOX_FILE_OPTS_READ) {
        mode[0] = 'r';
    } else if (opts & UTOX_FILE_OPTS_APPEND) {
        mode[0] = 'a';
    } else if (opts & UTOX_FILE_OPTS_WRITE) {
        mode[0] = 'w';
    }

    mode[1] = 'b';

    if ((opts & (UTOX_FILE_OPTS_WRITE | UTOX_FILE_OPTS_APPEND)) && (opts & UTOX_FILE_OPTS_READ)) {
        mode[2] = '+';
    }

    mode[3] = 0;
}

FILE *utox_get_file_simple(const char *path, UTOX_FILE_OPTS opts) {
    return native_get_file_simple(path, opts);
}

FILE *native_get_file_simple(const char *path, UTOX_FILE_OPTS opts) {
    char mode[4] = { 0 };
    opts_to_sysmode(opts, mode);

    FILE *fp = fopen(path, mode);
    if (!fp && (opts & UTOX_FILE_OPTS_READ) && (opts & UTOX_FILE_OPTS_WRITE)) {
        /* Create then reopen with the requested mode (binary via 'b'). */
        fp = fopen(path, "w+b");
        if (fp) {
            fclose(fp);
            fp = fopen(path, mode);
        }
    }

    return fp;
}

FILE *native_get_file(const uint8_t *name, size_t *size, UTOX_FILE_OPTS opts, bool portable_mode) {
    uint8_t path[UTOX_FILE_NAME_LENGTH] = { 0 };

    if (portable_mode) {
        snprintf((char *)path, UTOX_FILE_NAME_LENGTH, "./tox/");
    } else {
        snprintf((char *)path, UTOX_FILE_NAME_LENGTH, "%s/.config/tox/", getenv("HOME"));
    }

    assert(opts <= UTOX_FILE_OPTS_DELETE);
    assert((opts & UTOX_FILE_OPTS_WRITE && opts & UTOX_FILE_OPTS_APPEND) == false);

    if (opts & UTOX_FILE_OPTS_WRITE || opts & UTOX_FILE_OPTS_MKDIR) {
        if (!native_create_dir(path)) {
            return NULL;
        }
    }

    if (strlen((char *)path) + strlen((char *)name) >= UTOX_FILE_NAME_LENGTH) {
        LOG_ERR("Filesys", "Load directory name too long");
        return NULL;
    }

    snprintf((char *)path + strlen((char *)path), UTOX_FILE_NAME_LENGTH - strlen((char *)path), "%s", name);

    if (opts == UTOX_FILE_OPTS_DELETE) {
        remove((char *)path);
        return NULL;
    }

    if (opts & UTOX_FILE_OPTS_MKDIR) {
        uint8_t push;
        uint8_t *p = path + strlen((char *)path);
        while (*--p != '/') {
        }
        push = *++p;
        *p   = 0;
        native_create_dir(path);
        *p = push;
    }

    FILE *fp = native_get_file_simple((char *)path, opts);
    if (fp == NULL) {
        return NULL;
    }

    if (size != NULL) {
        fseek(fp, 0, SEEK_END);
        *size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
    }

    return fp;
}

FILE *utox_get_file(const char *name, size_t *size, UTOX_FILE_OPTS opts) {
    return native_get_file((uint8_t *)name, size, opts, settings.portable_mode);
}

char *utox_get_filepath(const char *name) {
    char *path = calloc(1, UTOX_FILE_NAME_LENGTH);
    if (!path) {
        return NULL;
    }

    if (settings.portable_mode) {
        snprintf(path, UTOX_FILE_NAME_LENGTH, "./tox/%s", name);
    } else {
        const char *home = getenv("HOME");
        if (!home) {
            home = ".";
        }
        snprintf(path, UTOX_FILE_NAME_LENGTH, "%s/.config/tox/%s", home, name);
    }

    return path;
}

bool utox_remove_file(const uint8_t *full_name, size_t length) {
    return native_remove_file(full_name, length, settings.portable_mode);
}

bool native_move_file(const uint8_t *current_name, const uint8_t *new_name) {
    if (!current_name || !new_name) {
        return false;
    }
    return rename((const char *)current_name, (const char *)new_name) == 0;
}

bool utox_move_file(const uint8_t *current_name, const uint8_t *new_name) {
    return native_move_file(current_name, new_name);
}

#else /* !_WIN32 */

#include "../../src/filesys.c"
#include "../../src/posix/filesys.c"

#endif /* _WIN32 */

// TODO copied from xlib/filesys.c might be possible to keep this DRY at some point
bool native_remove_file(const uint8_t *name, size_t length, bool portable_mode) {
    char path[UTOX_FILE_NAME_LENGTH] = { 0 };

    // TODO this is duplicated in more methods, make this portable thing a common method in filesys.c
    if (portable_mode) {
        snprintf((char *)path, UTOX_FILE_NAME_LENGTH, "./tox/");
    } else {
        snprintf((char *)path, UTOX_FILE_NAME_LENGTH, "%s/.config/tox/", getenv("HOME"));
    }

    if (strlen((const char *)path) + length >= UTOX_FILE_NAME_LENGTH) {
        LOG_DEBUG("Filesys", "File/directory name too long, unable to remove");
    } else {
        snprintf((char *)path + strlen((const char *)path), UTOX_FILE_NAME_LENGTH - strlen((const char *)path), "%.*s",
                 (int)length, (char *)name);
    }

    if (remove((const char *)path)) {
        LOG_ERR("NATIVE", "Unable to delete file!\n\t\t%s", path);
        return false;
    }

    LOG_INFO("NATIVE", "File deleted!");
    LOG_DEBUG("Filesys", "\t%s", path);
    return true;
}
