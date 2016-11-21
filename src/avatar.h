#ifndef AVATAR_H
#define AVATAR_H

#include "main.h"

// TODO: remove?
#define UTOX_AVATAR_MAX_DATA_LENGTH (64 * 1024) // NOTE: increasing this above 64k might cause
                                                // issues with other clients who do stupid things.
#define UTOX_AVATAR_FORMAT_NONE 0
#define UTOX_AVATAR_FORMAT_PNG 1

/* data needed for each avatar in memory */
typedef struct avatar {
    NATIVE_IMAGE *img; /* converted avatar image to draw */

    size_t   size;
    uint16_t width, height;         /* width and height of image (in pixels) */
    uint8_t  format;                /* one of TOX_AVATAR_FORMAT */
    uint8_t  hash[TOX_HASH_LENGTH]; /* tox_hash for the png data of this avatar */
} AVATAR;

/* whether user's avatar is set */
#define self_has_avatar() (self.avatar && self.avatar->format != UTOX_AVATAR_FORMAT_NONE)
/* whether friend f's avatar is set, where f is a pointer to a friend struct */
#define friend_has_avatar(f) (f) && (f->avatar.format != UTOX_AVATAR_FORMAT_NONE)

bool avatar_init(char hexid[TOX_PUBLIC_KEY_SIZE * 2], AVATAR *avatar);

/* converts png data given by data to a NATIVE_IMAGE and uses that to populate the avatar struct
 *  avatar is pointer to an avatar struct to store result in. Remains unchanged if function fails.
 *  data is pointer to png data to convert
 *  size is size of data
 *
 *  on success: returns true
 *  on failure: returns false
 *
 *  notes: fails if given size is larger than UTOX_AVATAR_MAX_DATA_LENGTH or data is not valid PNG data
 */
bool avatar_set(AVATAR *avatar, const uint8_t *data, size_t size);

/** Helper function for the users avatar */
bool avatar_set_self(const uint8_t *data, size_t size);

/* unsets an avatar by setting its format to UTOX_AVATAR_FORMAT_NONE and
 * freeing its image */
void avatar_unset(AVATAR *avatar);

/* Helper function to unset the user's avatar */
void avatar_unset_self(void);

/* sets own avatar based on given png data and saves it to disk if successful
 *  data is png data to set avatar to
 *  size is size of data
 *
 *  on success: returns 1
 *  on failure: returns 0
 *
 *  notes: fails if size is too large or data is not a valid png file
 */
bool self_set_and_save_avatar(const uint8_t *data, uint32_t size);

/* unsets own avatar and removes it from disk */
bool avatar_remove_self(void);

/* Call this every time friend_number goes online from the tox_do thread.
 *
 * return 1 on success.
 * return 0 on failure.
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

#endif
