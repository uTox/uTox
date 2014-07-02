#include "main.h"
#include <tox/toxdns.h>

static struct tox3
{
    uint8_t *name;
    void *dns3;
    uint8_t key[32];
} tox3_server[] = {
    {
        .name = (uint8_t*)"utox.org",
        .key = {0xD3, 0x15, 0x4F, 0x65, 0xD2, 0x8A, 0x5B, 0x41, 0xA0, 0x5D, 0x4A, 0xC7, 0xE4, 0xB3, 0x9C, 0x6B,
        0x1C, 0x23, 0x3C, 0xC8, 0x57, 0xFB, 0x36, 0x5C, 0x56, 0xE8, 0x39, 0x27, 0x37, 0x46, 0x2A, 0x12}
    },
    {
        .name = (uint8_t*)"toxme.se",
        .key = {0x5D, 0x72, 0xC5, 0x17, 0xDF, 0x6A, 0xEC, 0x54, 0xF1, 0xE9, 0x77, 0xA6, 0xB6, 0xF2, 0x59, 0x14,
        0xEA, 0x4C, 0xF7, 0x27, 0x7A, 0x85, 0x02, 0x7C, 0xD9, 0xF5, 0x19, 0x6D, 0xF1, 0x7E, 0x0B, 0x13}
    }
};

static void* istox3(uint8_t *name, uint16_t name_length)
{
    int i;
    for(i = 0; i != countof(tox3_server); i++) {
        struct tox3 *t = &tox3_server[i];
        if(memcmp(name, t->name, name_length) == 0 && t->name[name_length] == 0) {
            //what if two threads reach this point at the same time?->initialize all dns3 at start instead
            if(!t->dns3) {
                t->dns3 = tox_dns3_new(t->key);
            }
            return t->dns3;
        }
    }

    return NULL;
}

static void writechecksum(uint8_t *address)
{
    uint8_t *checksum = address + 36;
    uint32_t i;

    for (i = 0; i < 36; ++i)
        checksum[i % 2] ^= address[i];
}

static uint32_t parseargument(uint8_t *dest, char_t *src, uint16_t length, void **pdns3)
{
    /* parses format groupbot@utox.org -> groupbot._tox.utox.org */

    _Bool reset = 0, at = 0;
    char_t *a = src, *b = src + length;
    uint8_t *d = dest;
    uint32_t pin = 0;
    while(a != b) {
        if(*a == ':')
        {
            a++;
            int bits = 32;
            while(a != b)
            {
                uint8_t ch;
                if(*a >= 'A' && *a <= 'Z')
                {
                    ch = (*a - 'A');
                } else if(*a >= 'a' && *a <= 'z')
                {
                    ch = (*a - 'a' + 26);
                } else if(*a >= '0' && *a <= '9')
                {
                    ch = (*a - '0' + 52);
                } else if(*a == '+') {
                    ch = 62;
                } else if(*a == '/') {
                    ch = 63;
                } else
                {
                    break;
                }

                bits -= 6;
                if(bits >= 0){pin |= (uint32_t)ch << bits;
                } else
                {
                    pin |= (uint32_t)ch >> -bits;
                }

                a++;
            }
            break;
        }
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
            void *dns3;
            if((dns3 = istox3(a + 1, b - (a + 1)))) {
                uint8_t out[256];
                int len = tox_generate_dns3_string(dns3, out, sizeof(out), &pin, dest, d - dest);
                if(len != -1) {
                    dest[0] = '_';
                    memcpy(dest + 1, out, len);
                    d = dest + 1 + len;
                    *pdns3 = dns3;
                }
            }

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
        memcpy(d, "._tox.utox.org", 14);
        d += 14;
    }

    *d = 0;

    debug("Parsed: (%.*s)->(%s) pin=%X\n", length, src, dest, pin);

    return pin;
}

static _Bool parserecord(uint8_t *dest, uint8_t *src, uint32_t pin, void *dns3)
{
    _Bool _id = 0, _pub = 0, b = 0;
    uint32_t version = 0;
    uint8_t *id = dest;
    uint8_t *a = src;
    while(*a) {
        if(_id) {
            uint8_t ch;
            if(*a >= '0' && *a <= '9') {
                if(id == dest + 38) {
                    debug("id too long\n");
                    return 0;
                }
                ch = *a - '0';
            } else if(*a >= 'A' && *a <= 'F') {
                if(id == dest + 38) {
                    debug("id too long\n");
                    return 0;
                }
                ch = *a - 'A' + 10;
            } else if(*a != ' ') {
                if(id != dest + 38) {
                    debug("id too short\n");
                    return 0;
                }
                _id = 0;
            }

            if(_id) {
                if(!b) {
                    *id = ch << 4;
                } else {
                    *id |= ch;
                    id++;
                }
                b = !b;
            }
        }

        if(_pub) {
            uint8_t ch;
            if(*a >= '0' && *a <= '9') {
                if(id == dest + 32) {
                    debug("id too long\n");
                    return 0;
                }
                ch = *a - '0';
            } else if(*a >= 'A' && *a <= 'F') {
                if(id == dest + 32) {
                    debug("id too long\n");
                    return 0;
                }
                ch = *a - 'A' + 10;
            } else if(*a != ' ') {
                if(id != dest + 32) {
                    debug("id too short\n");
                    return 0;
                }
                _pub = 0;

                memcpy(id, &pin, 4); id += 4;
                writechecksum(dest);
            }

            if(_pub) {
                if(!b) {
                    *id = ch << 4;
                } else {
                    *id |= ch;
                    id++;
                }
                b = !b;
            }
        }


        if(version == 1 && dest == id && memcmp("id=", a, 3) == 0) {
            _id = 1;
            a += 2;
        }

        if(version == 3 && dest == id && memcmp("id=", a, 3) == 0) {
            a += 3;
            return (tox_decrypt_dns3_TXT(dns3, id, a, strlen((char*)a), pin) == 0);
        }

        if(version == 2 && dest == id && memcmp("pub=", a, 4) == 0) {
            _pub = 1;
            a += 3;
        }

        if(!version && memcmp("v=tox", a, 5) == 0) {
            if(a[5] >= '1' && a[5] <= '3') {
                version = a[5] - '0';
                a += 5;
            }
        }

        a++;
    }

    if(!version) {
        debug("invalid version\n");
        return 0;
    }

    return 1;
}

static void dns_thread(void *data)
{
    uint16_t length = *(uint16_t*)data;
    uint8_t result[256];

    void *dns3 = NULL;
    uint32_t pin = parseargument(result, data + 2, length, &dns3);
    _Bool success = 0;

    #ifdef __WIN32__
    DNS_RECORD *record = NULL;
    DnsQuery((char*)result, DNS_TYPE_TEXT, 0, NULL, &record, NULL);
    while(record) {
        /* just take the first successfully parsed record (for now), and only parse the first string (seems to work) */
        DNS_TXT_DATA *txt = &record->Data.Txt;
        if(txt->dwStringCount) {
            debug("Attempting:\n%s\n", txt->pStringArray[0]);
            if((success = parserecord(data, (uint8_t*)txt->pStringArray[0], pin, dns3))) {
                break;
            }
        }

        record = record->pNext;
    }
    #else
    uint8_t answer[PACKETSZ + 1], *answend, *pt;
    char host[128];
    int len, type;
    unsigned int size, txtlen = 0;

    if((len = res_query((char*)result, C_IN, T_TXT, answer, PACKETSZ)) >= 0) {
        answend = answer + len;
        pt = answer + sizeof(HEADER);

        if((len = dn_expand(answer, answend, pt, host, sizeof(host))) < 0) {
            printf("^dn_expand failed\n");
            goto FAIL;
        }

        pt += len;
        if(pt > answend - 4) {
            printf("^Bad (too short) DNS reply\n");
            goto FAIL;
        }

        GETSHORT(type, pt);
        if(type != T_TXT) {
            printf("^Broken DNS reply.\n");
            goto FAIL;
        }

        pt += INT16SZ; /* class */
        size = 0;
        do { /* recurse through CNAME rr's */
            pt += size;
            if((len = dn_expand(answer, answend, pt, host, sizeof(host))) < 0) {
                printf("^second dn_expand failed\n");
                goto FAIL;
            }
            printf("^Host: %s\n", host);
            pt += len;
            if(pt > answend-10) {
                printf("^Bad (too short) DNS reply\n");
                goto FAIL;
            }
            GETSHORT(type, pt);
            pt += INT16SZ; /* class */
            pt += 4;//GETLONG(cttl, pt);
            GETSHORT(size, pt);
            if(pt + size < answer || pt + size > answend) {
                printf("^DNS rr overflow\n");
                goto FAIL;
            }
        } while(type == T_CNAME);

        if(type != T_TXT) {
            printf("^Not a TXT record\n");
            goto FAIL;
        }

        if(!size || (txtlen = *pt) >= size || !txtlen) {
            printf("^Broken TXT record (txtlen = %d, size = %d)\n", txtlen, size);
            goto FAIL;
        }

        //if(!(txt = (char *) malloc(txtlen + 1)))
            //return NULL;

        //memcpy(txt, pt+1, txtlen);
        //txt[txtlen] = 0;
        //*ttl = cttl;

        //answer[len] = 0;
        //debug("%u %u\n", txtlen, answend - (pt + 1));
        debug("Attempting:\n%.*s\n", txtlen, pt + 1);

        pt[txtlen + 1] = 0;
        success = parserecord(data, pt + 1, pin, dns3);
    } else {
        debug("timeout\n");
    }
    FAIL:
    #endif

    postmessage(DNS_RESULT, success, 0, data);
}

void dns_request(char_t *name, uint16_t length)
{
    void *data = malloc((2 + length < TOX_FRIEND_ADDRESS_SIZE) ? TOX_FRIEND_ADDRESS_SIZE : 2 + length * sizeof(char_t));
    memcpy(data, &length, 2);
    memcpy(data + 2, name, length * sizeof(char_t));

    thread(dns_thread, data);
}
