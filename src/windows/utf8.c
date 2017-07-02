#include "utf8.h"
#include <windows.h>

int utf8tonative(const char *str, wchar_t *out, int length) {
    return MultiByteToWideChar(CP_UTF8, 0, (char *)str, length, out, length);
}

/**
 * Caution!
 *
 * Using the MultiByteToWideChar function incorrectly can compromise the security of your application. Calling this
 * function can easily cause a buffer overrun because the size of the input buffer indicated by lpMultiByteStr equals
 * the number of bytes in the string, while the size of the output buffer indicated by lpWideCharStr equals the number
 * of characters. To avoid a buffer overrun, your application must specify a buffer size appropriate for the data type
 * the buffer receives.
 * For more information, see Security Considerations: International Features.
 */
int utf8_to_nativestr(const char *str, wchar_t *out, int length) {
    /* must be null terminated string            ↓ */
    return MultiByteToWideChar(CP_UTF8, 0, str, -1, out, length);
}

/**
 * Caution!
 *
 * Using the WideCharToMultiByte function incorrectly can compromise the security of your application. Calling this
 * function can easily cause a buffer overrun because the size of the input buffer indicated by lpWideCharStr equals the
 * number of characters in the Unicode string, while the size of the output buffer indicated by lpMultiByteStr equals
 * the number of bytes. To avoid a buffer overrun, your application must specify a buffer size appropriate for the data
 * type the buffer receives.
 * Data converted from UTF-16 to non-Unicode encodings is subject to data loss, because a code page might not be able to
 * represent every character used in the specific Unicode data.
 * For more information, see Security Considerations: International Features.
 */
int native_to_utf8str(const wchar_t *str_in, char *str_out, uint32_t max_size) {
    return WideCharToMultiByte(CP_UTF8, 0, str_in, -1, str_out, max_size, NULL, NULL);
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
