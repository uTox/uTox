/**
 *  friend.h
 */
typedef struct friend_meta_data {
    uint8_t version;

    uint8_t ft_autoaccept      : 1;
    uint8_t ft_autoaccept_path : 1;
    uint8_t skip_msg_logging   : 1;
    uint8_t unused             : 5;

    uint8_t zero[30];

    size_t alias_length;
    size_t ft_autoaccept_path_length;

    uint8_t data[];
} FRIEND_META_DATA;
#define METADATA_VERSION 0


typedef struct friend_meta_data_old {
    size_t alias_length;
    uint8_t data[];
} FRIEND_META_DATA_OLD;

typedef struct friend {
    uint8_t cid[TOX_PUBLIC_KEY_SIZE];
    uint8_t id_str[TOX_PUBLIC_KEY_SIZE * 2];
    uint8_t number;

    uint8_t *name;
    uint8_t *alias;
    uint8_t *status_message;
    uint8_t *typed;

    size_t  name_length;
    size_t  alias_length;
    size_t  status_length;
    size_t  typed_length;

    /* Friend Status */
    uint8_t status;
    _Bool online;
    _Bool typing;
    _Bool video_inline;

    AVATAR avatar;

    /* Messages */
    _Bool skip_msg_logging;
    _Bool unread_msg;
    MESSAGES msg;
    EDIT_CHANGE **edit_history;
    uint16_t edit_history_cur, edit_history_length;

    /* Audio / Video */
    int32_t  call_state_self, call_state_friend;
    uint16_t video_width, video_height;
    ALuint   audio_dest;

    /* File transfers */
    uint16_t    transfer_count;
    _Bool       ft_autoaccept;

} FRIEND;

typedef struct {
    uint16_t length;
    uint8_t id[TOX_FRIEND_ADDRESS_SIZE], msg[0];
} FRIENDREQ;


#define friend_id(f) (f -  friend)

#define UTOX_FRIEND_NAME(f) ((f->alias) ? f->alias : f->name)
#define UTOX_FRIEND_NAME_LENGTH(f) ((f->alias) ? f->alias_length : f->name_length)

void utox_friend_init(Tox *tox, uint32_t friend_number);

void friend_setname(FRIEND *f, char_t *name, uint16_t length);
void friend_set_alias(FRIEND *f, char_t *alias, uint16_t length);
void friend_sendimage(FRIEND *f, UTOX_NATIVE_IMAGE *, uint16_t width, uint16_t height, UTOX_IMAGE, size_t png_size);
void friend_recvimage(FRIEND *f, UTOX_NATIVE_IMAGE *native_image, uint16_t width, uint16_t height);

void friend_notify_msg(FRIEND *f, const uint8_t *msg, size_t msg_length);

/* set friend online status. Returns: true if status changed, false otherwise */
_Bool friend_set_online(FRIEND *f, _Bool online);

void friend_set_typing(FRIEND *f, int typing);

void friend_addid(uint8_t *id, char_t *msg, uint16_t msg_length);
void friend_add(char_t *name, uint16_t length, char_t *msg, uint16_t msg_length);

void friend_history_clear(FRIEND *f);

void friend_free(FRIEND *f);
