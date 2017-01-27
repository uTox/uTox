#ifndef MESSAGES_H
#define MESSAGES_H

#include "ui.h"

#include <time.h>
#include <pthread.h>

pthread_mutex_t messages_lock;

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

typedef struct {
    char    *author;
    uint16_t author_length;

    uint16_t length;
    char     *msg;
} MSG_TEXT;

typedef struct {
    char    *author;
    uint16_t author_length;

    uint16_t length;
    char     *msg;

    uint32_t author_id;
    uint32_t author_color;
} MSG_GROUP;

typedef struct {
    uint32_t      w, h;
    bool          zoom;
    double        position;
    NATIVE_IMAGE *image;
} MSG_IMG;

typedef struct msg_file {
    uint8_t file_status;
    uint32_t file_number;

    char   *name;
    size_t  name_length;

    // Location on disk
    uint8_t *path;
    size_t   path_length;
    // In memory pointer
    uint8_t *data;
    size_t   data_size;

    uint32_t speed;
    uint64_t size, progress;
    bool     inline_png;
} MSG_FILE;

/* Generic Message type */
typedef struct msg_header {
    uint8_t msg_type;

    // true, if we're the author, false, if someone else.
    bool    our_msg;
    bool    from_disk;

    uint32_t height;
    time_t   time;


    uint64_t disk_offset;

    uint32_t receipt;
    time_t   receipt_time;

    union {
        MSG_TEXT txt;
        MSG_TEXT action;
        MSG_TEXT notice;
        MSG_TEXT notice_day;

        MSG_GROUP grp;

        MSG_IMG img;

        MSG_FILE ft;
    } via;
} MSG_HEADER;

// Type for indexing into MSG_DATA->data array of messages
typedef struct messages {
    PANEL panel;

    // false for Friendchat, true for Groupchat.
    bool is_groupchat;
    // Tox friendnumber/groupnumber
    uint32_t id;
    int      height, width;

    // Position and length of an URL in the message under the mouse,
    // if present. cursor_over_uri == UINT16_MAX if there's none.
    uint32_t cursor_over_uri, urllen;

    // Was the url pressed by the mouse.
    uint32_t cursor_down_uri;

    uint32_t cursor_over_msg, cursor_over_position, cursor_down_msg, cursor_down_position;
    uint32_t sel_start_msg, sel_end_msg, sel_start_position, sel_end_position;
    // true if we're in the middle of selection operation
    // (mousedown without mouseup yet).
    bool selecting_text;
    bool cursor_over_time;

    // Number of messages in data array.
    uint32_t number;
    // Number of extra to speedup realloc.
    int8_t extra;

    // Pointers at various message structs, at most MAX_BACKLOG_MESSAGES.
    MSG_HEADER **data;

    // Field for preserving position of text scroll
    double scroll;
} MESSAGES;

uint32_t message_add_group(MESSAGES *m, MSG_HEADER *msg);

uint32_t message_add_type_text(MESSAGES *m, bool auth, const char *msgtxt, uint16_t length, bool log, bool send);
uint32_t message_add_type_action(MESSAGES *m, bool auth, const char *msgtxt, uint16_t length, bool log, bool send);
uint32_t message_add_type_notice(MESSAGES *m, const char *msgtxt, uint16_t length, bool log);
uint32_t message_add_type_image(MESSAGES *m, bool auth, NATIVE_IMAGE *img, uint16_t width, uint16_t height, bool log);

MSG_HEADER *message_add_type_file(MESSAGES *m, uint32_t file_number, bool incoming, bool image, uint8_t status,
                                const uint8_t *name, size_t name_size, size_t target_size, size_t current_size);

bool message_log_to_disk(MESSAGES *m, MSG_HEADER *msg);
bool messages_read_from_log(uint32_t friend_number);

void messages_send_from_queue(MESSAGES *m, uint32_t friend_number);
void messages_clear_receipt(MESSAGES *m, uint32_t receipt_number);

/** Formats all messages from self and friends, and then call draw functions
 * to write them to the UI.
 *
 * accepts: messages struct *pointer, int x,y positions, int width,height
 */
void messages_draw(PANEL *panel, int x, int y, int width, int height);

bool messages_mmove(PANEL *panel, int px, int py, int width, int height, int mx, int my, int dx, int dy);
bool messages_mdown(PANEL *panel);
bool messages_dclick(PANEL *panel, bool triclick);
bool messages_mright(PANEL *panel);
bool messages_mwheel(PANEL *panel, int height, double d, bool smooth);
bool messages_mup(PANEL *panel);
bool messages_mleave(PANEL *m);
bool messages_char(uint32_t ch);
int messages_selection(PANEL *panel, void *buffer, uint32_t len, bool names);

void messages_updateheight(MESSAGES *m, int width);


void messages_init(MESSAGES *m, uint32_t friend_number);
void message_free(MSG_HEADER *msg);
void messages_clear_all(MESSAGES *m);

#endif
