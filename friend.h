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
}FRIEND;

void friend_setname(FRIEND *f, uint8_t *name, uint16_t length);

