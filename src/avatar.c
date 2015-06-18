#include "main.h"

/* gets avatar filepath for given client id string and stores it in dest,
 * returns number of chars written */
int get_avatar_location(char_t *dest, const char_t *id)
{
    char_t *p = dest + datapath_subdir(dest, AVATAR_DIRECTORY);
    memcpy((char *)p, id, TOX_PUBLIC_KEY_SIZE * 2); p += TOX_PUBLIC_KEY_SIZE * 2;
    strcpy((char *)p, ".png"); p += sizeof(".png") - 1;

    return p - dest;
}

/* frees the image of an avatar, does nothing if image is NULL */
void avatar_free_image(AVATAR *avatar)
{
    if (avatar->image) {
        image_free(avatar->image);
        avatar->image = NULL;
    }
}


int load_avatar(const char_t *id, uint8_t *dest, uint32_t *size_out)
{
    char_t path[UTOX_FILE_NAME_LENGTH];
    uint32_t size;

    get_avatar_location(path, id);

    uint8_t *avatar_data = file_raw((char *)path, &size);
    if (!avatar_data) {
        return 0;
    }
    if (size > UTOX_AVATAR_MAX_DATA_LENGTH) {
        free(avatar_data);
        debug("Avatars:\t saved avatar file(%s) too large for tox\n", path);
        return 0;
    }

    memcpy(dest, avatar_data, size);
    free(avatar_data);
    if (size_out) {
        *size_out = size;
    }
    return 1;
}

int save_avatar(const char_t *id, const uint8_t *data, uint32_t size)
{
    char_t path[UTOX_FILE_NAME_LENGTH];

    get_avatar_location(path, id);

    FILE *file = fopen((char*)path, "wb");
    if (file) {
        fwrite(data, size, 1, file);
        flush_file(file);
        fclose(file);
        return 1;
    } else {
        debug("Avatars:\terror opening avatar file (%s) for writing\n", (char *)path);
        return 0;
    }
}

int delete_saved_avatar(const char_t *id)
{
    char_t path[UTOX_FILE_NAME_LENGTH];

    get_avatar_location(path, id);

    return remove((char *)path);
}


int set_avatar(AVATAR *avatar, const uint8_t *data, uint32_t size)
{
    if (size > UTOX_AVATAR_MAX_DATA_LENGTH) {
        debug("Avatars:\t avatar too large\n");
        return 0;
    }

    uint16_t w, h;
    UTOX_NATIVE_IMAGE *image = png_to_image((UTOX_PNG_IMAGE)data, size, &w, &h, 1);
    if(!UTOX_NATIVE_IMAGE_IS_VALID(image)) {
        debug("Avatars:\t avatar is invalid\n");
        return 0;
    } else {

        avatar_free_image(avatar);

        avatar->image = image;
        avatar->width = w;
        avatar->height = h;
        avatar->format = UTOX_AVATAR_FORMAT_PNG;
        tox_hash(avatar->hash, data, size);

        return 1;
    }
}

void unset_avatar(AVATAR *avatar)
{
    avatar->format = UTOX_AVATAR_FORMAT_NONE;
    avatar_free_image(avatar);
}

/* sets self avatar, see self_set_and_save_avatar */
int self_set_avatar(const uint8_t *data, uint32_t size)
{
    if (!set_avatar(&self.avatar, data, size)) {
        return 0;
    }

    uint8_t *png_data = malloc(size);
    memcpy(png_data, data, size);
    tox_postmessage(TOX_SETAVATAR, UTOX_AVATAR_FORMAT_PNG, size, png_data);
    return 1;
}

int self_set_and_save_avatar(const uint8_t *data, uint32_t size)
{
    if (self_set_avatar(data, size)) {
        uint8_t hex_id[TOX_FRIEND_ADDRESS_SIZE * 2];
        id_to_string(hex_id, self.id_binary);
        save_avatar(hex_id, data, size);
        return 1;
    } else {
        return 0;
    }
}

void self_remove_avatar()
{
    unset_avatar(&self.avatar);
    uint8_t hex_id[TOX_FRIEND_ADDRESS_SIZE * 2];
    id_to_string(hex_id, self.id_binary);
    delete_saved_avatar(hex_id);
    tox_postmessage(TOX_UNSETAVATAR, 0, 0, NULL);
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

    if(size <= 0){
        postmessage(FRIEND_UNSETAVATAR, friend_number, 0, NULL);
    } else {
        postmessage(FRIEND_SETAVATAR, friend_number, size, avatar);
    }
}
