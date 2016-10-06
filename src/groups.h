#define MAX_GROUP_PEERS 256

/*  UTOX_SAVE limits 8 as the max */
typedef enum {
    GNOTIFY_NEVER,      /* 0: never send notifications, */
    GNOTIFY_HIGHLIGHTS, /* 1: only send when mentioned, */
    GNOTIFY_ALWAYS,     /* 2: always send notifications */
} GNOTIFY_TYPE;

typedef struct group_peer {
    uint32_t id;

    uint32_t name_color; /* TODO chose a color for the peer name from a list,
                                * and persist across name changes*/
    size_t   name_length;
    uint8_t  name[0];
} GROUP_PEER;

typedef struct groupchat {
    _Bool audio_calling, unread_msg;
    uint32_t our_peer_number;

    uint8_t number;
    uint8_t av_group;

    GNOTIFY_TYPE notify;

    volatile _Bool muted;
    ALuint audio_dest;

    uint16_t name_length, topic_length, typed_length;
    char_t name[128], topic[128]; /* TODO magic numbers */

    char_t *typed;

    /* Audio sources */
    unsigned int source[MAX_GROUP_PEERS];
    volatile uint64_t last_recv_audio[MAX_GROUP_PEERS]; /* TODO: thread safety (This should work fine but it isn't very clean.) */

    MESSAGES msg;
    EDIT_CHANGE **edit_history;
    uint16_t edit_history_cur, edit_history_length;

    uint32_t peer_count;
    void **peer;
} GROUPCHAT;


void group_init(GROUPCHAT *g, uint32_t group_number, _Bool av_group);

uint32_t group_add_message(GROUPCHAT *g, int peer_id, const uint8_t *message, size_t length, uint8_t m_type);

void group_peer_add(GROUPCHAT *g, uint32_t peer_id, _Bool our_peer_number, uint32_t name_color);

void group_peer_del(GROUPCHAT *g, uint32_t peer_id);

void group_peer_name_change(GROUPCHAT *g, uint32_t peer_id, const uint8_t *name, size_t length);

void group_reset_peerlist(GROUPCHAT *g);

void group_free(GROUPCHAT *g);

void group_notify_msg(GROUPCHAT *g, const uint8_t *message, size_t length);
