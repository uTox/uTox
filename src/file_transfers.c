#include "file_transfers.h"

#include "friend.h"
#include "main.h"
#include "util.h"

#define MAX_INLINE_FILESIZE (1024 * 1024 * 4)

static FILE_TRANSFER *get_file_transfer(uint32_t friend_number, uint32_t file_number) {
    FRIEND *f = get_friend(friend_number);
    if (!f) {
        return NULL;
    }

    if (file_number >= (1 << 16)) {
        // it's an incoming file ( this is some toxcore magic we know about )
        file_number = (file_number >> 16) - 1;
        if (f->file_transfers_incoming_size <= file_number) {
            debug("FileTransfer:\tRealloc incoming %u|%u\n", friend_number, file_number + 1);
            FILE_TRANSFER *new_ftlist = realloc(f->file_transfers_incoming, sizeof(FILE_TRANSFER) * (file_number + 1));

            if (new_ftlist) {
                f->file_transfers_incoming = new_ftlist;
                f->file_transfers_incoming_size = file_number + 1;
                return &(f->file_transfers_incoming[file_number]);
            }
        } else {
            return &(f->file_transfers_incoming[file_number]);
        }
    } else {
        if (f->file_transfers_outgoing_size <= file_number) {
            debug("FileTransfer:\tRealloc outgoing %u|%u\n", friend_number, file_number + 1);
            FILE_TRANSFER *new_ftlist = realloc(f->file_transfers_outgoing, sizeof(FILE_TRANSFER) * (file_number + 1));

            if (new_ftlist) {
                f->file_transfers_outgoing = new_ftlist;
                f->file_transfers_outgoing_size = file_number + 1;
                return &(f->file_transfers_outgoing[file_number]);
            }
        } else {
            return &(f->file_transfers_outgoing[file_number]);
        }
    }
    return NULL;
}

/* Calculate the transfer speed for the UI. */
static void calculate_speed(FILE_TRANSFER *file) {
    if ((file->speed) > file->num_packets * 20 * 1371) {
        ++file->num_packets;
        return;
    }

    file->num_packets = 0;

    uint64_t time = get_time();
    if (!file->last_check_time) {
        file->last_check_time = time;
        return;
    }

    // TODO repalce magic number with something real. (grayhatter> I things it's cpu clock ticks)
    if (time - file->last_check_time >= 1000 * 1000 * 100) {
        file->speed = (((double)(file->current_size - file->last_check_transferred) * 1000.0 * 1000.0 * 1000.0)
                       / (double)(time - file->last_check_time))
                      + 0.5;
        file->last_check_time        = time;
        file->last_check_transferred = file->current_size;
    }

    postmessage(FILE_UPDATE_STATUS, 0, 0, file);
}

static void ft_decon(uint32_t friend_number, uint32_t file_number) {
    debug_info("FileTransfer:\tCleaning up file transfers! (%u & %u)\n", friend_number, file_number);
    FILE_TRANSFER *ft = get_file_transfer(friend_number, file_number);
    if (ft && ft->in_use) {
        if (ft->name) {
            free(ft->name);
        }

        if (ft->in_memory) {
            // do nothing
        } else if (ft->avatar) {
            // still do nothing
        } else if (ft->via.file) {
            fclose(ft->via.file);
        }
        memset(ft, 0, sizeof(FILE_TRANSFER));
    }
}

static bool resumeable_name(FILE_TRANSFER *ft, char *name) {
    if (ft->incoming) {
        uint8_t blank_id[TOX_HASH_LENGTH] = { 0 };
        if (memcmp(ft->data_hash, blank_id, TOX_HASH_LENGTH) == 0) {
            debug_error("FileTransfer:\tUnable to use current data_hash...uTox can't resume file %.*s\n",
                        (uint32_t)ft->name_length, ft->name);
            return false;
        }
        char hex[TOX_HASH_LENGTH * 2];
        fid_to_string(hex, ft->data_hash);
        snprintf(name, UTOX_FILE_NAME_LENGTH, "%.*s.ftinfo", TOX_HASH_LENGTH * 2, hex);
    } else {
        snprintf(name, UTOX_FILE_NAME_LENGTH, "%.*s%02i.ftoutfo",
                    TOX_PUBLIC_KEY_SIZE * 2, friend[ft->friend_number].id_str,
                    ft->file_number % 100);
    }

    return true;
}

static bool ft_update_resumable(FILE_TRANSFER *ft) {
    debug_error("FileTransfer:\tUnable to save file info... uTox can't resume file %.*s\n",
                (int)ft->name_length, ft->name);

    if (ft->resume_data) {
        if (fwrite(ft, 1, sizeof(FILE_TRANSFER), ft->resume_data)) {
            return true;
        }
    }

    return false;
}

/* Create the file transfer resume info file. */
static bool ft_init_resumable(FILE_TRANSFER *ft) {
    char name[UTOX_FILE_NAME_LENGTH];
    if (!resumeable_name(ft, name)) {
        return false;
    }

    ft->resume_data = native_get_file(name, NULL, UTOX_FILE_OPTS_WRITE | UTOX_FILE_OPTS_MKDIR);
    if (ft->resume_data) {
        debug("FileTransfer:\t.ftinfo for file %.*s set; ready to resume!\n", (uint32_t)ft->name_length, ft->name);
        return ft_update_resumable(ft);
    }

    return false;
}

/* Free/Remove/Unlink the file transfer resume info file. */
static void ft_decon_resumable(FILE_TRANSFER *ft) {
    char name[UTOX_FILE_NAME_LENGTH];
    if (!resumeable_name(ft, name)) {
        return;
    }

    debug_info("FileTransfer:\tGoing to decon file %s.\n", name);
    size_t size = 0;
    FILE *file = native_get_file(name, &size, UTOX_FILE_OPTS_READ | UTOX_FILE_OPTS_WRITE);
    if (!file) {
        return;
    }

    // Write \0 and flush before delete
    fwrite(0, 1, size, file);
    fflush(file);
    fclose(file);
    file = native_get_file(name, &size, UTOX_FILE_OPTS_DELETE);
}

static bool ft_restart_outgoing(FILE_TRANSFER *ft) {
    return false;
}

static bool ft_find_resumeable(FILE_TRANSFER *ft) {
    char resume_name[UTOX_FILE_NAME_LENGTH];
    if (!resumeable_name(ft, resume_name)) {
        return false;
    }

    size_t size = 0;
    FILE *resume_file = native_get_file(resume_name, &size, UTOX_FILE_OPTS_READ);

    if (!resume_file) {
        if (ft->incoming) {
            debug("FileTransfer:\tUnable to load saved info... uTox can't resume file %.*s\n",
                  (uint32_t)ft->name_length, ft->name);
        }
        ft->status = 0;
        return false;
    }

    FILE_TRANSFER resume_data;
    if (size == sizeof(FILE_TRANSFER)) {
        fread(&resume_data, 1, size, resume_file);

        if (!resume_data.in_use
            || resume_data.in_memory
            || resume_data.avatar
            || resume_data.inline_img
            || !resume_data.resumeable ) {
            return false;
        }
    }

    memcpy(ft, &resume_data, sizeof(FILE_TRANSFER));

    ft->name_length = 0;
    uint8_t *p = ft->path + strlen((char *)ft->path);
    while (*--p != '/') {
        ++ft->name_length;
    }
    ++p;
    ++ft->name_length;

    ft->name = calloc(1, ft->name_length + 1);
    snprintf((char *)ft->name, ft->name_length + 1, "%s", p);

    ft->via.file = NULL;
    ft->resume_data = NULL;
    ft->ui_data = NULL;

    return true;
}

/* Cancel active file. */
static void kill_file(FILE_TRANSFER *file) {
    if (file->status == FILE_TRANSFER_STATUS_KILLED) {
        debug_notice("FileTransfer:\tFile already killed.\n");
        return;
    } else if (file->status == FILE_TRANSFER_STATUS_COMPLETED) {
        debug_notice("FileTransfer:\tFile already completed.\n");
        return;
    } else {
        file->status = FILE_TRANSFER_STATUS_KILLED;

        if (((MSG_FILE *)file->ui_data)) {
            ((MSG_FILE *)file->ui_data)->file_status = FILE_TRANSFER_STATUS_KILLED;
        }
    }

    postmessage(FILE_UPDATE_STATUS, 0, 0, file);

    if (!file->incoming && friend[file->friend_number].file_transfers_incoming_size) {
        // get_friend(file->friend_number)->file_transfers_incoming_active_count--;
    } else if (friend[file->friend_number].file_transfers_outgoing_size) {
        // get_friend(file->friend_number)->file_transfers_outgoing_active_count--;
    }

    if (file->resumeable) {
        ft_decon_resumable(file);
    }
    ft_decon(file->friend_number, file->file_number);
}

/* Break active file, (when a friend goes offline). */
static void break_file(FILE_TRANSFER *file) {
    if (file->status == FILE_TRANSFER_STATUS_NONE) {
        return kill_file(file); /* We don't save unstarted files */
    } else if (file->status == FILE_TRANSFER_STATUS_COMPLETED
        || file->status == FILE_TRANSFER_STATUS_KILLED) {
            /* We don't touch these files! */
            return;
    }

    file->status = FILE_TRANSFER_STATUS_BROKEN;

    if (((MSG_FILE *)file->ui_data)) {
        ((MSG_FILE *)file->ui_data)->file_status = FILE_TRANSFER_STATUS_BROKEN;
    }

    postmessage(FILE_UPDATE_STATUS, 0, 0, file);
    ft_update_resumable(file);
    if (file->in_use) {
        ft_decon(file->friend_number, file->file_number);
    }
}

/* Pause active file. */
static void utox_pause_file(FILE_TRANSFER *file, uint8_t us) {
    switch (file->status) {
        case FILE_TRANSFER_STATUS_NONE: {
            if (!file->incoming) {
                // New transfers start as paused them
                file->status = FILE_TRANSFER_STATUS_PAUSED_THEM;
            } else {
                debug("FileTransfer:\tWe can't pause an unaccepted file!\n");
            }
            break;
        }
        case FILE_TRANSFER_STATUS_ACTIVE: {
            if (us) {
                debug("FileTransfer:\tFile now paused by us.\n");
                file->status = FILE_TRANSFER_STATUS_PAUSED_US;
            } else {
                debug("FileTransfer:\tFile now paused by them.\n");
                file->status = FILE_TRANSFER_STATUS_PAUSED_THEM;
            }
            break;
        }
        case FILE_TRANSFER_STATUS_PAUSED_US:
        case FILE_TRANSFER_STATUS_PAUSED_BOTH:
        case FILE_TRANSFER_STATUS_PAUSED_THEM: {
            if (us) {
                if (file->status == FILE_TRANSFER_STATUS_PAUSED_US) {
                    debug("FileTransfer:\tFile already paused by us!\n");
                } else if (file->status == FILE_TRANSFER_STATUS_PAUSED_THEM) {
                    file->status = FILE_TRANSFER_STATUS_PAUSED_BOTH;
                    debug("FileTransfer:\tFile now paused by both!\n");
                } else if (file->status == FILE_TRANSFER_STATUS_PAUSED_BOTH) {
                    debug("FileTransfer:\tFile already paused by both!\n");
                } else {
                    file->status = FILE_TRANSFER_STATUS_PAUSED_US;
                }
            } else {
                if (file->status == FILE_TRANSFER_STATUS_PAUSED_US) {
                    file->status = FILE_TRANSFER_STATUS_PAUSED_BOTH;
                    debug("FileTransfer:\tFile now paused by both!\n");
                } else if (file->status == FILE_TRANSFER_STATUS_PAUSED_THEM) {
                    debug("FileTransfer:\tFile was already paused by them!\n");
                } else if (file->status == FILE_TRANSFER_STATUS_PAUSED_BOTH) {
                    debug("FileTransfer:\tFile already paused by both!\n");
                } else {
                    file->status = FILE_TRANSFER_STATUS_PAUSED_THEM;
                }
            }
            break;
        }
        case FILE_TRANSFER_STATUS_BROKEN: {
            debug("FileTransfer:\tCan't pause a broken file;\n");
            break;
        }
        case FILE_TRANSFER_STATUS_COMPLETED: {
            debug("FileTransfer:\tCan't pause a completed file;\n");
            break;
        }
        case FILE_TRANSFER_STATUS_KILLED: {
            debug("FileTransfer:\tCan't pause a killed file;\n");
            break;
        }
    }
    postmessage(FILE_UPDATE_STATUS, 0, 0, file);
    // TODO free not freed data.
}

/* Start/Resume active file. */
static void run_file_local(FILE_TRANSFER *file) {
    if (file->status == FILE_TRANSFER_STATUS_ACTIVE) {
        return;
    }
    if (file->status == FILE_TRANSFER_STATUS_NONE) {
        file->status = FILE_TRANSFER_STATUS_ACTIVE;

        if (!file->resumeable && file->incoming) {
            /* Set resuming info TODO MOVE TO AFTER FILE IS ACCEPED BY USER */
            file->resumeable = ft_init_resumable(file);
        }
    } else if (file->status == FILE_TRANSFER_STATUS_PAUSED_US) {
        file->status = FILE_TRANSFER_STATUS_ACTIVE;
    } else if (file->status == FILE_TRANSFER_STATUS_PAUSED_BOTH) {
        file->status = FILE_TRANSFER_STATUS_PAUSED_THEM;
    } else if (file->status == FILE_TRANSFER_STATUS_PAUSED_THEM) {
        // Do nothing;
    } else if (file->status == FILE_TRANSFER_STATUS_BROKEN) {
        file->status = FILE_TRANSFER_STATUS_ACTIVE;
    } else {
        debug_error("FileTransfer:\tWe tried to run file from an unknown state! (%u)\n", file->status);
    }

    postmessage(FILE_UPDATE_STATUS, 0, 0, file);
}

static void run_file_remote(FILE_TRANSFER *file) {
    if (file->status == FILE_TRANSFER_STATUS_PAUSED_US) {
        // Do nothing;
    } else if (file->status == FILE_TRANSFER_STATUS_PAUSED_BOTH) {
        file->status = FILE_TRANSFER_STATUS_PAUSED_US;
    } else if (file->status == FILE_TRANSFER_STATUS_PAUSED_THEM) {
        file->status = FILE_TRANSFER_STATUS_ACTIVE;
    } else if (file->status == FILE_TRANSFER_STATUS_BROKEN) {
        file->status = FILE_TRANSFER_STATUS_ACTIVE;
    } else {
        debug_error("FileTransfer:\tThey tried to run file from an unknown state! (%u)\n", file->status);
    }
    postmessage(FILE_UPDATE_STATUS, 0, 0, file);
}

static void decode_inline_png(uint32_t friend_id, uint8_t *data, uint64_t size) {
    // TODO: start a new thread and decode the png in it.
    uint16_t      width, height;
    NATIVE_IMAGE *native_image = utox_image_to_native((UTOX_IMAGE)data, size, &width, &height, 0);
    if (NATIVE_IMAGE_IS_VALID(native_image)) {
        void *msg = malloc(sizeof(uint16_t) * 2 + sizeof(uint8_t *));
        memcpy(msg, &width, sizeof(uint16_t));
        memcpy(msg + sizeof(uint16_t), &height, sizeof(uint16_t));
        memcpy(msg + sizeof(uint16_t) * 2, &native_image, sizeof(uint8_t *));
        postmessage(FILE_INLINE_IMAGE, friend_id, 0, msg);
    }
}

/* Complete active file, (when the whole file transfer is successful). */
static void utox_complete_file(FILE_TRANSFER *ft) {
    if (ft->status == FILE_TRANSFER_STATUS_ACTIVE) {
        if (ft->incoming) {
            if (ft->inline_img) {
                decode_inline_png(ft->friend_number, ft->via.memory, ft->current_size);
            } else if (ft->avatar) {
                postmessage(FRIEND_AVATAR_SET, ft->friend_number, ft->current_size, ft->via.avatar);
            } else { // Is a file
                ((MSG_FILE *)ft->ui_data)->path = strdup((const char *)ft->path);
            }
        } else {
            if (ft->in_memory) {
                // TODO, might want to do something here.
            } else { // Is a file
                ((MSG_FILE *)ft->ui_data)->path = strdup((const char *)ft->path);
            }
        }
        ft->status = FILE_TRANSFER_STATUS_COMPLETED;
        if (((MSG_FILE *)ft->ui_data)) {
            ((MSG_FILE *)ft->ui_data)->file_status = FILE_TRANSFER_STATUS_COMPLETED;
        }
        postmessage(FILE_UPDATE_STATUS, 0, 0, ft);
    } else {
        debug_error("FileTransfer:\tUnable to complete file in non-active state (file:%u)\n", ft->file_number);
    }
    debug_notice("FileTransfer:\tIncoming transfer is done (%u & %u)\n", ft->friend_number, ft->file_number);

    if (ft->resumeable) {
        ft_decon_resumable(ft);
    }

    ft_decon(ft->friend_number, ft->file_number);
}

/* Friend has come online, restart our outgoing transfers to this friend. */
void ft_friend_online(Tox *tox, uint32_t friend_number) {
    (void)tox; // FIXME temp warning hiding
    for (uint16_t i = 0; i < MAX_FILE_TRANSFERS; i++) {
        FILE_TRANSFER *file = calloc(1, sizeof(*file));
        file->friend_number = friend_number;
        file->file_number   = i;
        file->incoming      = false;
        ft_find_resumeable(file);
        if (file->path[0]) {
            /* If we got a path from utox_file_load we should try to resume! */
            // uint32_t f_n = ft_send_data(tox, friend_number, NULL, (uint8_t *)file, sizeof(*file));
        }

        free(file);
    }
}

/* Friend has gone offline, break our outgoing transfers to this friend. */
void ft_friend_offline(Tox *tox, uint32_t friend_number) {
    (void)tox; // FIXME temp warning hiding
    debug_notice("FileTransfer:\tFriend %u has gone offline, breaking transfers\n", friend_number);

    FRIEND *f = get_friend(friend_number);
    if (!f) {
        return;
    }

    for (uint16_t i = 0; i < f->file_transfers_outgoing_size ; ++i) {
        break_file(&f->file_transfers_outgoing[i]);
    }
    for (uint16_t i = 0; i < f->file_transfers_incoming_size ; ++i) {
        break_file(&f->file_transfers_incoming[i]);
    }
}

/* Local command callback to change a file status. */
void file_transfer_local_control(Tox *tox, uint32_t friend_number, uint32_t file_number, TOX_FILE_CONTROL control) {
    TOX_ERR_FILE_CONTROL error = 0;
    FILE_TRANSFER *info  = get_file_transfer(friend_number, file_number);

    switch (control) {
        case TOX_FILE_CONTROL_RESUME: {
            if (info->status != FILE_TRANSFER_STATUS_ACTIVE) {
                if (get_friend(friend_number)->file_transfers_outgoing_size < MAX_FILE_TRANSFERS) {
                    if (tox_file_control(tox, friend_number, file_number, control, &error)) {
                        debug("FileTransfer:\tWe just resumed file (%u & %u)\n", friend_number, file_number);
                    } else {
                        debug("FileTransfer:\tToxcore doesn't like us! (%u & %u)\n", friend_number, file_number);
                    }
                } else {
                    debug("FileTransfer:\tCan't start file, max file transfer limit reached! (%u & %u)\n",
                          friend_number, file_number);
                }
            } else {
                debug("FileTransfer:\tFile already active (%u & %u)\n", friend_number, file_number);
            }
            run_file_local(info);
            break;
        }
        case TOX_FILE_CONTROL_PAUSE:
            if (info->status != FILE_TRANSFER_STATUS_PAUSED_US || info->status != FILE_TRANSFER_STATUS_PAUSED_BOTH) {
                if (tox_file_control(tox, friend_number, file_number, control, &error)) {
                    debug("FileTransfer:\tWe just paused file (%u & %u)\n", friend_number, file_number);
                } else {
                    debug("FileTransfer:\tToxcore doesn't like us! (%u & %u)\n", friend_number, file_number);
                }
            } else {
                debug("FileTransfer:\tFile already paused (%u & %u)\n", friend_number, file_number);
            }
            utox_pause_file(info, 1);
            break;
        case TOX_FILE_CONTROL_CANCEL: {
            if (info->status != FILE_TRANSFER_STATUS_KILLED) {
                if (tox_file_control(tox, friend_number, file_number, control, &error)) {
                    debug("FileTransfer:\tWe just killed file (%u & %u)\n", friend_number, file_number);
                } else {
                    debug("FileTransfer:\tToxcore doesn't like us! (%u & %u)\n", friend_number, file_number);
                }
            } else {
                debug("FileTransfer:\tFile already killed (%u & %u)\n", friend_number, file_number);
            }
            if (info->friend_number == friend_number) {
                kill_file(info);
            }
            break;
        }
    }
    /* Do something with the error! */
    if (error) {
        if (error == TOX_ERR_FILE_CONTROL_FRIEND_NOT_CONNECTED) {
            debug("FileTransfer:\tUnable to send command, Friend (%u) offline!\n", info->friend_number);
        } else {
            debug("FileTransfer:\tThere was an error(%u) sending the command, you probably want to see to that!\n",
                  error);
        }
    }
}

/* Remote command callback for friends to change a file status */
static void file_transfer_callback_control(Tox *UNUSED(tox), uint32_t friend_number, uint32_t file_number,
                                           TOX_FILE_CONTROL control, void *UNUSED(userdata))
{
    FILE_TRANSFER *ft = get_file_transfer(friend_number, file_number);
    if (!ft || !ft->in_use) {
        return;
    }

    switch (control) {
        case TOX_FILE_CONTROL_RESUME: {
            debug("FileTransfer:\tFriend (%i) has resumed file (%i)\n", friend_number, file_number);
            run_file_remote(ft);
            break;
        }
        case TOX_FILE_CONTROL_PAUSE: {
            debug("FileTransfer:\tFriend (%i) has paused file (%i)\n", friend_number, file_number);
            utox_pause_file(ft, 0);
            break;
        }
        case TOX_FILE_CONTROL_CANCEL: {
            if (ft->avatar) {
                debug("FileTransfer:\tFriend (%i) rejected avatar\n", friend_number);
            } else {
                debug("FileTransfer:\tFriend (%i) has canceled file (%i)\n", friend_number, file_number);
            }
            kill_file(ft);
            break;
        }
    }
}

static void incoming_avatar(Tox *tox, uint32_t friend_number, uint32_t file_number, uint64_t size)
{
    debug("FileTransfer:\tIncoming avatar from friend %u.\n", friend_number);

    if (size == 0) {
        debug("FileTransfer:\tAvatar from friend %u deleted.\n", friend_number);
        postmessage(FRIEND_AVATAR_UNSET, friend_number, 0, NULL);
        file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL);
        return;
    }

    if (size > UTOX_AVATAR_MAX_DATA_LENGTH) {
        debug("FileTransfer:\tAvatar from friend(%u) rejected. (Too Large %lu)\n", friend_number, size);
        file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL);
        return;
    }

    uint8_t file_id[TOX_FILE_ID_LENGTH] = { 0 };
    tox_file_get_file_id(tox, friend_number, file_number, file_id, 0);

    /* Verify this is a new avatar */
    if ((friend[friend_number].avatar.format)
        && memcmp(friend[friend_number].avatar.hash, file_id, TOX_HASH_LENGTH) == 0) {
        debug("FileTransfer:\tAvatar from friend (%u) rejected: Same as Current\n", friend_number);
        file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL);
        return;
    }

    FILE_TRANSFER *ft = get_file_transfer(friend_number, file_number);
    memset(ft, 0, sizeof(FILE_TRANSFER));
    ft->in_use = true;

    ft->friend_number = friend_number;
    ft->file_number   = file_number;
    ft->target_size   = size;

    tox_file_get_file_id(tox, friend_number, file_number, ft->data_hash, NULL);

    ft->incoming  = true;
    ft->avatar    = true;

    ft->via.avatar = calloc(size, sizeof(uint8_t));
    if (ft->via.avatar) {
        ft->status = FILE_TRANSFER_STATUS_PAUSED_US;
        file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_RESUME);
    } else {
        debug_error("FileTransfer:\tUnable to malloc for incoming avatar\n");
        file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL);
    }

    return;
}

static void incoming_inline_image(Tox *tox, uint32_t friend_number, uint32_t file_number, size_t size) {
    debug_info("FileTransfer:\tGetting an incoming inline image\n");


    FILE_TRANSFER *ft = get_file_transfer(friend_number, file_number);
    if (!ft) {
        debug_error("FileTransfer:\tUnable to malloc ft to accept incoming inline image!\n");
        tox_file_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL, NULL);
        return;
    }

    memset(ft, 0, sizeof(*ft));
    ft->incoming    = true;
    ft->in_use      = true;
    ft->in_memory   = true;
    ft->inline_img  = true;

    ft->friend_number = friend_number;
    ft->file_number = file_number;

    ft->target_size = size;

    ft->via.memory = calloc(1, size);
    if (ft->via.memory) {
        debug_notice("FileTransfer:\tStarting incoming inline image of size %lu\n", size);
        file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_RESUME);
    } else {
        debug_error("FileTransfer:\tUnable to malloc enough memory for incoming inline image of size %lu\n", size);
        file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL);
    }

    postmessage(FILE_INCOMING_NEW, friend_number, 0, ft);
    return;
}

/* Function called by core with a new file send request from a friend. */
static void incoming_file_callback_request(Tox *tox, uint32_t friend_number, uint32_t file_number, uint32_t kind,
                                           uint64_t size, const uint8_t *filename, size_t filename_length,
                                           void *UNUSED(user_data))
{
    debug_notice("FileTransfer:\tNew incoming file transfer request from friend %u\n", friend_number);

    if (kind == TOX_FILE_KIND_AVATAR) {
        return incoming_avatar(tox, friend_number, file_number, size);
    }

    if (settings.accept_inline_images
        && size < MAX_INLINE_FILESIZE
        && filename_length == (sizeof("utox-inline.png") - 1)
        && memcmp(filename, "utox-inline.png", filename_length) == 0) {
        return incoming_inline_image(tox, friend_number, file_number, size);
    }

    FILE_TRANSFER *ft = get_file_transfer(friend_number, file_number);
    if (!ft) {
        debug_error("FileTransfer:\tUnable to get memory handle for transfer, canceling friend/file number (%u/%u)\n",
              friend_number, file_number);
        tox_file_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL, 0);
        return;
    }

    uint8_t file_id[TOX_FILE_ID_LENGTH] = { 0 };
    tox_file_get_file_id(tox, friend_number, file_number, file_id, 0);
    /* access the correct memory location for this file */
    /* Load saved information about this file */
    ft->friend_number = friend_number;
    ft->file_number   = file_number;
    memcpy(ft->data_hash, file_id, TOX_FILE_ID_LENGTH);
    if (ft_find_resumeable(ft)) {
        debug_notice("FileTransfer:\tIncoming Existing file from friend (%u) \n", friend_number);
        FILE *   file = fopen((const char *)ft->path, "rb+");
        uint64_t size = 0;
        if (file) {
            debug_info("FileTransfer:\tCool file exists, let try to restart it.\n");
            ft->in_use = 1;
            ft->friend_number = friend_number;
            ft->file_number   = file_number;
            ft->target_size   = size;

            ft->incoming  = true;
            ft->in_memory = false;
            ft->avatar    = false;

            ft->via.file     = file;

            postmessage(FILE_SEND_NEW, friend_number, 0, ft);
            TOX_ERR_FILE_SEEK error = 0;
            tox_file_seek(tox, friend_number, file_number, ft->current_size, &error);
            if (error) {
                debug_error("FileTransfer:\tseek error %i\n", error);
            } else {
                debug_info("FileTransfer:\tseek & resume\n");
                file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_RESUME);
            }
            return;
        }
        debug_error("FileTransfer:\tUnable to open file suggested by resume!\n");
    }
    memset(ft, 0, sizeof(FILE_TRANSFER));
    ft->in_use   = true;
    ft->incoming = true;

    ft->friend_number = friend_number;
    ft->file_number   = file_number;

    tox_file_get_file_id(tox, friend_number, file_number, ft->data_hash, 0);

    ft->name = calloc(1, filename_length +1);
    snprintf((char *)ft->name, filename_length +1, "%.*s", (int)filename_length, filename);
    ft->name_length = filename_length;

    ft->target_size = size;

    postmessage(FILE_INCOMING_NEW, friend_number, file_number, ft);
    /* The file doesn't exist on disk where we expected, let's prompt the user to accept it as a new file */
    debug_notice("FileTransfer:\tNew incoming file from friend (%u) file number (%u)\nFileTransfer:\t\tfilename: %s\n",
          friend_number, file_number, filename);
    /* Auto accept if it's a utox-inline image, with the correct size */
}

/* Called by toxcore to deliver the next chunk of incoming data. */
static void incoming_file_callback_chunk(Tox *tox, uint32_t friend_number, uint32_t file_number,
                                         uint64_t position, const uint8_t *data, size_t length, void *UNUSED(user_data))
{
    debug("FileTransfer:\tIncoming chunk friend(%u), file(%u), start(%lu), end(%lu), \n",
            friend_number, file_number, position, length);

    FILE_TRANSFER *ft = get_file_transfer(friend_number, file_number);
    if (!ft->in_use) {
        debug_error("FileTransfer:\tERROR incoming chuck for an out of use file transfer!\n");
    }

    if (length == 0) {
        utox_complete_file(ft);
        return;
    }

    if (ft->inline_img && ft->via.memory) {
        if (position == 0) {
            uint8_t png_header[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a };
            if (memcmp(data, png_header, 8 ) != 0) {
                // this isn't a png header, just die
                debug_error("FileTransfer:\tFriend %u sent an inline image thats' not a PNG\n", friend_number);
                file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL);
            }
        }
        memcpy(ft->via.memory + position, data, length);
    } else if (ft->avatar && ft->via.avatar) {
        memcpy(ft->via.avatar + position, data, length);
    } else if (ft->via.file) {
        uint8_t count = 10;
        while (!file_lock(ft->via.file, position, length)) {
            debug_error("FileTransfer:\tCan't get lock, sleeping...\n");
            yieldcpu(10);
            if (count == 0) {
                break;
            }
            count--;
            // If you get a bug report about this hanging utox, just disable it, it's unlikely to be needed!
        }
        fseeko(ft->via.file, position, SEEK_SET);
        size_t write_size = fwrite(data, 1, length, ft->via.file);
        fflush(ft->via.file);
        file_unlock(ft->via.file, position, length);
        if (write_size != length) {
            debug_error("\n\nFileTransfer:\tERROR WRITING DATA TO FILE! (%u & %u)\n\n", friend_number, file_number);
            postmessage_toxcore(TOX_FILE_CANCEL, friend_number, file_number, NULL);
            return;
        }
    } else {
        debug("FileTransfer:\tFile Handle failed!\n");
        postmessage_toxcore(TOX_FILE_CANCEL, friend_number, file_number, NULL);
        return;
    }
    calculate_speed(ft);
    ft->current_size += length;
    if (ft->resume_update) {
        --ft->resume_update;
    } else {
        ft_update_resumable(ft);
        ft->resume_update = 20; // every 20 packets we update
    }
    // TODO dirty hack, this needs to be replaced
    // moved it cal_speed() // ft_update_resumable(ft);
}

uint32_t ft_send_avatar(Tox *tox, uint32_t friend_number) {
    if (!tox || self.avatar) {
        debug_error("FileTransfer:\tCan't send an avatar without data\n");
        return UINT32_MAX;
    }
    debug("FileTransfer:\tStarting avatar to friend %u.\n", friend_number);

    // TODO send the uset avatar command.

    FRIEND *f = get_friend(friend_number);
    if (f->file_transfers_outgoing_active_count > MAX_FILE_TRANSFERS) {
        debug_error("FileTransfer:\tCan't send this avatar too many in progress...\n");
        return UINT32_MAX;
    }

    /* While It's not ideal, we don't make sure we can alloc the FILE_TRANSFER until
     * we get the file number from toxcore. This could happen, but I assume it'll be
     * rare enough. Either way, it'll be noisy if it fails so here's to hoping! */
    uint8_t hash[TOX_HASH_LENGTH];
    tox_hash(hash, self.png_data, self.png_size);

    TOX_ERR_FILE_SEND error = 0;
    uint32_t file_number = tox_file_send(tox, friend_number, TOX_FILE_KIND_AVATAR, self.png_size, hash, NULL, 0, &error);
    if (error || file_number == UINT32_MAX) {
        debug_error("tox_file_send() failed error code %u\n", error);
        return UINT32_MAX;
    };

    FILE_TRANSFER *ft = get_file_transfer(friend_number, file_number);
    if (!ft) {
        // This is the noisy case noted above.
        debug_error("FileTransfer:\tUnable to malloc to actually send avatar!\n");
        tox_file_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL, NULL);
        return UINT32_MAX;
    }

    memset(ft, 0, sizeof(*ft));
    ft->incoming = false;
    ft->in_use = true;
    ft->avatar = true;

    ft->friend_number = friend_number;
    ft->file_number = file_number;

    memcpy(ft->data_hash, hash, TOX_HASH_LENGTH);

    ft->target_size = self.png_size;

    ft->status = FILE_TRANSFER_STATUS_PAUSED_THEM;

    return file_number;
}

uint32_t ft_send_file(Tox *tox, uint32_t friend_number, FILE *file, uint8_t *path, size_t path_length) {
    if (!tox || !file) {
        debug_error("FileTransfer:\tCan't send a file without data\n");
        return UINT32_MAX;
    }
    debug("FileTransfer:\tStarting FILE to friend %u.\n", friend_number);

    FRIEND *f = get_friend(friend_number);
    if (f->file_transfers_outgoing_active_count > MAX_FILE_TRANSFERS) {
        debug_error("FileTransfer:\tCan't send this file too many in progress...\n");
        return UINT32_MAX;
    }

    fseeko(file, 0, SEEK_END);
    size_t size = ftello(file);

    const uint8_t *name;
    size_t name_length = 0;
    name = path + path_length;
    while (*--name != '/') {
        ++name_length;
    }
    ++name;
    ++name_length;

    TOX_ERR_FILE_SEND error = 0;
    uint32_t file_number = tox_file_send(tox, friend_number, TOX_FILE_KIND_DATA, size, NULL, name, name_length, &error);
    if (error || file_number == UINT32_MAX) {
        debug_error("tox_file_send() failed error code %u\n", error);
        return UINT32_MAX;
    }

    FILE_TRANSFER *ft = get_file_transfer(friend_number, file_number);
    if (!ft) {
        // This is the noisy case noted above.
        debug_error("FileTransfer:\tUnable to malloc to actually send file!\n");
        tox_file_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL, NULL);
        return UINT32_MAX;
    }

    memset(ft, 0, sizeof(FILE_TRANSFER));
    ft->in_use = true;
    ft->incoming = false;

    ft->friend_number = friend_number;
    ft->file_number   = file_number;

    ft->target_size = size;

    ft->name = (uint8_t*)strdup((char*)name);
    ft->name_length = name_length;
    snprintf((char *)ft->path, UTOX_FILE_NAME_LENGTH, "%.*s", (int)path_length, path);

    ft->via.file = file;

    ft->resumeable = ft_init_resumable(ft);

    ft->status = FILE_TRANSFER_STATUS_PAUSED_THEM;

    postmessage(FILE_SEND_NEW, friend_number, file_number, ft);
    return file_number;
}

/* Returns file number on success, UINT32_MAX on failure. */
uint32_t ft_send_data(Tox *tox, uint32_t friend_number, uint8_t *data, size_t size, uint8_t *name, size_t name_length) {
    if (!tox || !data || !name) {
        debug_error("FileTransfer:\tCan't send data to friend without data\n");
        return UINT32_MAX;
    }
    debug("FileTransfer:\tStarting raw data transfer to friend %u.\n", friend_number);

    // TODO send the unset avatar command.

    FRIEND *f = get_friend(friend_number);
    if (f->file_transfers_outgoing_active_count > MAX_FILE_TRANSFERS) {
        debug_error("FileTransfer:\tCan't send raw data too many in progress...\n");
        return UINT32_MAX;
    }

    /* While It's not ideal, we don't make sure we can alloc the FILE_TRANSFER until
     * we get the file number from toxcore. This could happen, but I assume it'll be
     * rare enough. Either way, it'll be noisy if it fails so here's to hoping! */
    uint8_t hash[TOX_HASH_LENGTH];
    tox_hash(hash, data, size); // TODO skip this if the file is HUGE!

    TOX_ERR_FILE_SEND error = 0;
    uint32_t file_number = tox_file_send(tox, friend_number, TOX_FILE_KIND_DATA, size, hash, name, name_length, &error);
    if (error || file_number == UINT32_MAX) {
        debug_error("tox_file_send() failed error code %u\n", error);
        return UINT32_MAX;
    };

    FILE_TRANSFER *ft = get_file_transfer(friend_number, file_number);
    if (!ft) {
        // This is the noisy case noted above.
        debug_error("FileTransfer:\tUnable to malloc to actually send data!\n");
        tox_file_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL, NULL);
        return UINT32_MAX;
    }

    memset(ft, 0, sizeof(*ft));
    ft->incoming   = false;
    ft->in_use     = true;
    ft->in_memory  = true;
    ft->inline_img = true;

    ft->friend_number = friend_number;
    ft->file_number = file_number;

    memcpy(ft->data_hash, hash, TOX_HASH_LENGTH);

    ft->via.memory = data;

    ft->target_size = size;

    ft->status = FILE_TRANSFER_STATUS_PAUSED_THEM;

    postmessage(FILE_SEND_NEW, friend_number, file_number, ft);
    return file_number;
}

static void outgoing_file_callback_chunk(Tox *tox, uint32_t friend_number, uint32_t file_number, uint64_t position,
                                         size_t length, void *UNUSED(user_data))
{
    debug("FileTransfer:\tChunk requested for friend_id (%u), and file_id (%u). Start (%lu), End (%zu).\r",
            friend_number, file_number, position, length);

    FILE_TRANSFER *ft = get_file_transfer(friend_number, file_number);

    if (length == 0) {
        debug_notice("FileTransfer:\tOutgoing transfer is done (%u & %u)\n", friend_number, file_number);
        utox_complete_file(ft);
        return;
    }

    if (position + length > ft->target_size) {
        debug_error("FileTransfer:\tOuting transfer size mismatch!\n");
        return;
    }

    TOX_ERR_FILE_SEND_CHUNK error;
    if (ft->in_memory) {
        if (ft->via.memory) {
            tox_file_send_chunk(tox, friend_number, file_number, position,
                                ft->via.memory + position, length, &error);
            if (error) {
                debug_error("FileTransfer:\tOutgoing chunk error on memory (%u)\n", error);
            }
        } else {
            debug_error("FileTransfer:\tERROR READING FROM MEMORY! (%u & %u)\n", friend_number, file_number);
            return;
        }
    } else if (ft->avatar) {
        if (self.png_data) {
            tox_file_send_chunk(tox, friend_number, file_number, position,
                                self.png_data + position, length, &error);
            if (error) {
                debug_error("FileTransfer:\tOutgoing chunk error on avatar (%u)\n", error);
            }
        } else {
            debug_error("FileTransfer:\tERROR READING FROM AVATAR! (%u & %u)\n", friend_number, file_number);
            return;
        }
    } else { // File
        if (ft->via.file) {
            uint8_t buffer[length];
            fseeko(ft->via.file, position, SEEK_SET);
            size_t read_size = fread(buffer, 1, length, ft->via.file);
            if (read_size != length) {
                debug_error("FileTransfer:\tERROR READING FILE! (%u & %u)\n", friend_number, file_number);
                debug_info("FileTransfer:\t\tSize (%lu), Position (%lu), Length(%lu), Read_size (%lu), size_transferred (%lu).\n",
                   ft->target_size, position, length, read_size, ft->current_size);
                file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL);
                return;
            }
            tox_file_send_chunk(tox, friend_number, file_number, position, buffer, length, &error);
            if (error) {
                debug_error("FileTransfer:\tOutgoing chunk error on file (%u)\n", error);
            }
        }
    }

    ft->current_size += length;
    calculate_speed(ft);
}

int utox_file_start_write(uint32_t friend_number, uint32_t file_number, const char *filepath) {
    FILE_TRANSFER *ft = get_file_transfer(friend_number, file_number);

    snprintf((char *)ft->path, UTOX_FILE_NAME_LENGTH, "%s", filepath);

    // TODO use native functions to open this file
    ft->via.file = fopen(filepath, "wb");

    if (!ft->via.file) {
        debug_error("FileTransfer:\tThe file we're supposed to write to couldn't be opened\n\t\t\"%s\"\n",
                    ft->path);
        break_file(ft);
        return -1;
    }

    // Removed until we can find a better way of working this in;
    // if(ft->in_tmp_loc){
    //     fseeko(ft->tmp_file, 0, SEEK_SET);
    //     fseeko(ft->file, 0, SEEK_SET);
    //     fwrite(ft->tmp_file, 1, ft->size_transferred, ft->file);
    //     fclose(ft->tmp_file);
    //     ft->in_tmp_loc = 0;
    //     // free(ft->tmp_path); // Freed by xlib probably...
    //     // TODO unlink();
    //     debug("FileTransfer: Data copied from tmp_file to save_file\n");
    // }
    return 0;
}


void utox_set_callbacks_file_transfer(Tox *tox) {
    /* Incoming files */
    /* This is the callback for a new incoming file. */
    tox_callback_file_recv(tox, incoming_file_callback_request);
    /* This is the callback with friend's actions for a file */
    tox_callback_file_recv_control(tox, file_transfer_callback_control);
    /* This is the callback with a chunk data for a file. */
    tox_callback_file_recv_chunk(tox, incoming_file_callback_chunk);
    /* Outgoing files */
    /* This is the callback send to request a new file chunk */
    tox_callback_file_chunk_request(tox, outgoing_file_callback_chunk);
}
