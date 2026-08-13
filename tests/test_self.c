#include "test.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/self.c"
#include "../src/text.c"

#include "../src/ui/edit.h"

struct utox_self self;

static char edit_name_buf[128];
static char edit_status_buf[128];
static char edit_nospam_buf[16];

EDIT edit_name = {
    .data      = edit_name_buf,
    .data_size = sizeof edit_name_buf,
};
EDIT edit_status_msg = {
    .data      = edit_status_buf,
    .data_size = sizeof edit_status_buf,
};
EDIT edit_nospam = {
    .data      = edit_nospam_buf,
    .data_size = sizeof edit_nospam_buf,
};

static uint8_t mock_address[TOX_ADDRESS_SIZE];
static uint32_t mock_nospam;
static int avatar_init_self_calls;
static int qr_setup_calls;

void edit_setstr(EDIT *edit, char *str, uint16_t length) {
    if (!edit || !edit->data) {
        return;
    }
    if (length >= edit->data_size) {
        length = (uint16_t)(edit->data_size ? edit->data_size - 1 : 0);
    }
    if (str && length) {
        memcpy(edit->data, str, length);
    }
    if (edit->data_size) {
        edit->data[length] = 0;
    }
    edit->length = length;
}

void qr_setup(const char *id_str, uint8_t **qr_data, int *qr_data_size, NATIVE_IMAGE **qr_image, int *qr_image_size) {
    qr_setup_calls++;
    if (qr_data) {
        *qr_data = malloc(4);
        if (*qr_data) {
            memcpy(*qr_data, "QR", 2);
        }
    }
    if (qr_data_size) {
        *qr_data_size = 2;
    }
    if (qr_image) {
        *qr_image = (NATIVE_IMAGE *)(uintptr_t)1;
    }
    if (qr_image_size) {
        *qr_image_size = 21;
    }
    (void)id_str;
}

bool avatar_init_self(void) {
    avatar_init_self_calls++;
    return true;
}

void tox_self_get_address(const Tox *tox, uint8_t address[TOX_ADDRESS_SIZE]) {
    (void)tox;
    memcpy(address, mock_address, TOX_ADDRESS_SIZE);
}

uint32_t tox_self_get_nospam(const Tox *tox) {
    (void)tox;
    return mock_nospam;
}

void id_to_string(char *dest, uint8_t *src) {
    to_hex(dest, src, TOX_ADDRESS_SIZE);
}

static void reset_self(void) {
    free(self.qr_data);
    memset(&self, 0, sizeof self);
    memset(edit_name_buf, 0, sizeof edit_name_buf);
    memset(edit_status_buf, 0, sizeof edit_status_buf);
    memset(edit_nospam_buf, 0, sizeof edit_nospam_buf);
    edit_name.length = 0;
    edit_status_msg.length = 0;
    edit_nospam.length = 0;
    avatar_init_self_calls = 0;
    qr_setup_calls = 0;
    memset(mock_address, 0xAB, sizeof mock_address);
    mock_address[0] = 0x11;
    mock_nospam = 0x1234ABCD;
}

bool test_init_self_copies_id_and_nospam(void) {
    reset_self();
    memcpy(self.name, "Ada", 3);
    self.name_length = 3;
    memcpy(self.statusmsg, "hi", 2);
    self.statusmsg_length = 2;

    init_self((Tox *)(uintptr_t)1);

    if (edit_name.length != 3 || memcmp(edit_name.data, "Ada", 3) != 0) {
        FAIL("name edit");
    }
    if (edit_status_msg.length != 2 || memcmp(edit_status_msg.data, "hi", 2) != 0) {
        FAIL("status edit");
    }
    if (self.id_binary[0] != 0x11 || self.id_binary[1] != 0xAB) {
        FAIL("binary id");
    }
    if (self.id_str_length != TOX_ADDRESS_SIZE * 2) {
        FAIL("id hex length");
    }
    if (self.id_str[0] != '1' || self.id_str[1] != '1') {
        FAIL("id hex prefix, got %c%c", self.id_str[0], self.id_str[1]);
    }
    if (self.nospam != 0x1234ABCD || self.old_nospam != self.nospam) {
        FAIL("nospam fields");
    }
    if (strcmp(self.nospam_str, "1234ABCD") != 0) {
        FAIL("nospam string '%s'", self.nospam_str);
    }
    if (edit_nospam.length != 8 || memcmp(edit_nospam.data, "1234ABCD", 8) != 0) {
        FAIL("nospam edit");
    }
    if (qr_setup_calls != 1 || !self.qr_data) {
        FAIL("qr_setup");
    }
    if (avatar_init_self_calls != 1) {
        FAIL("avatar_init_self");
    }

    free(self.qr_data);
    self.qr_data = NULL;
    return true;
}

int main(void) {
    int result = 0;
    setvbuf(stdout, NULL, _IONBF, 0);
    settings.portable_mode = true;
    RUN_TEST(test_init_self_copies_id_and_nospam);
    free(self.qr_data);
    return result;
}
