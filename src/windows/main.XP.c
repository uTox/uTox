#ifdef __WIN_LEGACY

#include "main.h"
#include "utf8.h"

#include "../chatlog.h"
#include "../file_transfers.h"
#include "../filesys.h"
#include "../friend.h"
#include "../debug.h"
#include "../main.h"
#include "../tox.h"
#include "../settings.h"

#include <io.h>

void native_export_chatlog_init(uint32_t friend_number) {
    char *path = calloc(1, UTOX_FILE_NAME_LENGTH);
    if (!path){
        LOG_ERR("WinXP", "Could not allocate memory.");
        return;
    }

    FRIEND *f = get_friend(friend_number);
    if (!f) {
        LOG_ERR("WinXP", "Could not get friend with number: %u", friend_number);
        return;
    }

    snprintf(path, UTOX_FILE_NAME_LENGTH, "%.*s.txt", (int)f->name_length, f->name);

    wchar_t filepath[UTOX_FILE_NAME_LENGTH] = { 0 };
    utf8_to_nativestr(path, filepath, UTOX_FILE_NAME_LENGTH);

    OPENFILENAMEW ofn = {
        .lStructSize = sizeof(OPENFILENAMEW),
        .lpstrFilter = L".txt",
        .lpstrFile   = filepath,
        .nMaxFile    = UTOX_FILE_NAME_LENGTH,
        .Flags       = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT,
        .lpstrDefExt = L"txt",
    };

    if (GetSaveFileNameW(&ofn)) {
        path = calloc(1, UTOX_FILE_NAME_LENGTH);
        if (!path){
            LOG_ERR("WinXP", " Could not allocate memory." );
            return;
        }

        native_to_utf8str(filepath, path, UTOX_FILE_NAME_LENGTH);

        FILE *file = utox_get_file_simple(path, UTOX_FILE_OPTS_WRITE);
        if (file) {
            utox_export_chatlog(f->id_str, file);
        } else {
            LOG_ERR("WinXP", "Opening file %s failed", path);
        }
    } else {
        LOG_TRACE("WinXP", "GetSaveFileName() failed" );
    }
    free(path);
}

void native_select_dir_ft(uint32_t fid, uint32_t num, FILE_TRANSFER *file) {
    if (!sanitize_filename(file->name)) {
        LOG_ERR("WinXP", "Filename is invalid and could not be sanitized");
        return;
    }

    wchar_t filepath[UTOX_FILE_NAME_LENGTH] = { 0 };
    utf8_to_nativestr((char *)file->name, filepath, file->name_length);

    OPENFILENAMEW ofn = {
        .lStructSize = sizeof(OPENFILENAMEW),
        .lpstrFile   = filepath,
        .nMaxFile    = UTOX_FILE_NAME_LENGTH,
        .Flags       = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT,
    };

    if (GetSaveFileNameW(&ofn)) {
        char *path = calloc(1, UTOX_FILE_NAME_LENGTH);
        if (!path) {
            LOG_ERR("WinXP", "Could not allocate memory for path.");
            return;
        }

        native_to_utf8str(filepath, path, UTOX_FILE_NAME_LENGTH);
        postmessage_toxcore(TOX_FILE_ACCEPT, fid, num, path);
    } else {
        LOG_ERR("WinXP", "GetSaveFileName() failed");
    }
}

void native_autoselect_dir_ft(uint32_t fid, FILE_TRANSFER *file) {
    wchar_t *autoaccept_folder = calloc(1, MAX_PATH);
    if (!autoaccept_folder) {
        LOG_ERR("WinXP", "Unable to malloc for autoaccept path.");
        return;
    }

    if (settings.portable_mode) {
        utf8_to_nativestr(portable_mode_save_path, autoaccept_folder, strlen(portable_mode_save_path));
    } else if (SHGetFolderPathW(NULL, CSIDL_DESKTOP, NULL, 0, autoaccept_folder) != S_OK) {
        LOG_ERR("WinXP", "Unable to get auto accept file folder!");
        free(autoaccept_folder);
        return;
    }

    wchar_t subpath[UTOX_FILE_NAME_LENGTH] = { 0 };
    swprintf(subpath, UTOX_FILE_NAME_LENGTH, L"%ls%ls", autoaccept_folder, L"\\Tox_Auto_Accept");

    free(autoaccept_folder);

    CreateDirectoryW(subpath, NULL);

    if (!sanitize_filename(file->name)) {
        LOG_ERR("WinXP", "Filename is invalid and could not be sanitized");
        return;
    }

    wchar_t filename[UTOX_FILE_NAME_LENGTH] = { 0 };
    utf8_to_nativestr((char *)file->name, filename, file->name_length);

    wchar_t fullpath[UTOX_FILE_NAME_LENGTH] = { 0 };
    swprintf(fullpath, UTOX_FILE_NAME_LENGTH, L"%ls\\%ls", subpath, filename);

    char *path = calloc(1, UTOX_FILE_NAME_LENGTH);
    if (!path) {
        LOG_ERR("WinXP", "Could not allocate memory for path.");
        return;
    }

    native_to_utf8str(fullpath, path, UTOX_FILE_NAME_LENGTH);
    postmessage_toxcore(TOX_FILE_ACCEPT_AUTO, fid, file->file_number, path);
}

void launch_at_startup(bool should) {
    HKEY hKey;
    const wchar_t *run_key_path = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

    wchar_t path[UTOX_FILE_NAME_LENGTH * 2];
    if (should) {
        if (ERROR_SUCCESS == RegOpenKeyW(HKEY_CURRENT_USER, run_key_path, &hKey)) {
            uint16_t path_length  = GetModuleFileNameW(NULL, path + 1, UTOX_FILE_NAME_LENGTH * 2);
            path[0]               = '\"';
            path[path_length + 1] = '\"';
            path[path_length + 2] = '\0';
            path_length += 2;
            uint16_t ret = RegSetValueExW(hKey, L"uTox", 0, REG_SZ, (uint8_t *)path, path_length * 2); /*2 bytes per wchar_t */
            if (ret == ERROR_SUCCESS) {
                LOG_TRACE("WinXP", "Successful auto start addition." );
            }
            RegCloseKey(hKey);
        }
    } else {
        LOG_TRACE("WinXP", "Going to delete auto start key." );
        if (ERROR_SUCCESS == RegOpenKeyW(HKEY_CURRENT_USER, run_key_path, &hKey)) {
            LOG_TRACE("WinXP", "Successful key opened." );
            uint16_t ret = RegDeleteValueW(hKey, L"uTox");
            if (ret == ERROR_SUCCESS) {
                LOG_TRACE("WinXP", "Successful auto start deletion." );
            } else {
                LOG_TRACE("WinXP", "UN-Successful auto start deletion." );
            }
            RegCloseKey(hKey);
        }
    }
}

#endif
