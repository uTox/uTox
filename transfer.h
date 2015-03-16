enum { // TODO replace these
    FT_NONE,

    FT_SEND,
    FT_PENDING,
    FT_PAUSE,
    FT_BROKE,
    FT_KILL,
};

enum UTOX_FILE_TRANSFER_STATUS{
    FILE_TRANS_STATUS_NONE,
    FILE_TRANS_STATUS_ACTIVE,
    FILE_TRANS_STATUS_PAUSED_THEM,
    FILE_TRANS_STATUS_PAUSED_US,
    //FILE_TRANS_STATUS_PAUSED_BOTH,
    FILE_TRANS_STATUS_BROKEN,
    //FILE_TRANS_STATUS_COMPLETED,
    FILE_TRANS_STATUS_KILLED,
    //TODO replace the above and remove these slashes
};

enum {
    FILE_PENDING,
    FILE_OK,
    FILE_PAUSED,
    FILE_PAUSED_OTHER,
    FILE_BROKEN,
    FILE_KILLED,
    FILE_DONE,
};

typedef struct {
    /* used by the tox thread */
    uint8_t status, filenumber, name_length;
    _Bool finish, inline_png;
    uint16_t sendsize, buffer_bytes;
    uint32_t fid;
    void *data, *buffer;
    uint64_t bytes, total;
    uint8_t name[64];
    uint8_t *path;

    uint64_t lastupdate, lastprogress;

    /* used by the main thread */
    void *chatdata;
} FILE_T;

typedef struct
{
    uint64_t bytes;
    uint32_t speed;
} FILE_PROGRESS;

void utox_transfer_start_file(Tox *tox, uint32_t fid, uint8_t *path, uint8_t *name, uint16_t name_length);
void utox_transfer_start_memory(Tox *tox, uint16_t fid, void *pngdata, size_t size);

static void reset_file_transfer(FILE_T *ft, uint64_t start);
// Tox file callbacks
static void callback_file_send_request(Tox *tox, int32_t fid, uint8_t filenumber, uint64_t filesize, const uint8_t *filename, uint16_t filename_length, void *UNUSED(userdata));
static void callback_file_control(Tox *tox, int32_t fid, uint8_t receive_send, uint8_t filenumber, uint8_t control, const uint8_t *data, uint16_t length, void *UNUSED(userdata));
static void callback_file_data(Tox *UNUSED(tox), int32_t fid, uint8_t filenumber, const uint8_t *data, uint16_t length, void *UNUSED(userdata));
/* Function called by core with a new incoming file. */
static void incoming_file_callback_request(Tox *tox, uint32_t friendnumber, uint32_t filenumber, uint32_t kind, uint64_t file_size, const uint8_t *filename, size_t filename_length, void *user_data);
static void incoming_file_callback_control(Tox *tox, uint32_t friend_number, uint32_t file_number, TOX_FILE_CONTROL control, void *UNUSED(userdata));
static void incoming_file_callback_chunk(Tox *tox, uint32_t friend_number, uint32_t file_number, uint64_t position, const uint8_t *data, size_t length, void *user_data);

void outgoing_file_send_new(Tox *tox, uint32_t friend_number, uint8_t *path, const uint8_t *filename, size_t filename_length);
static void outgoing_file_callback_chunk(Tox *tox, uint32_t friend_number, uint32_t file_number, uint64_t position, size_t length, void *user_data);


void utox_set_callbacks_for_transfer(Tox *tox);
// Called from tox_thread main loop to do transfer work.
// Tox API can be called directly.
void utox_thread_work_for_transfers(Tox *tox, uint64_t time);
