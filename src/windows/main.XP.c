#ifdef __WIN_LEGACY

#include "../main.h"

void native_select_dir_ft(uint32_t fid, MSG_FILE *file) {
    char *path = malloc(UTOX_FILE_NAME_LENGTH);
    memcpy(path, file->file_name, file->name_length);
    path[file->name_length] = 0;

    OPENFILENAME ofn = {
        .lStructSize = sizeof(OPENFILENAME),
        .hwndOwner   = hwnd,
        .lpstrFile   = path,
        .nMaxFile    = UTOX_FILE_NAME_LENGTH,
        .Flags       = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT,
    };

    if (GetSaveFileName(&ofn)) {
        postmessage_toxcore(TOX_FILE_ACCEPT, fid, file->file->file_number, path);
    } else {
        debug("GetSaveFileName() failed\n");
    }
}

void native_autoselect_dir_ft(uint32_t fid, FILE_TRANSFER *file) {
    char *path[UTOX_FILE_NAME_LENGTH];

    if (settings.portable_mode) {
        char *send = calloc(UTOX_FILE_NAME_LENGTH, sizeof(char *));
        snprintf(send, UTOX_FILE_NAME_LENGTH, "%s\\Tox_Auto_Accept", portable_mode_save_path);

        debug_notice("Native:\tAuto Accept Directory: \"%s\"\n", send);
        postmessage_toxcore(TOX_FILE_ACCEPT_AUTO, fid, file->file_number, send);
        return;
    }

    if (!SHGetFolderPath(NULL, CSIDL_DESKTOP, NULL, 0, (char *)path)) {
        wchar_t first[UTOX_FILE_NAME_LENGTH];
        wchar_t second[UTOX_FILE_NAME_LENGTH];
        wchar_t longname[UTOX_FILE_NAME_LENGTH];

        swprintf(first, UTOX_FILE_NAME_LENGTH, L"%ls%ls", *path, L"\\Tox_Auto_Accept");
        CreateDirectoryW(first, NULL);

        MultiByteToWideChar(CP_UTF8, 0, (char *)file->name, file->name_length, longname, file->name_length);

        swprintf(second, UTOX_FILE_NAME_LENGTH, L"%ls\\%ls", first, longname);

        char *send = calloc(UTOX_FILE_NAME_LENGTH, sizeof(char *));
        native_to_utf8str(second, send, UTOX_FILE_NAME_LENGTH);

        debug_notice("Native:\tAuto Accept Directory: \"%s\"", send);

        postmessage_toxcore(TOX_FILE_ACCEPT_AUTO, fid, file->file_number, send);
    } else {
        debug("NATIVE:\tUnable to auto save file!\n");
    }
}

void launch_at_startup(int is_launch_at_startup) {
    HKEY           hKey;
    const wchar_t *run_key_path = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    wchar_t        path[UTOX_FILE_NAME_LENGTH * 2];
    uint16_t       path_length = 0, ret = 0;
    if (is_launch_at_startup == 1) {
        if (ERROR_SUCCESS == RegOpenKeyW(HKEY_CURRENT_USER, run_key_path, &hKey)) {
            path_length           = GetModuleFileNameW(NULL, path + 1, UTOX_FILE_NAME_LENGTH * 2);
            path[0]               = '\"';
            path[path_length + 1] = '\"';
            path[path_length + 2] = '\0';
            path_length += 2;
            ret = RegSetValueExW(hKey, L"uTox", NULL, REG_SZ, (uint8_t *)path, path_length * 2); /*2 bytes per wchar */
            if (ret == ERROR_SUCCESS) {
                debug("Successful auto start addition.\n");
            }
            RegCloseKey(hKey);
        }
    }
    if (is_launch_at_startup == 0) {
        debug("Going to delete auto start key.\n");
        if (ERROR_SUCCESS == RegOpenKeyW(HKEY_CURRENT_USER, run_key_path, &hKey)) {
            debug("Successful key opened.\n");
            ret = RegDeleteValueW(hKey, L"uTox");
            if (ret == ERROR_SUCCESS) {
                debug("Successful auto start deletion.\n");
            } else {
                debug("UN-Successful auto start deletion.\n");
            }
            RegCloseKey(hKey);
        }
    }
}

#endif
