#ifndef _WIN32_WINNT
 #define _WIN32_WINNT 0x0600
#endif

#include <windows.h>

void os_window_interactions(int type, int delta_x, int delta_y){
    debug("delta x == %i\n", delta_x);
    debug("delta y == %i\n", delta_y);
    utox_window_y += delta_y;
    SetWindowPos(hwnd, 0, 0, utox_window_y, utox_window_width, utox_window_height, SWP_NOSIZE | SWP_NOZORDER | SWP_NOREDRAW);
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
