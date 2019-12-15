#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include "main.h"
#include "utf8.h"

#include "../chatlog.h"
#include "../debug.h"
#include "../file_transfers.h"
#include "../filesys.h"
#include "../friend.h"
#include "../settings.h"
#include "../tox.h"

#include <shlobj.h>
#include <io.h>

void native_export_chatlog_init(uint32_t friend_number) {
    FRIEND *f = get_friend(friend_number);
    if (!f) {
        LOG_ERR("Windows7", "Could not get friend with number: %u", friend_number);
        return;
    }

    char *path = calloc(1, UTOX_FILE_NAME_LENGTH);
    if (!path){
        LOG_ERR("Windows7", " Could not allocate memory." );
        return;
    }

    snprintf(path, UTOX_FILE_NAME_LENGTH, "%.*s.txt", (int)f->name_length, f->name);

    wchar_t filepath[UTOX_FILE_NAME_LENGTH] = { 0 };
    utf8_to_nativestr(path, filepath, UTOX_FILE_NAME_LENGTH * 2);

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
            LOG_ERR("Windows7", " Could not allocate memory." );
            return;
        }

        native_to_utf8str(filepath, path, UTOX_FILE_NAME_LENGTH);

        FILE *file = utox_get_file_simple(path, UTOX_FILE_OPTS_WRITE);
        if (file) {
            utox_export_chatlog(f->id_str, file);
        } else {
            LOG_ERR("Windows7", "Opening file %s failed", path);
        }
    } else {
        LOG_ERR("Windows7", "Unable to open file and export chatlog.");
    }
    free(path);
}

void native_select_dir_ft(uint32_t fid, uint32_t num, FILE_TRANSFER *file) {
    if (!sanitize_filename(file->name)) {
        LOG_ERR("Windows7", "Filename is invalid and could not be sanitized");
        return;
    }

    wchar_t filepath[UTOX_FILE_NAME_LENGTH] = { 0 };
    utf8_to_nativestr((char *)file->name, filepath, file->name_length * 2);

    OPENFILENAMEW ofn = {
        .lStructSize = sizeof(OPENFILENAMEW),
        .lpstrFile   = filepath,
        .nMaxFile    = UTOX_FILE_NAME_LENGTH,
        .Flags       = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT,
    };

    if (GetSaveFileNameW(&ofn)) {
        char *path = calloc(1, UTOX_FILE_NAME_LENGTH);
        if (!path) {
            LOG_ERR("Windows7", "Could not allocate memory for path.");
            return;
        }

        native_to_utf8str(filepath, path, UTOX_FILE_NAME_LENGTH * 2);
        postmessage_toxcore(TOX_FILE_ACCEPT, fid, num, path);
    } else {
        LOG_ERR("Windows7", "Unable to Get save file for incoming FT.");
    }
}

void native_autoselect_dir_ft(uint32_t fid, FILE_TRANSFER *file) {
    wchar_t *autoaccept_folder = NULL;

    if (settings.portable_mode) {
        autoaccept_folder = calloc(1, UTOX_FILE_NAME_LENGTH * sizeof(wchar_t));
        utf8_to_nativestr(portable_mode_save_path, autoaccept_folder, strlen(portable_mode_save_path) * 2);
    } else if (SHGetKnownFolderPath((REFKNOWNFOLDERID)&FOLDERID_Downloads,
                                    KF_FLAG_CREATE, NULL, &autoaccept_folder) != S_OK) {
        LOG_ERR("Windows7", "Unable to get auto accept file folder!");
        return;
    }

    wchar_t subpath[UTOX_FILE_NAME_LENGTH] = { 0 };
    swprintf(subpath, UTOX_FILE_NAME_LENGTH, L"%ls%ls", autoaccept_folder, L"\\Tox_Auto_Accept");

    if (settings.portable_mode) {
        free(autoaccept_folder);
    } else {
        CoTaskMemFree(autoaccept_folder);
    }

    CreateDirectoryW(subpath, NULL);

    if (!sanitize_filename(file->name)) {
        LOG_ERR("Windows7", "Filename is invalid and could not be sanitized");
        return;
    }

    wchar_t filename[UTOX_FILE_NAME_LENGTH] = { 0 };
    utf8_to_nativestr((char *)file->name, filename, file->name_length * 2);

    wchar_t fullpath[UTOX_FILE_NAME_LENGTH] = { 0 };
    swprintf(fullpath, UTOX_FILE_NAME_LENGTH, L"%ls\\%ls", subpath, filename);

    char *path = calloc(1, UTOX_FILE_NAME_LENGTH);
    if (!path) {
        LOG_ERR("Windows7", "Could not allocate memory for path.");
        return;
    }

    native_to_utf8str(fullpath, path, UTOX_FILE_NAME_LENGTH);
    postmessage_toxcore(TOX_FILE_ACCEPT_AUTO, fid, file->file_number, path);
}

void launch_at_startup(bool should) {
    const wchar_t *run_key_path = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

    if (should) {
        HKEY hKey;
        if (RegOpenKeyW(HKEY_CURRENT_USER, run_key_path, &hKey) == ERROR_SUCCESS) {
            wchar_t path[UTOX_FILE_NAME_LENGTH * 2];
            uint16_t path_length  = GetModuleFileNameW(NULL, path + 1, UTOX_FILE_NAME_LENGTH * 2);
            path[0]               = '\"';
            path[path_length + 1] = '\"';
            path[path_length + 2] = '\0';
            path_length += 2;

            // 2 bytes per wchar_t
            uint16_t ret = RegSetKeyValueW(hKey, NULL, L"uTox", REG_SZ, path, path_length * 2);
            if (ret == ERROR_SUCCESS) {
                LOG_INFO("Windows7", "Set uTox to run at startup.");
            } else {
                LOG_ERR("Windows7", "Unable to set Registry key for startup.");
            }

            RegCloseKey(hKey);
        }
    } else {
        HKEY hKey;
        if (ERROR_SUCCESS == RegOpenKeyW(HKEY_CURRENT_USER, run_key_path, &hKey)) {
            uint16_t ret = RegDeleteKeyValueW(hKey, NULL, L"uTox");
            if (ret == ERROR_SUCCESS) {
                LOG_INFO("Windows7", "Set uTox to not run at startup.");
            } else {
                LOG_ERR("Windows7", "Unable to delete Registry key for startup.");
            }
            RegCloseKey(hKey);
        }
    }
}
