#ifndef __WIN_LEGACY

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include "main.h"

#include "../main.h"

#include "../chatlog.h"
#include "../filesys.h"
#include "../file_transfers.h"
#include "../friend.h"
#include "../debug.h"
#include "../tox.h"
#include "../settings.h"

#include <shlobj.h>

void native_export_chatlog_init(uint32_t friend_number) {
    char *path = calloc(1, UTOX_FILE_NAME_LENGTH);
    if (path == NULL){
        LOG_ERR("SelectDir", " Could not allocate memory." );
        return;
    }

    FRIEND *f = get_friend(friend_number);
    if (!f) {
        LOG_ERR("Windows7", "Could not get friend with number: %u", friend_number);
        return;
    }

    snprintf(path, UTOX_FILE_NAME_LENGTH, "%.*s.txt", (int)f->name_length,
             f->name);

    OPENFILENAME ofn = {
        .lStructSize = sizeof(OPENFILENAME),
        .lpstrFilter = ".txt",
        .lpstrFile   = path,
        .nMaxFile    = UTOX_FILE_NAME_LENGTH,
        .Flags       = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT,
        .lpstrDefExt = "txt",
    };

    if (GetSaveFileName(&ofn)) {
        FILE *file = fopen(path, "wb");
        if (file) {
            utox_export_chatlog(f->id_str, file);
        } else {
            LOG_ERR("Windows7", "Opening file %s failed", path);
        }
    } else {
        LOG_ERR("Windows7", "Unable to open file and export chatlog");
    }
}

void native_select_dir_ft(uint32_t fid, uint32_t num, FILE_TRANSFER *file) {
    char *path = calloc(1, UTOX_FILE_NAME_LENGTH);
    if (path == NULL){
        LOG_ERR("SelectDir", " Could not allocate memory for path." );
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
        LOG_ERR("Windows7", "Unable to Get save file for incoming FT");
    }
}

void native_autoselect_dir_ft(uint32_t fid, FILE_TRANSFER *file) {
    char *send = calloc(UTOX_FILE_NAME_LENGTH, sizeof(char *));
    if (send == NULL){
        LOG_ERR("AutoSelectDir", " Could not allocate memory." );
        return;
    }

    wchar_t *path[UTOX_FILE_NAME_LENGTH];
    wchar_t  sub_path[UTOX_FILE_NAME_LENGTH] = { 0 }; /* I don't trust swprintf on windows anymore, so let's help it */
    wchar_t  fullpath[UTOX_FILE_NAME_LENGTH] = { 0 }; /* out a bit by initialing everything to 0                     */
    wchar_t  longname[UTOX_FILE_NAME_LENGTH] = { 0 };

    if (settings.portable_mode) {
        snprintf(send, UTOX_FILE_NAME_LENGTH, "%s\\Tox_Auto_Accept", portable_mode_save_path);
        LOG_NOTE("Windows7", "Auto Accept Directory: \"%s\"" , send);
    } else if (!SHGetKnownFolderPath((REFKNOWNFOLDERID)&FOLDERID_Downloads, KF_FLAG_CREATE, 0, path)) {
        swprintf(sub_path, UTOX_FILE_NAME_LENGTH, L"%ls%ls", *path, L"\\Tox_Auto_Accept");
        CreateDirectoryW(sub_path, NULL);
    } else {
        LOG_ERR("Windows7", "Unable to auto save file!" );
    }
    LOG_NOTE("Windows7", "Auto Accept Directory: \"%s\"" , send);

    /* UTF8 name to windows version*/
    MultiByteToWideChar(CP_UTF8, 0, (char *)file->name, file->name_length, longname, UTOX_FILE_NAME_LENGTH);
    swprintf(fullpath, UTOX_FILE_NAME_LENGTH, L"%ls\\%ls", sub_path, longname);
    /* Windows doesn't like UTF-8 strings, so we have to hold it's hand. */
    FILE *f = _fdopen(_open_osfhandle((intptr_t)CreateFileW(fullpath, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                                                            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL),
                                      0),
                      "wb");
    postmessage_toxcore(TOX_FILE_ACCEPT_AUTO, fid, file->file_number, f);
}

void launch_at_startup(int is_launch_at_startup) {
    HKEY         hKey;
    const wchar_t *run_key_path = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    wchar_t        path[UTOX_FILE_NAME_LENGTH * 2];
    uint16_t     path_length = 0, ret = 0;
    if (is_launch_at_startup == 1) {
        if (ERROR_SUCCESS == RegOpenKeyW(HKEY_CURRENT_USER, run_key_path, &hKey)) {
            path_length           = GetModuleFileNameW(NULL, path + 1, UTOX_FILE_NAME_LENGTH * 2);
            path[0]               = '\"';
            path[path_length + 1] = '\"';
            path[path_length + 2] = '\0';
            path_length += 2;
            ret = RegSetKeyValueW(hKey, NULL, (LPCSTR)(L"uTox"), REG_SZ, path, path_length * 2); /*2 bytes per wchar_t */
            if (ret == ERROR_SUCCESS) {
                LOG_ERR("Windows7", "Unable to set Registry key for startup.");
            }
            RegCloseKey(hKey);
        }
    }
    if (is_launch_at_startup == 0) {
        if (ERROR_SUCCESS == RegOpenKeyW(HKEY_CURRENT_USER, run_key_path, &hKey)) {
            ret = RegDeleteKeyValueW(hKey, NULL, L"uTox");
            if (ret == ERROR_SUCCESS) {
                LOG_ERR("Windows7", "Unable to delete Registry key for startup.");
            }
            RegCloseKey(hKey);
        }
    }
}

#endif
