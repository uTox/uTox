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
    _Bool finish;
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

struct friend {
    _Bool online, typing, notify;
    uint8_t calling, status;
    int32_t callid;
    uint16_t call_width, call_height;

    uint8_t cid[TOX_CLIENT_ID_SIZE];
    uint16_t name_length, status_length, typed_length;
    char_t *name, *status_message, *typed;

    MSG_DATA msg;

    EDIT_CHANGE *current, *next, *last;

    FILE_T incoming[16];
    FILE_T outgoing[16];
};

#define friend_id(f) (f -  friend)

void friend_setname(FRIEND *f, uint8_t *name, uint16_t length);
void friend_addmessage(FRIEND *f, void *data);

void friend_notify(FRIEND *f, uint8_t *str, uint16_t str_length, uint8_t *msg, uint16_t msg_length);
#define friend_notifystr(f, str, msg, mlen) friend_notify(f, (uint8_t*)str, sizeof(str) - 1, msg, mlen)
void friend_add_str_message(FRIEND *f, char_t *data, uint16_t length);

void friend_addid(uint8_t *id, char_t *msg, uint16_t msg_length);
void friend_add(char_t *name, uint16_t length, char_t *msg, uint16_t msg_length);
