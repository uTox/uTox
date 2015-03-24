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

/* Create a FILE_TRANSFER struct with the supplied data. */
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

/* Copy the data from active FILE_TRANSFER, and pass it along to the UI with it's update. */
static void utox_update_user_file(FILE_TRANSFER *file){
    FILE_TRANSFER *file_copy = calloc(1, sizeof(FILE_TRANSFER));

    memcpy(file_copy, file, sizeof(FILE_TRANSFER));
    postmessage(FRIEND_FILE_UPDATE, 0, 0, file_copy);
}

static void utox_new_user_file(FILE_TRANSFER *file){
    FILE_TRANSFER *file_copy = calloc(1, sizeof(FILE_TRANSFER));

    memcpy(file_copy, file, sizeof(FILE_TRANSFER));
    postmessage(FRIEND_FILE_NEW, 0, 0, file_copy);
}

/* Calculate the transfer speed for the UI. */
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

/* Create the file transfer resume info file. */
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

    file->saveinfo = fopen((const char*)path, "wb");
    if(!file->saveinfo) {
        debug("FileTransfer:\tUnable to save file info... uTox can't resume file %.*s\n", (uint32_t)file->name_length, file->name);
        return 0;
    }
    debug("FileTransfer:\t.ftinfo for file %.*s set; ready to resume!\n", (uint32_t)file->name_length, file->name);
    utox_file_save_ftinfo(file);
    return 1;
}

/* Free/Remove/Unlink the file transfer resume info file. */
static void utox_file_free_ftinfo(FILE_TRANSFER *file){
    if(file->saveinfo){
        fclose(file->saveinfo);
        file->saveinfo = NULL;
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

    debug("Removing. %s\n", path);
    remove((const char*)path);
}

static void utox_file_ftoutfo_move(unsigned int friend_number, unsigned int source_num, unsigned int dest_num){
    uint8_t path_src[UTOX_FILE_NAME_LENGTH], path_dst[UTOX_FILE_NAME_LENGTH];
    size_t path_length;
    path_length = datapath(path_src);
    uint8_t hex_id[TOX_PUBLIC_KEY_SIZE * 2];
    cid_to_string(hex_id, friend[friend_number].cid);
    memcpy(path_src + path_length, hex_id, TOX_PUBLIC_KEY_SIZE * 2);
    memcpy(path_dst, path_src, (path_length + TOX_PUBLIC_KEY_SIZE * 2));
    sprintf((char*)path_src + (path_length + TOX_PUBLIC_KEY_SIZE * 2), "%02i.ftoutfo", source_num % 100);
    sprintf((char*)path_dst + (path_length + TOX_PUBLIC_KEY_SIZE * 2), "%02i.ftoutfo", dest_num % 100);
    rename((char*)path_src, (char*)path_dst);
}

/* Cancel active file. */
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
    if(file->resume){
        utox_file_save_ftinfo(file);
        utox_file_free_ftinfo(file);
    }
    utox_cleanup_file_transfers(file->friend_number, file->file_number);
}

/* Break active file, (when a friend goes offline). */
static void utox_break_file(FILE_TRANSFER *file){
    if(file->status == FILE_TRANSFER_STATUS_NONE){
        return utox_kill_file(file, 1); /* We don't save unstarted files */
    } else if(file->status == FILE_TRANSFER_STATUS_COMPLETED || file->status == FILE_TRANSFER_STATUS_KILLED) {
        /* We don't touch these files! */
        return;
    }
    if(friend[file->friend_number].transfer_count){
        /* Decrement if > 0 */
        --friend[file->friend_number].transfer_count;
    }
    file->status = FILE_TRANSFER_STATUS_BROKEN;
    utox_update_user_file(file);
    utox_file_save_ftinfo(file);
    utox_cleanup_file_transfers(file->friend_number, file->file_number);
}

/* Pause active file. */
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

/* Start/Resume active file. */
static void utox_run_file(FILE_TRANSFER *file, uint8_t us){
    if(file->status == FILE_TRANSFER_STATUS_ACTIVE){
        return;
    }
    if(us){
        if(file->status == FILE_TRANSFER_STATUS_NONE){
            file->status = FILE_TRANSFER_STATUS_ACTIVE;
            /* Set resuming info TODO MOVE TO AFTER FILE IS ACCEPED BY USER */
            if(file->resume == 0){
                file->resume = utox_file_alloc_ftinfo(file);
            }
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
    // debug("utox_run_file\n");
}

static void decode_inline_png(uint32_t friend_id, uint8_t *data, uint64_t size)
{
    //TODO: start a new thread and decode the png in it.
    uint16_t width, height;
    UTOX_NATIVE_IMAGE *native_image = png_to_image(data, size, &width, &height, 0);
    if (UTOX_NATIVE_IMAGE_IS_VALID(native_image)) {
        void *msg = malloc(sizeof(uint16_t) * 2 + sizeof(uint8_t *));
        memcpy(msg, &width, sizeof(uint16_t));
        memcpy(msg + sizeof(uint16_t), &height, sizeof(uint16_t));
        memcpy(msg + sizeof(uint16_t) * 2, &native_image, sizeof(uint8_t *));
        postmessage(FRIEND_INLINE_IMAGE, friend_id, 0, msg);
    }
}

/* Complete active file, (when the whole file transfer is successful). */
static void utox_complete_file(FILE_TRANSFER *file){
    if(file->status == FILE_TRANSFER_STATUS_ACTIVE){
        if(file->incoming){
            if(file->in_memory){
                if(file->is_avatar){
                    utox_incoming_avatar(file->friend_number, file->avatar, file->size);
                    file->avatar = NULL;
                    file->size = 0;
                } else {
                    decode_inline_png(file->friend_number, file->memory, file->size);
                }
            } else { // Is a file
                file->ui_data->path = (uint8_t*)strdup((const char*)file->path);
            }
        } else {
            if(file->in_memory){
                // TODO, might want to do something here.
            } else { // Is a file
                file->ui_data->path = (uint8_t*)strdup((const char*)file->path);
            }
            if(friend[file->friend_number].transfer_count){
                /* Decrement if > 0 */
                --friend[file->friend_number].transfer_count;
            }
        }
        file->status = FILE_TRANSFER_STATUS_COMPLETED;
        utox_update_user_file(file);
    } else {
        debug("FileTransfer:\tUnable to complete file in non-active state (file:%u)\n", file->file_number);
    }
    debug("\nFileTransfer:\tIncoming transfer is done (%u & %u)\n", file->friend_number, file->file_number);
    utox_file_save_ftinfo(file);
    utox_file_free_ftinfo(file);
    utox_cleanup_file_transfers(file->friend_number, file->file_number);
    file->resume = 0; // We don't need to always be resetting this broken number anymore
}

/* Friend has come online, restart our outgoing transfers to this friend. */
void ft_friend_online(Tox *tox, uint32_t friend_number){
    for(int i = 0; i < MAX_FILE_TRANSFERS; i++){
        FILE_TRANSFER *file = calloc(1, sizeof(*file));
        file->friend_number = friend_number;
        file->file_number   = i;
        file->incoming = 0;
        utox_file_load_ftinfo(file);
        if(file->path){
            /* If we got a path from utox_file_load we should try to resume! */
            uint32_t f_n = outgoing_file_send(tox, friend_number, NULL, (uint8_t*)file, sizeof(*file), TOX_FILE_KIND_EXISTING);

            if (f_n != UINT32_MAX && f_n != i) {
                 utox_file_ftoutfo_move(friend_number, i, f_n);
            }
        }

        free(file);
    }
    /* Else look in filetransfer info dir; */
}

/* Friend has gone offline, break our outgoing transfers to this friend. */
void ft_friend_offline(Tox *tox, uint32_t friend_number){
    unsigned int i;
    for (i = 0; i < MAX_FILE_TRANSFERS; ++i) {
        utox_break_file(&incoming_transfer[friend_number][i]);
        utox_break_file(&outgoing_transfer[friend_number][i]);
    }
}

/* Local command callback to change a file status. */
void file_transfer_local_control(Tox *tox, uint32_t friend_number, uint32_t file_number, TOX_FILE_CONTROL control){
    TOX_ERR_FILE_CONTROL error = 0;
    FILE_TRANSFER *info = get_file_transfer(friend_number, file_number);
    switch(control){
        case TOX_FILE_CONTROL_RESUME:{
            if(info->status != FILE_TRANSFER_STATUS_ACTIVE){
                if(friend[friend_number].transfer_count < MAX_FILE_TRANSFERS){
                    if(tox_file_control(tox, friend_number, file_number, control, &error)){
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
            if(info->status != FILE_TRANSFER_STATUS_PAUSED_US || info->status != FILE_TRANSFER_STATUS_PAUSED_BOTH ){
                if(tox_file_control(tox, friend_number, file_number, control, &error)){
                    debug("FileTransfer:\tWe just paused file (%u & %u)\n", friend_number, file_number);
                } else {
                    debug("FileTransfer:\tToxcore doesn't like us! (%u & %u)\n", friend_number, file_number);
                }
            } else {
                debug("FileTransfer:\tFile already paused (%u & %u)\n", friend_number, file_number);
            }
            utox_pause_file(info, 1);
            break;
        case TOX_FILE_CONTROL_CANCEL:{
            if(info->status != FILE_TRANSFER_STATUS_KILLED){
                if(tox_file_control(tox, friend_number, file_number, control, &error)){
                    debug("FileTransfer:\tWe just killed file (%u & %u)\n", friend_number, file_number);
                } else {
                    debug("FileTransfer:\tToxcore doesn't like us! (%u & %u)\n", friend_number, file_number);
                }
            } else {
                debug("FileTransfer:\tFile already killed (%u & %u)\n", friend_number, file_number);
            }
            if(info->friend_number == friend_number){
                utox_kill_file(info, 1);
            }
            break;
        }
    }
    /* Do something with the error! */
    if(error){
        if(error == TOX_ERR_FILE_CONTROL_FRIEND_NOT_CONNECTED){
            debug("FileTransfer:\tUnable to send command, Friend (%u) offline!\n", info->friend_number);
        } else {
            debug("FileTransfer:\tThere was an error(%u) sending the command, you probably want to see to that!\n", error);
        }
    }
}

/* Remote command callback for friends to change a file status */
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

/* Function called by core with a new file send request from a friend. */
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
        utox_build_file_transfer(file_handle, friend_number, file_number, file_size, 1, 1, 1, TOX_FILE_KIND_AVATAR, NULL, 0, NULL, 0, NULL, tox);
        file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_RESUME);
        return;
    }
    case TOX_FILE_KIND_DATA:{
        /* Load saved information about this file */
        file_handle->friend_number = friend_number;
        file_handle->file_number   = file_number;
        memcpy(file_handle->file_id, file_id, TOX_FILE_ID_LENGTH);
        if(utox_file_load_ftinfo(file_handle)){
            debug("FileTransfer:\tIncoming Existing file from friend (%u) \n", friend_number);
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
                    free(file_handle->path);
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
                        NULL, tox);
                    file_handle->file = file;
                    file_handle->size_transferred = seek_size;
                    /* TODO try to re-access the original message box for this file transfer, without segfaulting! */
                    file_handle->ui_data = message_add_type_file(file_handle);
                    file_handle->resume = utox_file_alloc_ftinfo(file_handle);
                    utox_new_user_file(file_handle);
                    TOX_ERR_FILE_SEEK error = 0;
                    tox_file_seek(tox, friend_number, file_number, seek_size, &error);
                    debug("FileTransfer:\tseek %i\n", error);
                    file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_RESUME);
                    return;
                } else {
                    debug("FileTransfer:\tFile opened for reading, but unable to get write access, canceling file!\n");
                    file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL);
                    free(file_handle->path);
                    return;
                }
            }
        }
        /* The file doesn't exist on disk where we expected, let's prompt the user to accept it as a new file */
        debug("FileTransfer:\tNew incoming file from friend (%u) file number (%u)\nFileTransfer:\t\tfilename: %s\n", friend_number, file_number, filename);
        /* Auto accept if it's a utox-inline image, with the correct size */
        if(file_size < 1024 * 1024 * 4
            && filename_length == (sizeof("utox-inline.png") - 1)
            && memcmp(filename, "utox-inline.png", filename_length) == 0)
            {
            utox_build_file_transfer(file_handle, friend_number, file_number, file_size, 1, 1, 0,
                TOX_FILE_KIND_DATA, filename, filename_length, NULL, 0, NULL, tox);

            file_handle->ui_data = message_add_type_file(file_handle);
            file_handle->resume = 0;
            /* Notify the user! */
            utox_new_user_file(file_handle);
            file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_RESUME);
        } else {
            utox_build_file_transfer(file_handle, friend_number, file_number, file_size, 1, 0, 0,
                TOX_FILE_KIND_DATA, filename, filename_length, NULL, 0, NULL, tox);
            /* Set UI values */
            file_handle->ui_data = message_add_type_file(file_handle);
            file_handle->resume = 0;
            /* Notify the user! */
            utox_new_user_file(file_handle);
        }
        break; /*We shouldn't reach here, but just in case! */
    } /* last case */
    } /* switch */
}

/* Called by toxcore to deliver the next chunk of incoming data. */
static void incoming_file_callback_chunk(Tox *UNUSED(tox), uint32_t friend_number, uint32_t file_number, uint64_t position, const uint8_t *data, size_t length, void *UNUSED(user_data)){
    // debug("FileTransfer:\tIncoming chunk friend(%u), file(%u), start(%u), end(%u), \n", friend_number, file_number, position, length);

    FILE_TRANSFER *file_handle = get_file_transfer(friend_number, file_number);

    if(length == 0){
        debug_v2(1, "\nFileTransfer:\tIncoming transfer is done (%u & %u)\n", friend_number, file_number);
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

/* Returns file number on success, UINT32_MAX on failure. */
uint32_t outgoing_file_send(Tox *tox, uint32_t friend_number, uint8_t *path, uint8_t *file_data, size_t file_data_size, uint32_t kind){
    /* Enforce max transfer count! */
    if(friend[friend_number].transfer_count >= MAX_FILE_TRANSFERS) {
        debug("FileTransfer:\tMaximum outgoing file sending limit reached(%u/%u) for friend(%u). ABORTING!\n",
            friend[friend_number].transfer_count, MAX_FILE_TRANSFERS, friend_number);
        return UINT32_MAX;
    } else {
        friend[friend_number].transfer_count++;
    }
    /* Declare vars */
    uint32_t file_number;
    TOX_ERR_FILE_SEND error;
    uint8_t file_id[TOX_HASH_LENGTH] = {0};
    uint64_t file_size = 0, transfer_size = 0;
    FILE *file = NULL;
    uint8_t memory = 0, avatar = 0;
    uint8_t *filename;
    size_t path_length = 0, filename_length = 0;


    switch (kind){
    case TOX_FILE_KIND_DATA:{
        if(path){
            /* It's a file from the disk. */
            file = fopen((char*)path, "rb");
            if(!file) {
                debug("FileTransfer:\tUnable to open file for reading!\n");
                return UINT32_MAX;
            }
            /* Get file size. */
            fseeko(file, 0, SEEK_END);
            file_size = ftello(file);
            path_length = strlen((char*)path);

            debug("FileTransfer:\tStarting outgoing file to friend %u. (filename, %s)\n", friend_number, file_data);
            file_number = tox_file_send(tox, friend_number, TOX_FILE_KIND_DATA, file_size, NULL, file_data,
                file_data_size, &error);

            filename        = file_data;
            filename_length = file_data_size;

        } else {
            /* It's a file from memory, probably an image. */
            if(!file_data){
                debug("FileTransfer:\tUnable to use *image!\n");
                return UINT32_MAX;
            }
            /* Generate the file name. */
            if(file_data_size < 1024 * 1024 * 4){
                filename_length = sizeof("utox-inline.png") - 1;
                filename = malloc(filename_length + 1);
                memcpy(filename, "utox-inline.png", filename_length + 1);
            } else {
                filename_length = sizeof("utox-image.png")  - 1;
                filename = malloc(filename_length + 1);
                memcpy(filename, "utox-image.png",  filename_length + 1);
            }

            debug("FileTransfer:\tStarting outgoing image to friend %u.\n", friend_number);
            file_number = tox_file_send(tox, friend_number, TOX_FILE_KIND_DATA, file_data_size, NULL, filename,
                filename_length, &error);

            file_size = file_data_size;
            memory = 1;
        }
        break;
    }
    case TOX_FILE_KIND_EXISTING:{
        debug("FileTransfer:\tAttempting to restart existing transfer to friend%u.\n",friend_number);
        /* Load the saved info. */
        FILE_TRANSFER *existing_file_info = (FILE_TRANSFER*)file_data;
        if(!existing_file_info->path) {
            debug("FileTransfer:\tUnable to get file location!\n");
            return UINT32_MAX;
        }

        /* See if this file still exists and if we have access*/
        file = fopen((const char*)existing_file_info->path, "rb");
        if(file){
            fseeko(file, 0, SEEK_END);
            file_size = ftello(file);
        } else {
            debug("FileTransfer:\tUnable to re-access file, must fail!\n");
            return UINT32_MAX;
        }

        /* get the file_id to resume the file */
        memcpy(file_id, existing_file_info->file_id, TOX_FILE_ID_LENGTH);

        /* Get file name from the existing file path. */
        uint8_t *p = existing_file_info->path, *name = existing_file_info->path;
        while(*p != '\0') {
            if(*p == '/' || *p == '\\') {
                name = p + 1;
            }
            p++;
        }

        file_number = tox_file_send(tox, friend_number, TOX_FILE_KIND_DATA, file_size, file_id, name, p - name, &error);

        filename        = name;
        filename_length = p - name;
        path        = existing_file_info->path;
        path_length = strlen((const char*)existing_file_info->path);
        transfer_size = existing_file_info->size_transferred;

        break;
    }
    case TOX_FILE_KIND_AVATAR:{
        /* Verify avatar info. */
        if(!file_data && file_data_size) {
            debug("FileTransfer:\tUnable to use *avatar!\n");
            return UINT32_MAX;
        }
        /* Get file_id for this avatar. */
        if(!tox_hash(file_id, file_data, file_data_size)){
            debug("FileTransfer:\tUnable to get hash for avatar!\n");
            return UINT32_MAX;
        }

        debug("FileTransfer:\tStarting avatar to friend %u.\n", friend_number);
        file_number = tox_file_send(tox, friend_number, TOX_FILE_KIND_AVATAR, file_data_size, file_id, NULL, 0, &error);

        file_size = file_data_size;
        memory = 1;
        avatar = 1;
        break;
    } /* last case */
    } /* switch */

    /* process file! */
    if(file_number != UINT32_MAX) {
        /* Toxcore accepted our file, build internal info. */
        FILE_TRANSFER *file_handle = get_file_transfer(friend_number, file_number);
        utox_build_file_transfer(file_handle, friend_number, file_number, file_size, 0, memory, avatar, kind, filename,
            filename_length, path, path_length, NULL, tox);

        /* Build UI info, and create resume file. */
        if(!memory){
            file_handle->file = file;
            file_handle->ui_data = message_add_type_file(file_handle);
            file_handle->resume = utox_file_alloc_ftinfo(file_handle);
            if(kind == TOX_FILE_KIND_EXISTING){
                free(path);
            }
            if(transfer_size){
                file_handle->size_transferred = transfer_size;
            }

            utox_new_user_file(file_handle);
        } else {
            if(avatar){
                memcpy(file_handle->avatar, file_data, file_data_size);
            } else {
                /* Allow saving of in-lines */
                file_handle->ui_data = message_add_type_file(file_handle);
                memcpy(file_handle->memory, file_data, file_data_size);
                free(filename);
                utox_new_user_file(file_handle);
            }
            file_handle->status = FILE_TRANSFER_STATUS_PAUSED_THEM;
            file_handle->resume = 0;
        }

    } else {
        debug("tox_file_send() failed\n");
        if(avatar){
            free(file_data);
        } else if(memory){
            free(filename);
            free(file_data);
        }
        friend[friend_number].transfer_count--;
    }

    return file_number;
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
        debug("FileTransfer:\t\tSize (%lu), Position (%lu), Length(%lu), Read_size (%lu), size_transferred (%lu).\n",
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
        fclose(file_handle->file);
        file_handle->file = NULL;
        utox_break_file(file_handle);
        remove(filepath);
        free(filepath);
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
    debug("FileTransfer:\tCleaning up file transfers! (%u & %u)\n", friend_number, file_number);
    FILE_TRANSFER *transfer = get_file_transfer(friend_number, file_number);
    if(transfer->name){
        free(transfer->name);
    }

    if(transfer->in_memory){
        if(transfer->avatar){
            free(transfer->avatar);
        }
    }

    if(transfer->file){
        fclose(transfer->file);
    }

    if(transfer->saveinfo){
        fclose(transfer->saveinfo);
    }

    memset(transfer, 0, sizeof(FILE_TRANSFER));
}

void utox_file_save_ftinfo(FILE_TRANSFER *file){
    if(!file->saveinfo){
        return;
    }
    fwrite(file, sizeof(*file), 1, file->saveinfo);
    fwrite(file->path, sizeof(uint8_t), file->path_length, file->saveinfo);
    fflush(file->saveinfo);
    fseeko(file->saveinfo, 0, SEEK_SET);
    fflush(file->saveinfo);
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
    info->saveinfo      = 0;

    memcpy(file, info, sizeof(*file));
    free(info);
    free(load);
    return 1;
}
