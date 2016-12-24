#include "main.h"

#include "../main.h"

static FILE* get_file(wchar_t path[UTOX_FILE_NAME_LENGTH], UTOX_FILE_OPTS opts) {
    // assert(UTOX_FILE_NAME_LENGTH <= (32,767 wide characters) );
    DWORD rw  = 0;
    char mode[4] = { 0 };
    DWORD create = OPEN_EXISTING;

    if (opts & UTOX_FILE_OPTS_READ) {
        rw |= GENERIC_READ;
        mode[0] = 'r';
    }

    if (opts & UTOX_FILE_OPTS_APPEND) {
        rw |= GENERIC_WRITE;
        mode[0] = 'a';
        create = OPEN_ALWAYS;
    } else if (opts & UTOX_FILE_OPTS_WRITE) {
        rw |= GENERIC_WRITE;
        mode[0] = 'w';
        create = CREATE_ALWAYS;
    }

    mode[1] = 'b';
    if ((opts & (UTOX_FILE_OPTS_WRITE | UTOX_FILE_OPTS_APPEND)) && (opts & UTOX_FILE_OPTS_READ)) {
        mode[2] = '+';
    }

    HANDLE WINAPI winFile = CreateFileW(path, rw, FILE_SHARE_READ, NULL,
                                        create, FILE_ATTRIBUTE_NORMAL, NULL);

    return _fdopen(_open_osfhandle((intptr_t)winFile, 0), mode);
}

FILE *native_get_file(const uint8_t *name, size_t *size, UTOX_FILE_OPTS opts) {
    uint8_t path[UTOX_FILE_NAME_LENGTH] = { 0 };

    if (settings.portable_mode) {
        strcpy((char *)path, portable_mode_save_path);
    } else {
        if (FAILED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path))) {
            if (FAILED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
                strcpy((char *)path, portable_mode_save_path);
            }
        }
    }

    if (opts > UTOX_FILE_OPTS_DELETE) {
        debug_error("NATIVE:\tDon't call native_get_file with UTOX_FILE_OPTS_DELETE in combination with other options.\n");
        return NULL;
    } else if (opts & UTOX_FILE_OPTS_WRITE && opts & UTOX_FILE_OPTS_APPEND) {
        debug_error("NATIVE:\tDon't call native_get_file with UTOX_FILE_OPTS_WRITE in combination with UTOX_FILE_OPTS_APPEND.\n");
        return NULL;
    }

    snprintf((char *)path + strlen((char *)path), UTOX_FILE_NAME_LENGTH - strlen((char *)path), "/Tox/");

    if (strlen((char *)path) + strlen((char *)name) >= UTOX_FILE_NAME_LENGTH) {
        debug_error("NATIVE:\tLoad directory name too long\n");
        return NULL;
    }

    // Append the subfolder to the path and remove it from the name.
    for (char *folder_divider = strstr(name, "/"); folder_divider != NULL; folder_divider = strstr(name, "/")) {
        ++folder_divider; // Skip over the / we're pointing to.
        snprintf((char *)path + strlen((char *)path), strlen(name) - strlen(folder_divider), name);
        char *new_name = name + strlen(name) - strlen(folder_divider);
        name = new_name;
    }

    if (opts & UTOX_FILE_OPTS_WRITE || opts & UTOX_FILE_OPTS_MKDIR) {
        if (!native_create_dir(path)) {
            debug_error("NATIVE:\t: Failed to create path %s.\n", path);
        }
    }

    snprintf((char *)path + strlen((char *)path), UTOX_FILE_NAME_LENGTH - strlen((char *)path), "/%s", (char *)name);

    for (size_t i = 0; path[i] != '\0'; ++i) {
        if (path[i] == '/') {
            path[i] = '\\';
        }
    }

    wchar_t wide[UTOX_FILE_NAME_LENGTH] = { 0 };
    MultiByteToWideChar(CP_UTF8, 0, path, strlen((char *)path), wide, UTOX_FILE_NAME_LENGTH);

    if (opts == UTOX_FILE_OPTS_DELETE) {
        if (!DeleteFile(path)) {
            debug_error("NATIVE:\tCould not delete file: %s - Error: %d\n", path, GetLastError());
        }
        return NULL;
    }

    FILE *fp = get_file(wide, opts);

    if (fp == NULL) {
        if (opts > UTOX_FILE_OPTS_READ) {
            debug_error("NATIVE:\tCould not open %S for writing.\n", wide);
        }
        return NULL;
    }

    if (size != NULL && opts & UTOX_FILE_OPTS_READ) {
        fseek(fp, 0, SEEK_END);
        *size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
    }

    return fp;
}

/** Try to create a path;
 *
 * Accepts null-terminated utf8 path.
 * Returns: true if folder exists, false otherwise
 *
 */
bool native_create_dir(const uint8_t *filepath) {
    // Maybe switch this to SHCreateDirectoryExW at some point.
    uint8_t path[UTOX_FILE_NAME_LENGTH] = { 0 };
    strcpy((char *)path, (char *)filepath);

    for (size_t i = 0; path[i] != '\0'; ++i) {
        if (path[i] == '/') {
            path[i] = '\\';
        }
    }

    const int error = SHCreateDirectoryEx(NULL, path, NULL);
    switch(error) {
        case ERROR_SUCCESS:
        case ERROR_FILE_EXISTS:
        case ERROR_ALREADY_EXISTS:
            debug_notice("NATIVE:\tCreated path: `%s` - %d\n", filepath, error);
            return true;
            break;

        case ERROR_BAD_PATHNAME:
            debug_error("NATIVE:\tUnable to create path: `%s` - bad path name.\n", filepath);
            return false;
            break;

        case ERROR_FILENAME_EXCED_RANGE:
        case ERROR_PATH_NOT_FOUND:
        case ERROR_CANCELLED:
        default:
            debug_error("NATIVE:\tUnable to create path: `%s` - error %d\n", filepath, error);
            return false;
            break;
    }
}

bool native_remove_file(const uint8_t *name, size_t length) {
    uint8_t path[UTOX_FILE_NAME_LENGTH] = { 0 };

    if (settings.portable_mode) {
        strcpy((char *)path, portable_mode_save_path);
    } else {
        bool have_path = false;
        have_path      = SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, (char *)path));

        if (!have_path) {
            have_path = SUCCEEDED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, (char *)path));
        }

        if (!have_path) {
            strcpy((char *)path, portable_mode_save_path);
            have_path = true;
        }
    }


    if (strlen((const char *)path) + length >= UTOX_FILE_NAME_LENGTH) {
        debug("NATIVE:\tFile/directory name too long, unable to remove\n");
        return false;
    } else {
        snprintf((char *)path + strlen((const char *)path), UTOX_FILE_NAME_LENGTH - strlen((const char *)path),
                 "\\Tox\\%.*s", (int)length, (char *)name);
    }

    if (remove((const char *)path)) {
        debug_error("NATIVE:\tUnable to delete file!\n\t\t%s\n", path);
        return false;
    } else {
        debug_info("NATIVE:\tFile deleted!\n");
        debug("NATIVE:\t\t%s\n", path);
    }

    return true;
}
