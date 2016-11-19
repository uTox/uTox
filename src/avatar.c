#include "avatar.h"

#include "friend.h"
#include "main.h"
#include "util.h"

/* frees the image of an avatar, does nothing if image is NULL */
static void avatar_free_image(AVATAR *avatar) {
    if (avatar) {
        image_free(avatar->img);
        avatar->img = NULL;
    }
}

bool save_avatar(uint32_t friend_number, const uint8_t *data, uint32_t length) {
    char    name[sizeof("avatars/") + TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".png")];
    FILE *  fp;
    size_t *size = 0;

    if (friend_number == -1) {
        /* load current user's avatar */
#ifdef __WIN32__
        snprintf(name, sizeof("avatars/") + TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".png"), "avatars\\%.*s.png",
                 TOX_PUBLIC_KEY_SIZE * 2, self.id_str);
#else
        snprintf(name, sizeof("avatars/") + TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".png"), "avatars/%.*s.png",
                 TOX_PUBLIC_KEY_SIZE * 2, self.id_str);
#endif
    } else {
        FRIEND *f = &friend[friend_number];
#ifdef __WIN32__
        snprintf(name, sizeof("avatars/") + TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".png"), "avatars\\%.*s.png",
                 TOX_PUBLIC_KEY_SIZE * 2, f->id_str);
#else
        snprintf(name, sizeof("avatars/") + TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".png"), "avatars/%.*s.png",
                 TOX_PUBLIC_KEY_SIZE * 2, f->id_str);
#endif
    }

    fp = native_get_file(name, size, UTOX_FILE_OPTS_WRITE);

    if (fp == NULL) {
        debug("Could not open avatar for friend number: %u\n", friend_number);
        return false;
    }

    fwrite(data, length, 1, fp);
    flush_file(fp);
    fclose(fp);
    return true;
}

uint8_t *load_img_data(char hexid[64], size_t *out_size){
    char name[sizeof("avatars/") + sizeof(*hexid) + sizeof(".png")];

#ifdef __WIN32__
    snprintf(name, sizeof(*name), "avatar\\%.*s.png", (int)sizeof(*hexid), hexid);
#else
    snprintf(name, sizeof(*name), "avatars/%.*s.png", (int)sizeof(*hexid), hexid);
#endif

    size_t size = 0;

    FILE *fp = native_get_file(name, &size, UTOX_FILE_OPTS_READ);
    if (fp == NULL) {
        debug("Could not open avatar for friend : %.*s", (int)sizeof(*hexid), hexid);
        return NULL;
    }

    uint8_t *data = calloc(size, 1);
    if (data == NULL) {
        debug("Could not allocate memory for avatar of size %lu.\n", size);
        fclose(fp);
        return NULL;
    }

    if (fread(data, 1, size, fp) != size) {
        debug("Could not read: avatar for friend : %.*s", (int)sizeof(*hexid), hexid);
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

bool avatar_delete(uint32_t friend_number) {
    char    hex[TOX_PUBLIC_KEY_SIZE * 2];
    uint8_t name[sizeof("avatars/") + TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".png")];

    if (friend_number == -1) {
        memcpy(hex, self.id_str, TOX_PUBLIC_KEY_SIZE * 2);
    } else {
        /* load current user's avatar */
        FRIEND *f = &friend[friend_number];
        cid_to_string(hex, f->cid);
    }
#ifdef __WIN32__
    int name_len = snprintf((char *)name, sizeof("avatars/") + TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".png"),
                            "avatars/%.*s.png", TOX_PUBLIC_KEY_SIZE * 2, (char *)hex);
#else
    int name_len = snprintf((char *)name, sizeof("avatars/") + TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".png"),
                        "avatars\\%.*s.png", TOX_PUBLIC_KEY_SIZE * 2, (char *)hex);
#endif

    return native_remove_file(name, name_len);
}

bool load_avatar(char hexid[64], AVATAR *avatar, size_t *size_out) {
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
        if (size_out) {
            *size_out = size;
        }
        return true;
    }

    return false;
}

bool avatar_set(AVATAR *avatar, const uint8_t *data, size_t size) {
    if (!avatar) {
        debug("Avatars:\t avatar is not allocated.\n");
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
    tox_hash(avatar->hash, data, size);

    return true;
}

/* sets self avatar, see self_set_and_save_avatar */
bool avatar_set_self(const uint8_t *data, size_t size) {
    return avatar_set(self.avatar, data, size);
}

void avatar_unset(AVATAR *avatar) {
    if (!avatar) {
        return;
    }

    avatar->format = UTOX_AVATAR_FORMAT_NONE;
    avatar_free_image(avatar);
}

void avatar_unset_self(void) {
    avatar_unset(self.avatar);
}


/** tries to load avatar from disk for given client id string and set avatar based on saved png data
 *  avatar is avatar to initialize. Will be unset if no file is found on disk or if file is corrupt or too large,
 *      otherwise will be set to avatar found on disk
 *  id is cid string of whose avatar to find(see also load_avatar in avatar.h)
 *  if png_data_out is not NULL, the png data loaded from disk will be copied to it.
 *      if it is not null, it should be at least UTOX_AVATAR_MAX_DATA_LENGTH bytes long
 *  if png_size_out is not null, the size of the png data will be stored in it
 *
 *  returns: 1 on successful loading, 0 on failure
 */
bool avatar_init(char hexid[64], AVATAR *avatar) {
    avatar_unset(avatar);
    return load_avatar(hexid, avatar, NULL);
}

bool avatar_init_self(char hexid[64]) {
    self.avatar = calloc(1, sizeof(AVATAR));
    if (!self.avatar) {
        return false;
    }

    return load_avatar(hexid, self.avatar, NULL);
}

bool self_set_and_save_avatar(const uint8_t *data, uint32_t size) {
    if (avatar_set_self(data, size)) {
        save_avatar(-1, data, size);
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
    uint8_t *avatar_data = load_img_data(-1, &avatar_size);
    if (!avatar_data) {
        return false;
    }

    outgoing_file_send(tox, friend_number, NULL, avatar_data, avatar_size, TOX_FILE_KIND_AVATAR);
    free(avatar_data);
    return true;
}

void utox_incoming_avatar(uint32_t friend_number, uint8_t *avatar, size_t size) {
    // save avatar and hash to disk

    if (size == 0) {
        postmessage(FRIEND_AVATAR_UNSET, friend_number, 0, NULL);
    } else {
        postmessage(FRIEND_AVATAR_SET, friend_number, size, avatar);
    }
}
