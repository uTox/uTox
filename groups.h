#define MAX_GROUP_PEERS 256

typedef struct groupchat {
    _Bool audio_calling, notify;
    uint32_t peers;
    uint32_t our_peer_number;
    volatile _Bool muted;
    size_t name_length,
           topic_length,
           typed_length;
    char_t name[TOX_GROUP_MAX_GROUP_NAME_LENGTH],
           topic[TOX_GROUP_MAX_TOPIC_LENGTH];
    char_t *typed;
    char_t *peername[MAX_GROUP_PEERS];
    unsigned int source[MAX_GROUP_PEERS];
    volatile uint64_t last_recv_audio[MAX_GROUP_PEERS]; /* TODO: thread safety (This should work fine but it isn't very clean.) */

    EDIT_CHANGE **edit_history;
    uint16_t edit_history_cur, edit_history_length;

    MSG_DATA msg;
} GROUPCHAT;

void group_free(GROUPCHAT *g);

_Bool group_append_mesage(_Bool author, uint8_t type, uint32_t groupnumber,
                          size_t msg_length, const uint8_t *message,
                          size_t name_length, const uint8_t *name);

void utox_set_callbacks_for_groupchats(Tox *tox);
