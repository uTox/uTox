#include "avatar.h"

#include "friend.h"
#include "main.h"
#include "util.h"

/* frees the image of an avatar, does nothing if image is NULL */
static void avatar_free_image(AVATAR *avatar) {
    if (avatar->img) {
        image_free(avatar->img);
        avatar->img = NULL;
    }
}

static bool save_avatar(uint32_t friend_number, const uint8_t *data, uint32_t size) {
    return utox_data_save_avatar(friend_number, data, size);
}

static bool avatar_delete(uint32_t friend_number) {
    return utox_data_del_avatar(friend_number);
}

static bool load_avatar(char hexid[64], AVATAR *avatar, size_t *size_out) {
    size_t size = 0;

    uint8_t *img = utox_data_load_avatar(hexid, &size);
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

int avatar_set(AVATAR *avatar, const uint8_t *data, size_t size) {
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
    avatar->format = UTOX_AVATAR_FORMAT_NONE;
    avatar_free_image(avatar);
}

bool avatar_unset_self(void) {
    return true;
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

int self_set_and_save_avatar(const uint8_t *data, uint32_t size) {
    if (avatar_set_self(data, size)) {
        save_avatar(-1, data, size);
        return 1;
    } else {
        return 0;
    }
}

void avatar_delete_self(void) {
    avatar_unset(self.avatar);
    postmessage_toxcore(TOX_AVATAR_UNSET, 0, 0, NULL);
}

bool avatar_on_friend_online(Tox *tox, uint32_t friend_number) {
    size_t   avatar_size;
    uint8_t *avatar_data = utox_data_load_avatar(-1, &avatar_size);

    outgoing_file_send(tox, friend_number, NULL, avatar_data, avatar_size, TOX_FILE_KIND_AVATAR);
    return 1;
}

void utox_incoming_avatar(uint32_t friend_number, uint8_t *avatar, size_t size) {
    // save avatar and hash to disk

    if (size == 0) {
        postmessage(FRIEND_AVATAR_UNSET, friend_number, 0, NULL);
    } else {
        postmessage(FRIEND_AVATAR_SET, friend_number, size, avatar);
    }
}
