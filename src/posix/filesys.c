#include "../filesys.h"

#include "../debug.h"
#include "../settings.h"

#include "../native/filesys.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

bool native_create_dir_tree(const char *path) {
    size_t size = strlen(path);
    if (size < 2) { // memory bounds check
        return false;
    }

    char *buff = calloc(1, size);
    if (!buff) {
        LOG_ERR("Filesys", "Unable to allocate memory for buffer.");
        return false;
    }

    for (size_t i = 1; i < size; ++i) { // i = 1 to skip root '/'
        if (path[i] == '/') {
            memcpy(buff, path, i + 1);
            if (!native_create_dir((uint8_t *)buff)) {
                free(buff);
                return false;
            }
        }
    }
    free(buff);
    return true;
}

// native get "valid" file path
char *native_get_filepath(const char *name) {
    char *path = calloc(1, UTOX_FILE_NAME_LENGTH);

    if (!path) {
        LOG_ERR("Filesys", "Unable to allocate memory for file path.");
        return NULL;
    }

    if (settings.portable_mode) {
        snprintf(path, UTOX_FILE_NAME_LENGTH, "./tox/");
    } else {
        snprintf(path, UTOX_FILE_NAME_LENGTH, "%s/.config/tox/", getenv("HOME"));
    }

    if (strlen(path) + strlen(name) >= UTOX_FILE_NAME_LENGTH) {
        LOG_ERR("Filesys", "Load directory name too long" );
        free(path);
        return NULL;
    }

    if (!native_create_dir_tree(path)) {
        free(path);
        return NULL;
    }

    // add file name
    snprintf(path + strlen(path), UTOX_FILE_NAME_LENGTH - strlen(path), "%s", name);

    return path;
}

bool native_create_dir(const uint8_t *filepath) {
    const int status = mkdir((char *)filepath, S_IRWXU);
    if (status == 0 || errno == EEXIST) {
        return true;
    }
    LOG_WARN("Filesys", "Unable to create directory %s. Error: %d", filepath, errno);
    return false;
}

// TODO: DRY. This function exists in both posix/filesys.c and in android/main.c
static void opts_to_sysmode(UTOX_FILE_OPTS opts, char *mode) {
    if (opts & UTOX_FILE_OPTS_READ) {
        mode[0] = 'r'; // Reading is first, don't clobber files.
    } else if (opts & UTOX_FILE_OPTS_APPEND) {
        mode[0] = 'a'; // Then appending, again, don't clobber files.
    } else if (opts & UTOX_FILE_OPTS_WRITE) {
        mode[0] = 'w'; // Writing is the final option we'll look at.
    }

    mode[1] = 'b'; // does nothing on posix >C89, but hey, why not?

    if ((opts & (UTOX_FILE_OPTS_WRITE | UTOX_FILE_OPTS_APPEND)) && (opts & UTOX_FILE_OPTS_READ)) {
        mode[2] = '+';
    }

    mode[3] = 0;
}

FILE *native_get_file_simple(const char *path, UTOX_FILE_OPTS opts) {
    char mode[4] = { 0 };
    opts_to_sysmode(opts, mode);

    FILE *fp = fopen(path, mode);

    if (!fp && opts & UTOX_FILE_OPTS_READ && opts & UTOX_FILE_OPTS_WRITE) {
        LOG_WARN("POSIX", "Unable to simple open, falling back to fd" );
        // read won't create a file if it doesn't already exist. If we're allowed to write, let's try
        // to create the file, then reopen it.
        int fd = open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        fp = fdopen(fd, mode);
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

    // native_get_file should never be called with DELETE in combination with other FILE_OPTS.
    assert(opts <= UTOX_FILE_OPTS_DELETE);
    // WRITE and APPEND are mutually exclusive. WRITE will serve you a blank file. APPEND will append (duh).
    assert((opts & UTOX_FILE_OPTS_WRITE && opts & UTOX_FILE_OPTS_APPEND) == false);

    if (opts & UTOX_FILE_OPTS_WRITE || opts & UTOX_FILE_OPTS_MKDIR) {
        if (!native_create_dir(path)) {
            return NULL;
        }
    }

    if (strlen((char *)path) + strlen((char *)name) >= UTOX_FILE_NAME_LENGTH) {
        LOG_ERR("Filesys", "Load directory name too long" );
        return NULL;
    } else {
        snprintf((char *)path + strlen((char *)path), UTOX_FILE_NAME_LENGTH - strlen((char *)path), "%s", name);
    }

    if (opts == UTOX_FILE_OPTS_DELETE) {
        LOG_DEBUG("Filesys", "removing file %s", path);
        remove((char *)path);
        return NULL;
    }

    if (opts & UTOX_FILE_OPTS_MKDIR) {
        // remove file name from path
        uint8_t push;
        uint8_t *p = path + strlen((char *)path);
        while (*--p != '/');
        push = *++p;
        *p = 0;
        native_create_dir(path);
        *p = push;
    }

    FILE *fp = native_get_file_simple((char *)path, opts);

    if (fp == NULL) {
        LOG_TRACE("Filesys", "Could not open %s" , path);
        return NULL;
    }

    if (size != NULL) {
        fseek(fp, 0, SEEK_END);
        *size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
    }

    return fp;
}

bool native_move_file(const uint8_t *current_name, const uint8_t *new_name) {
    if(!current_name || !new_name) {
        return false;
    }

    return rename((char *)current_name, (char *)new_name);
}
