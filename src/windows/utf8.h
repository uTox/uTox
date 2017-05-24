#ifndef WIN_UTF8_H
#define WIN_UTF8_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// TODO: Maybe this should be a more generic text-util Windows header.

/** Translate a char* from UTF-8 encoding to OS native;
 *
 * Accepts char pointer, native array pointer, length of input;
 * Returns: number of chars writen, or 0 on failure.
 *
 */
int utf8tonative(const char *str, wchar_t *out, int length);
int utf8_to_nativestr(const char *str, wchar_t *out, int length);

/**
* @brief Translate an OS native string to UTF-8 char*.
*
* @param str_in native array pointer.
* @param str_out char pointer.
* @param length of input string.
*
*/
int native_to_utf8str(const wchar_t *str_in, char *str_out, uint32_t max_size);

/**
 * Replaces all Windows-forbidden characters in the filename with underscores.
 *
 * @param filename a null-terminated string.
 * @return resulted filename is valid or not.
 */
bool sanitize_filename(uint8_t *filename);

#endif
