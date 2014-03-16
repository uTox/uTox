
typedef struct
{
    uint32_t peers, msg;
    double scroll;
    uint16_t name_length, topic_length;
    uint8_t name[128], topic[128]; //static sizes for now
    uint8_t *peername[256];
    void *message[2048];
}GROUPCHAT;

