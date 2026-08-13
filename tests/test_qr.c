#include "test.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/qr.c"

#include "mock/mock_domain.h"

bool test_qr_setup_png(void) {
    mock_domain_reset();

    char id[TOX_ADDRESS_STR_SIZE + 1];
    memset(id, 'A', TOX_ADDRESS_STR_SIZE);
    id[TOX_ADDRESS_STR_SIZE] = 0;

    uint8_t *qr_data = NULL;
    int qr_data_size = 0;
    NATIVE_IMAGE *qr_image = NULL;
    int qr_image_size = 0;

    qr_setup(id, &qr_data, &qr_data_size, &qr_image, &qr_image_size);

    if (!qr_data || qr_data_size <= 0) {
        free(qr_image);
        FAIL("expected PNG bytes from qr_setup");
    }
    if (qr_data_size < 8 || qr_data[0] != 0x89 || qr_data[1] != 'P' || qr_data[2] != 'N' || qr_data[3] != 'G') {
        free(qr_data);
        free(qr_image);
        FAIL("qr_data is not a PNG");
    }
    if (qr_image_size < 21 + 8) { /* QR version 1 is 21 modules + 4-module border each side */
        free(qr_data);
        free(qr_image);
        FAIL("qr image size too small: %d", qr_image_size);
    }
    if (!qr_image) {
        free(qr_data);
        FAIL("native image stub should be non-NULL");
    }

    free(qr_data);
    free(qr_image);
    return true;
}

bool test_qr_empty_and_short_id(void) {
    mock_domain_reset();

    uint8_t *qr_data = NULL;
    int qr_data_size = 0;
    NATIVE_IMAGE *qr_image = NULL;
    int qr_image_size = 0;

    qr_setup("", &qr_data, &qr_data_size, &qr_image, &qr_image_size);
    if (!qr_data || qr_data_size <= 0 || !qr_image) {
        free(qr_data);
        free(qr_image);
        FAIL("empty id should still encode tox:");
    }
    free(qr_data);
    free(qr_image);

    qr_data = NULL;
    qr_image = NULL;
    qr_setup("abc", &qr_data, &qr_data_size, &qr_image, &qr_image_size);
    if (!qr_data || !qr_image) {
        free(qr_data);
        free(qr_image);
        FAIL("short id should encode");
    }
    free(qr_data);
    free(qr_image);
    return true;
}

int main(void) {
    int result = 0;
    RUN_TEST(test_qr_setup_png);
    RUN_TEST(test_qr_empty_and_short_id);
    return result;
}
