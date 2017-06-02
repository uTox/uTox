#include "utf8.h"
#include <windows.h>

int utf8tonative(const char *str, wchar_t *out, int length) {
    return MultiByteToWideChar(CP_UTF8, 0, (char *)str, length, out, length);
}

int utf8_to_nativestr(const char *str, wchar_t *out, int length) {
    /* must be null terminated string                   ↓ */
    return MultiByteToWideChar(CP_UTF8, 0, (char *)str, -1, out, length);
}

// TODO, add utf8 support
bool sanitize_filename(uint8_t *filename) {
    for (size_t i = 0; filename[i] != '\0'; ++i) {
        if (filename[i] < 32) {
            return false;
        }

        if (strchr("<>:\"/\\|?*", filename[i])) {
            filename[i] = '_';
        }
    }

    return true;
}
