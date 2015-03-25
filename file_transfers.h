#define MAX_FILE_TRANSFERS 32

#define FILE_TRANSFER_TEMP_PATH "transfers"

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
    uint8_t status, resume;
    uint32_t kind;
    _Bool incoming, in_memory, is_avatar, in_tmp_loc;
    uint8_t *path, *name, *tmp_path, *file_id;
    size_t path_length, name_length, tmp_path_length;
    uint64_t size, size_transferred;
    uint8_t *memory, *avatar;

    /* speed + progress calculations. */
    uint32_t speed, num_packets;
    uint64_t last_check_time, last_check_transferred;

    FILE *file, *tmp_file;
    MSG_FILE *ui_data;
} FILE_TRANSFER;

typedef struct {
    _Bool used, outgoing;
    uint32_t friend_number, file_number;
    uint8_t *file_id;

    FILE_TRANSFER *data;
} BROKEN_TRANSFER;

/** local callback for file transfers
 *
 * Called with a friend & file number, and will update it with control.
 * It will also call the other needed functions for you because it's nice like
 * that.
 */
void file_transfer_local_control(Tox *tox, uint32_t friend_number, uint32_t file_number, TOX_FILE_CONTROL control);
/** Callback for file transfer changes made by friend.
 *
 * Called via toxcore every time a friend changes file status.
 */
//static void file_transfer_callback_control(Tox *tox, uint32_t friend_number, uint32_t file_number, TOX_FILE_CONTROL control, void *UNUSED(userdata));

/* Incoming files */
  /* Called internally to handle avatar data */
  //static void incoming_file_avatar(Tox *tox, uint32_t friendnumber, uint32_t filenumber, uint32_t kind, uint64_t file_size, const uint8_t *filename, size_t filename_length, void *user_data);
  /* Function called by core with a new incoming file. */
  //static void incoming_file_callback_request(Tox *tox, uint32_t friendnumber, uint32_t filenumber, uint32_t kind, uint64_t file_size, const uint8_t *filename, size_t filename_length, void *user_data);
  //static void incoming_file_callback_chunk(Tox *tox, uint32_t friend_number, uint32_t file_number, uint64_t position, const uint8_t *data, size_t length, void *user_data);
/* Outgoing files */
  /* Send out a new file.
   *
   * TODO, support resuming file. */
  void outgoing_file_send_new(Tox *tox, uint32_t friend_number, uint8_t *path, const uint8_t *filename, size_t filename_length);
  /* Restarts a broken file. */
  void outgoing_file_send_existing(Tox *tox, FILE_TRANSFER *broken_data, uint8_t broken_number);
  /* Send an inline file/image. */
  void outgoing_file_send_inline(Tox *tox, uint32_t friend_number, uint8_t *image, size_t image_size);
  /* Send a newly changed avatar, called by avatar functions. */
  int outgoing_file_send_avatar(Tox *tox, uint32_t friend_number, uint8_t *avatar, size_t avatar_size);
  /* Function called by core to send of next chuck of *length size. */
  //static void outgoing_file_callback_chunk(Tox *tox, uint32_t friend_number, uint32_t file_number, uint64_t position, size_t length, void *user_data);

/** Helper functions
 *
 * utox_file_start_write() opens and sets the file handle on the disk (return -1 on failure, 0 on success)
 * utox_set_callbacks_for_transfers() interfaces with toxcore setting the
 *   callbacks for incoming or outgoing transfers
 */
int utox_file_start_write(uint32_t friend_number, uint32_t file_number, void *filepath);
int utox_file_start_temp_write(uint32_t friend_number, uint32_t file_number);
void utox_set_callbacks_for_transfer(Tox *tox);

/* Functions called when friend goes online or offline.
 */
void ft_friend_online(Tox *tox, uint32_t friend_number);
void ft_friend_offline(Tox *tox, uint32_t friend_number);
