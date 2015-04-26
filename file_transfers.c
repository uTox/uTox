#include "main.h"

//static FILE_TRANSFER *file_t[256], **file_tend = file_t;
static FILE_TRANSFER outgoing_transfer[MAX_NUM_FRIENDS][MAX_FILE_TRANSFERS] = {{{0}}};
static FILE_TRANSFER incoming_transfer[MAX_NUM_FRIENDS][MAX_FILE_TRANSFERS] = {{{0}}};

// Remove if supported by core
#define TOX_FILE_KIND_EXISTING 3

FILE_TRANSFER *get_file_transfer(uint32_t friend_number, uint32_t file_number){
    _Bool incoming = 0;
    if (file_number >= (1 << 16)) {
        file_number = (file_number >> 16) - 1;
        incoming = 1;
    }

    if (file_number >= MAX_FILE_TRANSFERS)
        return NULL;

    FILE_TRANSFER *ft;
    if (incoming) {
        ft = &incoming_transfer[friend_number][file_number];
        ft->incoming = 1;
    } else {
        ft = &outgoing_transfer[friend_number][file_number];
        ft->incoming = 0;
    }

    return ft;
}

static void utox_build_file_transfer(FILE_TRANSFER *ft, uint32_t friend_number, uint32_t file_number,
    uint64_t file_size, _Bool incoming, _Bool in_memory, _Bool is_avatar, uint8_t kind, const uint8_t *name,
    size_t name_length, const uint8_t *path, size_t path_length, const uint8_t *file_id, Tox *tox){
    FILE_TRANSFER *file = ft;

    memset(file, 0, sizeof(FILE_TRANSFER));

    file->friend_number = friend_number;
    file->file_number   = file_number;
    file->size          = file_size;

    file->incoming      = incoming;
    file->in_memory     = in_memory;
    file->is_avatar     = is_avatar;
    file->kind          = kind;

    if(name){
        file->name        = malloc(name_length + 1);
        memcpy(file->name, name, name_length);
        file->name_length = name_length;
        file->name[file->name_length] = 0;
    } else {
        file->name        = NULL;
        file->name_length = 0;
    }

    if(path){
        file->path        = malloc(path_length + 1);
        memcpy(file->path, path, path_length);
        file->path_length = path_length;
        file->path[file->path_length] = 0;
    } else {
        file->path        = NULL;
        file->path_length = 0;
    }

    if(file_id){
        memcpy(file->file_id, file_id, TOX_FILE_ID_LENGTH);
    } else {
        tox_file_get_file_id(tox, friend_number, file_number, file->file_id, 0);
    }

    // TODO size correction error checking for this...
    if(in_memory){
        if (is_avatar){
            file->avatar = calloc(file_size, sizeof(uint8_t));
        } else {
            file->memory = calloc(file_size, sizeof(uint8_t));
        }
    }

    if(!incoming){
        /* Outgoing file */
        file->status = FILE_TRANSFER_STATUS_PAUSED_THEM;
    }
}


static void utox_update_user_file(FILE_TRANSFER *file){
    FILE_TRANSFER *file_copy = malloc(sizeof(FILE_TRANSFER));

    memcpy(file_copy, file, sizeof(FILE_TRANSFER));
    postmessage(FRIEND_FILE_UPDATE, 0, 0, file_copy);
}

static void calculate_speed(FILE_TRANSFER *file){
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

    if(time - file->last_check_time >= 1000 * 1000 * 100) {
        file->speed = (((double)(file->size_transferred - file->last_check_transferred) * 1000.0 * 1000.0 * 1000.0) / (double)(time - file->last_check_time)) + 0.5;
        file->last_check_time = time;
        file->last_check_transferred = file->size_transferred;
    }

    utox_update_user_file(file);
    utox_file_save_ftinfo(file);
}

static int utox_file_alloc_ftinfo(FILE_TRANSFER *file){
    uint8_t blank_id[TOX_FILE_ID_LENGTH] = {0};
    if(memcmp(file->file_id, blank_id, TOX_FILE_ID_LENGTH) == 0){
        debug("FileTransfer:\tUnable to get file id from tox... uTox can't resume file %.*s\n", (uint32_t)file->name_length, file->name);
        return 0;
    }

    uint8_t path[UTOX_FILE_NAME_LENGTH];
    size_t path_length;
    path_length = datapath(path);

    if(file->incoming){
        uint8_t hex_id[TOX_FILE_ID_LENGTH * 2];
        fid_to_string(hex_id, file->file_id);
        memcpy(path + path_length, hex_id, TOX_FILE_ID_LENGTH * 2);
        strcpy((char*)path + (path_length + TOX_FILE_ID_LENGTH * 2), ".ftinfo");
    } else {
        uint8_t hex_id[TOX_PUBLIC_KEY_SIZE * 2];
        cid_to_string(hex_id, friend[file->friend_number].cid);
        memcpy(path + path_length, hex_id, TOX_PUBLIC_KEY_SIZE * 2);
        sprintf((char*)path + (path_length + TOX_PUBLIC_KEY_SIZE * 2), "%02i.ftoutfo", file->file_number % 100);
    }

    FILE *saveinfo = fopen((const char*)path, "wb");
    if(!file) {
        debug("FileTransfer:\tUnable to save file info... uTox can't resume file %.*s\n", (uint32_t)file->name_length, file->name);
        return 0;
    }
    file->saveinfo = saveinfo;
    debug("FileTransfer:\t.ftinfo for file %.*s set; ready to resume!\n", (uint32_t)file->name_length, file->name);
    utox_file_save_ftinfo(file);
    return 1;
}

static void utox_file_free_ftinfo(FILE_TRANSFER *file){
    if(file->saveinfo){
        uint8_t path[UTOX_FILE_NAME_LENGTH];
        size_t path_length;
        path_length = datapath(path);

        if(file->incoming){
            uint8_t hex_id[TOX_FILE_ID_LENGTH * 2];
            fid_to_string(hex_id, file->file_id);
            memcpy(path + path_length, hex_id, TOX_FILE_ID_LENGTH * 2);
            strcpy((char*)path + (path_length + TOX_FILE_ID_LENGTH * 2), ".ftinfo");
        } else {
            uint8_t hex_id[TOX_PUBLIC_KEY_SIZE * 2];
            cid_to_string(hex_id, friend[file->friend_number].cid);
            memcpy(path + path_length, hex_id, TOX_PUBLIC_KEY_SIZE * 2);
            sprintf((char*)path + (path_length + TOX_PUBLIC_KEY_SIZE * 2), "%02i.ftoutfo", file->file_number % 100);
        }

        remove((const char*)path);
    }
}

static void utox_kill_file(FILE_TRANSFER *file, uint8_t us){
    if (file->status == FILE_TRANSFER_STATUS_KILLED) {
        debug("File already killed.\n");
        return;
    } else if(file->status == FILE_TRANSFER_STATUS_COMPLETED){
        debug("File already completed.\n");
        return;
    } else {
        file->status = FILE_TRANSFER_STATUS_KILLED;
    }

    utox_update_user_file(file);

    if (!file->incoming && friend[file->friend_number].transfer_count) {
        --friend[file->friend_number].transfer_count;
    }
    utox_cleanup_file_transfers(file->friend_number, file->file_number);
    utox_file_save_ftinfo(file);
    utox_file_free_ftinfo(file);
}

static void utox_break_file(FILE_TRANSFER *file){
    if(file->status == FILE_TRANSFER_STATUS_NONE && file->resume){
        return utox_kill_file(file, 1); /* We don't save unstarted files */
    } else if(file->status == FILE_TRANSFER_STATUS_COMPLETED || file->status == FILE_TRANSFER_STATUS_KILLED) {
        /* We don't touch these files! */
        return;
    }
    file->status = FILE_TRANSFER_STATUS_BROKEN;
    utox_update_user_file(file);
}

static void utox_pause_file(FILE_TRANSFER *file, uint8_t us){
    switch(file->status){
    case FILE_TRANSFER_STATUS_NONE:{
        if(!file->incoming){
            // New transfers start as paused them
            file->status = FILE_TRANSFER_STATUS_PAUSED_THEM;
        } else {
            debug("FileTransfer:\tWe can't pause an unaccepted file!\n");
        }
        break;
    }
    case FILE_TRANSFER_STATUS_ACTIVE:{
        if(us){
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
    case FILE_TRANSFER_STATUS_PAUSED_THEM:{
        if(us){
            if(file->status == FILE_TRANSFER_STATUS_PAUSED_US){
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
            if(file->status == FILE_TRANSFER_STATUS_PAUSED_US){
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
    case FILE_TRANSFER_STATUS_BROKEN:{
        debug("FileTransfer:\tCan't pause a broken file;\n");
        break;
    }
    case FILE_TRANSFER_STATUS_COMPLETED:{
        debug("FileTransfer:\tCan't pause a completed file;\n");
        break;
    }
    case FILE_TRANSFER_STATUS_KILLED:{
        debug("FileTransfer:\tCan't pause a killed file;\n");
        break;
    }
    }
    utox_update_user_file(file);
    //TODO free not freed data.
}

static void utox_run_file(FILE_TRANSFER *file, uint8_t us){
    if(file->status == FILE_TRANSFER_STATUS_ACTIVE){
        return;
    }
    if(us){
        if(file->status == FILE_TRANSFER_STATUS_NONE){
            file->status = FILE_TRANSFER_STATUS_ACTIVE;
        } else if(file->status == FILE_TRANSFER_STATUS_PAUSED_US){
            file->status = FILE_TRANSFER_STATUS_ACTIVE;
        } else if(file->status == FILE_TRANSFER_STATUS_PAUSED_BOTH) {
            file->status = FILE_TRANSFER_STATUS_PAUSED_THEM;
        } else if(file->status == FILE_TRANSFER_STATUS_PAUSED_THEM) {
            // Do nothing;
        } else if(file->status == FILE_TRANSFER_STATUS_BROKEN) {
            file->status = FILE_TRANSFER_STATUS_ACTIVE;
        } else {
            debug("FileTransfer:\tWe tried to run file from an unknown state! (%u)\n", file->status);
        }
    } else {
        if(file->status == FILE_TRANSFER_STATUS_PAUSED_US){
            // Do nothing;
        } else if(file->status == FILE_TRANSFER_STATUS_PAUSED_BOTH) {
            file->status = FILE_TRANSFER_STATUS_PAUSED_US;
        } else if(file->status == FILE_TRANSFER_STATUS_PAUSED_THEM) {
            file->status = FILE_TRANSFER_STATUS_ACTIVE;
        } else if(file->status == FILE_TRANSFER_STATUS_BROKEN) {
            file->status = FILE_TRANSFER_STATUS_ACTIVE;
        } else {
            debug("FileTransfer:\tThey tried to run incoming file from an unknown state! (%u)\n", file->status);
        }
    }
    utox_update_user_file(file);
    debug("utox_run_file\n");
}

static void utox_complete_file(FILE_TRANSFER *file){
    if(file->status == FILE_TRANSFER_STATUS_ACTIVE){
        if(file->incoming){
            if(file->in_memory){
                if(file->is_avatar){
                    utox_incoming_avatar(file->friend_number, file->avatar, file->size);
                    file->avatar = NULL;
                    file->size = 0;
                } else {
                    friend_recvimage(&friend[file->friend_number], (UTOX_PNG_IMAGE)file->memory, file->size);
                }
            } else { // Is a file
                fclose(file->file);
                file->ui_data->path = file->path;
            }
        } else {
            if(file->in_memory){
                // TODO, might want to do something here.
            } else { // Is a file
                fclose(file->file);
                file->ui_data->path = file->path;
            }
            if(friend[file->friend_number].transfer_count){
                /* Decrement if > 0 this might be better held inside local_control */
                --friend[file->friend_number].transfer_count;
            }
        }
        file->status = FILE_TRANSFER_STATUS_COMPLETED;
        utox_update_user_file(file);
    } else {
        debug("FileTransfer:\tUnable to complete file in non-active state (file:%u)\n", file->file_number);
    }
    utox_cleanup_file_transfers(file->friend_number, file->file_number);
    utox_file_save_ftinfo(file);
    utox_file_free_ftinfo(file);
    file->resume = 0; // We don't need to always be resetting this broken number anymore
}

void ft_friend_online(Tox *tox, uint32_t friend_number){
    for(int i = 0; i < MAX_FILE_TRANSFERS; i++){
        FILE_TRANSFER *file = &outgoing_transfer[friend_number][i];

        if(file->status > FILE_TRANSFER_STATUS_NONE && file->status < FILE_TRANSFER_STATUS_COMPLETED && file->path){
        /* Hack! added `&& file->path` because sometimes this is true when it shouldn't be TODO find out why! */
                outgoing_file_send_existing(tox, friend_number, i);
        } else {
            file->friend_number = friend_number;
            file->file_number   = i;
            file->incoming = 0;
            utox_file_load_ftinfo(file);
            if(file->path){
                /* If we got a path from utox_file_load we should try to resume! */
                outgoing_file_send_existing(tox, friend_number, i);
            }
        }
    }
    /* Else look in filetransfer info dir; */
}

void ft_friend_offline(Tox *tox, uint32_t friend_number){
    unsigned int i;
    for (i = 0; i < MAX_FILE_TRANSFERS; ++i) {
        utox_break_file(&incoming_transfer[friend_number][i]);
        utox_break_file(&outgoing_transfer[friend_number][i]);
    }
}

void file_transfer_local_control(Tox *tox, uint32_t friend_number, uint32_t file_number, TOX_FILE_CONTROL control){
    TOX_ERR_FILE_CONTROL error = 0;

    FILE_TRANSFER *info = get_file_transfer(friend_number, file_number);

    switch(control){
        case TOX_FILE_CONTROL_RESUME:
            if(info->status != FILE_TRANSFER_STATUS_ACTIVE){
                if(tox_file_control(tox, friend_number, file_number, control, &error)){
                    debug("FileTransfer:\tWe just resumed file (%u & %u)\n", friend_number, file_number);
                } else {
                    debug("FileTransfer:\tToxcore doesn't like us! (%u & %u)\n", friend_number, file_number);
                }
            } else {
                debug("FileTransfer:\tFile already active (%u & %u)\n", friend_number, file_number);
            }
            utox_run_file(info, 1);
            break;
        case TOX_FILE_CONTROL_PAUSE:
            debug("FileTransfer:\tWe just paused file (%u & %u)\n", friend_number, file_number);
            utox_pause_file(info, 1);
            tox_file_control(tox, friend_number, file_number, control, &error);
            break;
        case TOX_FILE_CONTROL_CANCEL:
            debug("FileTransfer:\tWe just killed file (%u & %u)\n", friend_number, file_number);
            tox_file_control(tox, friend_number, file_number, control, &error);

            utox_kill_file(info, 1);
            break;
    }
    if(error){
        if(error == TOX_ERR_FILE_CONTROL_FRIEND_NOT_CONNECTED){
            debug("FileTransfer:\tUnable to send command, Friend (%u) offline!\n", info->friend_number);
        } else {
            debug("FileTransfer:\tThere was an error(%u) sending the command, you probably want to see to that!\n", error);
        }
        debug("FileTransfer:\t\tFriend %u, and File %u. \n", friend_number, file_number);
    } else {
        utox_update_user_file(info);
    }
}

static void file_transfer_callback_control(Tox *UNUSED(tox), uint32_t friend_number, uint32_t file_number, TOX_FILE_CONTROL control, void *UNUSED(userdata)){

    FILE_TRANSFER *info = get_file_transfer(friend_number, file_number);

    switch(control){
        case TOX_FILE_CONTROL_RESUME:{
            debug("FileTransfer:\tFriend (%i) has resumed file (%i)\n", friend_number, file_number);
            utox_run_file(info, 0);
            break;
        }
        case TOX_FILE_CONTROL_PAUSE:{
            debug("FileTransfer:\tFriend (%i) has paused file (%i)\n", friend_number, file_number);
            utox_pause_file(info, 0);
            break;
        }
        case TOX_FILE_CONTROL_CANCEL:{
            if(info->avatar){
                debug("FileTransfer:\tFriend (%i) rejected avatar\n", friend_number);
            } else {
                debug("FileTransfer:\tFriend (%i) has canceled file (%i)\n", friend_number, file_number);
            }
            utox_kill_file(info, 0);
            break;
        }
    }
}

/* Function called by core with a new incoming file. */
static void incoming_file_callback_request(Tox *tox, uint32_t friend_number, uint32_t file_number, uint32_t kind, uint64_t file_size, const uint8_t *filename, size_t filename_length, void *user_data){
    /* First things first, get the file_id from core */
    uint8_t file_id[TOX_FILE_ID_LENGTH] = {0};
    tox_file_get_file_id(tox, friend_number, file_number, file_id, 0);
    /* access the correct memory location for this file */
    FILE_TRANSFER *file_handle = get_file_transfer(friend_number, file_number);
    if(!file_handle) {
        debug("FileTransfer:\tUnable to get memory handle for transfer, canceling friend/file number (%u/%u)\n", friend_number, file_number);
        tox_file_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL, 0);
        return;
    }

    switch(kind){
    case TOX_FILE_KIND_DATA:{
        debug("FileTransfer:\tNew incoming file from friend (%u) file number (%u)\nFileTransfer:\t\tfilename: %s\n", friend_number, file_number, filename);
        /* Auto accept if it's a utox-inline image, with the correct size */
        if(file_size < 1024 * 1024 * 4
            && filename_length == (sizeof("utox-inline.png") - 1)
            && memcmp(filename, "utox-inline.png", filename_length) == 0)
            {
            utox_build_file_transfer(file_handle, friend_number, file_number, file_size, 1, 1, 0,
                TOX_FILE_KIND_DATA, filename, filename_length, NULL, 0, NULL, tox);
            file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_RESUME);
        } else {
            utox_build_file_transfer(file_handle, friend_number, file_number, file_size, 1, 0, 0,
                TOX_FILE_KIND_DATA, filename, filename_length, NULL, 0, NULL, tox);
            /* Set UI values */
            file_handle->ui_data = message_add_type_file(file_handle);
            /* Set resuming info TODO MOVE TO AFTER FILE IS ACCEPED BY USER */
            file_handle->resume = utox_file_alloc_ftinfo(file_handle);
            /* Notify the user! */
            postmessage(FRIEND_FILE_NEW, 0, 0, file_handle);
        }
        break;
    }
    case TOX_FILE_KIND_AVATAR:{
        debug("FileTransfer:\tNew Avatar from friend (%u)\n", friend_number);
        /* Verify Avatar Size */
        if(file_size == 0){
            /* Size is 0, unset the avatar! */
            utox_incoming_avatar(friend_number, NULL, 0);
            debug("FileTransfer:\tAvatar Unset from friend(%u).\n", friend_number);
            file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL);
            return;
        } else if (file_size > UTOX_AVATAR_MAX_DATA_LENGTH){
            /* If incoming avatar is too big, cancel */
            file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL);
            debug("FileTransfer:\tAvatar from friend(%u) rejected, TOO LARGE (%u)\n", friend_number, (uint32_t)file_size);
            return;
        }
        /* Verify this is a new avatar */
        if((friend[friend_number].avatar.format > 0) && memcmp(friend[friend_number].avatar.hash, file_id, TOX_HASH_LENGTH) == 0) {
            debug("FileTransfer:\tAvatar from friend (%u) rejected: Same as Current\n", friend_number);
            file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL);
            return;
        }
        /* Avatar size is valid, and it's a new avatar, lets accept it */
        utox_build_file_transfer(file_handle, friend_number, file_number, file_size, 1, 1, 1, TOX_FILE_KIND_AVATAR, NULL, 0, NULL, 0, file_id, tox);
        file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_RESUME);
        return;
    }
    case TOX_FILE_KIND_EXISTING:{
        debug("FileTransfer:\tIncoming Existing file from friend (%u) \n", friend_number);
        /* Load saved information about this file */
        file_handle->friend_number = friend_number;
        file_handle->file_number   = file_number;
        memcpy(file_handle->file_id, file_id, TOX_FILE_ID_LENGTH);
        if(utox_file_load_ftinfo(file_handle)){
            /* We were able to load incoming file info from disk; validate date! */
            /* First, backup transfered size, because we're about to overwrite it*/
            uint64_t seek_size = file_handle->size_transferred;
            /* Try to reopen incoming file */
            FILE *file = fopen((const char*)file_handle->path, "rb");
            uint64_t size = 0;
            if(file){
                /* File exist, and is readable, now get current size! */
                debug("FileTransfer:\tCool file exists, let try to restart it.\n");
                fseeko(file, 0, SEEK_END); size = ftello(file); fclose(file);
                if(file_size != size){
                    debug("FileTransfer:\tIncoming size (%lu), and size on disk (%lu) mismatch, aborting!\n", size, file_size);
                    file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL);
                    return;
                }
                file = fopen((const char*)file_handle->path, "rb+");
                /* We have to open as rb+, so we also need to re-seek to the start! */
                fseeko(file, 0, SEEK_SET);
                /* We can read, but can we write? */
                if(file){
                    /* We can read and write, build a new file handle to work with! */
                    utox_build_file_transfer(file_handle, friend_number, file_number, size, 1, 0, 0,
                        TOX_FILE_KIND_DATA, filename, filename_length, file_handle->path, file_handle->path_length,
                        file_id, tox);
                    file_handle->file = file;
                    file_handle->size_transferred = seek_size;
                    /* TODO try to re-access the original message box for this file transfer, without segfaulting! */
                    file_handle->ui_data = message_add_type_file(file_handle);
                    file_handle->resume = utox_file_alloc_ftinfo(file_handle);
                    postmessage(FRIEND_FILE_NEW, 0, 0, file_handle);
                    TOX_ERR_FILE_SEEK error = 0;
                    tox_file_seek(tox, friend_number, file_number, seek_size, &error);
                    debug("FileTransfer:\tseek %i\n", error);
                    file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_RESUME);
                    return;
                } else {
                    debug("FileTransfer:\tFile opened for reading, but unable to get write access, canceling file!\n");
                    file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL);
                    return;
                }
            } else {
                /* The file doesn't exist on disk where we expected, let's prompt the user to accept it as a new file */
                debug("FileTransfer:\tUnable to open that file; treating as new.\n");
                utox_build_file_transfer(file_handle, friend_number, file_number, size, 1, 0, 0, TOX_FILE_KIND_DATA,
                    filename, filename_length, NULL, 0, file_id, tox);
                file_handle->ui_data = message_add_type_file(file_handle);
                file_handle->resume = utox_file_alloc_ftinfo(file_handle);
                postmessage(FRIEND_FILE_NEW, 0, 0, file_handle);
                return;
            }
        } else {
            debug("FileTransfer:\tWe don't know anything about this file, so for safety we're just going to reject it!.\n");
            file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL);
            return;
        }
        break; /*We shouldn't reach here, but just in case! */
    } /* Last case */
    } /* Switch */
}

static void incoming_file_callback_chunk(Tox *UNUSED(tox), uint32_t friend_number, uint32_t file_number, uint64_t position, const uint8_t *data, size_t length, void *UNUSED(user_data)){
    // debug("FileTransfer:\tIncoming chunk friend(%u), file(%u), start(%u), end(%u), \n", friend_number, file_number, position, length);

    FILE_TRANSFER *file_handle = get_file_transfer(friend_number, file_number);

    if(length == 0){
        debug("\nFileTransfer:\tIncoming transfer is done (%u & %u)\n", friend_number, file_number);
        utox_complete_file(file_handle);
        return;
    }

    if(file_handle->in_memory) {
        if(file_handle->is_avatar){
            memcpy(file_handle->avatar + position, data, length);
        } else {
            memcpy(file_handle->memory + position, data, length);
        }
    } else {
            // Removed until we can better implement temp files
            // if(file_handle->in_tmp_loc){
            //     fseeko(file_handle->tmp_file, position, SEEK_SET);
            //     size_t write_size = fwrite(data, 1, length, file_handle->tmp_file);
            //     if(write_size != length){
            //         debug("\n\nFileTransfer:\tERROR WRITING DATA TO TEMP FILE! (%u & %u)\n\n", friend_number, file_number);
            //         tox_postmessage(TOX_FILE_INCOMING_CANCEL, friend_number, file_number, NULL);
            //         return;
            //     }
            // }
        if(file_handle->file) {
            while(!file_lock(file_handle->file, position, length)){
                debug("FileTransfer:\tCan't get lock, sleeping...\n");
                yieldcpu(10);
                // If you get a bug report about this hanging utox, just disable it, it's unlikely to be needed!
            }
            fseeko(file_handle->file, position, SEEK_SET);
            size_t write_size = fwrite(data, 1, length, file_handle->file);
            fflush(file_handle->file);
            file_unlock(file_handle->file, position, length);
            if(write_size != length){
                debug("\n\nFileTransfer:\tERROR WRITING DATA TO FILE! (%u & %u)\n\n", friend_number, file_number);
                tox_postmessage(TOX_FILE_INCOMING_CANCEL, friend_number, file_number, NULL);
                return;
            }
        } else {
            debug("FileTransfer:\tFile Handle failed!\n");
            tox_postmessage(TOX_FILE_INCOMING_CANCEL, friend_number, file_number, NULL);
            return;
        }
    }
    file_handle->size_transferred += length;
    // TODO dirty hack, this needs to be replaced
        // moved it cal_speed() // utox_file_save_ftinfo(file_handle);
    calculate_speed(file_handle);
}

void outgoing_file_send_new(Tox *tox, uint32_t friend_number, uint8_t *path, const uint8_t *filename, size_t filename_length){
    debug("FileTransfer:\tStarting outgoing file to friend %u. (filename, %s)\n", friend_number, filename);

    //     FILE_TRANSFER *file_handle = active_transfer[friend_number][file_number]; TODO
    if(friend[friend_number].transfer_count >= MAX_FILE_TRANSFERS) {
        debug("FileTransfer:\tMaximum outgoing file sending limit reached(%u/%u) for friend(%u). ABORTING!\n",
                                            friend[friend_number].transfer_count, MAX_FILE_TRANSFERS, friend_number);
        return;
    }

    FILE *file = fopen((char*)path, "rb");
    if(!file) {
        debug("FileTransfer:\tUnable to open file for reading!\n");
        return;
    }

    TOX_ERR_FILE_SEND error;
    const uint8_t *file_id = NULL;

    uint64_t file_size = 0;
    fseeko(file, 0, SEEK_END);
    file_size = ftello(file);
    fseeko(file, 0, SEEK_SET);

    int file_number = tox_file_send(tox, friend_number, TOX_FILE_KIND_DATA, file_size, file_id, filename, filename_length, &error);

    if(file_number != -1) {
        FILE_TRANSFER *file_handle = get_file_transfer(friend_number, file_number);

        utox_build_file_transfer(file_handle, friend_number, file_number, file_size, 0, 0, 0, TOX_FILE_KIND_DATA,
                                filename, filename_length, path, strlen((char*)path), NULL, tox);

        file_handle->file = file;
        if(!file_handle->file) {
            debug("FileTransfer:\tUnable to regain file for reading!\n");
            file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL);
            return;
        }

        file_handle->ui_data = message_add_type_file(file_handle);
        file_handle->resume = utox_file_alloc_ftinfo(file_handle);

        // TODO move this to utox_file_run(ish)
        ++friend[friend_number].transfer_count;

        debug("Sending file %d of %d(max) to friend(%d).\n", friend[friend_number].transfer_count, MAX_FILE_TRANSFERS, friend_number);

        // Create a new msg for the UI and save it's pointer
        postmessage(FRIEND_FILE_NEW, 0, 0, file_handle);
    } else {
        debug("tox_file_send() failed\n");
    }
}

void outgoing_file_send_existing(Tox *tox, uint32_t friend_number, uint32_t file_number){
    FILE_TRANSFER *file_handle = get_file_transfer(friend_number, file_number);
    debug("FileTransfer:\tRestarting outgoing file to friend %u. (filename, %s)\n", file_handle->friend_number, file_handle->path);

    if(friend[file_handle->friend_number].transfer_count >= MAX_FILE_TRANSFERS) {
        debug("FileTransfer:\tMaximum outgoing file sending limit reached(%u/%u) for friend(%u). ABORTING!\n",
            friend[file_handle->friend_number].transfer_count, MAX_FILE_TRANSFERS, file_handle->friend_number);
        return;
    }

    FILE *file;
    if(!file_handle->file) {
        debug("FileTransfer:\tUnable to open file for reading, trying to re-access!\n");
        if(file_handle->path_length){
            file = fopen((const char*)file_handle->path, "rb");
            if(file){
                file_handle->file = file;
                fseeko(file, 0, SEEK_END);
                file_handle->size = ftello(file);
                fseeko(file, 0, SEEK_SET);
            } else {
                debug("FileTransfer:\tUnable to re-access file, must fail!\n");
                file_transfer_local_control(tox, file_handle->friend_number, file_handle->file_number, TOX_FILE_CONTROL_CANCEL);
                return;
            }
        }
    }

    TOX_ERR_FILE_SEND error;
    const uint8_t *file_id = file_handle->file_id;
    uint8_t *p = file_handle->path, *name = file_handle->path;
    while(*p != '\0') {
        if(*p == '/' || *p == '\\') {
            name = p + 1;
        }
        p++;
    }

    int new_file_number = tox_file_send(tox, file_handle->friend_number, TOX_FILE_KIND_EXISTING, file_handle->size, file_id, name, p - name, &error);
    if(new_file_number != -1) {
        FILE_TRANSFER *file_handle_new = get_file_transfer(file_handle->friend_number, new_file_number);

        uint64_t tmp_transfer_size = file_handle->size_transferred;

        utox_build_file_transfer(file_handle_new, friend_number, new_file_number, file_handle->size, 0, 0, 0,
            TOX_FILE_KIND_EXISTING, name, p - name, file_handle->path, file_handle->path_length, file_id, tox);

        file_handle_new->file = file;
        file_handle_new->size_transferred = tmp_transfer_size;
        if(file_handle->ui_data){
            file_handle_new->ui_data = file_handle->ui_data;
        } else {
            file_handle_new->ui_data = message_add_type_file(file_handle_new);
        }
        file_handle_new->resume = utox_file_alloc_ftinfo(file_handle_new);

        // TODO, move into RUN
        ++friend[friend_number].transfer_count;

        debug("Resending file %d of %d(max) to friend(%d).\n", friend[friend_number].transfer_count, MAX_FILE_TRANSFERS, friend_number);
        utox_update_user_file(file_handle_new);
    } else {
        debug("tox_file_send() failed\n");
    }
}

void outgoing_file_send_inline(Tox *tox, uint32_t friend_number, uint8_t *image, size_t image_size){

    debug("FileTransfer:\tStarting outgoing inline to friend %u.\n", friend_number);

    if(friend[friend_number].transfer_count >= MAX_FILE_TRANSFERS) {
        debug("FileTransfer:\tMaximum outgoing file limit reached(%u/%u) for friend(%u). ABORTING!\n", friend[friend_number].transfer_count, MAX_FILE_TRANSFERS, friend_number);
        return;
    }
    if(!image) {
        debug("FileTransfer:\tUnable to use *image!\n");
        return;
    }

    TOX_ERR_FILE_SEND error;
    uint8_t file_id[TOX_FILE_ID_LENGTH] = {0};
    uint8_t *filename;
    size_t filename_length = 0;
    if(!tox_hash(file_id, image, image_size)){
        debug("FileTransfer:\tUnable to get hash for image!\n");
        return;
    }

    if( image_size < 1024 * 1024 * 4 ) {
        filename_length = sizeof("utox-inline.png") - 1;
        filename = malloc(filename_length + 1);
        memcpy(filename, "utox-inline.png", filename_length);
    } else {
        filename_length = sizeof("utox-image.png") - 1;
        filename = malloc(filename_length + 1);
        memcpy(filename, "utox-image.png", filename_length);
    }
    filename[filename_length] = 0; // null term for the string // TODO WRAP THIS INTO BUILD

    uint32_t file_number = tox_file_send(tox, friend_number, TOX_FILE_KIND_DATA, image_size, file_id, (const uint8_t*)filename, filename_length, &error);

    if(file_number != -1) {
        FILE_TRANSFER *file_handle = get_file_transfer(friend_number, file_number);

        utox_build_file_transfer(file_handle, friend_number, file_number, image_size, 0, 1, 0, TOX_FILE_KIND_DATA,
                                file_id, TOX_FILE_ID_LENGTH, NULL, 0, file_id, tox);

        file_handle->kind = TOX_FILE_KIND_DATA;
        file_handle->resume = 0;

        memcpy(file_handle->memory, image, image_size);

        utox_pause_file(file_handle, 1);
        ++friend[friend_number].transfer_count;
        debug("FileTransfer:\tSending image file %d of %d(max) to friend(%d).\n", friend[friend_number].transfer_count, MAX_FILE_TRANSFERS, friend_number);
        // Create a new msg for the UI and save it's pointer
    } else {
        free(image);
        free(filename);
        debug("tox_file_send() failed for image\n");
        return;
    }
    free(filename);
    return;
}

int outgoing_file_send_avatar(Tox *tox, uint32_t friend_number, uint8_t *avatar, size_t avatar_size){

    debug("FileTransfer:\tStarting avatar to friend %u.\n", friend_number);

    if(friend[friend_number].transfer_count >= MAX_FILE_TRANSFERS) {
        debug("FileTransfer:\tMaximum outgoing file sending limit reached(%u/%u) for friend(%u). ABORTING!\n",
                                            friend[friend_number].transfer_count, MAX_FILE_TRANSFERS, friend_number);
        return 1;
    }

    if(!avatar && avatar_size) {
        debug("FileTransfer:\tUnable to use *avatar!\n");
        return 1;
    }


    TOX_ERR_FILE_SEND error;
    uint8_t file_id[TOX_HASH_LENGTH] = {0};

    if(!tox_hash(file_id, avatar, avatar_size)){
        debug("FileTransfer:\tUnable to get hash for avatar!\n");
        return 1;
    }

    int file_number = tox_file_send(tox, friend_number, TOX_FILE_KIND_AVATAR, avatar_size, file_id, NULL, 0, &error);

    if(file_number != -1) {
        FILE_TRANSFER *file_handle = get_file_transfer(friend_number, file_number);

        utox_build_file_transfer(file_handle, friend_number, file_number, avatar_size, 0, 1, 1, TOX_FILE_KIND_AVATAR,
                                NULL, 0, NULL, 0, (const uint8_t*)file_id, tox);

        file_handle->kind = TOX_FILE_KIND_AVATAR;
        file_handle->status = FILE_TRANSFER_STATUS_PAUSED_THEM;
        file_handle->resume = 0;

        memcpy(file_handle->avatar, avatar, avatar_size);

        ++friend[friend_number].transfer_count;
        // Create a new msg for the UI and save it's pointer
    } else {
        debug("tox_file_send() failed for avatar\n");
        return 1;
    }
    return 0;
}

static void outgoing_file_callback_chunk(Tox *tox, uint32_t friend_number, uint32_t file_number, uint64_t position, size_t length, void *UNUSED(user_data)){

    // debug("FileTransfer:\tChunk requested for friend_id (%u), and file_id (%u). Start (%lu), End (%zu).\r", friend_number, file_number, position, length);

    FILE_TRANSFER *file_handle = get_file_transfer(friend_number, file_number);

    if(length == 0){
        debug("FileTransfer:\tOutgoing transfer is done (%u & %u)\n", friend_number, file_number);
        utox_complete_file(file_handle);
        return;
    }

    uint8_t buffer[length];
    size_t read_size = 0;

    if(file_handle->in_memory){
        // Memory
        if(file_handle->is_avatar){
            if(file_handle->size < length){
                debug("FileTransfer:\tAvatar size mismatch!\n");
                return;
            }
            memcpy(buffer, file_handle->avatar + position, length);
            read_size = length; /* We hope!! */
        } else {
            if(file_handle->size < length){
                debug("FileTransfer:\tMemory size mismatch!\n");
                return;
            }
            memcpy(buffer, file_handle->memory + position, length);
            read_size = length; /* We hope!! */
        }
    } else {
        // File
        FILE *file = file_handle->file;
        if(file){
            fseeko(file, position, SEEK_SET);
            read_size = fread(buffer, 1, length, file);
            // debug("File okay\n");
        }
    }

    if(read_size != length){
        debug("FileTransfer:\tERROR READING FILE! (%u & %u)\n", friend_number, file_number);
        debug("FileTransfer:\t\tSize (%lu), Position (%lu), Length(%zu), Read_size (%zu), size_transferred (%zu).\n",
            file_handle->size, position, length, read_size, file_handle->size_transferred);
        file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL);
        return;
    }

    TOX_ERR_FILE_SEND_CHUNK error;

    tox_file_send_chunk(tox, friend_number, file_number, position, buffer, length, &error);
    file_handle->size_transferred += length;

    calculate_speed(file_handle);
}

int utox_file_start_write(uint32_t friend_number, uint32_t file_number, void *filepath){
    FILE_TRANSFER *file_handle = get_file_transfer(friend_number, file_number);
    file_handle->file = fopen(filepath, "wb");
    if(!file_handle->file) {
        debug("FileTransfer:\tThe file we're supposed to write to couldn't be opened\n");
        free(filepath);
        utox_break_file(file_handle);
        return -1;
    }
    if(resize_file(file_handle->file, file_handle->size) != 0){
        debug("FileTransfer:\tThe file size was unable to be changed!\n");
        free(filepath);
        fclose(file_handle->file);
        utox_break_file(file_handle);
        remove(filepath);
        return -1;
    }

    file_handle->path = (uint8_t*)strdup((const char*)filepath);
    file_handle->path_length = strlen(filepath);
    free(filepath);
            // Removed until we can find a better way of working this in;
            // if(file_handle->in_tmp_loc){
            //     fseeko(file_handle->tmp_file, 0, SEEK_SET);
            //     fseeko(file_handle->file, 0, SEEK_SET);
            //     fwrite(file_handle->tmp_file, 1, file_handle->size_transferred, file_handle->file);
            //     fclose(file_handle->tmp_file);
            //     file_handle->in_tmp_loc = 0;
            //     // free(file_handle->tmp_path); // Freed by xlib probably...
            //     // TODO unlink();
            //     debug("FileTransfer: Data copied from tmp_file to save_file\n");
            // }
    return 0;
}


void utox_set_callbacks_for_transfer(Tox *tox){
    /* Incoming files */
        /* This is the callback for a new incoming file. */
        tox_callback_file_recv(tox, incoming_file_callback_request, NULL);
        /* This is the callback with friend's actions for a file */
        tox_callback_file_recv_control(tox, file_transfer_callback_control, NULL);
        /* This is the callback with a chunk data for a file. */
        tox_callback_file_recv_chunk(tox, incoming_file_callback_chunk, NULL);
    /* Outgoing files */
        /* This is the callback send to request a new file chunk */
        tox_callback_file_chunk_request(tox, outgoing_file_callback_chunk, NULL);
}

void utox_cleanup_file_transfers(uint32_t friend_number, uint32_t file_number){
    debug("FileTransfer:\tCleaning up file transfers!\n");
    FILE_TRANSFER *transfer = get_file_transfer(friend_number, file_number);
    if(transfer->name)
        free(transfer->name);

    if(transfer->memory){
        if(transfer->avatar){
            free(transfer->avatar);
        } else {
            free(transfer->memory);
        }
    }

    if(transfer->file){
        fclose(transfer->file);
    }
}

void utox_file_save_ftinfo(FILE_TRANSFER *file){
    if(!file->saveinfo){
        return;
    }
    fwrite(file, sizeof(*file), 1, file->saveinfo);
    fwrite(file->path, sizeof(uint8_t), file->path_length, file->saveinfo);
    fflush(file->saveinfo);
    fseeko(file->saveinfo, 0, SEEK_SET);
}

_Bool utox_file_load_ftinfo(FILE_TRANSFER *file){
    uint8_t path[UTOX_FILE_NAME_LENGTH];
    size_t path_length;
    uint32_t size_read;

    path_length = datapath(path);

    if(file->incoming){
        uint8_t hex_id[TOX_FILE_ID_LENGTH * 2];
        fid_to_string(hex_id, file->file_id);
        memcpy(path + path_length, hex_id, TOX_FILE_ID_LENGTH * 2);
        strcpy((char*)path + (path_length + TOX_FILE_ID_LENGTH * 2), ".ftinfo");
    } else {
        uint8_t hex_id[TOX_PUBLIC_KEY_SIZE * 2];
        cid_to_string(hex_id, friend[file->friend_number].cid);
        memcpy(path + path_length, hex_id, TOX_PUBLIC_KEY_SIZE * 2);
        sprintf((char*)path + (path_length + TOX_PUBLIC_KEY_SIZE * 2), "%02i.ftoutfo", file->file_number % 100);
    }

    void *load = file_raw((char*)path, &size_read);

    if(!load) {
        if(file->incoming){
            debug("FileTransfer:\tUnable to load saved info... uTox can't resume file %.*s\n", (uint32_t)file->name_length, file->name);
        }
        file->status = 0;
        return 0;
    }

    FILE_TRANSFER *info = calloc(1, sizeof(*info));
    memcpy(info, load, sizeof(*info));
    info->path_length = (size_read - sizeof(*info));
    info->path = malloc(info->path_length + 1);
    memcpy(info->path, load + (sizeof(*info)), info->path_length);
    info->path[info->path_length] = 0;

    info->friend_number = file->friend_number;
    info->file_number   = file->file_number;
    info->name          = NULL;
    info->name_length   = 0;
    info->ui_data       = NULL;
    info->file          = NULL;
    info->saveinfo = fopen((const char*)path, "wb");

    memcpy(file, info, sizeof(*file));
    return 1;
}
