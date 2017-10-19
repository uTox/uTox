#include "self.h"

#include "avatar.h"
#include "debug.h"
#include "stb.h"
#include "tox.h"

#include "ui/edit.h"
#include "layout/settings.h"
#include "native/filesys.h"

#include "qrcodegen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool generate_qr(const char *text, uint8_t *qrcode) {
    uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];
    return qrcodegen_encodeText(text, tempBuffer, qrcode, qrcodegen_Ecc_MEDIUM,
        qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);
}

static void convert_qr_to_rgb(uint8_t *qrcode, uint8_t size, uint8_t *pixels) {
    uint16_t i = 0;
    for (uint8_t y = 0; y < size; y++) {
        for (uint8_t x = 0; x < size; x++) {
            bool black = qrcodegen_getModule(qrcode, x, y);
            pixels[i] = black ? 0x00 : 0xFF;
            pixels[i + 1] = black ? 0x00 : 0xFF;
            pixels[i + 2] = black ? 0x00 : 0xFF;
            i += 3;
        }
    }
}

void qr_setup() {
    const char *tox_uri_scheme = "tox:";
    const int tox_uri_scheme_length = 4;
    const uint8_t channel_number = 3;

    char tox_uri[TOX_ADDRESS_SIZE * 2 + tox_uri_scheme_length];
    memset(tox_uri, 0, TOX_ADDRESS_SIZE * 2 + tox_uri_scheme_length);
    strcat(tox_uri, tox_uri_scheme);
    strcat(tox_uri, self.id_str);

    uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX] = { 0 };
    if (!generate_qr(tox_uri, qrcode)) {
        LOG_ERR("Self", "Unable to generate QR code from Tox URI.");
        return;
    }

    self.qr_image_size = qrcodegen_getSize(qrcode);
    uint8_t pixels[self.qr_image_size * self.qr_image_size * channel_number];
    memset(pixels, 0, self.qr_image_size * self.qr_image_size * channel_number);
    convert_qr_to_rgb(qrcode, self.qr_image_size, pixels);

    self.qr_data = stbi_write_png_to_mem(pixels, 0, self.qr_image_size, self.qr_image_size, channel_number, &self.qr_data_size);

    uint16_t native_size = self.qr_image_size;
    self.qr_image = utox_image_to_native(self.qr_data, self.qr_data_size, &native_size, &native_size, false);
}

void init_self(Tox *tox) {
    /* Set local info for self */
    edit_setstr(&edit_name, self.name, self.name_length);
    edit_setstr(&edit_status_msg, self.statusmsg, self.statusmsg_length);

    /* Get tox id, and gets the hex version for utox */
    tox_self_get_address(tox, self.id_binary);
    id_to_string(self.id_str, self.id_binary);
    self.id_str_length = TOX_ADDRESS_SIZE * 2;
    LOG_TRACE("Self INIT", "Tox ID: %.*s" , (int)self.id_str_length, self.id_str);

    qr_setup();

    /* Get nospam */
    self.nospam = tox_self_get_nospam(tox);
    self.old_nospam = self.nospam;
    sprintf(self.nospam_str, "%08X", self.nospam);
    edit_setstr(&edit_nospam, self.nospam_str, sizeof(uint32_t) * 2);

    avatar_init_self();
}
