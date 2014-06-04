#include "main.h"
#include <windns.h>

static void parseargument(uint8_t *dest, uint8_t *src, uint16_t length)
{
    _Bool reset = 0, at = 0;
    uint8_t *a = src, *b = src + length, *d = dest;
    while(a != b) {
        switch(*a) {
        case '0' ... '9':
        case 'A' ... 'Z':
        case 'a' ... 'z':
        case '.': {
            if(reset) {
                d = dest;
                reset = 0;
                at = 0;
            }
            *d++ = *a;
            break;
        }

        case '@': {
            memcpy(d, "._tox.", 6);
            d += 6;
            at = 1;
            break;
        }

        default: {
            reset = 1;
            break;
        }
        }
        a++;
    }

    if(!at) {
        memcpy(d, "._tox.toxme.se", 14);
        d += 14;
    }

    *d = 0;

    debug("Parsed: (%.*s)->(%s)\n", length, src, dest);
}

static _Bool parserecord(uint8_t *dest, uint8_t *src)
{
    _Bool _id = 0, version = 0;
    uint8_t id[TOX_FRIEND_ADDRESS_SIZE * 2];
    uint8_t *a = src, *i = id;
    while(*a) {
        if(_id) {
            if((*a >= '0' && *a <= '9') || (*a >= 'A' && *a <= 'F')) {
                if(i == id + sizeof(id)) {
                    debug("id too long\n");
                    return 0;
                }
                *i++ = *a;
            } else if(*a != ' ') {
                if(i != id + sizeof(id)) {
                    debug("id too short\n");
                    return 0;
                }
                _id = 0;
            }
        }

        if(i == id && memcmp("id=", a, 3) == 0) {
            _id = 1;
            a += 2;
        }

        if(!version && memcmp("v=tox1", a, 6) == 0) {
            version = 1;
        }

        a++;
    }

    if(!version) {
        debug("invalid version\n");
        return 0;
    }

    debug("Parsed: %.*s\n", sizeof(id), id);

    return string_to_id(dest, id);
}

static void dns_thread(void *data)
{
    uint16_t length = *(uint16_t*)data;
    uint8_t result[256];

    parseargument(result, data + 2, length);

    DNS_RECORD *record = NULL;
    _Bool success = 0;
    DnsQuery((char*)result, DNS_TYPE_TEXT, 0, NULL, &record, NULL);
    while(record) {
        /* just take the first successfully parsed record (for now), and only parse the first string (seems to work) */
        DNS_TXT_DATA *txt = &record->Data.Txt;
        if(txt->pStringArray[0]) {
            if((success = parserecord(data, (uint8_t*)txt->pStringArray[0]))) {
                break;
            }
        }

        record = record->pNext;
    }

    postmessage(DNS_RESULT, success, 0, data);
}

void dns_request(uint8_t *name, uint16_t length)
{
    void *data = malloc((2 + length < TOX_FRIEND_ADDRESS_SIZE) ? TOX_FRIEND_ADDRESS_SIZE : 2 + length);
    memcpy(data, &length, 2);
    memcpy(data + 2, name, length);

    thread(dns_thread, data);
}
