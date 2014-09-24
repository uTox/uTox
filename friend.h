/*todo: */
enum {
    FT_NONE,

    FT_SEND,
    FT_PENDING,
    FT_PAUSE,
    FT_BROKE,
    FT_KILL,
};

enum {
    FILE_PENDING,
    FILE_OK,
    FILE_PAUSED,
    FILE_PAUSED_OTHER,
    FILE_BROKEN,
    FILE_KILLED,
    FILE_DONE,
};

typedef struct {
    /* used by the tox thread */
    uint8_t status, filenumber, name_length;
    _Bool finish, inline_png;
    uint16_t sendsize, buffer_bytes;
    uint32_t fid;
    void *data, *buffer;
    uint64_t bytes, total;
    uint8_t name[64];
    uint8_t *path;

    uint64_t lastupdate, lastprogress;

    /* used by the main thread */
    void *chatdata;
} FILE_T;

typedef struct friend {
    _Bool online, typing, notify;
    uint8_t calling, status;
    int32_t callid;
    uint16_t call_width, call_height;

    uint8_t cid[TOX_CLIENT_ID_SIZE];
    uint16_t name_length, status_length, typed_length;
    char_t *name, *status_message, *typed;

    MSG_DATA msg;

    EDIT_CHANGE **edit_history;
    uint16_t edit_history_cur, edit_history_length;

    FILE_T incoming[16];
    FILE_T outgoing[16];
} FRIEND;

typedef struct groupchat {
    uint32_t peers;
    uint16_t name_length, topic_length, typed_length;
    uint8_t name[128], topic[128]; //static sizes for now
    uint8_t *typed;
    uint8_t *peername[256];

    EDIT_CHANGE **edit_history;
    uint16_t edit_history_cur, edit_history_length;

    MSG_DATA msg;
} GROUPCHAT;

#define friend_id(f) (f -  friend)

void friend_setname(FRIEND *f, uint8_t *name, uint16_t length);
void friend_addmessage(FRIEND *f, void *data);
void friend_sendimage(FRIEND *f, void *data, void *pngdata, uint16_t width, uint16_t height);
void friend_recvimage(FRIEND *f, void *pngdata, uint32_t size);

void friend_notify(FRIEND *f, uint8_t *str, uint16_t str_length, uint8_t *msg, uint16_t msg_length);
#define friend_notifystr(f, str, msg, mlen) friend_notify(f, (uint8_t*)str, sizeof(str) - 1, msg, mlen)
void friend_addmessage_notify(FRIEND *f, char_t *data, uint16_t length);
void friend_set_typing(FRIEND *f, int typing);

void friend_addid(uint8_t *id, char_t *msg, uint16_t msg_length);
void friend_add(char_t *name, uint16_t length, char_t *msg, uint16_t msg_length);

void friend_free(FRIEND *f);
void group_free(GROUPCHAT *g);
