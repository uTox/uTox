#ifndef AVATAR_H
#define AVATAR_H

#include "main.h"

// TODO: remove?
#define UTOX_AVATAR_MAX_DATA_LENGTH (64 * 1024) // NOTE: increasing this above 64k might cause
                                                // issues with other clients who do stupid things.
#define UTOX_AVATAR_FORMAT_NONE 0
#define UTOX_AVATAR_FORMAT_PNG 1

/* data needed for each avatar in memory */
struct avatar {
    NATIVE_IMAGE *img; /* converted avatar image to draw */

    size_t   size;
    uint16_t width, height;         /* width and height of image (in pixels) */
    uint8_t  format;                /* one of TOX_AVATAR_FORMAT */
    uint8_t  hash[TOX_HASH_LENGTH]; /* tox_hash for the png data of this avatar */
};

/* Whether user's avatar is set. */
#define self_has_avatar() (self.avatar && self.avatar->format != UTOX_AVATAR_FORMAT_NONE)
/* Whether friend f's avatar is set, where f is a pointer to a friend struct */
#define friend_has_avatar(f) (f) && (f->avatar.format != UTOX_AVATAR_FORMAT_NONE)

/** tries to load avatar from disk for given client id string and set avatar based on saved png data
 * avatar is avatar to initialize. Will be unset if no file is found on disk or if file is corrupt or too large,
 *     otherwise will be set to avatar found on disk
 * id is cid string of whose avatar to find(see also avatar_load in avatar.c)
 * if png_data_out is not NULL, the png data loaded from disk will be copied to it.
 *     if it is not null, it should be at least UTOX_AVATAR_MAX_DATA_LENGTH bytes long
 * if png_size_out is not null, the size of the png data will be stored in it
 *
 * returns: true on successful loading, false on failure
 */
bool avatar_init(char hexid[TOX_PUBLIC_KEY_SIZE * 2], AVATAR *avatar);

/** Converts png data given by data to a NATIVE_IMAGE and uses that to populate the avatar struct
 * avatar is pointer to an avatar struct to store result in. Remains unchanged if function fails.
 * data is pointer to png data to convert
 * size is size of data
 *
 * on success: returns true
 * on failure: returns false
 *
 * notes: fails if given size is larger than UTOX_AVATAR_MAX_DATA_LENGTH or data is not valid PNG data
 */
bool avatar_set(AVATAR *avatar, const uint8_t *data, size_t size);

/* Helper function to set the user's avatar. */
bool avatar_set_self(const uint8_t *data, size_t size);

/* Helper function to unset the user's avatar. */
void avatar_unset_self(void);

/* Unsets an avatar by setting its format to UTOX_AVATAR_FORMAT_NONE and freeing its image. */
void avatar_unset(AVATAR *avatar);

/** Sets own avatar based on given png data and saves it to disk if successful.
 * data is png data to set avatar to.
 * size is size of data.
 *
 * on success: returns true
 * on failure: returns false
 *
 * Notes: Fails if size is too large or data is not a valid png file.
 */
bool self_set_and_save_avatar(const uint8_t *data, uint32_t size);

/* Unsets own avatar and removes it from disk */
bool avatar_remove_self(void);

/** Call this every time friend_number goes online from the tox_do thread.
 *
 *  on success: returns true
 *  on failure: returns false
 */
bool avatar_on_friend_online(Tox *tox, uint32_t friend_number);

/** Colled by incoming file transfers to change the avater.
 *
 * If size <=0, we'll unset the avatar, else we'll set and update the friend
 */
void utox_incoming_avatar(uint32_t friend_number, uint8_t *avatar, size_t size);

/* Saves the avatar for user with hexid
 *
 * returns true on success
 * returns false on failure
 */
bool avatar_save(char hexid[TOX_PUBLIC_KEY_SIZE * 2], const uint8_t *data, size_t length);

/* Deletes the avatar for user with hexid
 *
 * returns true on success
 * returns false on failure
 */
bool avatar_delete(char hexid[TOX_PUBLIC_KEY_SIZE * 2]);

/* Helper function to intialize the users avatar */
bool avatar_init_self(void);

/* Moves the avatar to its new name */
bool avatar_move(const uint8_t *source, const uint8_t *dest);

#endif
