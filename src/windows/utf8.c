#include "utf8.h"

#include <windows.h>

int utf8tonative(const char *str, wchar_t *out, int length) {
    return MultiByteToWideChar(CP_UTF8, 0, (char *)str, length, out, length);
}

int utf8_to_nativestr(const char *str, wchar_t *out, int length) {
    /* must be null terminated string                   ↓ */
    return MultiByteToWideChar(CP_UTF8, 0, (char *)str, -1, out, length);
}
