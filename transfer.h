enum {
    FT_NONE,

    FT_SEND,
    FT_PENDING,
    FT_PAUSE,
    FT_BROKE,
    FT_KILL,
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
void utox_transfer_start_memory(Tox *tox, uint16_t fid, void *pngdata);

void utox_set_callbacks_for_transfer(Tox *tox);
void utox_thread_work_for_transfers(Tox *tox, uint64_t time);
