#define AVATAR_DIRECTORY "avatars_utox"

typedef struct avatar {
    uint8_t format;
    uint8_t hash[TOX_HASH_LENGTH];
    UTOX_NATIVE_IMAGE image;
    uint16_t width, height;
}AVATAR;

#define self_has_avatar() (self.avatar.format != TOX_AVATAR_FORMAT_NONE)
#define friend_has_avatar(f) (f->avatar.format != TOX_AVATAR_FORMAT_NONE)

int load_avatar(const char_t *id, uint8_t *dest, uint32_t *size_out);
int save_avatar(const char_t *id, const uint8_t *data, uint32_t size);
int delete_saved_avatar(const char_t *id);

int set_avatar(AVATAR *avatar, const uint8_t *data, uint32_t size, _Bool create_hash);
void unset_avatar(AVATAR *avatar);

int self_set_and_save_avatar(const uint8_t *data, uint32_t size);
void self_remove_avatar();
