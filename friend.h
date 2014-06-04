/*todo: */
enum {
    FT_NONE,

    FT_RECV,
    FT_RECV_PENDING,
    FT_RECV_PAUSED,
    FT_RECV_BROKEN,

    FT_SEND,
    FT_SEND_PENDING,
    FT_SEND_PAUSED,
    FT_SEND_BROKEN,

    FT_KILL,
    FT_FINISHED,
};

typedef struct {
    uint8_t status, filenumber;
    _Bool finish;
    uint16_t sendsize, buffer_bytes;
    uint32_t fid;
    void *data, *buffer;
    uint64_t bytes, total;
    uint8_t name[128];
} FILE_T;

typedef struct {
    _Bool online, typing;
    uint8_t calling, status;
    //MESSAGES m;
    int32_t callid;
    uint32_t msg;
    double scroll;
    MSGSEL sel;
    uint8_t cid[TOX_CLIENT_ID_SIZE];
    uint16_t name_length, status_length, typed_length;
    uint8_t *name, *status_message, *typed;
    void **message;

    FILE_T incoming[16];
    FILE_T outgoing[16];
} FRIEND;

void friend_setname(FRIEND *f, uint8_t *name, uint16_t length);

void friend_addid(uint8_t *id, uint8_t *msg, uint16_t msg_length);
void friend_add(uint8_t *name, uint16_t length, uint8_t *msg, uint16_t msg_length);
