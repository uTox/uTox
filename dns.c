#include "main.h"

static uint32_t parseargument(uint8_t *dest, char_t *src, uint16_t length)
{
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

    debug("Parsed: (%.*s)->(%s) pin=%X\n", length, src, dest, pin);

    return pin;
}

static _Bool parserecord(uint8_t *dest, uint8_t *src, uint32_t pin)
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
                _id = 0;
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

        if(version == 2 && dest == id && memcmp("pub=", a, 4) == 0) {
            _pub = 1;
            a += 3;
        }

        if(!version && memcmp("v=tox1", a, 6) == 0) {
            version = 1;
        }

        if(!version && memcmp("v=tox2", a, 6) == 0) {
            version = 2;
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

    uint32_t pin = parseargument(result, data + 2, length);
    _Bool success = 0;

    #ifdef __WIN32__
    DNS_RECORD *record = NULL;
    DnsQuery((char*)result, DNS_TYPE_TEXT, 0, NULL, &record, NULL);
    while(record) {
        /* just take the first successfully parsed record (for now), and only parse the first string (seems to work) */
        DNS_TXT_DATA *txt = &record->Data.Txt;
        if(txt->pStringArray[0]) {
            debug("Attempting:\n%s\n", txt->pStringArray[0]);
            if((success = parserecord(data, (uint8_t*)txt->pStringArray[0], pin))) {
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
        success = parserecord(data, pt + 1, pin);
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
