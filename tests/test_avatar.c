#include "test.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/avatar.c"

#include "../src/filesys.h"
#include "../src/self.h"
#include "../src/settings.h"
#include "../src/tox.h"

struct utox_self self;
UTOX_TOX_THREAD_INIT tox_thread_init = UTOX_TOX_THREAD_INIT_SUCCESS;

static uint8_t last_tox_msg;
static uint32_t last_ft_friend = UINT32_MAX;
static int fail_native_image;

NATIVE_IMAGE *utox_image_to_native(const UTOX_IMAGE data, size_t size, uint16_t *w, uint16_t *h, bool keep_alpha) {
    (void)data;
    (void)keep_alpha;
    if (fail_native_image || size == 0) {
        return NULL;
    }
    if (w) {
        *w = 8;
    }
    if (h) {
        *h = 8;
    }
    return (NATIVE_IMAGE *)calloc(1, 1);
}

void image_free(NATIVE_IMAGE *image) {
    free(image);
}

#ifndef NATIVE_IMAGE_IS_VALID
/* Cocoa exposes this as a function; Win/X11 use a macro. */
int NATIVE_IMAGE_IS_VALID(NATIVE_IMAGE *img) {
    return img != NULL;
}
#endif

bool tox_hash(Tox_Hash hash, const uint8_t data[], size_t length) {
    if (!hash) {
        return false;
    }
    memset(hash, 0, TOX_HASH_LENGTH);
    if (data && length) {
        hash[0] = data[0];
        hash[1] = (uint8_t)length;
    }
    return true;
}

void postmessage_toxcore(uint8_t msg, uint32_t param1, uint32_t param2, void *data) {
    last_tox_msg = msg;
    (void)param1;
    (void)param2;
    (void)data;
}

uint32_t ft_send_avatar(Tox *tox, uint32_t friend_number) {
    (void)tox;
    last_ft_friend = friend_number;
    return 1;
}

static void hex64(char *out, char fill) {
    memset(out, fill, TOX_PUBLIC_KEY_SIZE * 2);
    out[TOX_PUBLIC_KEY_SIZE * 2] = 0;
}

static void reset_avatar(void) {
    fail_native_image = 0;
    last_tox_msg = 0;
    last_ft_friend = UINT32_MAX;
    settings.portable_mode = true;
    free(self.png_data);
    free(self.avatar);
    memset(&self, 0, sizeof self);
    hex64(self.id_str, 'A');
}

static void cleanup_avatar_file(const char *hex) {
    char name[sizeof("avatars/") + TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".png")];
    snprintf(name, sizeof name, "avatars/%.*s.png", TOX_PUBLIC_KEY_SIZE * 2, hex);
    utox_remove_file((const uint8_t *)name, strlen(name));
}

bool test_avatar_save_load_set_unset(void) {
    reset_avatar();
    char id[TOX_PUBLIC_KEY_SIZE * 2 + 1];
    hex64(id, 'B');
    cleanup_avatar_file(id);

    uint8_t png[32];
    memset(png, 0x11, sizeof png);
    png[0] = 0x89;

    if (!avatar_save(id, png, sizeof png)) {
        FAIL("save");
    }

    AVATAR av = { 0 };
    if (!avatar_init(id, &av)) {
        FAIL("init from saved file");
    }
    if (av.format != UTOX_AVATAR_FORMAT_PNG || !av.img || av.size != sizeof png) {
        avatar_unset(&av);
        FAIL("loaded avatar fields");
    }
    if (av.hash[0] != 0x89) {
        avatar_unset(&av);
        FAIL("hash from file");
    }

    uint8_t png2[16];
    memset(png2, 0x22, sizeof png2);
    if (!avatar_set(&av, png2, sizeof png2) || av.size != sizeof png2 || av.hash[0] != 0x22) {
        avatar_unset(&av);
        FAIL("avatar_set");
    }

    avatar_unset(&av);
    if (av.format != UTOX_AVATAR_FORMAT_NONE || av.img) {
        FAIL("unset");
    }
    avatar_unset(NULL);

    if (avatar_set(NULL, png, sizeof png)) {
        FAIL("set NULL avatar");
    }
    if (avatar_set(&av, png, UTOX_AVATAR_MAX_DATA_LENGTH + 1)) {
        FAIL("set too large");
    }

    fail_native_image = 1;
    if (avatar_set(&av, png, sizeof png)) {
        FAIL("invalid native image");
    }
    fail_native_image = 0;

    if (!avatar_delete(id)) {
        FAIL("delete");
    }
    AVATAR missing = { 0 };
    if (avatar_init(id, &missing)) {
        avatar_unset(&missing);
        FAIL("deleted avatar should not load");
    }
    return true;
}

bool test_avatar_self_and_online(void) {
    reset_avatar();
    cleanup_avatar_file(self.id_str);

    if (avatar_init_self()) {
        FAIL("init_self with no file is false but still allocates");
    }
    if (!self.avatar) {
        FAIL("init_self should allocate avatar");
    }

    uint8_t png[8];
    memset(png, 0x33, sizeof png);
    if (!self_set_and_save_avatar(png, sizeof png)) {
        FAIL("self_set_and_save");
    }
    if (self.avatar->format != UTOX_AVATAR_FORMAT_PNG) {
        FAIL("self avatar format");
    }

    avatar_unset_self();
    if (self.avatar->format != UTOX_AVATAR_FORMAT_NONE) {
        FAIL("unset self");
    }

    if (!avatar_set_self(png, sizeof png)) {
        FAIL("set_self");
    }

    last_tox_msg = 0;
    avatar_delete_self();
    if (last_tox_msg != TOX_AVATAR_UNSET) {
        FAIL("delete_self should unset tox avatar");
    }

    if (avatar_on_friend_online((Tox *)(uintptr_t)1, 3)) {
        FAIL("online with no png data and no file");
    }

    if (!avatar_save(self.id_str, png, sizeof png)) {
        FAIL("save self png for online");
    }
    last_ft_friend = UINT32_MAX;
    if (!avatar_on_friend_online((Tox *)(uintptr_t)1, 3) || last_ft_friend != 3 || !self.png_data) {
        FAIL("online should load png and send");
    }
    /* second call reuses self.png_data */
    last_ft_friend = UINT32_MAX;
    if (!avatar_on_friend_online((Tox *)(uintptr_t)1, 9) || last_ft_friend != 9) {
        FAIL("online reuse png_data");
    }

    cleanup_avatar_file(self.id_str);
    free(self.png_data);
    self.png_data = NULL;
    free(self.avatar);
    self.avatar = NULL;
    return true;
}

/* tox.c TOX_SELF_CHANGE_NOSPAM: old_id aliases self.id_str, then id_to_string
 * overwrites the same buffer. Avatar files are named by the public key
 * (first TOX_PUBLIC_KEY_SIZE*2 hex chars); nospam only changes the suffix. */
bool test_avatar_move_on_nospam_change(void) {
    reset_avatar();
    memset(self.id_str, 'C', TOX_PUBLIC_KEY_SIZE * 2);
    memset(self.id_str + TOX_PUBLIC_KEY_SIZE * 2, '0',
           (TOX_ADDRESS_SIZE - TOX_PUBLIC_KEY_SIZE) * 2);
    self.id_str_length = TOX_ADDRESS_SIZE * 2;
    cleanup_avatar_file(self.id_str);

    uint8_t png[12];
    memset(png, 0x44, sizeof png);
    if (!avatar_save(self.id_str, png, sizeof png)) {
        FAIL("save before nospam change");
    }

    char *old_id = self.id_str;
    memset(self.id_str + TOX_PUBLIC_KEY_SIZE * 2, 'F',
           (TOX_ADDRESS_SIZE - TOX_PUBLIC_KEY_SIZE) * 2);
    avatar_move((uint8_t *)old_id, (uint8_t *)self.id_str);

    AVATAR av = { 0 };
    if (!avatar_init(self.id_str, &av)) {
        FAIL("avatar is keyed by public key; nospam change must not lose it");
    }
    if (av.size != sizeof png || av.hash[0] != 0x44) {
        avatar_unset(&av);
        FAIL("avatar payload after nospam change");
    }
    avatar_unset(&av);
    cleanup_avatar_file(self.id_str);
    return true;
}

bool test_avatar_oversized_file(void) {
    reset_avatar();
    char id[TOX_PUBLIC_KEY_SIZE * 2 + 1];
    hex64(id, 'E');
    cleanup_avatar_file(id);

    size_t huge = UTOX_AVATAR_MAX_DATA_LENGTH + 8;
    uint8_t *buf = calloc(1, huge);
    if (!buf) {
        FAIL("oom");
    }
    if (!avatar_save(id, buf, huge)) {
        free(buf);
        FAIL("save huge");
    }
    free(buf);

    AVATAR av = { 0 };
    if (avatar_init(id, &av)) {
        avatar_unset(&av);
        FAIL("oversized saved file should not load");
    }
    cleanup_avatar_file(id);
    return true;
}

int main(void) {
    int result = 0;
    settings.portable_mode = true;
    RUN_TEST(test_avatar_save_load_set_unset);
    RUN_TEST(test_avatar_self_and_online);
    RUN_TEST(test_avatar_move_on_nospam_change);
    RUN_TEST(test_avatar_oversized_file);
    reset_avatar();
    return result;
}
