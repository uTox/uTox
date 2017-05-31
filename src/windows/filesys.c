#include "main.h"
#include "utf8.h"

#include "../debug.h"
#include "../filesys.h"
#include "../settings.h"

#include <io.h>
#include <stdio.h>
#include <string.h>

static FILE* get_file(wchar_t path[UTOX_FILE_NAME_LENGTH], UTOX_FILE_OPTS opts) {
    // assert(UTOX_FILE_NAME_LENGTH <= (32,767 wide characters) );
    DWORD rw  = 0;
    char mode[4] = { 0 };
    DWORD create = OPEN_EXISTING;

    if (opts & UTOX_FILE_OPTS_READ) {
        rw |= GENERIC_READ;
        mode[0] = 'r';
        if (opts & UTOX_FILE_OPTS_WRITE || opts & UTOX_FILE_OPTS_APPEND) {
            rw |= GENERIC_WRITE;
            create = OPEN_ALWAYS;
        }
    } else if (opts & UTOX_FILE_OPTS_APPEND) {
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

FILE *native_get_file_simple(const char *path, UTOX_FILE_OPTS opts) {
    //TODO: Check for forbidden opts (only read, write and append allowed)

    wchar_t wide_path[UTOX_FILE_NAME_LENGTH] = { 0 };
    utf8_to_nativestr(path, wide_path, UTOX_FILE_NAME_LENGTH * 2);

    FILE *f = get_file(wide_path, opts);

    if (!f) {
        LOG_ERR("WinFilesys", "Could not open file: %s", path);
        return NULL;
    }

    return f;
}

FILE *native_get_file(const uint8_t *name, size_t *size, UTOX_FILE_OPTS opts, bool portable_mode) {
    char path[UTOX_FILE_NAME_LENGTH] = { 0 };

    if (portable_mode) {
        strcpy(path, portable_mode_save_path);
    } else {
        if (FAILED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path))) {
            if (FAILED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
                strcpy(path, portable_mode_save_path);
            }
        }
    }

    if (opts > UTOX_FILE_OPTS_DELETE) {
        LOG_ERR("WinFilesys", "Don't call native_get_file with UTOX_FILE_OPTS_DELETE in combination with other options.");
        return NULL;
    } else if (opts & UTOX_FILE_OPTS_WRITE && opts & UTOX_FILE_OPTS_APPEND) {
        LOG_ERR("WinFilesys", "Don't call native_get_file with UTOX_FILE_OPTS_WRITE in combination with UTOX_FILE_OPTS_APPEND.");
        return NULL;
    }

    snprintf(path + strlen(path), UTOX_FILE_NAME_LENGTH - strlen(path), "/Tox/");

    if (strlen(path) + strlen((char *)name) >= UTOX_FILE_NAME_LENGTH) {
        LOG_ERR("WinFilesys", "Load directory name too long");
        return NULL;
    }

    char *tmp_path = _strdup((char *)name); // free() doesn't work if I touch this pointer at all, so..
    char *path_pointer = tmp_path;          // this pointer gets to hold the original location to free.
    if (!tmp_path) {
        LOG_FATAL_ERR(EXIT_MALLOC, "WinFilesys", "Unable to allocate memory for file path.");
    }

    // Append the subfolder to the path and remove it from the name.
    for (char *folder_divider = strstr(tmp_path, "/");
         folder_divider != NULL;
         folder_divider = strstr(tmp_path, "/"))
    {
        ++folder_divider; // Skip over the / we're pointing to.
        snprintf(path + strlen(path), strlen(tmp_path) - strlen(folder_divider), tmp_path);
        char *new_path = tmp_path + strlen(tmp_path) - strlen(folder_divider);
        tmp_path = new_path;
    }

    if (opts & UTOX_FILE_OPTS_WRITE || opts & UTOX_FILE_OPTS_MKDIR) {
        if (!native_create_dir((uint8_t *)path)) {
            LOG_ERR("WinFilesys", "Failed to create path %s.", path);
        }
    }

    snprintf(path + strlen(path), UTOX_FILE_NAME_LENGTH - strlen(path), "%s", tmp_path);

    free(path_pointer);

    for (size_t i = 0; path[i] != '\0'; ++i) {
        if (path[i] == '/') {
            path[i] = '\\';
        }
    }

    if (opts == UTOX_FILE_OPTS_DELETE) {
        if (!DeleteFile(path)) {
            LOG_ERR("WinFilesys", "Could not delete file: %s - Error: %d" , path, GetLastError());
        }

        return NULL;
    }


    FILE *fp = native_get_file_simple(path, opts);

    if (!fp) {
        if (opts > UTOX_FILE_OPTS_READ) {
            LOG_NOTE("WinFilesys", "Could not open %s for writing.", path);
        }

        return NULL;
    }

    if (size && opts & UTOX_FILE_OPTS_READ) {
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

    const int error = SHCreateDirectoryEx(NULL, (char *)path, NULL);
    switch(error) {
        case ERROR_SUCCESS:
            LOG_NOTE("WinFilesys", "Created path: `%s` - %d" , filepath, error);
        case ERROR_FILE_EXISTS:
        case ERROR_ALREADY_EXISTS:
            return true;
            break;

        case ERROR_BAD_PATHNAME:
            LOG_WARN("WinFilesys", "Unable to create path: `%s` - bad path name." , filepath);
            return false;
            break;

        case ERROR_FILENAME_EXCED_RANGE:
        case ERROR_PATH_NOT_FOUND:
        case ERROR_CANCELLED:
        default:
            LOG_ERR("WinFilesys", "Unable to create path: `%s` - error %d" , filepath, error);
            return false;
            break;
    }
}

bool native_remove_file(const uint8_t *name, size_t length, bool portable_mode) {
    char path[UTOX_FILE_NAME_LENGTH] = { 0 };

    if (portable_mode) {
        strcpy(path, portable_mode_save_path);
    } else {
        bool have_path = false;
        have_path      = SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path));

        if (!have_path) {
            have_path = SUCCEEDED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path));
        }

        if (!have_path) {
            strcpy(path, portable_mode_save_path);
            have_path = true;
        }
    }


    if (strlen(path) + length >= UTOX_FILE_NAME_LENGTH) {
        LOG_TRACE("WinFilesys", "File/directory name too long, unable to remove" );
        return false;
    } else {
        snprintf(path + strlen(path), UTOX_FILE_NAME_LENGTH - strlen(path),
                 "\\Tox\\%.*s", (int)length, (char *)name);
    }

    if (remove(path)) {
        LOG_ERR("WinFilesys", "Unable to delete file!\n\t\t%s" , path);
        return false;
    } else {
        LOG_INFO("WinFilesys", "File deleted!" );
        LOG_TRACE("WinFilesys", "\t%s" , path);
    }

    return true;
}

bool native_move_file(const uint8_t *current_name, const uint8_t *new_name) {
    if (!current_name || !new_name) {
        return false;
    }

    return MoveFile((char *)current_name, (char *)new_name);
}
