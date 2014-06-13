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
    uint16_t flags, height, length;
    char_t msg[0];
} MESSAGE;

typedef struct {
    uint16_t flags, height;
    //HBITMAP bitmap;
} MSG_IMG;

struct msg_file {
    uint16_t flags, height;
    uint8_t filenumber, status, name_length;
    uint64_t size, progress;
    uint8_t name[24];
};

void messages_draw(MESSAGES *m, int x, int y, int width, int height);
_Bool messages_mmove(MESSAGES *m, int mx, int my, int dy, int width, int height);
_Bool messages_mdown(MESSAGES *m);
_Bool messages_mright(MESSAGES *m);
_Bool messages_mwheel(MESSAGES *m, int height, double d);
_Bool messages_mup(MESSAGES *m);
_Bool messages_mleave(MESSAGES *m);

int messages_selection(MESSAGES *m, void *data, uint32_t len);

void messages_updateheight(MESSAGES *m) ;
void message_fileupdateheight(MESSAGES *m, MSG_FILE *file, MSG_DATA *p);
void message_add(MESSAGES *m, MESSAGE *msg, MSG_DATA *p);
