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
    uint8_t name[128];

    uint64_t lastupdate, lastprogress;

    /* used by the main thread */
    void *chatdata;
} FILE_T;

struct friend {
    _Bool online, typing;
    uint8_t calling, status;
    int32_t callid;

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

void friend_addid(uint8_t *id, char_t *msg, uint16_t msg_length);
void friend_add(char_t *name, uint16_t length, char_t *msg, uint16_t msg_length);
