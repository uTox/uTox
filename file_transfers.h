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
    uint8_t  file_id[TOX_FILE_ID_LENGTH];
    uint8_t  status, resume, kind;
    _Bool    incoming, in_memory, is_avatar; //, in_tmp_loc;
    uint8_t  *path, *name; //, *tmp_path;
    size_t   path_length, name_length; //, tmp_path_length;
    uint64_t size, size_transferred;
    uint8_t  *memory, *avatar;

    /* speed + progress calculations. */
    uint32_t speed, num_packets;
    uint64_t last_check_time, last_check_transferred;

    FILE *file, *saveinfo;
    MSG_FILE *ui_data;
} FILE_TRANSFER;

void file_transfer_local_control(Tox *tox, uint32_t friend_number, uint32_t file_number, TOX_FILE_CONTROL control);
void outgoing_file_send_new(Tox *tox, uint32_t friend_number, uint8_t *path, const uint8_t *filename, size_t filename_length);
void outgoing_file_send_existing(Tox *tox, uint32_t friend_number, uint32_t file_number);
void outgoing_file_send_inline(Tox *tox, uint32_t friend_number, uint8_t *image, size_t image_size);
int outgoing_file_send_avatar(Tox *tox, uint32_t friend_number, uint8_t *avatar, size_t avatar_size);
int utox_file_start_write(uint32_t friend_number, uint32_t file_number, void *filepath);
void utox_set_callbacks_for_transfer(Tox *tox);
void utox_cleanup_file_transfers(uint32_t friend_number, uint32_t file_number);
void ft_friend_online(Tox *tox, uint32_t friend_number);
void ft_friend_offline(Tox *tox, uint32_t friend_number);
void utox_file_save_ftinfo(FILE_TRANSFER *file);
void utox_file_load_ftinfo(FILE_TRANSFER *file);
