#define MAX_FILE_TRANSFERS 32

enum UTOX_FILE_TRANSFER_STATUS{
    FILE_TRANSFER_STATUS_NONE,
    FILE_TRANSFER_STATUS_ACTIVE,
    FILE_TRANSFER_STATUS_PAUSED_US,
    FILE_TRANSFER_STATUS_PAUSED_BOTH,
    FILE_TRANSFER_STATUS_PAUSED_THEM,
    FILE_TRANSFER_STATUS_BROKEN,
    FILE_TRANSFER_STATUS_COMPLETED,
    FILE_TRANSFER_STATUS_KILLED,
};

typedef struct FILE_TRANSFER {
    uint32_t friend_number, file_number;
    uint8_t status;
    _Bool in_memory, incoming;
    uint8_t *path, *name, *file_id;
    size_t path_length, name_length, size, size_transferred;
    uint8_t *memory, *avatar;
    FILE *file;
    time_t request_time, start_time, last_chunk_time, finish_time, pause_time;
    MSG_FILE *ui_data;
} FILE_TRANSFER;

typedef struct {
    uint64_t size_transferred;
    uint32_t speed;
} FILE_PROGRESS;

/* Callback for file transfer changes made by friend */
void file_transfer_local_control(Tox *tox, uint32_t friend_number, uint32_t file_number, TOX_FILE_CONTROL control);
static void file_transfer_callback_control(Tox *tox, uint32_t friend_number, uint32_t file_number, TOX_FILE_CONTROL control, void *UNUSED(userdata));

/* Incoming files */
    /* Function called by core with a new incoming file. */
    static void incoming_file_callback_request(Tox *tox, uint32_t friendnumber, uint32_t filenumber, uint32_t kind, uint64_t file_size, const uint8_t *filename, size_t filename_length, void *user_data);
    static void incoming_file_avatar(Tox *tox, uint32_t friendnumber, uint32_t filenumber, uint32_t kind, uint64_t file_size, const uint8_t *filename, size_t filename_length, void *user_data);
    static void incoming_file_callback_chunk(Tox *tox, uint32_t friend_number, uint32_t file_number, uint64_t position, const uint8_t *data, size_t length, void *user_data);
/* Outgoing files */
    /* Function called by core for outgoing files. */
    void outgoing_file_send_new(Tox *tox, uint32_t friend_number, uint8_t *path, const uint8_t *filename, size_t filename_length);
    static void outgoing_file_callback_chunk(Tox *tox, uint32_t friend_number, uint32_t file_number, uint64_t position, size_t length, void *user_data);

/* Helper functions */
void utox_file_start_write(uint32_t friend_number, uint32_t file_number, void *filepath);
void utox_set_callbacks_for_transfer(Tox *tox);

static void utox_update_user_file(FILE_TRANSFER *file);
static void utox_run_file(FILE_TRANSFER *file);
static void utox_kill_file(FILE_TRANSFER *file);
static void utox_pause_file(FILE_TRANSFER *file, uint8_t us);

// Empty
static void utox_break_file();
static void utox_complete_file();
static void utox_resume_broke_file();

