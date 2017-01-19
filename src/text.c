#include "text.h"

#include "main.h" // countof()

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int sprint_humanread_bytes(char *dest, unsigned int size, uint64_t bytes) {
    char * str[]  = { "B", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB", "ZiB", "YiB" };
    int    max_id = countof(str) - 1;
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
    if (!(*data & 0x80)) {
        return 1;
    }

    uint8_t bytes = 1, i;
    for (i = 6; i != 0xFF; i--) {
        if (!((*data >> i) & 1)) {
            break;
        }
        bytes++;
    }
    // no validation, instead validate all utf8 when recieved
    return bytes;
}

uint8_t utf8_len_read(const char *data, uint32_t *ch) {
    uint8_t a = data[0];
    if (!(a & 0x80)) {
        *ch = data[0];
        return 1;
    }

    if (!(a & 0x20)) {
        *ch = ((data[0] & 0x1F) << 6) | (data[1] & 0x3F);
        return 2;
    }

    if (!(a & 0x10)) {
        *ch = ((data[0] & 0xF) << 12) | ((data[1] & 0x3F) << 6) | (data[2] & 0x3F);
        return 3;
    }

    if (!(a & 8)) {
        *ch = ((data[0] & 0x7) << 18) | ((data[1] & 0x3F) << 12) | ((data[2] & 0x3F) << 6) | (data[3] & 0x3F);
        return 4;
    }

    if (!(a & 4)) {
        *ch = ((data[0] & 0x3) << 24) | ((data[1] & 0x3F) << 18) | ((data[2] & 0x3F) << 12) | ((data[3] & 0x3F) << 6)
              | (data[4] & 0x3F);
        return 5;
    }

    if (!(a & 2)) {
        *ch = ((data[0] & 0x1) << 30) | ((data[1] & 0x3F) << 24) | ((data[2] & 0x3F) << 18) | ((data[3] & 0x3F) << 12)
              | ((data[4] & 0x3F) << 6) | (data[5] & 0x3F);
        return 6;
    }

    // never happen
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

/* I've had some issues with this function in the past when it's given malformed data.
 * irungentoo has previouslly said, it'll never fail when given a valid utf-8 string, however the
 * utf8 standard says that applications are required to handle and correctlly respond to malformed
 * strings as they have been used in the past to create security expliots. This function is known to
 * enter an endless state, or segv on bad strings. Either way, that's bad and needs to be fixed.
 * TODO(grayhatter) TODO(anyone) */
int utf8_validate(const uint8_t *data, int len) {
    // stops when an invalid character is reached
    const uint8_t *a = data, *end = data + len;
    while (a != end) {
        if (!(*a & 0x80)) {
            a++;
            continue;
        }

        uint8_t bytes = 1, i;
        for (i = 6; i != 0xFF; i--) {
            if (!((*a >> i) & 1)) {
                break;
            }
            bytes++;
        }

        if (bytes == 1 || bytes == 8) {
            break;
        }

        // Validate the utf8
        if (a + bytes > end) {
            break;
        }

        for (i = 1; i < bytes; i++) {
            if (!(a[i] & 0x80) || (a[i] & 0x40)) {
                return a - data;
            }
        }

        a += bytes;
    }

    return a - data;
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
