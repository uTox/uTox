#ifndef __WIN_LEGACY

#ifndef _WIN32_WINNT
 #define _WIN32_WINNT 0x0600
#endif

#include "../main.h"

void native_select_dir_ft(uint32_t fid, MSG_FILE *file)
{
    char *path = malloc(UTOX_FILE_NAME_LENGTH);
    memcpy(path, file->name, file->name_length);
    path[file->name_length] = 0;

    OPENFILENAME ofn = {
        .lStructSize = sizeof(OPENFILENAME),
        .hwndOwner = hwnd,
        .lpstrFile = path,
        .nMaxFile = UTOX_FILE_NAME_LENGTH,
        .Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT,
    };

    if(GetSaveFileName(&ofn)) {
        postmessage_toxcore(TOX_FILE_ACCEPT, fid, file->filenumber, path);
    } else {
        debug("GetSaveFileName() failed\n");
    }
}

void native_autoselect_dir_ft(uint32_t fid, FILE_TRANSFER *file)
{
    wchar_t *path[UTOX_FILE_NAME_LENGTH];
    if(!SHGetKnownFolderPath((REFKNOWNFOLDERID)&FOLDERID_Downloads, KF_FLAG_CREATE, 0, path)) {
        wchar_t first[UTOX_FILE_NAME_LENGTH]    = {0}; /* I don't trust swprintf on windows anymore, so let's help it */
        wchar_t second[UTOX_FILE_NAME_LENGTH]   = {0}; /* out a bit by initialing everything to 0                     */
        wchar_t longname[UTOX_FILE_NAME_LENGTH] = {0};

        swprintf(first, UTOX_FILE_NAME_LENGTH, L"%ls%ls", *path, L"\\Tox_Auto_Accept");
        CreateDirectoryW(first, NULL);

        MultiByteToWideChar(CP_UTF8, 0, (char*)file->name, file->name_length, longname, UTOX_FILE_NAME_LENGTH);

        swprintf(second, UTOX_FILE_NAME_LENGTH, L"%ls\\%ls", first, longname);

        char *send = calloc(UTOX_FILE_NAME_LENGTH, sizeof(char*));

        /* Windows doesn't like UTF-8 strings, so we have to hold it's hand. */
        file->file = _fdopen(_open_osfhandle((intptr_t)CreateFileW(second,
                                                                   GENERIC_WRITE,
                                                                   FILE_SHARE_READ,
                                                                   NULL,
                                                                   CREATE_ALWAYS,
                                                                   FILE_ATTRIBUTE_NORMAL,
                                                                   NULL),
                                             0),
                             "wb");

        native_to_utf8str(second, send, UTOX_FILE_NAME_LENGTH);

        debug_notice("Native:\tAuto Accept Directory: \"%s\"\n", send);

        postmessage_toxcore(TOX_FILE_ACCEPT_AUTO, fid, file->file_number, send);
    } else {
        debug("NATIVE:\tUnable to auto save file!\n");
    }
}


void launch_at_startup(int is_launch_at_startup){
    HKEY hKey;
    const wchar_t* run_key_path = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    wchar_t path[UTOX_FILE_NAME_LENGTH*2];
    uint16_t path_length = 0, ret = 0;
    if(is_launch_at_startup == 1){
        if(ERROR_SUCCESS == RegOpenKeyW(HKEY_CURRENT_USER, run_key_path, &hKey)){
            path_length = GetModuleFileNameW(NULL, path+1, UTOX_FILE_NAME_LENGTH*2);
            path[0] = '\"';
            path[path_length+1] = '\"';
            path[path_length+2] = '\0';
            path_length += 2;
            ret = RegSetKeyValueW(hKey, NULL, (LPCSTR)(L"uTox"), REG_SZ, path, path_length*2); /*2 bytes per wchar */
            if(ret == ERROR_SUCCESS){
                debug("Successful auto start addition.\n");
            }
            RegCloseKey(hKey);
        }
    }
    if(is_launch_at_startup == 0){
        if(ERROR_SUCCESS == RegOpenKeyW(HKEY_CURRENT_USER, run_key_path, &hKey)){
            ret = RegDeleteKeyValueW(hKey, NULL, L"uTox");
            if(ret == ERROR_SUCCESS){
                debug("Successful auto start deletion.\n");
            }
            RegCloseKey(hKey);
        }
    }
}

#endif
