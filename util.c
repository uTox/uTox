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

static void to_hex(uint8_t *a, uint8_t *p, int size)
{
    uint8_t b, c, *end = p + size;

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

void id_to_string(uint8_t *dest, uint8_t *src)
{
    to_hex(dest, src, TOX_FRIEND_ADDRESS_SIZE);
}

void cid_to_string(uint8_t *dest, uint8_t *src)
{
    to_hex(dest, src, TOX_CLIENT_ID_SIZE);
}

_Bool string_to_id(uint8_t *w, uint8_t *a)
{
    uint8_t *end = w + TOX_FRIEND_ADDRESS_SIZE;
    while(w != end) {
        uint8_t c, v;

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
