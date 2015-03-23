/* subdirectory name to save avatars in */
#define AVATAR_DIRECTORY "avatars"

//TODO: remove
#define UTOX_AVATAR_MAX_DATA_LENGTH (64 * 1024) //NOTE: increasing this above 64k might cause issues.
#define UTOX_AVATAR_FORMAT_NONE 0
#define UTOX_AVATAR_FORMAT_PNG 1

/* data needed for each avatar in memory */
typedef struct avatar {
    uint8_t format; /* one of TOX_AVATAR_FORMAT */
    uint8_t hash[TOX_HASH_LENGTH]; /* tox_hash for the png data of this avatar */
    UTOX_NATIVE_IMAGE *image; /* converted avatar image to draw */
    uint16_t width, height; /* width and height of image (in pixels) */
}AVATAR;

/* whether user's avatar is set */
#define self_has_avatar() (self.avatar.format != UTOX_AVATAR_FORMAT_NONE)
/* whether friend f's avatar is set, where f is a pointer to a friend struct */
#define friend_has_avatar(f) (f->avatar.format != UTOX_AVATAR_FORMAT_NONE)

/* gets the avatar location on the disk and puts the result in dest.
 * id is the client id string for given client. To get the cid string from a cid, use cid_to_string
 *  returns the number of chars written
 */
int get_avatar_location(char_t *dest, const char_t *id);

/* loads an avatar from disk and puts the resulting png data in buffer given by dest.
 * id is the client id string for given client. To get the cid string from a cid, use cid_to_string
 *   id should be at least (TOX_PUBLIC_KEY_SIZE * 2) bytes long
 * if size_out is not NULL, load_avatar will store the length of the png data there
 *  on success: returns 1
 *  on failure: returns 0
 *  notes: dest should be at least UTOX_AVATAR_MAX_DATA_LENGTH bytes long.
 */
int load_avatar(const char_t *id, uint8_t *dest, uint32_t *size_out);

/* saves avatar png data to disk
 * id is cid string(see load_avatar), and size is size of data
 *  on success: returns 1
 *  on failure: returns 0
 *  see also: load_avatar
 */
int save_avatar(const char_t *id, const uint8_t *data, uint32_t size);

/* deletes saved avatar data for given id
 *  on success: returns 1
 *  on failure: returns 0
 *  see also: load_avatar
 */
int delete_saved_avatar(const char_t *id);

/* converts png data given by data to a UTOX_NATIVE_IMAGE and uses that to populate the avatar struct
 *  avatar is pointer to an avatar struct to store result in. Remains unchanged if function fails.
 *  data is pointer to png data to convert
 *  size is size of data
 *
 *  on success: returns 1
 *  on failure: returns 0
 *
 *  notes: fails if given size is larger than UTOX_AVATAR_MAX_DATA_LENGTH or data is not valid PNG data
 */
int set_avatar(AVATAR *avatar, const uint8_t *data, uint32_t size);

/* unsets an avatar by setting its format to UTOX_AVATAR_FORMAT_NONE and
 * freeing its image */
void unset_avatar(AVATAR *avatar);

/* sets own avatar based on given png data and saves it to disk if successful
 *  data is png data to set avatar to
 *  size is size of data
 *
 *  on success: returns 1
 *  on failure: returns 0
 *
 *  notes: fails if size is too large or data is not a valid png file
 */
int self_set_and_save_avatar(const uint8_t *data, uint32_t size);

/* unsets own avatar and removes it from disk */
void self_remove_avatar();

/* Call this every time friend_number goes online from the tox_do thread.
 *
 * return 1 on success.
 * return 0 on failure.
 */
_Bool avatar_on_friend_online(Tox *tox, uint32_t friend_number);

/** called once out new avatar is changed to update all of our friends
 *
 * returns 0 if there were no errors sending the avatars to every online friend.
 * returns +1 for each friend we could not send to.
 * returns negitive if any avatar or friend was left in an unknown state.
 */
int utox_avatar_update_friends(Tox *tox);

/** Colled by incoming file transfers to change the avater.
 *
 * If size <=0, we'll unset the avatar, else we'll set and update the friend
 */
void utox_incoming_avatar(uint32_t friend_number, uint8_t *avatar, size_t size);
