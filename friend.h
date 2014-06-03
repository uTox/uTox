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
    uint8_t name[256];
} FILE_T;

typedef struct
{
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

    uint32_t nincoming, noutgoing;
    FILE_T *incoming;
    FILE_T *outgoing;
} FRIEND;

void friend_setname(FRIEND *f, uint8_t *name, uint16_t length);
FILE_T* friend_newincoming(FRIEND *f, uint8_t filenumber);
FILE_T* friend_newoutgoing(FRIEND *f, uint8_t filenumber);
