
typedef struct
{
    _Bool online, typing;
    uint8_t status;
    //MESSAGES m;
    uint32_t msg;
    double scroll;
    MSGSEL sel;
    uint16_t name_length, status_length, typed_length;
    uint8_t *name, *status_message, *typed;
    void **message;
}FRIEND;

typedef struct
{
    uint16_t length;
    uint8_t id[TOX_FRIEND_ADDRESS_SIZE], msg[1];
}FRIENDREQ;

uint32_t requests;
FRIENDREQ* request[256], *sreq, **sreqq, *mreq, **mreqq;

FRIENDREQ** newfriendreq(uint8_t *id);
