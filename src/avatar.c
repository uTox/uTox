#include "avatar.h"

#include "debug.h"
#include "file_transfers.h"
#include "filesys.h"
#include "self.h"
#include "tox.h"

#include "native/image.h"

#include <stdlib.h>
#include <string.h>

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

    fp = utox_get_file(name, NULL, UTOX_FILE_OPTS_WRITE | UTOX_FILE_OPTS_MKDIR);

    if (!fp) {
        LOG_WARN("Avatar", "Could not save avatar for: %.*s", TOX_PUBLIC_KEY_SIZE * 2, hexid);
        return false;
    }

    fwrite(data, length, 1, fp);
    fclose(fp);
    return true;
}

static uint8_t *load_img_data(char hexid[TOX_PUBLIC_KEY_SIZE * 2], size_t *out_size) {
    char name[sizeof("avatars/") + TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".png")] = { 0 };

    snprintf(name, sizeof("avatars/") + TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".png"), "avatars/%.*s.png",
             TOX_PUBLIC_KEY_SIZE * 2, hexid);

    size_t size = 0;
    FILE *fp = utox_get_file(name, &size, UTOX_FILE_OPTS_READ);
    if (fp == NULL) {
        LOG_TRACE("Avatar", "Could not read: %s", name);
        return NULL;
    }

    uint8_t *data = calloc(1, size);
    if (data == NULL) {
        LOG_ERR("Avatar", "Could not allocate memory for file of size %zu.", size);
        fclose(fp);
        return NULL;
    }

    if (fread(data, size, 1, fp) != 1) {
        LOG_WARN("Avatar", "Could not read from open file: %s", name);
        fclose(fp);
        free(data);
        return NULL;
    }

    fclose(fp);
    if (out_size) {
        *out_size = size;
    }
    return data;
}

bool avatar_delete(char hexid[TOX_PUBLIC_KEY_SIZE * 2]) {
    char name[sizeof("avatars/") + TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".png")] = { 0 };

    snprintf(name, sizeof(name), "avatars/%.*s.png", TOX_PUBLIC_KEY_SIZE * 2, hexid);
    int name_len = strnlen(name, sizeof(name) - 1);

    return utox_remove_file((uint8_t *)name, name_len);
}

static bool avatar_load(char hexid[TOX_PUBLIC_KEY_SIZE * 2], AVATAR *avatar, size_t *size_out) {
    size_t size = 0;

    uint8_t *img = load_img_data(hexid, &size);
    if (!img) {
        LOG_DEBUG("Avatar", "Unable to get saved avatar from disk for friend %.*s" ,
                    TOX_PUBLIC_KEY_SIZE * 2, hexid);
        return false;
    }

    if (size > UTOX_AVATAR_MAX_DATA_LENGTH) {
        free(img);
        LOG_WARN("Avatar", "Saved avatar file for friend (%.*s) too large for tox" ,
                    TOX_PUBLIC_KEY_SIZE * 2, hexid);
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
        LOG_DEBUG("Avatar", "avatar is null.");
        return false;
    }

    if (size > UTOX_AVATAR_MAX_DATA_LENGTH) {
        LOG_ERR("Avatar", " avatar too large");
        return false;
    }

    avatar_free_image(avatar);
    NATIVE_IMAGE *image = utox_image_to_native((UTOX_IMAGE)data, size, &avatar->width, &avatar->height, true);
    if (!NATIVE_IMAGE_IS_VALID(image)) {
        LOG_DEBUG("Avatar", "avatar is invalid");
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
        LOG_TRACE("Avatar", " avatar is null" );
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
    avatar_delete(self.id_str);
    postmessage_toxcore(TOX_AVATAR_UNSET, 0, 0, NULL);
}

bool avatar_on_friend_online(Tox *tox, uint32_t friend_number) {
    if (!self.png_data) {
        uint8_t *avatar_data = load_img_data(self.id_str, &self.png_size);
        if (!avatar_data) {
            LOG_WARN("Avatar", "Unable to get out avatar data to send to friend.");
            self.png_data = NULL;
            self.png_size = 0;
            return false;
        }
        self.png_data = avatar_data;
    }

    ft_send_avatar(tox, friend_number);
    return true;
}

bool avatar_move(const uint8_t *source, const uint8_t *dest) {
    uint8_t current_name[sizeof("avatars/") + TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".png")] = { 0 };
    uint8_t new_name[sizeof("avatars/") + TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".png")] = { 0 };

    snprintf((char *)current_name, sizeof("avatars/") + TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".png"), "avatars/%.*s.png",
             TOX_PUBLIC_KEY_SIZE * 2, source);
    snprintf((char *)new_name, sizeof("avatars/") + TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".png"), "avatars/%.*s.png",
             TOX_PUBLIC_KEY_SIZE * 2, dest);

    return utox_move_file(current_name, new_name);
}
