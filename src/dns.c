#include "dns.h"

#include "debug.h"
#include "macros.h"
#include "main_native.h"
#include "settings.h"
#include "tox.h"
#include "utox.h"

#include <tox/toxdns.h>

static struct tox3 {
    uint8_t *name;
    void *   dns3;
    uint8_t  key[32];
} tox3_server[] = {
    {.name = (uint8_t *)"utox.org",
     .key  = { 0xD3, 0x15, 0x4F, 0x65, 0xD2, 0x8A, 0x5B, 0x41, 0xA0, 0x5D, 0x4A, 0xC7, 0xE4, 0xB3, 0x9C, 0x6B,
              0x1C, 0x23, 0x3C, 0xC8, 0x57, 0xFB, 0x36, 0x5C, 0x56, 0xE8, 0x39, 0x27, 0x37, 0x46, 0x2A, 0x12 } },
    {.name = (uint8_t *)"toxme.se",
     .key  = { 0x5D, 0x72, 0xC5, 0x17, 0xDF, 0x6A, 0xEC, 0x54, 0xF1, 0xE9, 0x77, 0xA6, 0xB6, 0xF2, 0x59, 0x14,
              0xEA, 0x4C, 0xF7, 0x27, 0x7A, 0x85, 0x02, 0x7C, 0xD9, 0xF5, 0x19, 0x6D, 0xF1, 0x7E, 0x0B, 0x13 } }
};

static void *istox3(char *name, size_t name_length) {
    int i;
    for (i = 0; i != COUNTOF(tox3_server); i++) {
        struct tox3 *t = &tox3_server[i];
        if (memcmp(name, t->name, name_length) == 0 && t->name[name_length] == 0) {
            // what if two threads reach this point at the same time?->initialize all dns3 at start instead
            if (!t->dns3) {
                t->dns3 = tox_dns3_new(t->key);
            }
            return t->dns3;
        }
    }

    return NULL;
}

static void writechecksum(uint8_t *address) {
    uint8_t *checksum = address + 36;
    uint32_t i;

    for (i = 0; i < 36; ++i)
        checksum[i % 2] ^= address[i];
}

static int64_t parseargument(uint8_t *dest, char *src, size_t length, void **pdns3) {
    /* parses format groupbot@utox.org -> groupbot._tox.utox.org */

    bool reset = 0;
    bool at = 0;
    char *a = src;
    char *b = src + length;
    uint8_t *d = dest;
    uint32_t pin = 0;
    while (a != b) {
        if (*a == ':') {
            a++;
            int bits = 32;
            while (a != b) {
                uint8_t ch;
                if (*a >= 'A' && *a <= 'Z') {
                    ch = (*a - 'A');
                } else if (*a >= 'a' && *a <= 'z') {
                    ch = (*a - 'a' + 26);
                } else if (*a >= '0' && *a <= '9') {
                    ch = (*a - '0' + 52);
                } else if (*a == '+') {
                    ch = 62;
                } else if (*a == '/') {
                    ch = 63;
                } else {
                    break;
                }

                bits -= 6;
                if (bits >= 0) {
                    pin |= (uint32_t)ch << bits;
                } else {
                    pin |= (uint32_t)ch >> -bits;
                }

                a++;
            }
            break;
        }
        switch (*a) {
            case '0' ... '9':
            case 'A' ... 'Z':
            case 'a' ... 'z':
            case '.': {
                if (reset) {
                    d     = dest;
                    reset = 0;
                    at    = 0;
                }
                *d++ = *a;
                break;
            }

            case '@': {
                void *dns3;
                if ((dns3 = istox3(a + 1, b - (a + 1)))) {
                    uint8_t out[256];
                    int     len = tox_generate_dns3_string(dns3, out, sizeof(out), &pin, dest, d - dest);
                    if (len != -1) {
                        dest[0] = '_';
                        memcpy(dest + 1, out, len);
                        d      = dest + 1 + len;
                        *pdns3 = dns3;
                    } else {
                        return -1;
                    }
                }

                memcpy(d, "._tox.", 6);
                d += 6;
                at = true;
                break;
            }

            default: {
                reset = 1;
                break;
            }
        }
        a++;
    }

    if (!at) {
        if ((d - dest) > TOXDNS_MAX_RECOMMENDED_NAME_LENGTH)
            return -1;

        void *dns3 = istox3("utox.org", sizeof("utox.org") - 1);

        if (!dns3)
            return -1;

        uint8_t out[256];
        int     len = tox_generate_dns3_string(dns3, out, sizeof(out), &pin, dest, d - dest);
        if (len == -1)
            return -1;

        dest[0] = '_';
        memcpy(dest + 1, out, len);
        d      = dest + 1 + len;
        *pdns3 = dns3;

        memcpy(d, "._tox.utox.org", 14);
        d += 14;
    }

    *d = 0;

    LOG_TRACE("DNS", "Parsed: (%.*s)->(%s) pin=%X" , (int)length, src, dest, pin);

    return pin;
}

static bool parserecord(uint8_t *dest, uint8_t *src, uint32_t pin, void *dns3) {
    bool     _id = 0, _pub = 0, b = 0;
    uint32_t version = 0;
    uint8_t *id      = dest;
    uint8_t *a       = src;
    while (*a) {
        if (_id) {
            uint8_t ch = '\0';
            if (*a >= '0' && *a <= '9') {
                if (id == dest + 38) {
                    LOG_TRACE("DNS", "id too long" );
                    return 0;
                }
                ch = *a - '0';
            } else if (*a >= 'A' && *a <= 'F') {
                if (id == dest + 38) {
                    LOG_TRACE("DNS", "id too long" );
                    return 0;
                }
                ch = *a - 'A' + 10;
            } else if (*a != ' ') {
                if (id != dest + 38) {
                    LOG_TRACE("DNS", "id too short" );
                    return 0;
                }
                _id = 0;
            }

            if (_id) {
                if (!b) {
                    *id = ch << 4;
                } else {
                    *id |= ch;
                    id++;
                }
                b = !b;
            }
        }

        if (_pub) {
            uint8_t ch = '\0';
            if (*a >= '0' && *a <= '9') {
                if (id == dest + 32) {
                    LOG_TRACE("DNS", "id too long" );
                    return 0;
                }
                ch = *a - '0';
            } else if (*a >= 'A' && *a <= 'F') {
                if (id == dest + 32) {
                    LOG_TRACE("DNS", "id too long" );
                    return 0;
                }
                ch = *a - 'A' + 10;
            } else if (*a != ' ') {
                if (id != dest + 32) {
                    LOG_TRACE("DNS", "id too short" );
                    return 0;
                }
                _pub = 0;

                memcpy(id, &pin, 4);
                id += 4;
                writechecksum(dest);
            }

            if (_pub) {
                if (!b) {
                    *id = ch << 4;
                } else {
                    *id |= ch;
                    id++;
                }
                b = !b;
            }
        }


        if (version == 1 && dest == id && memcmp("id=", a, 3) == 0) {
            _id = 1;
            a += 2;
        }

        if (version == 3 && dest == id && memcmp("id=", a, 3) == 0) {
            a += 3;
            return (tox_decrypt_dns3_TXT(dns3, id, a, strlen((char *)a), pin) == 0);
        }

        if (version == 2 && dest == id && memcmp("pub=", a, 4) == 0) {
            _pub = 1;
            a += 3;
        }

        if (!version && memcmp("v=tox", a, 5) == 0) {
            if (a[5] >= '1' && a[5] <= '3') {
                version = a[5] - '0';
                a += 5;
            }
        }

        a++;
    }

    if (!version) {
        LOG_TRACE("DNS", "invalid version" );
        return 0;
    }

    return 1;
}

static void dns_thread(void *data) {
    size_t  length = *(size_t *)data;
    uint8_t result[256];
    bool    success = false;

    void *  dns3 = NULL;
    int64_t ret  = parseargument(result, (char *)data + sizeof(size_t), length, &dns3);
    if (ret == -1)
        goto FAIL;

    uint32_t pin = ret;

#ifdef __WIN32__
    DNS_RECORD *record = NULL;
    DnsQuery((char *)result, DNS_TYPE_TEXT, 0, NULL, &record, NULL);
    while (record) {
        // just take the first successfully parsed record (for now), and only parse the first string (seems to work)
        DNS_TXT_DATA *txt = &record->Data.Txt;
        if (record->wType == DNS_TYPE_TEXT && txt->dwStringCount) {
            if (txt->pStringArray[0]) {
                LOG_TRACE("DNS", "Attempting:\n%s" , txt->pStringArray[0]);
                if ((success = parserecord(data, (uint8_t *)txt->pStringArray[0], pin, dns3))) {
                    break;
                }
            }
        }

        record = record->pNext;
    }
#else
#ifdef __ANDROID__
    /* get the dns IP and make a dns request manually */
    char value[PROP_VALUE_MAX];
    __system_property_get("net.dns1", value);
    struct sockaddr_in addr = {
        .sin_family = AF_INET, .sin_port = htons(53),
    };
    int     sock, len;
    uint8_t packet[256] = {
        0x24, 0x1a,                                     // transaction id
        0x01, 0x00,                                     // flags
        0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // counts
        // name
    };

    uint8_t *l = packet + 12, *p = l + 1, *a = result, ll = 0;
    while (*a) {
        if (*a == '.') {
            *l = ll;
            l  = p;
            ll = 0;
        } else {
            *p = *a;
            ll++;
        }
        a++;
        p++;
    }
    *l   = ll;
    *p++ = 0;

    uint8_t packet_end[] = { 0x00, 0x10, 0x00, 0x01 }; // request type + IN

    memcpy(p, packet_end, 4);
    p += 4;

    if (!inet_pton(AF_INET, value, &addr.sin_addr)) {
        goto FAIL;
    }

    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        goto FAIL;
    }

    struct timeval timeout = {.tv_sec = 5, .tv_usec = 0 };

    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
        goto FAIL;
    }

    LOG_TRACE("DNS", "%s %u" , value, p - packet);

    sendto(sock, packet, p - packet, 0, (struct sockaddr *)&addr, sizeof(addr));
    if ((len = recv(sock, packet, sizeof(packet), 0)) < 0) {
        goto FAIL;
    }

    LOG_TRACE("DNS", "%s reponded %u" , value, len);

    p = memmem(packet, len, "v=tox", 5);
    if (p) {
        LOG_TRACE("DNS", "test %u" , *(p - 1));
        p[*(p - 1)] = 0;
        success     = parserecord(data, p, pin, dns3);
    }

#else
    uint8_t      answer[PACKETSZ + 1], *answend, *pt;
    char         host[128];
    int          len, type;
    unsigned int size, txtlen = 0;

    if ((len = res_query((char *)result, C_IN, T_TXT, answer, PACKETSZ)) >= 0) {
        answend = answer + len;
        pt      = answer + sizeof(HEADER);

        if ((len = dn_expand(answer, answend, pt, host, sizeof(host))) < 0) {
            LOG_TRACE("DNS", "^dn_expand failed" );
            goto FAIL;
        }

        pt += len;
        if (pt > answend - 4) {
            LOG_TRACE("DNS", "^Bad (too short) DNS reply" );
            goto FAIL;
        }

        GETSHORT(type, pt);
        if (type != T_TXT) {
            LOG_TRACE("DNS", "^Broken DNS reply." );
            goto FAIL;
        }

        pt += INT16SZ; /* class */
        size = 0;
        do { /* recurse through CNAME rr's */
            pt += size;
            if ((len = dn_expand(answer, answend, pt, host, sizeof(host))) < 0) {
                LOG_TRACE("DNS", "^second dn_expand failed" );
                goto FAIL;
            }
            LOG_TRACE("DNS", "^Host: %s" , host);
            pt += len;
            if (pt > answend - 10) {
                LOG_TRACE("DNS", "^Bad (too short) DNS reply" );
                goto FAIL;
            }
            GETSHORT(type, pt);
            pt += INT16SZ; /* class */
            pt += 4;       // GETLONG(cttl, pt);
            GETSHORT(size, pt);
            if (pt + size < answer || pt + size > answend) {
                LOG_TRACE("DNS", "^DNS rr overflow" );
                goto FAIL;
            }
        } while (type == T_CNAME);

        if (type != T_TXT) {
            LOG_TRACE("DNS", "^Not a TXT record" );
            goto FAIL;
        }

        if (!size || (txtlen = *pt) >= size || !txtlen) {
            LOG_TRACE("DNS", "^Broken TXT record (txtlen = %d, size = %d)" , txtlen, size);
            goto FAIL;
        }

        // if(!(txt = (char *) malloc(txtlen + 1)))
        // return NULL;

        // memcpy(txt, pt+1, txtlen);
        // txt[txtlen] = 0;
        //*ttl = cttl;

        // answer[len] = 0;
        // LOG_TRACE("DNS", "%u %u" , txtlen, answend - (pt + 1));
        LOG_TRACE("DNS", "Attempting:\n%.*s" , txtlen, pt + 1);

        pt[txtlen + 1] = 0;
        success        = parserecord(data, pt + 1, pin, dns3);
    } else {
        LOG_TRACE("DNS", "timeout" );
    }
#endif
#endif
FAIL:

    postmessage_utox(DNS_RESULT, success, 0, data);
}

void dns_request(char *name, size_t length) {
    if (settings.force_proxy) {
        LOG_TRACE("DNS", "uTox DNS:\tUnable to do DNS lookup, because we're are using a proxy without UDP!" );
        return;
    }

    // alocate memory to put length and the string + 1 NULL byte into it
    uint8_t *data = calloc(1, sizeof(length) + length + 1);

    memcpy(data, &length, sizeof(length));
    memcpy(data + sizeof(length), name, length);

    thread(dns_thread, (void *)data);
}
