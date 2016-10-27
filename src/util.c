// util.c

#include <inttypes.h>
#include <stdbool.h>

#include "util.h"

#include "flist.h"
#include "main_native.h"
#include "ui/dropdowns.h"
#include "ui/switches.h"

void *file_raw(char *path, uint32_t *size) {
    FILE *file;
    char *data;
    int   len;

    file = fopen(path, "rb");
    if (!file) {
        // debug("File not found (%s)\n", path);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    len  = ftell(file);
    data = malloc(len);
    if (!data) {
        fclose(file);
        return NULL;
    }

    fseek(file, 0, SEEK_SET);

    if (fread(data, len, 1, file) != 1) {
        debug("Read error (%s)\n", path);
        fclose(file);
        free(data);
        return NULL;
    }

    fclose(file);

    // debug("Read %u bytes (%s)\n", len, path);

    if (size) {
        *size = len;
    }
    return data;
}

void file_write_raw(uint8_t *path, uint8_t *data, size_t size) {
    FILE *file;

    file = fopen((const char *)path, "wb");
    if (!file) {
        debug("Cannot open file to write (%s)\n", path);
        return;
    }

    size_t outsize;
    outsize = fwrite(data, sizeof(uint8_t), size, file);
    if (outsize < size) {
        /* I HATE YOU WINDOWS YOU POS */
        debug("File write raw size in  %u\n", (unsigned int)size);
        debug("File write raw size out %u\n", (unsigned int)outsize);
    }
    fflush(file);
    fclose(file);
    return;
}

void *file_text(char *path) {
    FILE *file;
    char *data;
    int   len;

    file = fopen(path, "rb");
    if (!file) {
        debug("File not found (%s)\n", path);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    len  = ftell(file);
    data = malloc(len + 1);
    if (!data) {
        fclose(file);
        return NULL;
    }

    fseek(file, 0, SEEK_SET);

    if (fread(data, len, 1, file) != 1) {
        debug("Read error (%s)\n", path);
        fclose(file);
        free(data);
        return NULL;
    }

    fclose(file);

    debug("Read %u bytes (%s)\n", len, path);

    data[len] = 0;
    return data;
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

static void to_hex(char *out, uint8_t *in, int size) {
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

void id_to_string(char *dest, uint8_t *src) {
    to_hex(dest, src, TOX_FRIEND_ADDRESS_SIZE);
}

void cid_to_string(char *dest, uint8_t *src) {
    to_hex(dest, src, TOX_PUBLIC_KEY_SIZE);
}

void fid_to_string(char *dest, uint8_t *src) {
    to_hex(dest, src, TOX_FILE_ID_LENGTH);
}

void hash_to_string(char *dest, uint8_t *src) {
    to_hex(dest, src, TOX_HASH_LENGTH);
}

bool string_to_id(uint8_t *w, char *a) {
    uint8_t *end = w + TOX_FRIEND_ADDRESS_SIZE;
    while (w != end) {
        char c, v;

        c = *a++;
        if (c >= '0' && c <= '9') {
            v = (c - '0') << 4;
        } else if (c >= 'A' && c <= 'F') {
            v = (c - 'A' + 10) << 4;
        } else if (c >= 'a' && c <= 'f') {
            v = (c - 'a' + 10) << 4;
        } else {
            return 0;
        }

        c = *a++;
        if (c >= '0' && c <= '9') {
            v |= (c - '0');
        } else if (c >= 'A' && c <= 'F') {
            v |= (c - 'A' + 10);
        } else if (c >= 'a' && c <= 'f') {
            v |= (c - 'a' + 10);
        } else {
            return 0;
        }

        *w++ = v;
    }

    return 1;
}

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

    int r;

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

void yuv420tobgr(uint16_t width, uint16_t height, const uint8_t *y, const uint8_t *u, const uint8_t *v,
                 unsigned int ystride, unsigned int ustride, unsigned int vstride, uint8_t *out) {
    unsigned long int i, j;
    for (i = 0; i < height; ++i) {
        for (j = 0; j < width; ++j) {
            uint8_t *point = out + 4 * ((i * width) + j);
            int      t_y   = y[((i * ystride) + j)];
            int      t_u   = u[(((i / 2) * ustride) + (j / 2))];
            int      t_v   = v[(((i / 2) * vstride) + (j / 2))];
            t_y            = t_y < 16 ? 16 : t_y;

            int r = (298 * (t_y - 16) + 409 * (t_v - 128) + 128) >> 8;
            int g = (298 * (t_y - 16) - 100 * (t_u - 128) - 208 * (t_v - 128) + 128) >> 8;
            int b = (298 * (t_y - 16) + 516 * (t_u - 128) + 128) >> 8;

            point[2] = r > 255 ? 255 : r < 0 ? 0 : r;
            point[1] = g > 255 ? 255 : g < 0 ? 0 : g;
            point[0] = b > 255 ? 255 : b < 0 ? 0 : b;
            point[3] = ~0;
        }
    }
}

void yuv422to420(uint8_t *plane_y, uint8_t *plane_u, uint8_t *plane_v, uint8_t *input, uint16_t width, uint16_t height) {
    uint8_t *end = input + width * height * 2;
    while (input != end) {
        uint8_t *line_end = input + width * 2;
        while (input != line_end) {
            *plane_y++ = *input++;
            *plane_v++ = *input++;
            *plane_y++ = *input++;
            *plane_u++ = *input++;
        }

        line_end = input + width * 2;
        while (input != line_end) {
            *plane_y++ = *input++;
            input++; // u
            *plane_y++ = *input++;
            input++; // v
        }
    }
}

static uint8_t rgb_to_y(int r, int g, int b) {
    int y = ((9798 * r + 19235 * g + 3736 * b) >> 15);
    return y > 255 ? 255 : y < 0 ? 0 : y;
}

static uint8_t rgb_to_u(int r, int g, int b) {
    int u = ((-5538 * r + -10846 * g + 16351 * b) >> 15) + 128;
    return u > 255 ? 255 : u < 0 ? 0 : u;
}

static uint8_t rgb_to_v(int r, int g, int b) {
    int v = ((16351 * r + -13697 * g + -2664 * b) >> 15) + 128;
    return v > 255 ? 255 : v < 0 ? 0 : v;
}

void bgrtoyuv420(uint8_t *plane_y, uint8_t *plane_u, uint8_t *plane_v, uint8_t *rgb, uint16_t width, uint16_t height) {
    uint16_t x, y;
    uint8_t *p;
    uint8_t  r, g, b;

    for (y = 0; y != height; y += 2) {
        p = rgb;
        for (x = 0; x != width; x++) {
            b          = *rgb++;
            g          = *rgb++;
            r          = *rgb++;
            *plane_y++ = rgb_to_y(r, g, b);
        }

        for (x = 0; x != width / 2; x++) {
            b          = *rgb++;
            g          = *rgb++;
            r          = *rgb++;
            *plane_y++ = rgb_to_y(r, g, b);

            b          = *rgb++;
            g          = *rgb++;
            r          = *rgb++;
            *plane_y++ = rgb_to_y(r, g, b);

            b = ((int)b + (int)*(rgb - 6) + (int)*p + (int)*(p + 3) + 2) / 4;
            p++;
            g = ((int)g + (int)*(rgb - 5) + (int)*p + (int)*(p + 3) + 2) / 4;
            p++;
            r = ((int)r + (int)*(rgb - 4) + (int)*p + (int)*(p + 3) + 2) / 4;
            p++;

            *plane_u++ = rgb_to_u(r, g, b);
            *plane_v++ = rgb_to_v(r, g, b);

            p += 3;
        }
    }
}

void bgrxtoyuv420(uint8_t *plane_y, uint8_t *plane_u, uint8_t *plane_v, uint8_t *rgb, uint16_t width, uint16_t height) {
    uint16_t x, y;
    uint8_t *p;
    uint8_t  r, g, b;

    for (y = 0; y != height; y += 2) {
        p = rgb;
        for (x = 0; x != width; x++) {
            b = *rgb++;
            g = *rgb++;
            r = *rgb++;
            rgb++;

            *plane_y++ = rgb_to_y(r, g, b);
        }

        for (x = 0; x != width / 2; x++) {
            b = *rgb++;
            g = *rgb++;
            r = *rgb++;
            rgb++;

            *plane_y++ = rgb_to_y(r, g, b);

            b = *rgb++;
            g = *rgb++;
            r = *rgb++;
            rgb++;

            *plane_y++ = rgb_to_y(r, g, b);

            b = ((int)b + (int)*(rgb - 8) + (int)*p + (int)*(p + 4) + 2) / 4;
            p++;
            g = ((int)g + (int)*(rgb - 7) + (int)*p + (int)*(p + 4) + 2) / 4;
            p++;
            r = ((int)r + (int)*(rgb - 6) + (int)*p + (int)*(p + 4) + 2) / 4;
            p++;
            p++;

            *plane_u++ = rgb_to_u(r, g, b);
            *plane_v++ = rgb_to_v(r, g, b);

            p += 4;
        }
    }
}

void scale_rgbx_image(uint8_t *old_rgbx, uint16_t old_width, uint16_t old_height, uint8_t *new_rgbx, uint16_t new_width,
                      uint16_t new_height) {
    int x, y, x0, y0, a, b;
    for (y = 0; y != new_height; y++) {
        y0 = y * old_height / new_height;
        for (x = 0; x != new_width; x++) {
            x0 = x * old_width / new_width;

            a                   = x + y * new_width;
            b                   = x0 + y0 * old_width;
            new_rgbx[a * 4]     = old_rgbx[b * 4];
            new_rgbx[a * 4 + 1] = old_rgbx[b * 4 + 1];
            new_rgbx[a * 4 + 2] = old_rgbx[b * 4 + 2];
        }
    }
}

UTOX_SAVE *config_load(void) {
    UTOX_SAVE *save;
    save = utox_load_data_utox();

    if (!save) {
        debug_notice("unable to load utox_save data\n");
        /* Create and set defaults */
        save              = calloc(1, sizeof(UTOX_SAVE));
        save->enableipv6  = 1;
        save->disableudp  = 0;
        save->proxyenable = 0;

        save->audio_filtering_enabled       = 1;
        save->audible_notifications_enabled = 1;
    }

    if (save->scale > 30) {
        save->scale = 30;
    } else if (save->scale < 5) {
        save->scale = 10;
    }

    if (save->window_width < MAIN_WIDTH) {
        save->window_width = MAIN_WIDTH;
    }
    if (save->window_height < MAIN_HEIGHT) {
        save->window_height = MAIN_HEIGHT;
    }

    dropdown_dpi.selected = dropdown_dpi.over = save->scale - 5;
    dropdown_proxy.selected = dropdown_proxy.over = save->proxyenable <= 2 ? save->proxyenable : 2;

    switch_ipv6.switch_on          = save->enableipv6;
    switch_udp.switch_on           = !save->disableudp;
    switch_logging.switch_on       = save->logging_enabled;
    switch_mini_contacts.switch_on = save->use_mini_flist;
    switch_auto_startup.switch_on  = save->auto_startup;

    switch_close_to_tray.switch_on = save->close_to_tray;
    switch_start_in_tray.switch_on = save->start_in_tray;

    switch_audible_notifications.switch_on = save->audible_notifications_enabled;
    switch_audio_filtering.switch_on       = save->audio_filtering_enabled;
    switch_push_to_talk.switch_on          = save->push_to_talk;
    switch_status_notifications.switch_on  = save->status_notifications;

    dropdown_theme.selected = dropdown_theme.over = save->theme;

    switch_typing_notes.switch_on = !save->no_typing_notifications;

    flist_set_filter(save->filter); /* roster list filtering */

    /* Network settings */
    settings.enable_ipv6 = save->enableipv6;
    settings.enable_udp  = !save->disableudp;
    settings.use_proxy   = !!save->proxyenable;
    settings.proxy_port  = save->proxy_port;

    strcpy((char *)proxy_address, (char *)save->proxy_ip);

    edit_proxy_ip.length = strlen((char *)save->proxy_ip);

    strcpy((char *)edit_proxy_ip.data, (char *)save->proxy_ip);

    if (save->proxy_port) {
        edit_proxy_port.length =
            snprintf((char *)edit_proxy_port.data, edit_proxy_port.maxlength + 1, "%u", save->proxy_port);
        if (edit_proxy_port.length >= edit_proxy_port.maxlength + 1) {
            edit_proxy_port.length = edit_proxy_port.maxlength;
        }
    }

    settings.logging_enabled     = save->logging_enabled;
    settings.close_to_tray       = save->close_to_tray;
    settings.start_in_tray       = save->start_in_tray;
    settings.start_with_system   = save->auto_startup;
    settings.ringtone_enabled    = save->audible_notifications_enabled;
    settings.audiofilter_enabled = save->audio_filtering_enabled;
    settings.use_mini_flist      = save->use_mini_flist;

    settings.send_typing_status   = !save->no_typing_notifications;
    settings.group_notifications  = save->group_notifications;
    settings.status_notifications = save->status_notifications;

    settings.window_width  = save->window_width;
    settings.window_height = save->window_height;

    settings.last_version = save->utox_last_version;

    loaded_audio_out_device = save->audio_device_out;
    loaded_audio_in_device  = save->audio_device_in;


    if (save->push_to_talk) {
        init_ptt();
    }

    return save;
}

void config_save(UTOX_SAVE *save_in) {
    UTOX_SAVE *save = calloc(1, sizeof(UTOX_SAVE) + 256);

    /* Copy the data from the in data to protect the calloc */
    save->window_x      = save_in->window_x;
    save->window_y      = save_in->window_y;
    save->window_width  = save_in->window_width;
    save->window_height = save_in->window_height;

    save->save_version                  = UTOX_SAVE_VERSION;
    save->scale                         = ui_scale - 1;
    save->proxyenable                   = dropdown_proxy.selected;
    save->logging_enabled               = settings.logging_enabled;
    save->close_to_tray                 = settings.close_to_tray;
    save->start_in_tray                 = settings.start_in_tray;
    save->auto_startup                  = settings.start_with_system;
    save->audible_notifications_enabled = settings.ringtone_enabled;
    save->audio_filtering_enabled       = settings.audiofilter_enabled;
    save->push_to_talk                  = settings.push_to_talk;
    save->use_mini_flist                = settings.use_mini_flist;

    save->disableudp              = !settings.enable_udp;
    save->enableipv6              = settings.enable_ipv6;
    save->no_typing_notifications = !settings.send_typing_status;

    save->filter     = flist_get_filter();
    save->proxy_port = settings.proxy_port;

    save->audio_device_in  = dropdown_audio_in.selected;
    save->audio_device_out = dropdown_audio_out.selected;
    save->theme            = settings.theme;

    save->utox_last_version    = settings.curr_version;
    save->group_notifications  = settings.group_notifications;
    save->status_notifications = settings.status_notifications;

    memcpy(save->proxy_ip, proxy_address, 256); /* Magic number inside toxcore */

    debug_notice("uTox:\tWriting uTox Save\n");
    utox_save_data_utox(save, sizeof(*save) + 256); /* Magic number inside toxcore */
}

void utox_write_metadata(FRIEND *f) {
    /* Create path */
    char dest[UTOX_FILE_NAME_LENGTH], *dest_p;
    dest_p = dest + datapath((uint8_t *)dest);
    cid_to_string(dest_p, f->cid);
    memcpy((char *)dest_p + (TOX_PUBLIC_KEY_SIZE * 2), ".fmetadata", sizeof(".fmetadata"));

    size_t           total_size = 0;
    FRIEND_META_DATA metadata[1];
    memset(metadata, 0, sizeof(*metadata));
    total_size += sizeof(*metadata);

    metadata->version          = METADATA_VERSION;
    metadata->ft_autoaccept    = f->ft_autoaccept;
    metadata->skip_msg_logging = f->skip_msg_logging;

    if (f->alias && f->alias_length) {
        metadata->alias_length = f->alias_length;
        total_size += metadata->alias_length;
    }

    uint8_t *data = calloc(1, total_size);

    memcpy(data, metadata, sizeof(*metadata));
    memcpy(data + sizeof(*metadata), f->alias, metadata->alias_length);

    /* Write */
    file_write_raw((uint8_t *)dest, (uint8_t *)data, total_size);
    free(data);
}
