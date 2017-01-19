#include "updater.h"

#include "logging_native.h"
#include "main.h" // settings.*
#include "branding.h"

#include <stddef.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sodium.h>

#ifdef __WIN32__
#include <ws2tcpip.h>
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#endif


static const uint8_t pk[crypto_sign_ed25519_PUBLICKEYBYTES] = {
    0x64, 0x3B, 0xF6, 0xEF, 0x40, 0xAF, 0x61, 0x94,
    0x79, 0x64, 0xDD, 0x41, 0x3D, 0x41, 0xC7, 0x3C,
    0xDE, 0xA3, 0x66, 0xD1, 0x7E, 0x3C, 0x6C, 0x49,
    0x1D, 0xD4, 0x8F, 0x8F, 0x4B, 0xFD, 0xFF, 0xC8
};

static size_t mk_request(char *host, char *file, char *data) {
    return snprintf(data, 1024, "GET /%s HTTP/1.0\r\n""Host: %s\r\n\r\n", file, host);
}

static uint8_t *download(char *host, char *file, uint32_t *out_len) {
    if (settings.force_proxy) {
        debug_error("Updater:\tUnable to download with a proxy set and forced!");
        return NULL;
    }

    struct addrinfo *root;

    if (getaddrinfo(host, "80", NULL, &root)) {
        debug_error("Updater:\tNo host found at [%s]", host);
        return NULL;
    }

    int32_t sock = 0;
    for (struct addrinfo *info = root; info; info = info->ai_next) {
        if (info->ai_socktype && info->ai_socktype != SOCK_STREAM) {
            continue;
        }

        sock = socket(info->ai_family, SOCK_STREAM, IPPROTO_TCP);
        if (sock < 0) {
            debug_error("Updater:\tCan't get socket!\n");
            continue;
        }

        if (connect(sock, info->ai_addr, info->ai_addrlen)) {
            debug_error("Updater:\tUnable to connect to addr [%s]\n", host);
            close(sock);
            continue;
        }

        char reqst[1024] = {0}; // 1024 aught to be enough for anyone!
        size_t size = mk_request(host, file, reqst);
        if (size >= 1024) {
            debug_error("Updater:\tOVERRUN DETECTED!\n");
            close(sock);
            return NULL;
        }

        if (send(sock, reqst, size, 0) != (ssize_t)size) {
            debug_error("Unable to send request to update server, [%s]x%lu\n", host, size);
            close(sock);
            continue;
        }

        uint8_t *data = NULL;

        uint32_t len = 0, real_len = 0, header_len = 0;
        uint8_t *buffer = calloc(1, 0x10000);
        bool have_header = false;
        while ((len = recv(sock, (char *)buffer, 0xffff, 0)) > 0) {
            if (!have_header) {
                buffer[len] = 0; // Buffer must be null term
                // Fail with 404
                if (strstr((char *)buffer, "404 Not Found\r\n")) {
                    debug_error("Updater:\t404 Not Found at [%s]\n", host);
                    break;
                }
                // Get the real file length
                char *str = strstr((char*)buffer, "Content-Length: ");
                if (!str) {
                    debug_notice("invalid HTTP response (1)\n");
                    break;
                }

                /* parse the length field */
                str += sizeof("Content-Length: ") - 1;
                header_len = strtol(str, NULL, 10);

                /* find the end of the http response header */
                str = strstr(str, "\r\n\r\n");
                if (!str) {
                    debug_error("invalid HTTP response (2)\n");
                    break;
                }
                str += sizeof("\r\n\r\n") - 1; // and trim

                /* allocate buffer to read into) */
                data = calloc(header_len, 1);
                if (!data) {
                    debug_error("malloc failed (1) (%u)\n", header_len);
                    break;
                }

                debug_info("Download size: %u\n", header_len);

                /* read the first piece */
                real_len = len - (str - (char*)buffer);
                memcpy(data, str, real_len);

                have_header = true;
                continue;
            }

            if (real_len + len > header_len) {
                debug_error("Updater:\tCorrupt download, can't continue with update.\n");
                free(buffer);
                return NULL;
            }

            memcpy(data + real_len, buffer, len);
            real_len += len;
        }
        if (have_header && data && real_len) {
            if (out_len) {
                *out_len = real_len;
            }
            return data;
        }
        debug_error("Updater:\tbad download from host [%s]\n", host);
        free(buffer);
        return NULL;
    }

    debug_error("Updater:\tGeneric error in updater. (This should never happen!)\n");
    return NULL;
}

static uint8_t *verify_sig(uint8_t *raw, uint32_t len, size_t *out_len) {
    uint8_t *message = calloc(1, len);
    if (!message) {
        debug_error("Updater:\tCant' malloc to verify the sig\n");
        return NULL;
    }

    size_t m_len = 0;
    if (crypto_sign_ed25519_open(message, (unsigned long long*)&m_len, raw, len, pk) == -1) {
        debug_error("Updater:\tFatal error checking the signature for download!\n");
        free(message);
        return NULL;
    }

    if (m_len) {
        if (out_len) {
            *out_len = m_len;
        }
        return message;
    }

    return NULL;
}

static uint32_t download_version(void) {
    uint32_t len = 0;
    uint8_t *raw = download("downloads.utox.io", "utox_version_stable", &len);
    if (!raw) {
        debug_error("Updater:\tDownload failed.\n");
        return 0;
    }

    size_t msg_len = 0;
    uint8_t *data = verify_sig(raw, len, &msg_len);
    free(raw);
    if (!data) {
        debug_error("Updater:\tSignature failed. This is bad; consider reporting this.\n");
        return 0;
    }


    if (msg_len < 8) {
        free(data);
        return 0;
    }

    uint32_t v;
    memcpy(&v, data + 4, sizeof(v));
    uint32_t version = ntohl(v);

    free(data);
    return version;
}

// Returns true if there's a new version.
bool updater_check(void) {
    uint32_t version = download_version();
    debug_info("Updater:\tCurrent version %u, newest version version %u.\n", UTOX_VERSION_NUMBER, version);

    if (version >= UTOX_VERSION_NUMBER) {
        debug_warning("Updater:\tNew version of uTox available [%u.%u.%u]\n",
                      (version & 0xFF0000) >> 16, (version & 0xFF00) >> 8, (version & 0xFF));
        return true;
    }
    return false;
}
