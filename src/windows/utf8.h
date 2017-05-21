#ifndef WIN_UTF8_H
#define WIN_UTF8_H

#include <stddef.h>
#include <stdint.h>

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
 * Replaces all Windows-forbidden characters in the filename with underscores.
 *
 * @param filename a null-terminated string.
 */
void sanitize_filename(uint8_t *filename);

#endif
