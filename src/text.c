#include "text.h"

#include "debug.h"
#include "macros.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int sprint_humanread_bytes(char *dest, unsigned int size, uint64_t bytes) {
    char * str[]  = { "B", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB", "ZiB", "YiB" };
    int    max_id = COUNTOF(str) - 1;
    int    i      = 0;
    double f      = bytes;
    while ((bytes >= 1024) && (i < max_id)) {
        bytes /= 1024;
        f /= 1024.0;
        i++;
    }

    size_t r;

    r = snprintf((char *)dest, size, "%u", (uint32_t)bytes);

    if (r >= size) { // truncated
        r = size - 1;
    } else {
        // missing decimals
        r += snprintf((char *)dest + r, size - r, "%s", str[i]);
        if (r >= size) { // truncated
            r = size - 1;
        }
    }

    return r;
}

uint8_t utf8_len(const char *data) {
    uint32_t unused;

    return utf8_len_read(data, &unused);
}

uint8_t utf8_len_read(const char *data, uint32_t *ch) {
    char a = data[0];

    if (!(a & 0x80)) { /* not a multi-byte character */
        *ch = data[0];
        return 1;
    }

    if (!(a & 0x20)) { /* 110xxxxx */
        *ch = ((data[0] & 0x1F) << 6) | (data[1] & 0x3F);
        return 2;
    }

    if (!(a & 0x10)) { /* 1110xxxx */
        *ch = ((data[0] & 0xF) << 12) | ((data[1] & 0x3F) << 6) | (data[2] & 0x3F);
        return 3;
    }

    if (!(a & 0x08)) { /* 11110xxx */
        *ch = ((data[0] & 0x7) << 18) | ((data[1] & 0x3F) << 12) | ((data[2] & 0x3F) << 6) | (data[3] & 0x3F);
        return 4;
    }

    // unreachable
    LOG_WARN("Text", "utf8_len_read() reached the unreachable; someone is passing invalid UTF-8 characters.");
    return 0;
}

uint8_t utf8_unlen(char *data) {
    uint8_t len = 1;
    if (*(data - 1) & 0x80) {
        do {
            len++;
        } while (!(*(data - len) & 0x40));
    }

    return len;
}

int utf8_validate(const uint8_t *data, int maxlen) {
    int len, n;

    len = 0;
    while (1) {
        if ('\0' == data[len])
            break;

        n = utf8_len((char *)data + len);
        if (!n || len + n > maxlen)
            break;

        len += n;
    }

    return len;
}

uint8_t unicode_to_utf8_len(uint32_t ch) {
    if (ch > 0x1FFFFF) {
        return 0;
    }
    return 4 - (ch <= 0xFFFF) - (ch <= 0x7FF) - (ch <= 0x7F);
}

void unicode_to_utf8(uint32_t ch, char *dst) {
    uint32_t HB = (uint32_t)0x80;
    uint32_t SB = (uint32_t)0x3F;
    if (ch <= 0x7F) {
        dst[0] = (uint8_t)ch;
        return; // 1;
    }
    if (ch <= 0x7FF) {
        dst[0] = (uint8_t)((ch >> 6) | (uint32_t)0xC0);
        dst[1] = (uint8_t)((ch & SB) | HB);
        return; // 2;
    }
    if (ch <= 0xFFFF) {
        dst[0] = (uint8_t)((ch >> 12) | (uint32_t)0xE0);
        dst[1] = (uint8_t)(((ch >> 6) & SB) | HB);
        dst[2] = (uint8_t)((ch & SB) | HB);
        return; // 3;
    }
    if (ch <= 0x1FFFFF) {
        dst[0] = (uint8_t)((ch >> 18) | (uint32_t)0xF0);
        dst[1] = (uint8_t)(((ch >> 12) & SB) | HB);
        dst[2] = (uint8_t)(((ch >> 6) & SB) | HB);
        dst[3] = (uint8_t)((ch & SB) | HB);
        return; // 4;
    }
    return; // 0;
}

bool memcmp_case(const char *s1, const char *s2, uint32_t n) {
    uint32_t i;

    for (i = 0; i < n; i++) {
        char c1, c2;

        c1 = s1[i];
        c2 = s2[i];

        if (c1 >= (char)'a' && c1 <= (char)'z') {
            c1 += ('A' - 'a');
        }

        if (c2 >= (char)'a' && c2 <= (char)'z') {
            c2 += ('A' - 'a');
        }

        if (c1 != c2) {
            return 1;
        }
    }

    return 0;
}

char *tohtml(const char *str, uint16_t length) {
    uint16_t i   = 0;
    int      len = 0;
    while (i != length) {
        switch (str[i]) {
            case '<':
            case '>': {
                len += 3;
                break;
            }

            case '&': {
                len += 4;
                break;
            }
        }

        i += utf8_len(str + i);
    }

    char *out = malloc(length + len + 1);
    i         = 0;
    len       = 0;
    while (i != length) {
        switch (str[i]) {
            case '<':
            case '>': {
                memcpy(out + len, str[i] == '>' ? "&gt;" : "&lt;", 4);
                len += 4;
                i++;
                break;
            }

            case '&': {
                memcpy(out + len, "&amp;", 5);
                len += 5;
                i++;
                break;
            }

            default: {
                uint16_t r = utf8_len(str + i);
                memcpy(out + len, str + i, r);
                len += r;
                i += r;
                break;
            }
        }
    }

    out[len] = 0;

    return out;
}

void to_hex(char *out, uint8_t *in, int size) {
    while (size--) {
        if (*in >> 4 < 0xA) {
            *out++ = '0' + (*in >> 4);
        } else {
            *out++ = 'A' + (*in >> 4) - 0xA;
        }

        if ((*in & 0xf) < 0xA) {
            *out++ = '0' + (*in & 0xF);
        } else {
            *out++ = 'A' + (*in & 0xF) - 0xA;
        }
        in++;
    }
}

bool strstr_case(const char *a, const char *b) {
    const char *c = b;
    while (*a) {
        if (tolower(*a) != tolower(*c)) {
            c = b;
        }

        if (tolower(*a) == tolower(*c)) {
            c++;
            if (!*c) {
                return 1;
            }
        }
        a++;
    }

    return 0;
}

uint16_t safe_shrink(const char *string, uint16_t string_length, uint16_t shrink_length) {
    if (!string) {
        return 0;
    }

    uint16_t length = 0;
    while (length < string_length) {
        uint8_t char_length = utf8_len(&string[length]);
        length += char_length;

        if (length >= shrink_length) {
            length -= char_length;
            break;
        }
    }

    return length;
}
