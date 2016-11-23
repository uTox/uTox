#include "file_transfers.h"

#include "friend.h"
#include "main.h"
#include "util.h"

static FILE_TRANSFER *get_file_transfer(uint32_t friend_number, uint32_t file_number) {
    FRIEND *f = get_friend(friend_number);
    if (!f) {
        return NULL;
    }

    if (file_number >= (1 << 16)) {
        // it's an incoming file ( this is some toxcore magic we know about )
        file_number = (file_number >> 16) - 1;
        if (f->file_transfers_incoming_size <= file_number) {
            f->file_transfers_incoming = realloc(f->file_transfers_incoming, sizeof(FILE_TRANSFER) * (file_number + 1));
            if (f->file_transfers_incoming) {
                f->file_transfers_incoming_size = file_number + 1;
                return (FILE_TRANSFER*)&f->file_transfers_incoming[file_number];
            }
        } else {
            return (FILE_TRANSFER*)&f->file_transfers_incoming[file_number];
        }
    } else {
        if (f->file_transfers_outgoing_size <= file_number) {
            f->file_transfers_outgoing = realloc(f->file_transfers_incoming, sizeof(FILE_TRANSFER) * (file_number + 1));

            if (f->file_transfers_outgoing) {
                f->file_transfers_outgoing_size = file_number + 1;
                return (FILE_TRANSFER*)&f->file_transfers_outgoing[file_number];
            }
        } else {
            return (FILE_TRANSFER*)&f->file_transfers_outgoing[file_number];
        }
    }
    return NULL;
}

/* Create a FILE_TRANSFER struct with the supplied data. */
static void build_file_transfer(FILE_TRANSFER *ft, uint32_t friend_number, uint32_t file_number, uint64_t file_size,
                                bool incoming, bool in_memory, bool is_avatar, const uint8_t *name,
                                size_t name_length, const uint8_t *path, size_t path_length, const uint8_t *file_id,
                                Tox *tox)
{
    FILE_TRANSFER *file = ft;

    memset(file, 0, sizeof(FILE_TRANSFER));

    file->in_use = 1;

    file->friend_number = friend_number;
    file->file_number   = file_number;
    file->target_size   = file_size;

    file->incoming  = incoming;
    file->in_memory = in_memory;
    file->avatar    = is_avatar;

    if (name) {
        file->name = malloc(name_length + 1);
        memcpy(file->name, name, name_length);
        file->name_length             = utf8_validate(name, name_length);
        file->name[file->name_length] = 0;
    } else {
        file->name        = NULL;
        file->name_length = 0;
    }

    if (path) {
        file->path = malloc(path_length + 1);
        memcpy(file->path, path, path_length);
        file->path_length             = path_length;
        file->path[file->path_length] = 0;
    } else {
        file->path        = NULL;
        file->path_length = 0;
    }

    if (file_id) {
        memcpy(file->data_hash, file_id, TOX_FILE_ID_LENGTH);
    } else {
        tox_file_get_file_id(tox, friend_number, file_number, file->data_hash, 0);
    }

    // TODO size correction error checking for this...
    if (in_memory) {
        if (is_avatar) {
            file->via.avatar = calloc(file_size, sizeof(uint8_t));
        } else {
            file->via.memory = calloc(file_size, sizeof(uint8_t));
        }
    }

    if (!incoming) {
        /* Outgoing file */
        file->status = FILE_TRANSFER_STATUS_PAUSED_THEM;
    }
}

/* Copy the data from active FILE_TRANSFER, and pass it along to the UI with it's update. */
static void notify_update_file(FILE_TRANSFER *file) {
    // FILE_TRANSFER *file_copy = calloc(1, sizeof(FILE_TRANSFER));

    // memcpy(file_copy, file, sizeof(FILE_TRANSFER));
    postmessage(FILE_UPDATE_STATUS, 0, 0, file);
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

    if (time - file->last_check_time >= 1000 * 1000 * 100) {
        file->speed = (((double)(file->current_size - file->last_check_transferred) * 1000.0 * 1000.0 * 1000.0)
                       / (double)(time - file->last_check_time))
                      + 0.5;
        file->last_check_time        = time;
        file->last_check_transferred = file->current_size;
    }

    notify_update_file(file);
    utox_file_save_ftinfo(file);
}

/* Create the file transfer resume info file. */
static int utox_file_alloc_ftinfo(FILE_TRANSFER *file) {
    uint8_t blank_id[TOX_FILE_ID_LENGTH] = { 0 };
    if (memcmp(file->data_hash, blank_id, TOX_FILE_ID_LENGTH) == 0) {
        debug_error("FileTransfer:\tUnable to get file id from tox... uTox can't resume file %.*s\n",
                    (uint32_t)file->name_length, file->name);
        return 0;
    }

    char path[UTOX_FILE_NAME_LENGTH];

    if (file->incoming) {
        char hex[TOX_FILE_ID_LENGTH * 2];
        fid_to_string(hex, file->data_hash);
        snprintf(path, UTOX_FILE_NAME_LENGTH, "%.*s.ftinfo", TOX_FILE_ID_LENGTH * 2, hex);
    } else {
        snprintf(path, UTOX_FILE_NAME_LENGTH, "%.*s%02i.ftoutfo",
                    TOX_PUBLIC_KEY_SIZE * 2, friend[file->friend_number].id_str,
                    file->file_number % 100);
    }

    debug_error("FileTransfer:\tUnable to save file info... uTox can't resume file %.*s\n", (uint32_t)file->name_length,
                file->name);
    debug("FileTransfer:\t.ftinfo for file %.*s set; ready to resume!\n", (uint32_t)file->name_length, file->name);
    utox_file_save_ftinfo(file);
    return 1;
}

/* Free/Remove/Unlink the file transfer resume info file. */
static void utox_file_free_ftinfo(FILE_TRANSFER *file) {
    return;
    // uint8_t path[UTOX_FILE_NAME_LENGTH];
    // size_t  path_length;
    // path_length = datapath(path);

    // if (file->incoming) {
    //     char hex_id[TOX_FILE_ID_LENGTH * 2];
    //     fid_to_string(hex_id, file->data_hash);
    //     memcpy(path + path_length, hex_id, TOX_FILE_ID_LENGTH * 2);
    //     strcpy((char *)path + (path_length + TOX_FILE_ID_LENGTH * 2), ".ftinfo");
    // } else {
    //     char hex_id[TOX_PUBLIC_KEY_SIZE * 2];
    //     cid_to_string(hex_id, friend[file->friend_number].cid);
    //     memcpy(path + path_length, hex_id, TOX_PUBLIC_KEY_SIZE * 2);
    //     sprintf((char *)path + (path_length + TOX_PUBLIC_KEY_SIZE * 2), "%02i.ftoutfo", file->file_number % 100);
    // }

    // debug("Removing. %s\n", path);
    // remove((const char *)path);
}

static void utox_file_ftoutfo_move(unsigned int friend_number, unsigned int source_num, unsigned int dest_num) {
    // uint8_t path_src[UTOX_FILE_NAME_LENGTH], path_dst[UTOX_FILE_NAME_LENGTH];
    // size_t  path_length;
    // path_length = datapath(path_src);
    // char hex_id[TOX_PUBLIC_KEY_SIZE * 2];
    // cid_to_string(hex_id, friend[friend_number].cid);
    // memcpy(path_src + path_length, hex_id, TOX_PUBLIC_KEY_SIZE * 2);
    // memcpy(path_dst, path_src, (path_length + TOX_PUBLIC_KEY_SIZE * 2));
    // sprintf((char *)path_src + (path_length + TOX_PUBLIC_KEY_SIZE * 2), "%02i.ftoutfo", source_num % 100);
    // sprintf((char *)path_dst + (path_length + TOX_PUBLIC_KEY_SIZE * 2), "%02i.ftoutfo", dest_num % 100);
    // rename((char *)path_src, (char *)path_dst);
}

/* Cancel active file. */
static void utox_kill_file(FILE_TRANSFER *file, uint8_t us) {
    if (file->status == FILE_TRANSFER_STATUS_KILLED) {
        debug("File already killed.\n");
        return;
    } else if (file->status == FILE_TRANSFER_STATUS_COMPLETED) {
        debug("File already completed.\n");
        return;
    } else {
        file->status = FILE_TRANSFER_STATUS_KILLED;

        if (((MSG_FILE *)file->ui_data)) {
            ((MSG_FILE *)file->ui_data)->file_status = FILE_TRANSFER_STATUS_KILLED;
        }
    }

    notify_update_file(file);

    if (!file->incoming && friend[file->friend_number].file_transfers_incoming_size) {
        get_friend(file->friend_number)->file_transfers_incoming_size--;
    } else if (friend[file->friend_number].file_transfers_outgoing_size) {
        get_friend(file->friend_number)->file_transfers_outgoing_size--;
    }

    if (file->resumeable) {
        utox_file_save_ftinfo(file);
        utox_file_free_ftinfo(file);
    }
    utox_cleanup_file_transfers(file->friend_number, file->file_number);
}

/* Break active file, (when a friend goes offline). */
static void utox_break_file(FILE_TRANSFER *file) {
    if (file->status == FILE_TRANSFER_STATUS_NONE) {
        return utox_kill_file(file, 1); /* We don't save unstarted files */
    } else if (file->status == FILE_TRANSFER_STATUS_COMPLETED || file->status == FILE_TRANSFER_STATUS_KILLED) {
        /* We don't touch these files! */
        return;
    }

    // I don't think we need/want to touch the size here
    // if (!file->incoming && friend[file->friend_number].file_transfers_incoming_size) {
    //     get_friend(file->friend_number)->file_transfers_incoming_size--;
    // } else if (friend[file->friend_number].file_transfers_outgoing_size) {
    //     get_friend(file->friend_number)->file_transfers_outgoing_size--;
    // }

    file->status = FILE_TRANSFER_STATUS_BROKEN;

    if (((MSG_FILE *)file->ui_data)) {
        ((MSG_FILE *)file->ui_data)->file_status = FILE_TRANSFER_STATUS_BROKEN;
    }

    notify_update_file(file);
    utox_file_save_ftinfo(file);
    if (file->in_use) {
        utox_cleanup_file_transfers(file->friend_number, file->file_number);
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
    notify_update_file(file);
    // TODO free not freed data.
}

/* Start/Resume active file. */
static void utox_run_file(FILE_TRANSFER *file, uint8_t us) {
    if (file->status == FILE_TRANSFER_STATUS_ACTIVE) {
        return;
    }
    if (us) {
        if (file->status == FILE_TRANSFER_STATUS_NONE) {
            file->status = FILE_TRANSFER_STATUS_ACTIVE;
            /* Set resuming info TODO MOVE TO AFTER FILE IS ACCEPED BY USER */
            if (file->resumeable == 0) {
                file->resumeable = utox_file_alloc_ftinfo(file);
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
            debug("FileTransfer:\tWe tried to run file from an unknown state! (%u)\n", file->status);
        }
    } else {
        if (file->status == FILE_TRANSFER_STATUS_PAUSED_US) {
            // Do nothing;
        } else if (file->status == FILE_TRANSFER_STATUS_PAUSED_BOTH) {
            file->status = FILE_TRANSFER_STATUS_PAUSED_US;
        } else if (file->status == FILE_TRANSFER_STATUS_PAUSED_THEM) {
            file->status = FILE_TRANSFER_STATUS_ACTIVE;
        } else if (file->status == FILE_TRANSFER_STATUS_BROKEN) {
            file->status = FILE_TRANSFER_STATUS_ACTIVE;
        } else {
            debug("FileTransfer:\tThey tried to run incoming file from an unknown state! (%u)\n", file->status);
        }
    }
    notify_update_file(file);
    // debug("utox_run_file\n");
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
static void utox_complete_file(FILE_TRANSFER *file) {
    if (file->status == FILE_TRANSFER_STATUS_ACTIVE) {
        if (file->incoming) {
            if (file->in_memory) {
                if (file->avatar) {
                    postmessage(FRIEND_AVATAR_SET, file->friend_number, file->current_size, file->via.avatar);
                    // file->avatar = NULL;
                    // file->size   = 0;
                } else {
                    decode_inline_png(file->friend_number, file->via.memory, file->current_size);
                }
            } else { // Is a file
                ((MSG_FILE *)file->ui_data)->path = strdup((const char *)file->path);
            }
        } else {
            if (file->in_memory) {
                // TODO, might want to do something here.
            } else { // Is a file
                ((MSG_FILE *)file->ui_data)->path = strdup((const char *)file->path);
            }
            // if (friend[file->friend_number].transfer_count) {
            //     --friend[file->friend_number].transfer_count;
            // }
        }
        file->status = FILE_TRANSFER_STATUS_COMPLETED;
        if (((MSG_FILE *)file->ui_data)) {
            ((MSG_FILE *)file->ui_data)->file_status = FILE_TRANSFER_STATUS_COMPLETED;
        }
        notify_update_file(file);
    } else {
        debug("FileTransfer:\tUnable to complete file in non-active state (file:%u)\n", file->file_number);
    }
    debug("FileTransfer:\tIncoming transfer is done (%u & %u)\n", file->friend_number, file->file_number);
    utox_file_save_ftinfo(file);
    utox_file_free_ftinfo(file);
    utox_cleanup_file_transfers(file->friend_number, file->file_number);
}

/* Friend has come online, restart our outgoing transfers to this friend. */
void ft_friend_online(Tox *tox, uint32_t friend_number) {
    for (int i = 0; i < MAX_FILE_TRANSFERS; i++) {
        FILE_TRANSFER *file = calloc(1, sizeof(*file));
        file->friend_number = friend_number;
        file->file_number   = i;
        file->incoming      = 0;
        utox_file_load_ftinfo(file);
        if (file->path) {
            /* If we got a path from utox_file_load we should try to resume! */
            // uint32_t f_n = ft_send_data(tox, friend_number, NULL, (uint8_t *)file, sizeof(*file));

            // if (f_n != UINT32_MAX && f_n != i) {
                // utox_file_ftoutfo_move(friend_number, i, f_n);
            // }
        }

        free(file);
    }
    /* Else look in filetransfer info dir; */
}

/* Friend has gone offline, break our outgoing transfers to this friend. */
void ft_friend_offline(Tox *tox, uint32_t friend_number) {
    debug("FileTransfer:\tFriend %u has gone offline, breaking transfers\n", friend_number);
    unsigned int i;
    for (i = 0; i < MAX_FILE_TRANSFERS; ++i) {
        // TODO
        // utox_break_file(&incoming_transfer[friend_number][i]);
        // utox_break_file(&outgoing_transfer[friend_number][i]);
    }
}

/* Local command callback to change a file status. */
void file_transfer_local_control(Tox *tox, uint32_t friend_number, uint32_t file_number, TOX_FILE_CONTROL control) {
    TOX_ERR_FILE_CONTROL error = 0;
    FILE_TRANSFER *      info  = get_file_transfer(friend_number, file_number);
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
            utox_run_file(info, 1);
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
                utox_kill_file(info, 1);
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

    switch (control) {
        case TOX_FILE_CONTROL_RESUME: {
            debug("FileTransfer:\tFriend (%i) has resumed file (%i)\n", friend_number, file_number);
            utox_run_file(ft, 0);
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
            utox_kill_file(ft, 0);
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

    /* Avatar size is valid, and it's a new avatar, lets accept it */
    build_file_transfer(get_file_transfer(friend_number, file_number), friend_number, file_number,
                        size, 1, 1, 1, NULL, 0, NULL, 0, NULL, tox);
    file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_RESUME);
    return;
}

/* Function called by core with a new file send request from a friend. */
static void incoming_file_callback_request(Tox *tox, uint32_t friend_number, uint32_t file_number, uint32_t kind,
                                           uint64_t file_size, const uint8_t *filename, size_t filename_length,
                                           void *UNUSED(user_data))
{
    /* First things first, get the file_id from core */
    uint8_t file_id[TOX_FILE_ID_LENGTH] = { 0 };
    tox_file_get_file_id(tox, friend_number, file_number, file_id, 0);
    /* access the correct memory location for this file */
    FILE_TRANSFER *file_handle = get_file_transfer(friend_number, file_number);
    if (!file_handle) {
        debug_error("FileTransfer:\tUnable to get memory handle for transfer, canceling friend/file number (%u/%u)\n",
              friend_number, file_number);
        tox_file_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL, 0);
        return;
    }

    switch (kind) {
        case TOX_FILE_KIND_AVATAR: {
            incoming_avatar(tox, friend_number, file_number, file_size);
            break;
            /* Verify Avatar Size */
        }
        case TOX_FILE_KIND_DATA: {
            /* Load saved information about this file */
            file_handle->friend_number = friend_number;
            file_handle->file_number   = file_number;
            memcpy(file_handle->data_hash, file_id, TOX_FILE_ID_LENGTH);
            if (utox_file_load_ftinfo(file_handle)) {
                debug_notice("FileTransfer:\tIncoming Existing file from friend (%u) \n", friend_number);
                /* We were able to load incoming file info from disk; validate date! */
                /* First, backup transfered size, because we're about to overwrite it*/
                uint64_t seek_size = file_handle->current_size;
                /* Try to reopen incoming file */
                /* TODO, use native file fxns */
                FILE *   file = fopen((const char *)file_handle->path, "rb");
                uint64_t size = 0;
                if (file) {
                    /* File exist, and is readable, now get current size! */
                    debug_info("FileTransfer:\tCool file exists, let try to restart it.\n");
                    /* Eventually we want to cancel the file if it's larger than the incoming size, but without an error
                    dialog we'll just wait for now...
                    fseeko(file, 0, SEEK_END); size = ftello(file); fclose(file);
                    if(file_size <= size){
                        debug("FileTransfer:\tIncoming size (%lu), and size on disk (%lu) mismatch, aborting!\n", size,
                    file_size);
                        file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL);
                        free(file_handle->path);
                        return;
                    }*/
                    fclose(file);
                    file = fopen((const char *)file_handle->path, "rb+");
                    /* We have to open as rb+, so we also need to re-seek to the start! */
                    fseeko(file, 0, SEEK_SET);
                    /* We can read, but can we write? */
                    if (file) {
                        /* We can read and write, build a new file handle to work with! */
                        build_file_transfer(file_handle, friend_number, file_number, size, 1, 0, 0,
                                            filename, filename_length, file_handle->path, file_handle->path_length,
                                            NULL, tox);
                        file_handle->via.file    = file;
                        file_handle->target_size = seek_size;
                        /* TODO try to re-access the original message box for this file transfer, without segfaulting!
                         */
                        file_handle->resumeable = utox_file_alloc_ftinfo(file_handle);
                        postmessage(FILE_SEND_NEW, friend_number, 0, file_handle);
                        TOX_ERR_FILE_SEEK error = 0;
                        tox_file_seek(tox, friend_number, file_number, seek_size, &error);
                        if (error) {
                            debug_error("FileTransfer:\tseek error %i\n", error);
                        } else {
                            debug_info("FileTransfer:\tseek & resume\n");
                            file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_RESUME);
                        }
                        return;
                    } else {
                        debug_error("FileTransfer:\tFile opened for reading, but unable to get write access, canceling "
                                    "file!\n");
                        file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL);
                        free(file_handle->path);
                        return;
                    }
                }
            }
            /* The file doesn't exist on disk where we expected, let's prompt the user to accept it as a new file */
            debug_notice("FileTransfer:\tNew incoming file from friend (%u) file number (%u)\nFileTransfer:\t\tfilename: %s\n",
                  friend_number, file_number, filename);
            /* Auto accept if it's a utox-inline image, with the correct size */
            if (file_size < 1024 * 1024 * 4 && filename_length == (sizeof("utox-inline.png") - 1)
                && memcmp(filename, "utox-inline.png", filename_length) == 0) {

                build_file_transfer(file_handle, friend_number, file_number, file_size, 1, 1, 0, filename, filename_length, NULL, 0, NULL, tox);

                file_handle->resumeable = 0;
                /* Notify the user! */
                postmessage(FILE_INCOMING_NEW, friend_number, 0, file_handle);
                file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_RESUME);
            } else {
                build_file_transfer(file_handle, friend_number, file_number, file_size, 1, 0, 0, filename, filename_length, NULL, 0, NULL, tox);
                /* Set UI values */
                file_handle->resumeable = 0;
                /* Notify the user! */
                postmessage(FILE_INCOMING_NEW, friend_number, 0, file_handle);
            }
            break; /*We shouldn't reach here, but just in case! */
        }          /* last case */
    }              /* switch */
}

/* Called by toxcore to deliver the next chunk of incoming data. */
static void incoming_file_callback_chunk(Tox *UNUSED(tox), uint32_t friend_number, uint32_t file_number,
                                         uint64_t position, const uint8_t *data, size_t length, void *UNUSED(user_data))
{
    debug("FileTransfer:\tIncoming chunk friend(%u), file(%u), start(%lu), end(%lu), \n",
            friend_number, file_number, position, length);

    FILE_TRANSFER *ft = get_file_transfer(friend_number, file_number);

    if (length == 0) {
        utox_complete_file(ft);
        return;
    }

    if (ft->in_memory) {
        if (ft->avatar) {
            memcpy(ft->via.avatar + position, data, length);
        } else {
            memcpy(ft->via.memory + position, data, length);
        }
    } else {
        // Removed until we can better implement temp files
        // if(ft->in_tmp_loc){
        //     fseeko(ft->tmp_file, position, SEEK_SET);
        //     size_t write_size = fwrite(data, 1, length, ft->tmp_file);
        //     if(write_size != length){
        //         debug("\n\nFileTransfer:\tERROR WRITING DATA TO TEMP FILE! (%u & %u)\n\n", friend_number,
        //         file_number);
        //         postmessage_toxcore(TOX_FILE_INCOMING_CANCEL, friend_number, file_number, NULL);
        //         return;
        //     }
        // }
        if (ft->via.file) {
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
    }
    ft->current_size += length;
    // TODO dirty hack, this needs to be replaced
    // moved it cal_speed() // utox_file_save_ftinfo(ft);
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
        debug("tox_file_send() failed error code %u\n", error);
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
    return file_number;
}

uint32_t ft_send_file(Tox *tox, uint32_t friend_number, FILE *file, uint8_t *name, size_t name_length) {
    if (!tox || !file) {
        debug_error("FileTransfer:\tCan't send a file without data\n");
        return UINT32_MAX;
    }
    debug("FileTransfer:\tStarting FILE to friend %u.\n", friend_number);

    FRIEND *f = get_friend(friend_number);
    if (f->file_transfers_outgoing_active_count > MAX_FILE_TRANSFERS) {
        debug_error("FileTransfer:\tCan't send this avatar too many in progress...\n");
        return UINT32_MAX;
    }

    fseeko(file, 0, SEEK_END);
    size_t size = ftello(file);

    TOX_ERR_FILE_SEND error = 0;
    uint32_t file_number = tox_file_send(tox, friend_number, TOX_FILE_KIND_DATA, size, NULL, name, name_length, &error);
    if (error || file_number == UINT32_MAX) {
        debug("tox_file_send() failed error code %u\n", error);
        return UINT32_MAX;
    }

    FILE_TRANSFER *ft = get_file_transfer(friend_number, file_number);
    if (!ft) {
        // This is the noisy case noted above.
        debug_error("FileTransfer:\tUnable to malloc to actually send file!\n");
        tox_file_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL, NULL);
        return UINT32_MAX;
    }

    memset(ft, 0, sizeof(*ft));
    ft->incoming = false;

    ft->friend_number = friend_number;
    ft->file_number   = file_number;

    ft->target_size = size;

    ft->via.file = file;

    return file_number;
}

/* Returns file number on success, UINT32_MAX on failure. */
uint32_t ft_send_data(Tox *tox, uint32_t friend_number, uint8_t *data, size_t size, uint8_t *name, size_t name_length) {
    if (!tox || !data || !name) {
        debug_error("FileTransfer:\tCan't send data to friend without data\n");
        return UINT32_MAX;
    }
    debug("FileTransfer:\tStarting raw data transfer to friend %u.\n", friend_number);

    // TODO send the uset avatar command.

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
        debug("tox_file_send() failed error code %u\n", error);
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
    ft->incoming  = false;
    ft->in_use    = true;
    ft->in_memory = true;

    ft->friend_number = friend_number;
    ft->file_number = file_number;

    memcpy(ft->data_hash, hash, TOX_HASH_LENGTH);

    ft->via.memory = data;

    ft->target_size = size;
    return file_number;
}
            // /* Generate the file name. */
            // if (file_data_size < 1024 * 1024 * 4) {
            //     filename_length = sizeof("utox-inline.png") - 1;
            //     filename        = malloc(filename_length + 1);
            //     memcpy(filename, "utox-inline.png", filename_length + 1);
            // } else {
            //     filename_length = sizeof("utox-image.png") - 1;
            //     filename        = malloc(filename_length + 1);
            //     memcpy(filename, "utox-image.png", filename_length + 1);
            // }

            // debug("FileTransfer:\tStarting outgoing image to friend %u.\n", friend_number);
            // file_number = tox_file_send(tox, friend_number, TOX_FILE_KIND_DATA, file_data_size, NULL, filename,
            //                             filename_length, &error);

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
        } else {
            debug_error("FileTransfer:\tERROR READING FROM MEMORY! (%u & %u)\n", friend_number, file_number);
            return;
        }
    } else if (ft->avatar) {
        if (self.png_data) {
            tox_file_send_chunk(tox, friend_number, file_number, position,
                                self.png_data + position, length, &error);
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
        }
    }

    ft->current_size += length;
    calculate_speed(ft);
}

int utox_file_start_write(uint32_t friend_number, uint32_t file_number, const char *filepath) {
    FILE_TRANSFER *ft = get_file_transfer(friend_number, file_number);

    ft->path        = (uint8_t *)strdup(filepath);
    ft->path_length = strlen(filepath);

    if (!ft->via.file) {
        debug_error("FileTransfer:\tUnable to use already open file.\n");
        // file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL);
        return -1;
    }

    if (!ft->via.file) {
        debug_error("FileTransfer:\tThe file we're supposed to write to couldn't be opened\n\t\t\"%s\"\n",
                    ft->path);
        utox_break_file(ft);
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

void utox_cleanup_file_transfers(uint32_t friend_number, uint32_t file_number) {
    FILE_TRANSFER *transfer = get_file_transfer(friend_number, file_number);
    if (transfer->name) {
        debug("FileTransfer:\tCleaning up file transfers! (%u & %u)\n", friend_number, file_number);
        free(transfer->name);
    }

    // if (transfer->via.file) {
    //     fclose(transfer->via.file);
    // }

    // if (transfer->saveinfo) {
    //     fclose(transfer->saveinfo);
    // }

    memset(transfer, 0, sizeof(FILE_TRANSFER));
}

void utox_file_save_ftinfo(FILE_TRANSFER *file) {
    return;
}

bool utox_file_load_ftinfo(FILE_TRANSFER *ft) {
    uint8_t  path[UTOX_FILE_NAME_LENGTH];
    size_t   path_length = 0;
    uint32_t size_read;

    // path_length = datapath(path);

    if (ft->incoming) {
        char hex_id[TOX_FILE_ID_LENGTH * 2];
        fid_to_string(hex_id, ft->data_hash);
        memcpy(path + path_length, hex_id, TOX_FILE_ID_LENGTH * 2);
        strcpy((char *)path + (path_length + TOX_FILE_ID_LENGTH * 2), ".ftinfo");
    } else {
        char hex_id[TOX_PUBLIC_KEY_SIZE * 2];
        cid_to_string(hex_id, friend[ft->friend_number].cid);
        memcpy(path + path_length, hex_id, TOX_PUBLIC_KEY_SIZE * 2);
        sprintf((char *)path + (path_length + TOX_PUBLIC_KEY_SIZE * 2), "%02i.ftoutfo", ft->file_number % 100);
    }

    void *load = file_raw((char *)path, &size_read);

    if (!load) {
        if (ft->incoming) {
            debug("FileTransfer:\tUnable to load saved info... uTox can't resume file %.*s\n",
                  (uint32_t)ft->name_length, ft->name);
        }
        ft->status = 0;
        if (ft->via.file) {
            /* Just in case we try to resume an active file. */
            fclose(ft->via.file);
        }
        return 0;
    } else {
        if (ft->via.file) {
            /* Just in case we try to resume an active file. */
            fclose(ft->via.file);
        }
    }

    FILE_TRANSFER *info = calloc(1, sizeof(*info));
    memcpy(info, load, sizeof(*info));
    info->path_length = (size_read - sizeof(*info));
    info->path        = malloc(info->path_length + 1);
    memcpy(info->path, load + (sizeof(*info)), info->path_length);
    info->path[info->path_length] = 0;

    info->friend_number = ft->friend_number;
    info->file_number   = ft->file_number;
    info->name          = NULL;
    info->name_length   = 0;
    info->ui_data       = NULL;
    info->via.file      = NULL;

    memcpy(ft, info, sizeof(*ft));
    free(info);
    free(load);
    return 1;
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
