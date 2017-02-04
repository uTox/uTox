#ifdef __WIN_LEGACY

#include "main.h"

#include "../chatlog.h"
#include "../file_transfers.h"
#include "../filesys.h"
#include "../friend.h"
#include "../debug.h"
#include "../main.h"
#include "../tox.h"
#include "../settings.h"

void native_export_chatlog_init(uint32_t friend_number) {
    char *path = calloc(1, UTOX_FILE_NAME_LENGTH);

    if (path == NULL){
        LOG_ERR("NATIVE", "Could not allocate memory.");
        return;
    }

    snprintf(path, UTOX_FILE_NAME_LENGTH, "%.*s.txt", (int)friend[friend_number].name_length,
             friend[friend_number].name);

    OPENFILENAME ofn = {
        .lStructSize = sizeof(OPENFILENAME),
        .lpstrFilter = ".txt",
        .lpstrFile   = path,
        .nMaxFile    = UTOX_FILE_NAME_LENGTH,
        .Flags       = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT,
        .lpstrDefExt = "txt",
    };

    if (GetSaveFileName(&ofn)) {
        // TODO: native_get_file instead of fopen.
        FILE *file = fopen(path, "wb");
        if (file) {
            utox_export_chatlog(friend_number, file);
        } else {
            LOG_ERR(__FILE__, "Opening file %s failed\n", path);
        }
    } else {
        LOG_TRACE(__FILE__, "GetSaveFileName() failed" );
    }
}

void native_select_dir_ft(uint32_t fid, uint32_t num, FILE_TRANSFER *file) {
    char *path = calloc(1, UTOX_FILE_NAME_LENGTH);
    if (!path) {
        LOG_ERR("WinXP", "Unable to calloc when selecting file directory");
        return;
    }
    memcpy(path, file->name, file->name_length);
    path[file->name_length] = 0;

    OPENFILENAME ofn = {
        .lStructSize = sizeof(OPENFILENAME),
        .lpstrFile   = path,
        .nMaxFile    = UTOX_FILE_NAME_LENGTH,
        .Flags       = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT,
    };

    if (GetSaveFileName(&ofn)) {
        postmessage_toxcore(TOX_FILE_ACCEPT, fid, num, path);
    } else {
        LOG_ERR("WinXP", "GetSaveFileName() failed");
    }
}

void native_autoselect_dir_ft(uint32_t fid, FILE_TRANSFER *file) {
    char *send = calloc(UTOX_FILE_NAME_LENGTH, sizeof(char *));
    char *path[UTOX_FILE_NAME_LENGTH];

    wchar_t first[UTOX_FILE_NAME_LENGTH];
    wchar_t second[UTOX_FILE_NAME_LENGTH];
    wchar_t longname[UTOX_FILE_NAME_LENGTH];

    if (settings.portable_mode) {
        snprintf(send, UTOX_FILE_NAME_LENGTH, "%s\\Tox_Auto_Accept", portable_mode_save_path);
        LOG_NOTE("Native", "Auto Accept Directory: \"%s\"" , send);
    } else if (!SHGetFolderPath(NULL, CSIDL_DESKTOP, NULL, 0, (char *)path)) {
        swprintf(first, UTOX_FILE_NAME_LENGTH, L"%ls%ls", *path, L"\\Tox_Auto_Accept");
        CreateDirectoryW(first, NULL);
    } else {
        LOG_TRACE("NATIVE", "Unable to auto save file!" );
    }


    MultiByteToWideChar(CP_UTF8, 0, (char *)file->name, file->name_length, longname, file->name_length);
    swprintf(second, UTOX_FILE_NAME_LENGTH, L"%ls\\%ls", first, longname);

    FILE *f = _fdopen(_open_osfhandle((intptr_t)CreateFileW(second, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                                                            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL),
                                      0),
                      "wb");

    postmessage_toxcore(TOX_FILE_ACCEPT_AUTO, fid, file->file_number, f);
    debug_notice("Native:\tAuto Accept Directory: \"%s\"", second);
}

void launch_at_startup(int is_launch_at_startup) {
    HKEY hKey;
    const wchar_t *run_key_path = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

    wchar_t path[UTOX_FILE_NAME_LENGTH * 2];
    uint16_t path_length = 0, ret = 0;
    if (is_launch_at_startup == 1) {
        if (ERROR_SUCCESS == RegOpenKeyW(HKEY_CURRENT_USER, run_key_path, &hKey)) {
            path_length           = GetModuleFileNameW(NULL, path + 1, UTOX_FILE_NAME_LENGTH * 2);
            path[0]               = '\"';
            path[path_length + 1] = '\"';
            path[path_length + 2] = '\0';
            path_length += 2;
            ret = RegSetValueExW(hKey, L"uTox", NULL, REG_SZ, (uint8_t *)path, path_length * 2); /*2 bytes per wchar_t */
            if (ret == ERROR_SUCCESS) {
                LOG_TRACE(__FILE__, "Successful auto start addition." );
            }
            RegCloseKey(hKey);
        }
    }
    if (is_launch_at_startup == 0) {
        LOG_TRACE(__FILE__, "Going to delete auto start key." );
        if (ERROR_SUCCESS == RegOpenKeyW(HKEY_CURRENT_USER, run_key_path, &hKey)) {
            LOG_TRACE(__FILE__, "Successful key opened." );
            ret = RegDeleteValueW(hKey, L"uTox");
            if (ret == ERROR_SUCCESS) {
                LOG_TRACE(__FILE__, "Successful auto start deletion." );
            } else {
                LOG_TRACE(__FILE__, "UN-Successful auto start deletion." );
            }
            RegCloseKey(hKey);
        }
    }
}

#endif
