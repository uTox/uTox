#include "main.h"

void* file_raw(char *path, uint32_t *size)
{
    FILE *file;
    char *data;
    int len;

    file = fopen(path, "rb");
    if(!file) {
        debug("File not found (%s)\n", path);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    len = ftell(file);
    data = malloc(len);
    if(!data) {
        fclose(file);
        return NULL;
    }

    fseek(file, 0, SEEK_SET);

    if(fread(data, len, 1, file) != 1) {
        debug("Read error (%s)\n", path);
        fclose(file);
        free(data);
        return NULL;
    }

    fclose(file);

    debug("Read %u bytes (%s)\n", len, path);

    if(size) {
        *size = len;
    }
    return data;
}

static void to_hex(char_t *a, char_t *p, int size)
{
    char_t b, c, *end = p + size;

    while(p != end) {
        b = *p++;

        c = (b & 0xF);
        b = (b >> 4);

        if(b < 10) {
            *a++ = b + '0';
        } else {
            *a++ = b - 10 + 'A';
        }

        if(c < 10) {
            *a++ = c + '0';
        } else {
            *a++ = c  - 10 + 'A';
        }
    }
}

void id_to_string(char_t *dest, char_t *src)
{
    to_hex(dest, src, TOX_FRIEND_ADDRESS_SIZE);
}

void cid_to_string(char_t *dest, char_t *src)
{
    to_hex(dest, src, TOX_CLIENT_ID_SIZE);
}

_Bool string_to_id(char_t *w, char_t *a)
{
    char_t *end = w + TOX_FRIEND_ADDRESS_SIZE;
    while(w != end) {
        char_t c, v;

        c = *a++;
        if(c >= '0' && c <= '9') {
            v = (c - '0') << 4;
        } else if(c >= 'A' && c <= 'F') {
            v = (c - 'A' + 10) << 4;
        } else {
            return 0;
        }

        c = *a++;
        if(c >= '0' && c <= '9') {
            v |= (c - '0');
        } else if(c >= 'A' && c <= 'F') {
            v |= (c - 'A' + 10);
        } else {
            return 0;
        }

        *w++ = v;
    }

    return 1;
}

int sprint_bytes(uint8_t *dest, uint64_t bytes)
{
    char *str[] = {"B", "KiB", "MiB", "GiB"};
    int i = 0;
    double f = bytes;
    while(bytes >= 1024)
    {
        bytes /= 1024;
        f /= 1024.0;
        i++;
    }

    int r;

    r = sprintf((char*)dest, "%u", (uint32_t)bytes);
    //missing decimals
    r += sprintf((char*)dest + r, "%s", str[i]);
    return r;
}

uint8_t utf8_len(char_t *data)
{
    if(!(*data & 0x80)) {
        return 1;
    }

    uint8_t bytes = 1, i;
    for(i = 6; i != 0xFF; i--) {
        if (!((*data >> i) & 1)) {
            break;
        }
        bytes++;
    }
    //no validation, instead validate all utf8 when recieved
    return bytes;
}

uint8_t utf8_len_read(char_t *data, uint32_t *ch)
{
    uint8_t a = data[0];
    if(!(a & 0x80)) {
        *ch = data[0];
        return 1;
    }

    if(!(a & 0x20)) {
        *ch = ((data[0] & 0x1F) << 6) | (data[1] & 0x3F);
        return 2;
    }

    if(!(a & 0x10)) {
        *ch =  ((data[0] & 0xF) << 12) | ((data[1] & 0x3F) << 6) | (data[2] & 0x3F);
        return 3;
    }

    if(!(a & 8)) {
        *ch =  ((data[0] & 0x7) << 18) | ((data[1] & 0x3F) << 12) | ((data[2] & 0x3F) << 6) | (data[3] & 0x3F);
        return 4;
    }

    if(!(a & 4)) {
        *ch =  ((data[0] & 0x3) << 24) | ((data[1] & 0x3F) << 18) | ((data[2] & 0x3F) << 12) | ((data[3] & 0x3F) << 6) | (data[4] & 0x3F);
        return 5;
    }

    if(!(a & 2)) {
        *ch =  ((data[0] & 0x1) << 30) | ((data[1] & 0x3F) << 24) | ((data[2] & 0x3F) << 18) | ((data[3] & 0x3F) << 12) | ((data[4] & 0x3F) << 6) | (data[5] & 0x3F);
        return 6;
    }

    //never happen
    return 0;
}

uint8_t utf8_unlen(char_t *data)
{
    uint8_t len = 1;
    if(*(data - 1) & 0x80) {
        do {
            len++;
        } while (!(*(data - len) & 0x40));
    }

    return len;
}

int utf8_validate(const char_t *data, int len)
{
    //stops when an invalid character is reached
    const char_t *a = data, *end = data + len;
    while(a != end) {
        if(!(*a & 0x80)) {
            a++;
            continue;
        }

        uint8_t bytes = 1, i;
        for(i = 6; i != 0xFF; i--) {
            if (!((*a >> i) & 1)) {
                break;
            }
            bytes++;
        }

        if(bytes == 1 || bytes == 8) {
            break;
        }

        // Validate the utf8
        if(a + bytes > end) {
            break;
        }

        for(i = 1; i < bytes; i++) {
            if(!(a[i] & 0x80) || (a[i] & 0x40)) {
                return a - data;
            }
        }

        a += bytes;
    }

    return a - data;
}
