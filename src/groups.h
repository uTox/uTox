#define MAX_GROUP_PEERS 256

typedef struct groupchat {
    _Bool audio_calling, notify;
    uint32_t peers;
    uint32_t our_peer_number;

    uint8_t av_group;

    volatile _Bool muted;
    ALuint audio_dest;

    uint16_t name_length, topic_length, typed_length;
    char_t name[128], topic[128]; //static sizes for now
    char_t *typed;
    char_t *peername[MAX_GROUP_PEERS];
    unsigned int source[MAX_GROUP_PEERS];
    volatile uint64_t last_recv_audio[MAX_GROUP_PEERS]; /* TODO: thread safety (This should work fine but it isn't very clean.) */

    EDIT_CHANGE **edit_history;
    uint16_t edit_history_cur, edit_history_length;

    MESSAGES msg;
} GROUPCHAT;

void group_free(GROUPCHAT *g);
