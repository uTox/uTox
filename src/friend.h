/**
 *  friend.h
 */
typedef struct friend_meta_data {
    uint8_t version;

    uint8_t ft_autoaccept      : 1;
    uint8_t ft_autoaccept_path : 1;
    uint8_t log_history        : 1;
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
    _Bool   online, typing, notify;
    uint8_t number, status;

    /* A/V calls */
    int32_t  call_state_self, call_state_friend;
    uint16_t video_width, video_height;
    ALuint   audio_dest;

    uint8_t     cid[TOX_PUBLIC_KEY_SIZE],    tooltip[8];
    char_t     *name, *alias, *status_message, *typed;
    STRING_IDX  name_length, alias_length, status_length, typed_length;

    MSG_DATA msg;

    EDIT_CHANGE **edit_history;
    uint16_t edit_history_cur, edit_history_length;

    AVATAR avatar;

    /* File transfers */
    uint16_t    transfer_count;
    _Bool       ft_autoaccept;

} FRIEND;

typedef struct {
    uint16_t length;
    uint8_t id[TOX_FRIEND_ADDRESS_SIZE], msg[0];
} FRIENDREQ;


#define friend_id(f) (f -  friend)

void utox_friend_init(Tox *tox, uint32_t friend_number);

void friend_setname(FRIEND *f, char_t *name, STRING_IDX length);
void friend_set_alias(FRIEND *f, char_t *alias, STRING_IDX length);
void friend_addmessage(FRIEND *f, void *data);
void friend_sendimage(FRIEND *f, UTOX_NATIVE_IMAGE *, uint16_t width, uint16_t height, UTOX_IMAGE, size_t png_size);
void friend_recvimage(FRIEND *f, UTOX_NATIVE_IMAGE *native_image, uint16_t width, uint16_t height);

void friend_notify(FRIEND *f, char_t *str, STRING_IDX str_length, char_t *msg, STRING_IDX msg_length);
#define friend_notifystr(f, str, msg, mlen) friend_notify(f, (char_t*)str, sizeof(str) - 1, msg, mlen)
void friend_addmessage_notify(FRIEND *f, char_t *data, STRING_IDX length);

/* set friend online status. Returns: true if status changed, false otherwise */
_Bool friend_set_online(FRIEND *f, _Bool online);

void friend_set_typing(FRIEND *f, int typing);

void friend_addid(uint8_t *id, char_t *msg, STRING_IDX msg_length);
void friend_add(char_t *name, STRING_IDX length, char_t *msg, STRING_IDX msg_length);

void friend_history_clear(FRIEND *f);

void friend_free(FRIEND *f);
