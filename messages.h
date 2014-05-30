struct
{
    void **message;
    uint32_t msg;
    double scroll;
}MESSAGES;

void draw_messages(int x, int y, FRIEND *f);
void draw_groupmessages(int x, int y, GROUPCHAT *g);

void messages_mousemove(int x, int y, void **message, uint32_t msg, double scroll, int width);
void messages_mousedown(int x, int y, void **message, uint32_t msg, double scroll, int width);
void messages_mouseup(void);
void messages_mousewheel(int x, int y, double d, double *scroll);
void messages_copy(void **message);
