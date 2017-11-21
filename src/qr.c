#include "qr.h"

#include "debug.h"
#include "stb.h"
#include "tox.h"

#include "native/image.h"

#include <qrcodegen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool generate_qr(const char *text, uint8_t *qrcode) {
    uint8_t temp_buffer[qrcodegen_BUFFER_LEN_MAX];
    return qrcodegen_encodeText(text, temp_buffer, qrcode, qrcodegen_Ecc_MEDIUM,
        qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);
}

static void convert_qr_to_rgb(const uint8_t *qrcode, uint8_t size, uint8_t *pixels) {
    uint16_t i = 0;
    for (uint8_t y = 0; y < size; y++) {
        for (uint8_t x = 0; x < size; x++) {
            bool black = qrcodegen_getModule(qrcode, x, y);
            pixels[i] = pixels[i + 1] = pixels[i + 2] = black ? 0x00 : 0xFF;
            i += 3;
        }
    }
}

void qr_setup(const char *id_str,
              uint8_t **qr_data,
              int *qr_data_size,
              NATIVE_IMAGE **qr_image,
              int *qr_image_size) {
    const char tox_uri_scheme[] = "tox:";
    const uint8_t channel_number = 3;
    const uint8_t tox_uri_length = TOX_ADDRESS_SIZE * 2 + sizeof(tox_uri_scheme);

    char tox_uri[tox_uri_length];
    memset(tox_uri, 0, tox_uri_length);
    strcat(tox_uri, tox_uri_scheme);
    strcat(tox_uri, id_str);

    uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX] = { 0 };
    if (!generate_qr(tox_uri, qrcode)) {
        LOG_ERR("QR", "Unable to generate QR code from Tox URI.");
        return;
    }

    *qr_image_size = qrcodegen_getSize(qrcode);
    uint8_t pixels[*qr_image_size * *qr_image_size * channel_number];
    memset(pixels, 0, *qr_image_size * *qr_image_size * channel_number);
    convert_qr_to_rgb(qrcode, *qr_image_size, pixels);

    *qr_data = stbi_write_png_to_mem(pixels, 0, *qr_image_size, *qr_image_size, channel_number, qr_data_size);

    uint16_t native_size = *qr_image_size;
    *qr_image = utox_image_to_native(*qr_data, *qr_data_size, &native_size, &native_size, false);
}
