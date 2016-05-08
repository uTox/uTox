//Type for indexing into MSG_DATA->data array of messages
struct messages {
    PANEL panel;

    // false for Friendchat, true for Groupchat.
    _Bool is_groupchat;
    // Tox friendnumber/groupnumber
    uint32_t id;
    int height, width;

    // Position and length of an URL in the message under the mouse,
    // if present. cursor_over_uri == UINT16_MAX if there's none.
    uint32_t cursor_over_uri, urllen;

    // Was the url pressed by the mouse.
    _Bool cursor_down_uri;

    uint32_t cursor_over_msg, cursor_over_position, cursor_down_msg, cursor_down_position;
    uint32_t sel_start_msg, sel_end_msg, sel_start_position, sel_end_position;
    // true if we're in the middle of selection operation
    // (mousedown without mouseup yet).
    _Bool selecting_text;

    // Number of messages in data array.
    uint32_t  number;
    // Number of extra to speedup realloc.
    int8_t extra;

    // Pointers at various message structs, at most MAX_BACKLOG_MESSAGES.
    void **data;

    // Field for preserving position of text scroll
    double scroll;
};

typedef enum UTOX_MSG_TYPE {
    MSG_TYPE_NULL,
    /* MSG_TEXT must start here */
    MSG_TYPE_TEXT,
    MSG_TYPE_ACTION_TEXT,
    MSG_TYPE_NOTICE,
    MSG_TYPE_NOTICE_DAY_CHANGE, // Seperated so I can localize this later!
    /* MSG_TEXT should end here */
    MSG_TYPE_OTHER, // Unused, expect to seperate MSG_TEXT type
    MSG_TYPE_IMAGE,
    MSG_TYPE_IMAGE_HISTORY,
    MSG_TYPE_FILE,
    MSG_TYPE_FILE_HISTORY,
    MSG_TYPE_CALL_ACTIVE,
    MSG_TYPE_CALL_HISTORY,
} UTOX_MSG_TYPE;

/* Generic Message type */
typedef struct {
    // true, if we're the author, false, if someone else.
    _Bool author;
    _Bool from_disk;
    uint8_t msg_type;

    uint32_t height;
    time_t time;

    uint32_t author_id;
    uint32_t author_length;

    uint64_t disk_offset;
} MSG_VOID;

typedef struct {
    _Bool author;
    _Bool from_disk;
    uint8_t msg_type;

    uint32_t height;
    time_t time;

    uint32_t author_id;
    uint32_t author_length;

    uint64_t disk_offset;

    uint32_t receipt;
    time_t receipt_time;

    uint16_t length;
    char_t msg[0];
} MSG_TEXT;


typedef struct {
    _Bool author;
    _Bool from_disk;
    uint8_t msg_type;

    uint32_t height;
    time_t time;

    uint32_t author_id;
    uint16_t author_length;

    uint64_t disk_offset;

    uint32_t receipt;
    time_t receipt_time;

    uint32_t author_color;

    uint16_t length;
    char_t msg[0];
} MSG_GROUP;

typedef struct {
    _Bool author;
    _Bool from_disk;
    uint8_t msg_type;

    uint32_t height;
    time_t time;

    uint32_t author_id;
    uint32_t author_length;

    uint64_t disk_offset;

    uint16_t w, h;
    _Bool zoom;
    double position;
    UTOX_NATIVE_IMAGE *image;
} MSG_IMG;

typedef struct msg_file {
    _Bool author;
    _Bool from_disk;
    uint8_t msg_type;

    uint32_t height;
    time_t time;

    uint32_t author_id;
    uint32_t author_length;

    uint64_t disk_offset;

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
MSG_FILE* message_create_type_file(struct FILE_TRANSFER *file);

uint32_t message_add_type_file_compat(MESSAGES *m, MSG_FILE *f);

_Bool message_log_to_disk(MESSAGES *m, MSG_VOID *msg);

uint32_t message_add_group(MESSAGES *m, MSG_TEXT *msg);

uint32_t message_add_type_text(MESSAGES *m, _Bool auth, const uint8_t *data, uint16_t length, _Bool log);
uint32_t message_add_type_action(MESSAGES *m, _Bool auth, const uint8_t *data, uint16_t length, _Bool log);
uint32_t message_add_type_notice(MESSAGES *m, const uint8_t *data, uint16_t length, _Bool log);
uint32_t message_add_type_image(MESSAGES *m, _Bool auth, UTOX_NATIVE_IMAGE *img,
                                uint16_t width, uint16_t height, _Bool log);

void messages_draw(PANEL *panel, int x, int y, int width, int height);
_Bool messages_mmove(PANEL *panel, int x, int y, int width, int height, int mx, int my, int dx, int dy);
_Bool messages_mdown(PANEL *panel);
_Bool messages_dclick(PANEL *panel, _Bool triclick);
_Bool messages_mright(PANEL *panel);
_Bool messages_mwheel(PANEL *panel, int height, double d, _Bool smooth);
_Bool messages_mup(PANEL *panel);
_Bool messages_mleave(PANEL *panel);

int messages_selection(PANEL *panel, void *data, uint32_t len, _Bool names);

_Bool messages_char(uint32_t ch);

void messages_updateheight(MESSAGES *m, int width);

void messages_init(MESSAGES *m, uint32_t friend_number);
void message_free(MSG_TEXT *msg);
void messages_clear_all(MESSAGES *m);
