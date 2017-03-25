#include "updater.h"

#include "debug.h"
#include "settings.h"
#include "branding.h"

#include <stddef.h>
#include <stdint.h>
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
        LOG_ERR("Updater", "Updater:\tUnable to download with a proxy set and forced!");
        return NULL;
    }

    struct addrinfo *root;

    if (getaddrinfo(host, "80", NULL, &root)) {
        LOG_ERR("Updater", "Updater:\tNo host found at [%s]", host);
        return NULL;
    }

    for (struct addrinfo *info = root; info; info = info->ai_next) {
        if (info->ai_socktype && info->ai_socktype != SOCK_STREAM) {
            continue;
        }

        int32_t sock = socket(info->ai_family, SOCK_STREAM, IPPROTO_TCP);
        if (sock < 0) {
            LOG_ERR("Updater", "Can't get socket!");
            continue;
        }

        if (connect(sock, info->ai_addr, info->ai_addrlen)) {
            LOG_ERR("Updater", "Unable to connect to addr [%s]" , host);
            close(sock);
            continue;
        }

        char reqst[1024] = {0}; // 1024 aught to be enough for anyone!
        size_t size = mk_request(host, file, reqst);
        if (size >= 1024) {
            LOG_ERR("Updater", "OVERRUN DETECTED!");
            close(sock);
            freeaddrinfo(root);
            return NULL;
        }

        if (send(sock, reqst, size, 0) != (ssize_t)size) {
            LOG_ERR("Updater", "Unable to send request to update server, [%s]x%lu", host, size);
            close(sock);
            continue;
        }
        freeaddrinfo(root);

        uint8_t *data = NULL;

        ssize_t len = 0;
        uint32_t real_len = 0, header_len = 0;
        uint8_t *buffer = calloc(1, 0x10000);
        if (!buffer) {
            LOG_ERR("Updater", "Unable to malloc for the updater");
            close(sock);
            return NULL;
        }

        bool have_header = false;
        while ((len = recv(sock, (char *)buffer, 0xffff, 0)) > 0) {
            if (!have_header) {
                buffer[len] = 0; // Buffer must be null term
                // Fail with 404
                if (strstr((char *)buffer, "404 Not Found\r\n")) {
                    LOG_ERR("Updater", "404 Not Found at [%s]" , host);
                    break;
                }
                // Get the real file length
                char *str = strstr((char*)buffer, "Content-Length: ");
                if (!str) {
                    LOG_NOTE("Updater", "invalid HTTP response (1)\n");
                    break;
                }

                /* parse the length field */
                str += sizeof("Content-Length: ") - 1;
                header_len = strtoul(str, NULL, 10);
                if (header_len > 100 * 1024 * 1024) {
                    LOG_ERR("Updater", "Can't download a file larger than 100MiB");
                    close(sock);
                    free(buffer);
                    return NULL;
                }

                /* find the end of the http response header */
                str = strstr(str, "\r\n\r\n");
                if (!str) {
                    LOG_ERR("Updater", "invalid HTTP response (2)");
                    break;
                }
                str += sizeof("\r\n\r\n") - 1; // and trim

                /* allocate buffer to read into) */
                data = calloc(header_len, 1);
                if (!data) {
                    LOG_ERR("Updater", "malloc failed (1) (%u)", header_len);
                    break;
                }

                LOG_INFO("Updater", "Download size: %u\n", header_len);

                /* read the first piece */
                real_len = len - (str - (char*)buffer);
                memcpy(data, str, real_len);

                have_header = true;
                continue;
            }

            if (real_len + len > header_len) {
                LOG_ERR("Updater", "Corrupt download, can't continue with update.");
                close(sock);
                free(buffer);
                free(data);
                return NULL;
            }

            memcpy(data + real_len, buffer, len);
            real_len += len;
        }
        close(sock);
        free(buffer);

        if (have_header && data && real_len) {
            if (out_len) {
                *out_len = real_len;
            }

            return data;
        }

        LOG_ERR("Updater", "bad download from host [%s]" , host);
        return NULL;
    }

    LOG_ERR("Updater", "Generic error in updater. (This should never happen!)");
    freeaddrinfo(root);
    return NULL;
}

static uint8_t *verify_sig(uint8_t *raw, uint32_t len, size_t *out_len) {
    uint8_t *message = calloc(1, len);
    if (!message) {
        LOG_ERR("Updater", "Cant' malloc to verify the sig");
        return NULL;
    }

    size_t m_len = 0;
    if (crypto_sign_ed25519_open(message, (unsigned long long*)&m_len, raw, len, pk) == -1) {
        LOG_ERR("Updater", "Fatal error checking the signature for download!");
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
        LOG_ERR("Updater", "Download failed.");
        return 0;
    }

    size_t msg_len = 0;
    uint8_t *data = verify_sig(raw, len, &msg_len);
    free(raw);
    if (!data) {
        LOG_ERR("Updater", "Signature failed. This is bad; consider reporting this.");
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
    LOG_INFO("Updater", "Current version %u, newest version version %u." , UTOX_VERSION_NUMBER, version);

    if (version > UTOX_VERSION_NUMBER) {
        LOG_WARN("Updater", "New version of uTox available [%u.%u.%u]",
                      (version & 0xFF0000) >> 16, (version & 0xFF00) >> 8, (version & 0xFF));
        return true;
    } else if (version == UTOX_VERSION_NUMBER) {
        LOG_WARN("Updater", "Running the latest version of uTox [%u.%u.%u]",
                      (version & 0xFF0000) >> 16, (version & 0xFF00) >> 8, (version & 0xFF));

    }

    return false;
}
