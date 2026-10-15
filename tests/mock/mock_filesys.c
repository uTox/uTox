/*
 * Chatlog and filesys tests compile the production filesys for this platform.
 * Windows needs utf8.c for utf8_to_nativestr. native_remove_file on Unix lives
 * in xlib/filesys.c, so the stub below stays for non-Windows builds.
 */
#ifdef _WIN32

/* filesys.c pulls in main.h and sets _WIN32_WINNT before windows.h. */
#include "../../src/windows/filesys.c"
#include "../../src/windows/utf8.c"
#include "../../src/filesys.c"

char portable_mode_save_path[MAX_PATH];

/* SHCreateDirectoryEx rejects a relative path. Fill this before main. */
static void init_portable_save_path(void) __attribute__((constructor));
static void init_portable_save_path(void) {
    if (!GetCurrentDirectoryA(MAX_PATH, portable_mode_save_path)) {
        strcpy(portable_mode_save_path, ".");
    }
}

#else /* !_WIN32 */

#include "../../src/filesys.h"
#include "../../src/native/filesys.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/filesys.c"
#include "../../src/posix/filesys.c"

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

#endif /* _WIN32 */
