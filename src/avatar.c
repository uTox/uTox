#include "avatar.h"

#include "friend.h"
#include "main.h"
#include "util.h"

/* frees the image of an avatar, does nothing if image is NULL */
static void avatar_free_image(AVATAR *avatar) {
    if (avatar) {
        image_free(avatar->img);
        avatar->img = NULL;
        avatar->size = 0;
    }
}

bool avatar_save(char hexid[TOX_PUBLIC_KEY_SIZE * 2], const uint8_t *data, size_t length) {
    char name[sizeof("avatars/") + TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".png")] = { 0 };
    FILE *fp;

    snprintf(name, sizeof("avatars/") + TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".png"), "avatars/%.*s.png",
             TOX_PUBLIC_KEY_SIZE * 2, hexid);

    fp = native_get_file(name, NULL, UTOX_FILE_OPTS_WRITE);

    if (fp == NULL) {
        debug("Avatars:\tCould not open avatar for: %.*s\n", (int)sizeof(*hexid), hexid);
        return false;
    }

    fwrite(data, length, 1, fp);
    flush_file(fp);
    fclose(fp);
    return true;
}

static uint8_t *load_img_data(char hexid[TOX_PUBLIC_KEY_SIZE * 2], size_t *out_size) {
    char name[sizeof("avatars/") + TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".png")] = { 0 };

    snprintf(name, sizeof("avatars/") + TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".png"), "avatars/%.*s.png",
             TOX_PUBLIC_KEY_SIZE * 2, hexid);

    return load_data((uint8_t *)name, out_size);
}

bool avatar_delete(char hexid[TOX_PUBLIC_KEY_SIZE * 2]) {
    char name[sizeof("avatars/") + TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".png")] = { 0 };

    int name_len = snprintf(name, sizeof("avatars/") + TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".png"),
                            "avatars/%.*s.png", TOX_PUBLIC_KEY_SIZE * 2, hexid);

    return native_remove_file((uint8_t *)name, name_len);
}

static bool avatar_load(char hexid[TOX_PUBLIC_KEY_SIZE * 2], AVATAR *avatar, size_t *size_out) {
    size_t size = 0;

    uint8_t *img = load_img_data(hexid, &size);
    if (!img) {
        debug_notice("Avatars:\tUnable to get saved avatar from disk for friend %.*s\n", 32, hexid);
        return false;
    }

    if (size > UTOX_AVATAR_MAX_DATA_LENGTH) {
        free(img);
        debug_error("Avatars:\tSaved avatar file for friend (%.*s) too large for tox\n", 32, hexid);
        return false;
    }

    avatar->img = utox_image_to_native(img, size, &avatar->width, &avatar->height, true);
    if (avatar->img) {
        avatar->format = UTOX_AVATAR_FORMAT_PNG;
        avatar->size   = size;
        tox_hash(avatar->hash, img, size);
        if (size_out) {
            *size_out = size;
        }
        if (avatar == self.avatar) {
            // We need to save our avatar in PNG format so we can send it to friends!
            self.png_data = img;
            self.png_size = size;
        } else {
            free(img);
        }
        return true;
    }

    free(img);
    return false;
}

bool avatar_set(AVATAR *avatar, const uint8_t *data, size_t size) {
    if (avatar == NULL) {
        debug("Avatars:\t avatar is null.\n");
        return false;
    }

    if (size > UTOX_AVATAR_MAX_DATA_LENGTH) {
        debug_error("Avatars:\t avatar too large\n");
        return false;
    }

    avatar_free_image(avatar);
    NATIVE_IMAGE *image = utox_image_to_native((UTOX_IMAGE)data, size, &avatar->width, &avatar->height, true);
    if (!NATIVE_IMAGE_IS_VALID(image)) {
        debug("Avatars:\t avatar is invalid\n");
        return false;
    }

    avatar->img    = image;
    avatar->format = UTOX_AVATAR_FORMAT_PNG;
    avatar->size   = size;
    tox_hash(avatar->hash, data, size);

    return true;
}

/* sets self avatar, see self_set_and_save_avatar */
bool avatar_set_self(const uint8_t *data, size_t size) {
    return avatar_set(self.avatar, data, size);
}

void avatar_unset(AVATAR *avatar) {
    if (avatar == NULL) {
        return;
    }

    avatar->format = UTOX_AVATAR_FORMAT_NONE;
    avatar_free_image(avatar);
}

void avatar_unset_self(void) {
    avatar_unset(self.avatar);
}

bool avatar_init(char hexid[TOX_PUBLIC_KEY_SIZE * 2], AVATAR *avatar) {
    avatar_unset(avatar);
    return avatar_load(hexid, avatar, NULL);
}

bool avatar_init_self(void) {
    self.avatar = calloc(1, sizeof(AVATAR));
    if (self.avatar == NULL) {
        return false;
    }

    return avatar_load(self.id_str, self.avatar, NULL);
}

bool self_set_and_save_avatar(const uint8_t *data, uint32_t size) {
    if (avatar_set_self(data, size)) {
        avatar_save(self.id_str, data, size);
        return true;
    }
    return false;
}

void avatar_delete_self(void) {
    avatar_unset(self.avatar);
    postmessage_toxcore(TOX_AVATAR_UNSET, 0, 0, NULL);
}

bool avatar_on_friend_online(Tox *tox, uint32_t friend_number) {
    size_t   avatar_size = 0;
    uint8_t *avatar_data = load_img_data(self.id_str, &avatar_size);
    if (!avatar_data) {
        return false;
    }

    ft_send_avatar(tox, friend_number);
    free(avatar_data);
    return true;
}
