#define MAX_GROUP_PEERS 256

typedef struct groupchat {
    /* Our information */
    _Bool audio_calling, notify;
    char_t *typed;
    EDIT_CHANGE **edit_history;
    uint16_t edit_history_cur, edit_history_length;

    /* Group chat information */
    size_t name_length,
           topic_length,
           typed_length;
    char_t name[TOX_GROUP_MAX_GROUP_NAME_LENGTH],
           topic[TOX_GROUP_MAX_TOPIC_LENGTH];

    /* Group chat peers */
    uint32_t our_peer_number;
    uint32_t peers;
    uint32_t peer_ids[MAX_GROUP_PEERS];
    char_t *peername[MAX_GROUP_PEERS];


    /* legacy audio information */
    unsigned int source[MAX_GROUP_PEERS];
    volatile uint64_t last_recv_audio[MAX_GROUP_PEERS]; /* TODO: thread safety (This should work fine but it isn't very clean.) */
    volatile _Bool muted;

    MSG_DATA msg;
} GROUPCHAT;

void group_free(GROUPCHAT *g);

_Bool group_append_mesage(_Bool author, uint8_t type, uint32_t groupnumber,
                          size_t msg_length, const uint8_t *message,
                          size_t name_length, const uint8_t *name);

void utox_set_callbacks_for_groupchats(Tox *tox);
