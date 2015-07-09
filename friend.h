typedef struct friend_meta_data {
    size_t alias_length;
    uint8_t data[];
} FRIEND_META_DATA;

typedef struct friend {
    _Bool online, typing, notify;
    uint8_t number, status;
    int32_t call_id, call_state, call_state_friend;
    uint16_t video_width, video_height;

    uint8_t cid[TOX_PUBLIC_KEY_SIZE], tooltip[8];
    char_t *name, *alias, *status_message, *typed;
    STRING_IDX name_length, alias_length, status_length, typed_length;

    MSG_DATA msg;

    EDIT_CHANGE **edit_history;
    uint16_t edit_history_cur, edit_history_length;

    AVATAR avatar;

    uint16_t transfer_count;

    FRIEND_META_DATA metadata;
} FRIEND;

#define MAX_GROUP_PEERS 256

typedef struct groupchat {
    uint32_t peers;
    uint32_t our_peer_number;
    uint8_t type;
    _Bool audio_calling;
    volatile _Bool muted;
    STRING_IDX name_length, topic_length, typed_length;
    char_t name[128], topic[128]; //static sizes for now
    char_t *typed;
    char_t *peername[MAX_GROUP_PEERS];
    unsigned int source[MAX_GROUP_PEERS];
    volatile uint64_t last_recv_audio[MAX_GROUP_PEERS]; /* TODO: thread safety (This should work fine but it isn't very clean.) */

    EDIT_CHANGE **edit_history;
    uint16_t edit_history_cur, edit_history_length;

    MSG_DATA msg;
} GROUPCHAT;

#define friend_id(f) (f -  friend)

void friend_setname(FRIEND *f, char_t *name, STRING_IDX length);
void friend_set_alias(FRIEND *f, char_t *alias, STRING_IDX length);
void friend_addmessage(FRIEND *f, void *data);
void friend_sendimage(FRIEND *f, UTOX_NATIVE_IMAGE *, uint16_t width, uint16_t height, UTOX_PNG_IMAGE, size_t png_size);
void friend_recvimage(FRIEND *f, UTOX_NATIVE_IMAGE *native_image, uint16_t width, uint16_t height);

void friend_notify(FRIEND *f, char_t *str, STRING_IDX str_length, char_t *msg, STRING_IDX msg_length);
#define friend_notifystr(f, str, msg, mlen) friend_notify(f, (char_t*)str, sizeof(str) - 1, msg, mlen)
void friend_addmessage_notify(FRIEND *f, char_t *data, STRING_IDX length);
void friend_set_typing(FRIEND *f, int typing);

void friend_addid(uint8_t *id, char_t *msg, STRING_IDX msg_length);
void friend_add(char_t *name, STRING_IDX length, char_t *msg, STRING_IDX msg_length);

void friend_history_clear(FRIEND *f);

void friend_free(FRIEND *f);
void group_free(GROUPCHAT *g);
