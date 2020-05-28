#include "qr.h"

#include "debug.h"
#include "stb.h"
#include "tox.h"
#include "self.h"

#include "native/image.h"

#include <qrcodegen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define QR_BORDER_SIZE 4 /* unit: QR modules */

static bool generate_qr(const char *text, uint8_t *qrcode)
{
    uint8_t temp_buffer[qrcodegen_BUFFER_LEN_MAX];
    return qrcodegen_encodeText(text, temp_buffer, qrcode, qrcodegen_Ecc_MEDIUM, qrcodegen_VERSION_MIN,
        qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);
}

static void convert_qr_to_rgb(const uint8_t *qrcode, uint8_t size, uint8_t *pixels)
{
    /* skip first border on y and x axis */
    uint16_t i = ((QR_BORDER_SIZE * size) + QR_BORDER_SIZE) * 3;
    for (uint8_t y = 0; y < qrcodegen_getSize(qrcode); y++) {
        for (uint8_t x = 0; x < qrcodegen_getSize(qrcode); x++) {
            bool black = qrcodegen_getModule(qrcode, x, y);
            pixels[i] = pixels[i + 1] = pixels[i + 2] = black ? 0x00 : 0xFF;
            i += 3;
        }
        /* skip border until end of line and border on start of next line */
        i += (QR_BORDER_SIZE * 2) * 3;
    }
}

void qr_setup(const char *id_str, uint8_t **qr_data, int *qr_data_size, NATIVE_IMAGE **qr_image, int *qr_image_size)
{
    const uint8_t channel_number = 3;
    uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX] = { 0 };
    char tox_uri[TOX_ADDRESS_STR_SIZE + sizeof("tox:")];

    snprintf(tox_uri, sizeof(tox_uri), "tox:%.*s", (unsigned int) TOX_ADDRESS_STR_SIZE, id_str);

    if (!generate_qr(tox_uri, qrcode)) {
        LOG_ERR("QR", "Unable to generate QR code from Tox URI.");
        return;
    }

    *qr_image_size = qrcodegen_getSize(qrcode);
    *qr_image_size += QR_BORDER_SIZE * 2; /* add border on both sides */
    uint8_t pixels[*qr_image_size * *qr_image_size * channel_number];
    memset(pixels, 0xFF, sizeof(pixels)); /* make it all white */
    convert_qr_to_rgb(qrcode, *qr_image_size, pixels);

    *qr_data = stbi_write_png_to_mem(pixels, 0, *qr_image_size, *qr_image_size, channel_number, qr_data_size);

    uint16_t native_size = *qr_image_size;
    *qr_image = utox_image_to_native(*qr_data, *qr_data_size, &native_size, &native_size, false);
}
