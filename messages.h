
//Type for indexing into MSG_DATA->data array of messages
typedef uint32_t MSG_IDX;
#define MSG_IDX_MAX (UINT32_MAX)

typedef struct
{
    uint32_t width, height;

    // Tox friendnumber/groupnumber
    uint32_t id;

    // Number of messages in data array.
    MSG_IDX n;

    // Indices of messages in data array, where text selection starts and ends.
    MSG_IDX istart, iend;
    // Indices in strings of corresponding messages, where selection starts/ends.
    STRING_IDX start, end;

    // Pointers at various message structs, at most MAX_BACKLOG_MESSAGES.
    void **data;

    // Field for preserving position of text scroll,
    // while this MSG_DATA is inactive.
    double scroll;
} MSG_DATA;

struct messages {
    PANEL panel;

    // false for Friendchat, true for Groupchat.
    _Bool type;

    // true if we're in the middle of selection operation
    // (mousedown without mouseup yet).
    _Bool select;

    // Position and length of an URL in the message under the mouse,
    // if present. urlover == STRING_IDX_MAX if there's none.
    STRING_IDX urlover, urllen;

    // Was the url pressed by the mouse.
    _Bool urlmdown;

    uint32_t height, width;

    // Indices of messages, that the mouse is over now/has been
    // pressed mousedown over. MSG_IDX_MAX, when the mouse isn't over
    // any message/when not in selection mode.
    MSG_IDX iover, idown;
    // For text messages encodes indices of chars in strings.
    // For non-text messages, encodes various logical parts of them.

    // over in file transfers (iover == 1):
    //   0  == not hovered
    //   1  == hover on the upper transfer button
    //   2  == hover on the lower transfer button
    //   3  == hover on the rest of the transfer box
    uint32_t over, down;

    MSG_DATA *data;
};

enum {
    MSG_TYPE_TEXT,
    MSG_TYPE_ACTION_TEXT,
    MSG_TYPE_IMAGE,
    MSG_TYPE_FILE,
};

typedef struct {
    // true, if we're the author, false, if someone else.
    _Bool author;
    uint8_t msg_type;

    uint32_t height;
    uint32_t time;

    STRING_IDX length;
    char_t msg[0];
} MESSAGE;

typedef struct {
    // true, if we're the author, false, if someone else.
    _Bool author;
    uint8_t msg_type;

    uint32_t height;
    uint32_t time;

    uint16_t w, h;
    _Bool zoom;
    double position;
    UTOX_NATIVE_IMAGE *image;
} MSG_IMG;

typedef struct msg_file {
    // true, if we're the author, false, if someone else.
    _Bool author;
    uint8_t msg_type;

    uint32_t height;
    uint32_t time;

    uint32_t speed;
    uint32_t filenumber;
    uint8_t status, name_length;
    uint64_t size, progress;
    _Bool inline_png;
    uint8_t *path;
    uint8_t name[64];
} MSG_FILE;

struct FILE_TRANSFER;

/* Called externally to add a message to the queue */
MSG_FILE* message_add_type_file(struct FILE_TRANSFER *file);

void messages_draw(MESSAGES *m, int x, int y, int width, int height);
_Bool messages_mmove(MESSAGES *m, int x, int y, int width, int height, int mx, int my, int dx, int dy);
_Bool messages_mdown(MESSAGES *m);
_Bool messages_dclick(MESSAGES *m, _Bool triclick);
_Bool messages_mright(MESSAGES *m);
_Bool messages_mwheel(MESSAGES *m, int height, double d);
_Bool messages_mup(MESSAGES *m);
_Bool messages_mleave(MESSAGES *m);

int messages_selection(MESSAGES *m, void *data, uint32_t len, _Bool names);

_Bool messages_char(uint32_t ch);

void messages_updateheight(MESSAGES *m);
void message_updateheight(MESSAGES *m, MESSAGE *msg, MSG_DATA *p);
void message_add(MESSAGES *m, MESSAGE *msg, MSG_DATA *p);
void message_clear(MESSAGES *m, MSG_DATA *p);
void messages_set_typing(MESSAGES *m, MSG_DATA *p, int typing);

void message_free(MESSAGE *msg);
