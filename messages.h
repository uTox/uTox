

struct
{
    void **message;
    uint32_t msg;
    double scroll;
}MESSAGES;

void draw_messages(int x, int y, FRIEND *f);
void draw_groupmessages(int x, int y, GROUPCHAT *g);
