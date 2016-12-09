#include "../main.h"

#ifdef __APPLE__
#include "../cocoa/main.h"
#else
#include "../xlib/main.h"
#endif

FILE *native_get_file(char *name, size_t *size, UTOX_FILE_OPTS opts) {
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
        native_create_dir((uint8_t *)path); // TODO @robinli char or uint8
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

    FILE *fp = NULL;
    if (opts & UTOX_FILE_OPTS_READ) {
        fp = fopen(path, "rb");
    } else if (opts & UTOX_FILE_OPTS_WRITE) {
        fp = fopen(path, "wb");
    } else if (opts & UTOX_FILE_OPTS_APPEND) {
        fp = fopen(path, "ab");
    }

    if (fp == NULL) {
        debug("Could not open %s\n", path);
        return NULL;
    }

    if (size != NULL) {
        fseek(fp, 0, SEEK_END);
        *size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
    }

    return fp;
}
