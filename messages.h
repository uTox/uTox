struct messages
{
    PANEL panel;
    _Bool type, select;
    uint16_t urlover, urllen;
    uint32_t height, width;
    uint32_t iover, over, idown, down;
    MSG_DATA *data;
};

typedef struct {
    uint16_t flags;
    uint32_t height;
    uint32_t time;
    uint16_t length;
    char_t msg[0];
} MESSAGE;

typedef struct {
    uint16_t flags;
    uint32_t height;
    uint32_t time;
    uint16_t w, h;
    _Bool zoom;
    double position;
    void *data;
} MSG_IMG;

struct msg_file {
    uint16_t flags;
    uint32_t height;
    uint32_t time, speed;
    uint8_t filenumber, status, name_length;
    uint64_t size, progress;
    _Bool inline_png;
    uint8_t *path;
    uint8_t name[64];
};

void messages_draw(MESSAGES *m, int x, int y, int width, int height);
_Bool messages_mmove(MESSAGES *m, int x, int y, int width, int height, int mx, int my, int dx, int dy);
_Bool messages_mdown(MESSAGES *m);
_Bool messages_dclick(MESSAGES *m, _Bool triclick);
_Bool messages_mright(MESSAGES *m);
_Bool messages_mwheel(MESSAGES *m, int height, double d);
_Bool messages_mup(MESSAGES *m);
_Bool messages_mleave(MESSAGES *m);

int messages_selection(MESSAGES *m, void *data, uint32_t len);

_Bool messages_char(uint32_t ch);

void messages_updateheight(MESSAGES *m);
void message_updateheight(MESSAGES *m, MESSAGE *msg, MSG_DATA *p);
void message_add(MESSAGES *m, MESSAGE *msg, MSG_DATA *p);

void message_free(MESSAGE *msg);
