#include "main.h"

void* file_raw(char *path, uint32_t *size)
{
    FILE *file;
    char *data;
    int len;

    file = fopen(path, "rb");
    if(!file) {
        // debug("File not found (%s)\n", path);
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

void file_write_raw(uint8_t *path, uint8_t *data, size_t size)
{
    FILE *file;

    file = fopen((const char*)path, "wb");
    if(!file) {
        debug("Cannot open file to write (%s)\n", path);
        return;
    }

    fwrite(data, size, 1, file);
    fflush(file);
    fclose(file);
    return;
}

void* file_text(char *path)
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
    data = malloc(len + 1);
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

    data[len] = 0;
    return data;
}

_Bool strstr_case(const char *a, const char *b)
{
    const char *c = b;
    while(*a) {
        if(tolower(*a) != tolower(*c)) {
            c = b;
        }

        if(tolower(*a) == tolower(*c)) {
            c++;
            if(!*c) {
                return 1;
            }
        }
        a++;
    }

    return 0;
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

void id_to_string(char_t *dest, char_t *src){
    to_hex(dest, src, TOX_FRIEND_ADDRESS_SIZE);
}

void cid_to_string(char_t *dest, char_t *src){
    to_hex(dest, src, TOX_PUBLIC_KEY_SIZE);
}

void fid_to_string(char_t *dest, char_t *src){
    to_hex(dest, src, TOX_FILE_ID_LENGTH);
}

void hash_to_string(char_t *dest, char_t *src){
    to_hex(dest, src, TOX_HASH_LENGTH);
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
        } else if(c >= 'a' && c <= 'f') {
            v = (c - 'a' + 10) << 4;
        } else {
            return 0;
        }

        c = *a++;
        if(c >= '0' && c <= '9') {
            v |= (c - '0');
        } else if(c >= 'A' && c <= 'F') {
            v |= (c - 'A' + 10);
        } else if(c >= 'a' && c <= 'f') {
            v |= (c - 'a' + 10);
        } else {
            return 0;
        }

        *w++ = v;
    }

    return 1;
}

int sprint_bytes(uint8_t *dest, unsigned int size, uint64_t bytes)
{
    char *str[] = {"B", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB", "ZiB", "YiB"};
    int max_id = countof(str) - 1;
    int i = 0;
    double f = bytes;
    while((bytes >= 1024) && (i < max_id)) {
        bytes /= 1024;
        f /= 1024.0;
        i++;
    }

    int r;

    r = snprintf((char*)dest, size, "%u", (uint32_t)bytes);

    if (r >= size) { // truncated
        r = size - 1;
    } else {
        //missing decimals
        r += snprintf((char*)dest + r, size - r, "%s", str[i]);
        if (r >= size) { // truncated
            r = size - 1;
        }
    }

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

uint8_t unicode_to_utf8_len(uint32_t ch)
{
    if (ch > 0x1FFFFF) {
        return 0;
    }
    return 4 - (ch <= 0xFFFF) - (ch <= 0x7FF) - (ch <= 0x7F);
}

void unicode_to_utf8(uint32_t ch, char_t *dst)
{
    uint32_t HB = (uint32_t)0x80;
    uint32_t SB = (uint32_t)0x3F;
    if (ch <= 0x7F) {
        dst[0] = (uint8_t)ch;
        return;//1;
    }
    if (ch <= 0x7FF) {
        dst[0] = (uint8_t)((ch >> 6) | (uint32_t)0xC0);
        dst[1] = (uint8_t)((ch & SB) | HB);
        return;//2;
    }
    if (ch <= 0xFFFF) {
        dst[0] = (uint8_t)((ch >> 12) | (uint32_t)0xE0);
        dst[1] = (uint8_t)(((ch >> 6) & SB) | HB);
        dst[2] = (uint8_t)((ch & SB) | HB);
        return;//3;
    }
    if (ch <= 0x1FFFFF) {
        dst[0] = (uint8_t)((ch >> 18) | (uint32_t)0xF0);
        dst[1] = (uint8_t)(((ch >> 12) & SB) | HB);
        dst[2] = (uint8_t)(((ch >> 6) & SB) | HB);
        dst[3] = (uint8_t)((ch & SB) | HB);
        return;//4;
    }
    return;// 0;
}

_Bool memcmp_case(const char_t *s1, const char_t *s2, uint32_t n)
{
    uint32_t i;

    for (i = 0; i < n; i++) {
        char_t c1, c2;

        c1 = s1[i];
        c2 = s2[i];

        if (c1 >= (char_t) 'a' && c1 <= (char_t) 'z') {
            c1 += ('A' - 'a');
        }

        if (c2 >= (char_t) 'a' && c2 <= (char_t) 'z') {
            c2 += ('A' - 'a');
        }

        if (c1 != c2) {
            return 1;
        }
    }

    return 0;
}

char_t* tohtml(char_t *str, STRING_IDX length)
{
    STRING_IDX i = 0;
    int len = 0;
    while(i != length) {
        switch(str[i]) {
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

    char_t *out = malloc(length + len + 1);
    i = 0; len = 0;
    while(i != length) {
        switch(str[i]) {
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
            STRING_IDX r = utf8_len(str + i);
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

void yuv420tobgr(uint16_t width, uint16_t height, const uint8_t *y, const uint8_t *u, const uint8_t *v, unsigned int ystride, unsigned int ustride, unsigned int vstride, uint8_t *out)
{
    unsigned long int i, j;
    for (i = 0; i < height; ++i) {
        for (j = 0; j < width; ++j) {
            uint8_t *point = out + 4 * ((i * width) + j);
            int t_y = y[((i * ystride) + j)];
            int t_u = u[(((i / 2) * ustride) + (j / 2))];
            int t_v = v[(((i / 2) * vstride) + (j / 2))];
            t_y = t_y < 16 ? 16 : t_y;

            int r = (298 * (t_y - 16) + 409 * (t_v - 128) + 128) >> 8;
            int g = (298 * (t_y - 16) - 100 * (t_u - 128) - 208 * (t_v - 128) + 128) >> 8;
            int b = (298 * (t_y - 16) + 516 * (t_u - 128) + 128) >> 8;

            point[2] = r>255? 255 : r<0 ? 0 : r;
            point[1] = g>255? 255 : g<0 ? 0 : g;
            point[0] = b>255? 255 : b<0 ? 0 : b;
            point[3] = ~0;
        }
    }
}

void yuv422to420(uint8_t *plane_y, uint8_t *plane_u, uint8_t *plane_v, uint8_t *input, uint16_t width, uint16_t height)
{
    uint8_t *end = input + width * height * 2;
    while(input != end) {
        uint8_t *line_end = input + width * 2;
        while(input != line_end) {
            *plane_y++ = *input++;
            *plane_v++ = *input++;
            *plane_y++ = *input++;
            *plane_u++ = *input++;
        }

        line_end = input + width * 2;
        while(input != line_end) {
            *plane_y++ = *input++;
            input++;//u
            *plane_y++ = *input++;
            input++;//v
        }

    }
}

static uint8_t rgb_to_y(int r, int g, int b)
{
    int y = ((9798 * r + 19235 * g + 3736 * b) >> 15);
    return y>255? 255 : y<0 ? 0 : y;
}

static uint8_t rgb_to_u(int r, int g, int b)
{
    int u = ((-5538 * r + -10846 * g + 16351 * b) >> 15) + 128;
    return u>255? 255 : u<0 ? 0 : u;
}

static uint8_t rgb_to_v(int r, int g, int b)
{
    int v = ((16351 * r + -13697 * g + -2664 * b) >> 15) + 128;
    return v>255? 255 : v<0 ? 0 : v;
}

void bgrtoyuv420(uint8_t *plane_y, uint8_t *plane_u, uint8_t *plane_v, uint8_t *rgb, uint16_t width, uint16_t height)
{
    uint16_t x, y;
    uint8_t *p;
    uint8_t r, g, b;

    for(y = 0; y != height; y += 2) {
        p = rgb;
        for(x = 0; x != width; x++) {
            b = *rgb++;
            g = *rgb++;
            r = *rgb++;
            *plane_y++ = rgb_to_y(r, g, b);
        }

        for(x = 0; x != width / 2; x++) {
            b = *rgb++;
            g = *rgb++;
            r = *rgb++;
            *plane_y++ = rgb_to_y(r, g, b);

            b = *rgb++;
            g = *rgb++;
            r = *rgb++;
            *plane_y++ = rgb_to_y(r, g, b);

            b = ((int)b + (int)*(rgb - 6) + (int)*p + (int)*(p + 3) + 2) / 4; p++;
            g = ((int)g + (int)*(rgb - 5) + (int)*p + (int)*(p + 3) + 2) / 4; p++;
            r = ((int)r + (int)*(rgb - 4) + (int)*p + (int)*(p + 3) + 2) / 4; p++;

            *plane_u++ = rgb_to_u(r, g, b);
            *plane_v++ = rgb_to_v(r, g, b);

            p += 3;
        }
    }
}

void bgrxtoyuv420(uint8_t *plane_y, uint8_t *plane_u, uint8_t *plane_v, uint8_t *rgb, uint16_t width, uint16_t height)
{
    uint16_t x, y;
    uint8_t *p;
    uint8_t r, g, b;

    for(y = 0; y != height; y += 2) {
        p = rgb;
        for(x = 0; x != width; x++) {
            b = *rgb++;
            g = *rgb++;
            r = *rgb++;
            rgb++;

            *plane_y++ = rgb_to_y(r, g, b);
        }

        for(x = 0; x != width / 2; x++) {
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

            b = ((int)b + (int)*(rgb - 8) + (int)*p + (int)*(p + 4) + 2) / 4; p++;
            g = ((int)g + (int)*(rgb - 7) + (int)*p + (int)*(p + 4) + 2) / 4; p++;
            r = ((int)r + (int)*(rgb - 6) + (int)*p + (int)*(p + 4) + 2) / 4; p++;
            p++;

            *plane_u++ = rgb_to_u(r, g, b);
            *plane_v++ = rgb_to_v(r, g, b);

            p += 4;
        }
    }
}

void scale_rgbx_image(uint8_t *old_rgbx, uint16_t old_width, uint16_t old_height, uint8_t *new_rgbx, uint16_t new_width, uint16_t new_height)
{
    int x, y, x0, y0, a, b;
    for(y = 0; y != new_height; y++) {
        y0 = y * old_height / new_height;
        for(x = 0; x != new_width; x++) {
            x0 = x * old_width / new_width;

            a = x + y*new_width;
            b = x0 + y0*old_width;
            new_rgbx[a*4  ] = old_rgbx[b*4  ];
            new_rgbx[a*4+1] = old_rgbx[b*4+1];
            new_rgbx[a*4+2] = old_rgbx[b*4+2];
        }
    }
}

typedef struct
{
    uint8_t version, scale, enableipv6, disableudp;
    uint16_t window_x, window_y, window_width, window_height;
    uint16_t proxy_port;
    uint8_t proxyenable;
    uint8_t logging_enabled : 1;
    uint8_t audible_notifications_enabled : 1;
    uint8_t audible_notifications_enabled_messages: 1;
    uint8_t filter : 1;
    uint8_t audio_filtering_enabled : 1;
    uint8_t zero : 4;
    uint8_t proxy_ip[0];
}UTOX_SAVE_V2;

UTOX_SAVE* config_load(void)
{
    uint8_t path[UTOX_FILE_NAME_LENGTH], *p;
    UTOX_SAVE *save;

    p = path + datapath(path);
    strcpy((char*)p, "utox_save");

    save = file_text((char*)path);

    if (!save) {
        p = path + datapath_old(path);
        strcpy((char*)p, "utox_save");
        save = file_text((char*)path);
    }

    if(save || (save = file_text("utox_save"))) {
        if(save->version == SAVE_VERSION) {
            /* validate values */
            if(save->scale > 4) {
                save->scale = 4;
            }
            goto NEXT;
        } else if (save->version == 2) {
            UTOX_SAVE_V2 *save_v2 = (UTOX_SAVE_V2*)save;
            save = calloc(sizeof(UTOX_SAVE) + 1 + strlen((char *)save_v2->proxy_ip), 1);

            memcpy(save, save_v2, sizeof(UTOX_SAVE_V2));
            save->version = SAVE_VERSION;

            if(save->scale > 4) {
                save->scale = 4;
            }

            strcpy((char*)save->proxy_ip, (char*)save_v2->proxy_ip);
            free(save_v2);
            goto NEXT;
        } else {
            free(save);
        }
    }

    save = malloc(sizeof(UTOX_SAVE) + 1);
    save->version = 1;
    save->scale = DEFAULT_SCALE - 1;
    save->enableipv6 = 1;
    save->disableudp = 0;
    save->proxy_port = 0;
    save->proxyenable = 0;
    save->logging_enabled = 1;
    save->close_to_tray = 0;
    save->start_in_tray = 0;
    save->auto_startup = 0;
    save->audible_notifications_enabled = 1;
    save->audible_notifications_enabled_messages = 0;
    save->audio_filtering_enabled = 1;
    save->proxy_ip[0] = 0;
    save->filter = 0;
    save->audio_device_in = ~0;
    save->theme = 0;
    save->no_typing_notifications = 0;

    config_osdefaults(save);
NEXT:
    dropdown_dpi.selected = dropdown_dpi.over = save->scale;
    dropdown_ipv6.selected = dropdown_ipv6.over = !save->enableipv6;
    dropdown_udp.selected = dropdown_udp.over = (save->disableudp != 0);
    dropdown_proxy.selected = dropdown_proxy.over = save->proxyenable <= 2 ? save->proxyenable : 2;
    dropdown_logging.selected = dropdown_logging.over = save->logging_enabled;
    dropdown_close_to_tray.selected = dropdown_close_to_tray.over = save->close_to_tray;
    dropdown_start_in_tray.selected = dropdown_start_in_tray.over = save->start_in_tray;
    dropdown_auto_startup.selected = dropdown_auto_startup.over = save->auto_startup;
    dropdown_audible_notification.selected = dropdown_audible_notification.over = save->audible_notifications_enabled;
    dropdown_audible_notification_messages.selected = dropdown_audible_notification_messages.over = save->audible_notifications_enabled_messages;
    dropdown_audio_filtering.selected = dropdown_audio_filtering.over = save->audio_filtering_enabled;
    dropdown_filter.selected = FILTER = save->filter;
    //dropdown_theme_onselect.selected = dropdown_theme_onselect.over = save->theme;
    dropdown_typing_notes.selected = save->no_typing_notifications;

    dont_send_typing_notes = save->no_typing_notifications;
    options.ipv6_enabled = save->enableipv6;
    options.udp_enabled = !save->disableudp;
    options.proxy_type = save->proxyenable ? TOX_PROXY_TYPE_SOCKS5 : TOX_PROXY_TYPE_NONE;
    options.proxy_port = save->proxy_port;
    strcpy((char*)proxy_address, (char*)save->proxy_ip);
    edit_proxy_ip.length = strlen((char*)save->proxy_ip);
    strcpy((char*)edit_proxy_ip.data, (char*)save->proxy_ip);
    if(save->proxy_port) {
        edit_proxy_port.length = snprintf((char*)edit_proxy_port.data, edit_proxy_port.maxlength + 1, "%u", save->proxy_port);
        if (edit_proxy_port.length >= edit_proxy_port.maxlength + 1) {
            edit_proxy_port.length = edit_proxy_port.maxlength;
        }
    }

    logging_enabled                         = save->logging_enabled;
    close_to_tray                           = save->close_to_tray;
    start_in_tray                           = save->start_in_tray;
    auto_startup                            = save->auto_startup;
    audible_notifications_enabled           = save->audible_notifications_enabled;
    audible_notifications_enabled_messages  = save->audible_notifications_enabled_messages;
    audio_filtering_enabled                 = save->audio_filtering_enabled;
    loaded_audio_out_device                 = save->audio_device_out;
    loaded_audio_in_device                  = save->audio_device_in;

    return save;
}

void config_save(UTOX_SAVE *save)
{
    uint8_t path[UTOX_FILE_NAME_LENGTH], *p;
    FILE *file;

    p = path + datapath(path);
    strcpy((char*)p, "utox_save");

    file = fopen((char*)path, "wb");
    if(!file) {
        debug("Unable to open uTox Save	::\n");
        return;
    }

    save->version = SAVE_VERSION;
    save->scale = SCALE - 1;
    save->enableipv6 = !dropdown_ipv6.selected;
    save->disableudp = dropdown_udp.selected;
    save->proxyenable = dropdown_proxy.selected;
    save->logging_enabled = logging_enabled;
    save->close_to_tray = close_to_tray;
    save->start_in_tray = start_in_tray;
    save->auto_startup = auto_startup;
    save->audible_notifications_enabled = audible_notifications_enabled;
    save->audible_notifications_enabled_messages = audible_notifications_enabled_messages;
    save->audio_filtering_enabled = audio_filtering_enabled;

    save->filter = FILTER;
    save->proxy_port = options.proxy_port;

    save->audio_device_in = dropdown_audio_in.selected;
    save->audio_device_out = dropdown_audio_out.selected;
    save->theme = theme;
    save->no_typing_notifications = dont_send_typing_notes;
    memset(save->unused, 0, sizeof(save->unused));

    debug("Writing uTox Save	::\n");
    fwrite(save, sizeof(*save), 1, file);
    fwrite(options.proxy_host, strlen(options.proxy_host), 1, file);
    fclose(file);
}

void utox_write_metadata(FRIEND *f){
    /* data sizes */
    size_t data_sz = 0, current_sz = 0;
    size_t alias_sz = strlen((const char*)f->alias);
    data_sz += alias_sz;

    /* Create path */
    uint8_t dest[UTOX_FILE_NAME_LENGTH], *dest_p;
    dest_p = dest + datapath(dest);
    cid_to_string(dest_p, f->cid);
    memcpy((char*)dest_p + (TOX_PUBLIC_KEY_SIZE * 2), ".fmetadata", sizeof(".fmetadata"));

    /* Create data */
    uint8_t *data = malloc(sizeof(FRIEND_META_DATA) + data_sz + 1);
    memcpy(data, &f->metadata, sizeof(FRIEND_META_DATA));
    current_sz = sizeof(FRIEND_META_DATA);
    /* copy the data */
        /* alias */
        memcpy(data + current_sz, f->alias, alias_sz);
        current_sz += alias_sz;

    /* Write */
    file_write_raw(dest, data, current_sz);
}
