#include "main.h"

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
_Bool init_avatar(AVATAR *avatar, uint32_t friend_number, uint8_t *png_data_out, uint32_t *png_size_out) {
    unset_avatar(avatar);
    uint8_t *avatar_data = NULL;
    size_t size = 0;
    if (load_avatar(friend_number, &avatar_data, &size)) {
        if (set_avatar(friend_number, avatar_data, size)) {
            if (png_data_out) {
                memcpy(png_data_out, avatar_data, size);
            }
            if (png_size_out) {
                *png_size_out = size;
            }

            return 1;
        }
    }
    return 0;
}

/* frees the image of an avatar, does nothing if image is NULL */
static void avatar_free_image(AVATAR *avatar) {
    if (avatar->image) {
        image_free(avatar->image);
        avatar->image = NULL;
    }
}


int load_avatar(uint32_t friend_number, uint8_t **dest, size_t *size_out) {
    size_t size = 0;

    uint8_t *img = utox_load_data_avatar(friend_number, &size);
    if (!img) {
        debug_notice("Avatars:\tUnable to get saved avatar from disk for friend %u\n", friend_number);
        return 0;
    }

    if (size > UTOX_AVATAR_MAX_DATA_LENGTH) {
        free(img);
        debug_error("Avatars:\tSaved avatar file for friend (%u) too large for tox\n", friend_number);
        return 0;
    }

    *dest = img;

    // free(avatar_data);

    if (size_out) {
        *size_out = size;
    }
    return 1;
}

_Bool save_avatar(uint32_t friend_number, const uint8_t *data, uint32_t size) {
    return utox_save_data_avatar(friend_number, data, size);
}

_Bool delete_saved_avatar(uint32_t friend_number) {
    return utox_remove_file_avatar(friend_number);
}


int set_avatar(uint32_t friend_number, const uint8_t *data, uint32_t size) {
    if (size > UTOX_AVATAR_MAX_DATA_LENGTH) {
        debug("Avatars:\t avatar too large\n");
        return 0;
    }

    uint16_t w, h;
    UTOX_NATIVE_IMAGE *image = decode_image_rgb((UTOX_IMAGE)data, size, &w, &h, 1);
    if(!UTOX_NATIVE_IMAGE_IS_VALID(image)) {
        debug("Avatars:\t avatar is invalid\n");
        return 0;
    } else {
        if (friend_number == -1) {
            avatar_free_image(&self.avatar);
            self.avatar.image  = image;
            self.avatar.width  = w;
            self.avatar.height = h;
            self.avatar.format = UTOX_AVATAR_FORMAT_PNG;
            tox_hash(self.avatar.hash, data, size);
        } else {
            FRIEND *f = &friend[friend_number];
            avatar_free_image(&f->avatar);

            f->avatar.image  = image;
            f->avatar.width  = w;
            f->avatar.height = h;
            f->avatar.format = UTOX_AVATAR_FORMAT_PNG;
            tox_hash(f->avatar.hash, data, size);
        }

        return 1;
    }
}

void unset_avatar(AVATAR *avatar) {
    avatar->format = UTOX_AVATAR_FORMAT_NONE;
    avatar_free_image(avatar);
}

/* sets self avatar, see self_set_and_save_avatar */
int self_set_avatar(const uint8_t *data, uint32_t size) {
    if (!set_avatar(-1, data, size)) {
        return 0;
    }

    uint8_t *png_data = malloc(size);
    memcpy(png_data, data, size);
    postmessage_toxcore(TOX_AVATAR_SET, UTOX_AVATAR_FORMAT_PNG, size, png_data);
    return 1;
}

int self_set_and_save_avatar(const uint8_t *data, uint32_t size)
{
    if (self_set_avatar(data, size)) {
        save_avatar(-1, data, size);
        return 1;
    } else {
        return 0;
    }
}

void self_remove_avatar(void) {
    unset_avatar(&self.avatar);
    delete_saved_avatar(-1);
    postmessage_toxcore(TOX_AVATAR_UNSET, 0, 0, NULL);
}

_Bool avatar_on_friend_online(Tox *tox, uint32_t friend_number){
    uint8_t *avatar_data = self.avatar_data;
    size_t avatar_size = self.avatar_size;

    outgoing_file_send(tox, friend_number, NULL, avatar_data, avatar_size, TOX_FILE_KIND_AVATAR);
    return 1;
}

int utox_avatar_update_friends(Tox *tox){
    uint32_t i, friend_count, error_count = 0;
    friend_count = tox_self_get_friend_list_size(tox);
    uint32_t friend_loop[friend_count];
    tox_self_get_friend_list(tox, friend_loop);

    for(i = 0; i < friend_count; i++){
        if (tox_friend_get_connection_status(tox, friend_loop[i], 0)) {
            error_count += !avatar_on_friend_online(tox, friend_loop[i]);
        }
    }

    return error_count;
}

void utox_incoming_avatar(uint32_t friend_number, uint8_t *avatar, size_t size){
    // save avatar and hash to disk

    if (size == 0){
        postmessage(FRIEND_AVATAR_UNSET, friend_number, 0, NULL);
    } else {
        postmessage(FRIEND_AVATAR_SET, friend_number, size, avatar);
    }
}
