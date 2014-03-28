
typedef struct
{
    uint32_t peers, msg;
    double scroll;
    //MESSAGES m;
    MSGSEL sel;
    uint16_t name_length, topic_length, typed_length;
    uint8_t name[128], topic[128]; //static sizes for now
    uint8_t *typed;
    uint8_t *peername[256];
    void **message;
}GROUPCHAT;

