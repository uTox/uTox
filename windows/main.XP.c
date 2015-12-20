#ifdef __WIN_LEGACY

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
            ret = RegSetValueExW(hKey, L"uTox", NULL, REG_SZ, (uint8_t*)path, path_length*2); /*2 bytes per wchar */
            if(ret == ERROR_SUCCESS){
                debug("Successful auto start addition.\n");
            }
            RegCloseKey(hKey);
        }
    }
    if(is_launch_at_startup == 0){
        debug("Going to delete auto start key.\n");
        if(ERROR_SUCCESS == RegOpenKeyW(HKEY_CURRENT_USER, run_key_path, &hKey)){
            debug("Successful key opened.\n");
            ret = RegDeleteValueW(hKey, L"uTox");
            if(ret == ERROR_SUCCESS){
                debug("Successful auto start deletion.\n");
            } else {
                debug("UN-Successful auto start deletion.\n");
            }
            RegCloseKey(hKey);
        }
    }
}

#endif
