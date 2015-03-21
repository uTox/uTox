#include "main.h"

//static FILE_TRANSFER *file_t[256], **file_tend = file_t;
static FILE_TRANSFER outgoing_transfer[MAX_NUM_FRIENDS][MAX_FILE_TRANSFERS];
static FILE_TRANSFER incoming_transfer[MAX_NUM_FRIENDS][MAX_FILE_TRANSFERS];

FILE_TRANSFER *get_file_transfer(uint32_t friend_number, uint32_t file_number)
{
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

/* The following are internal file status helper functions */
static void utox_update_user_file(FILE_TRANSFER *file){
    MSG_FILE *msg = file->ui_data;
    if(!msg){
        return;
    }
    msg->status = file->status;
    msg->progress = file->size_transferred;
    msg->speed = 0;
    msg->path = file->path;
    redraw();
}

static void utox_run_file(FILE_TRANSFER *file, uint8_t us){
    if(us){
        if(file->status == FILE_TRANSFER_STATUS_PAUSED_US){
            file->status = FILE_TRANSFER_STATUS_ACTIVE;
        } else if(file->status == FILE_TRANSFER_STATUS_PAUSED_BOTH) {
            file->status = FILE_TRANSFER_STATUS_PAUSED_THEM;
        } else if(file->status == FILE_TRANSFER_STATUS_PAUSED_THEM) {
            // Do nothing;
        } else {
            debug("FileTransfer:\tTried to run a file from an unknown state!\n");
        }
    } else {
        if(file->status == FILE_TRANSFER_STATUS_PAUSED_US){
            // Do nothing;
        } else if(file->status == FILE_TRANSFER_STATUS_PAUSED_BOTH) {
            file->status = FILE_TRANSFER_STATUS_PAUSED_US;
        } else if(file->status == FILE_TRANSFER_STATUS_PAUSED_THEM) {
            file->status = FILE_TRANSFER_STATUS_ACTIVE;
        } else {
            debug("FileTransfer:\tTried to run a file from an unknown state!\n");
        }
    }
    utox_update_user_file(file);
}

static void utox_kill_file(FILE_TRANSFER *file, uint8_t us){
    if (file->status == FILE_TRANSFER_STATUS_KILLED) {
        debug("File already killed.\n");
        return;
    }

    file->status = FILE_TRANSFER_STATUS_KILLED;
    if (!us) debug("File was killed by remote\n");
    utox_update_user_file(file);

    if (!file->incoming) {
        --friend[file->friend_number].transfer_count;
    }
}

static void utox_pause_file(FILE_TRANSFER *file, uint8_t us){
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
            debug("FileTransfer:\tFile now paused by them, and still paused by us!\n");
        } else {
            file->status = FILE_TRANSFER_STATUS_PAUSED_THEM;
        }
    }
    utox_update_user_file(file);
}

static void utox_complete_file(FILE_TRANSFER *file){
    if(file->status == FILE_TRANSFER_STATUS_ACTIVE){
        if(file->incoming){
            if(file->in_memory){
                if(file->is_avatar){
                    utox_incoming_avatar(file->friend_number, file->avatar, file->size, file->name);
                } else {
                    friend_recvimage(&friend[file->friend_number], file->memory, file->size);
                }
            } else { // Is a file
                fclose(file->file);
            }
        } else {
            if(file->in_memory){

            } else { // Is a file
                fclose(file->file);
            }

            --friend[file->friend_number].transfer_count;
        }
        file->status = FILE_TRANSFER_STATUS_COMPLETED;
        utox_update_user_file(file);
    } else {
        debug("FileTransfer:\tUnable to complete file in non-active state (file:%u)\n", file->file_number);
    }
}

static void utox_break_file(){}
static void utox_resume_broken_file(){}

void file_transfer_local_control(Tox *tox, uint32_t friend_number, uint32_t file_number, TOX_FILE_CONTROL control){
    TOX_ERR_FILE_CONTROL error = 0;

    FILE_TRANSFER *info = get_file_transfer(friend_number, file_number);

    switch(control){
        case TOX_FILE_CONTROL_RESUME:
            // if not started
            debug("FileTransfer:\tWe just accepted file (%u & %u)\n", friend_number, file_number);
            // else
            debug("FileTransfer:\tWe just resumed file (%u & %u)\n", friend_number, file_number);
            utox_run_file(info, 1);
            tox_file_control(tox, friend_number, file_number, control, &error);
            break;
        case TOX_FILE_CONTROL_PAUSE:
            debug("FileTransfer:\tWe just paused file (%u & %u)\n", friend_number, file_number);
            utox_pause_file(info, 1);
            tox_file_control(tox, friend_number, file_number, control, &error);
            break;
        case TOX_FILE_CONTROL_CANCEL:
            debug("FileTransfer:\tWe just canceled file (%u & %u)\n", friend_number, file_number);
            tox_file_control(tox, friend_number, file_number, control, &error);

            utox_kill_file(info, 1);
            break;
    }
    if(error){
        debug("FileTransfer:\tThere was an error(%u) sending the command, you probably want to see to that!\n", error);
        debug("FileTransfer:\t\tFriend %u, and File %u. \n", friend_number, file_number);
    } else {
        utox_update_user_file(info);
    }
}

static void file_transfer_callback_control(Tox *UNUSED(tox), uint32_t friend_number, uint32_t file_number, TOX_FILE_CONTROL control, void *UNUSED(userdata)){

    FILE_TRANSFER *info = get_file_transfer(friend_number, file_number);

    switch(control){
        case TOX_FILE_CONTROL_RESUME:
            // if not started
            debug("FileTransfer:\tFriend (%i) has accepted file (%i)\n", friend_number, file_number);
            // else
            debug("FileTransfer:\tFriend (%i) has resumed file (%i)\n", friend_number, file_number);
            utox_run_file(info, 0);
            break;
        case TOX_FILE_CONTROL_PAUSE:
            debug("FileTransfer:\tFriend (%i) has paused file (%i)\n", friend_number, file_number);
            utox_pause_file(info, 0);
            break;
        case TOX_FILE_CONTROL_CANCEL:
            debug("FileTransfer:\tFriend (%i) has canceled file (%i)\n", friend_number, file_number);
            utox_kill_file(info, 0);
            break;
    }
}

/* Function called by core with a new incoming file. */
static void incoming_file_avatar(Tox *tox, uint32_t friend_number, uint32_t file_number, uint32_t kind, uint64_t file_size, const uint8_t *filename, size_t filename_length, void *UNUSED(user_data)){
    //new incoming file
    debug("FileTransfer:\tNew Avatar from friend (%u) \nFileTransfer:\t\tHash: %s\n", friend_number, filename);

    FILE_TRANSFER *file_handle = get_file_transfer(friend_number, file_number);

    if(file_size <= 0){
        utox_incoming_avatar(file_handle->friend_number, file_handle->avatar, file_handle->size, file_handle->name);
        file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL);
    }

    if(file_size > UTOX_AVATAR_MAX_DATA_LENGTH){
        file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL);
        debug("FileTransfer:\tAvatar from friend(%u) rejected, TOO LARGE (%llu)\n", friend_number, file_size);
        return;
    }

    if( (friend[friend_number].avatar.format > 0 ) && (friend[friend_number].avatar.hash == filename) ){
        file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL);
        debug("FileTransfer:\tAvatar from friend(%u) rejected, SAME AS CURRENT\n", friend_number);
        return;
    }

    // Reset the file handle for new data.
    memset(file_handle, 0, sizeof(FILE_TRANSFER));

    // Set ids
    file_handle->friend_number = friend_number;
    file_handle->file_number = file_number;
    file_handle->name = (uint8_t*)filename;
    file_handle->name_length = filename_length;
    file_handle->incoming = 1;
    file_handle->in_memory = 1;
    file_handle->is_avatar = 1;
    file_handle->size = file_size;

    file_handle->avatar = malloc(file_size);
    file_handle->status = FILE_TRANSFER_STATUS_ACTIVE;
    file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_RESUME);
}

/* Function called by core with a new incoming file. */
static void incoming_file_callback_request(Tox *tox, uint32_t friend_number, uint32_t file_number, uint32_t kind, uint64_t file_size, const uint8_t *filename, size_t filename_length, void *user_data){
    debug("FileTransfer:\tNew incoming file from friend (%u) file number (%u)\nFileTransfer:\t\tfilename: %s\n", friend_number, file_number, filename);

    FILE_TRANSFER *file_handle = get_file_transfer(friend_number, file_number);

    if (!file_handle) {
        tox_file_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL, 0);
        return;
    }

    if(kind == TOX_FILE_KIND_AVATAR){
        incoming_file_avatar(tox, friend_number, file_number, kind, file_size, filename, filename_length, user_data);
        return;
    }

    // Reset the file handle for new data.
    memset(file_handle, 0, sizeof(FILE_TRANSFER));

    // Set ids
    file_handle->friend_number = friend_number;
    file_handle->file_number = file_number;
    file_handle->incoming = 1;
    file_handle->in_memory = 0;
    file_handle->size = file_size;
    // FILE_T->filename_length is our max length, make sure that's enforced!
    file_handle->name = (uint8_t*)strdup((char*)filename);
    file_handle->name_length = filename_length;

    // If it's a small inline image, just accept it!
    if( file_size < 1024 * 1024 * 4 &&
        filename_length == (sizeof("utox-inline.png") - 1) &&
        memcmp(filename, "utox-inline.png", filename_length) == 0) {
            file_handle->in_memory = 1;
            file_handle->status = FILE_TRANSFER_STATUS_ACTIVE;
            file_handle->memory = malloc(file_size);
            file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_RESUME);
        // postmessage(FRIEND_FILE_IN_NEW_INLINE, friend_number, file_number, NULL);
    } else {
        file_handle->ui_data = message_add_type_file(file_handle);
        // postmessage(FRIEND_FILE_IN_NEW, friend_number, file_number, NULL);
    }
}

static void incoming_file_callback_chunk(Tox *UNUSED(tox), uint32_t friend_number, uint32_t file_number, uint64_t position, const uint8_t *data, size_t length, void *UNUSED(user_data)){
    debug("FileTransfer:\tIncoming chunk for friend (%u), and file (%u). Start (%u), End (%u).\r",
                                                                      friend_number, file_number, position, length);

    TOX_ERR_FILE_SEND_CHUNK error;
    FILE_TRANSFER *file_handle = get_file_transfer(friend_number, file_number);

    if(length == 0){
        debug("\nFileTransfer:\tIncoming transfer is done (%u & %u)\n", friend_number, file_number);
        utox_complete_file(file_handle);
        return;
    }

    uint64_t last_bit = position + length;
    time_t time_e = time(NULL);

    if(file_handle->in_memory) {
        if(file_handle->is_avatar){
            memcpy(file_handle->avatar + position, data, length);
        } else {
            memcpy(file_handle->memory + position, data, length);
        }
    } else {
        fseeko(file_handle->file, 0, position);
        size_t write_size = fwrite(data, 1, length, file_handle->file);
        if(write_size != length){
            debug("\n\nFileTransfer:\tERROR WRITING DATA TO FILE! (%u & %u)\n\n", friend_number, file_number);
            tox_postmessage(TOX_FILE_INCOMING_CANCEL, friend_number, file_number, NULL);
            return;
        }
    }
    file_handle->size_transferred += length;

    /* TODO re-implement file transfer speed and time remaining...
    if(time(NULL) - file_handle->last_chunk_time >= 10) {
        debug("FileTransfer:\ttime update running\n");
    // TODO divide total size by total time to get ave speed.
    // include last active, and total paused time to exclude paused time.
        FILE_PROGRESS *p = malloc(sizeof(FILE_PROGRESS));
        p->size_transferred = file_handle->size_transferred;
        p->speed = ( (file_handle->size_transferred - file_handle->last_chunk_time) )
                        / (time(NULL) - file_handle->last_chunk_time);
        postmessage(FRIEND_FILE_IN_PROGRESS, friend_number, file_number, p);
        file_handle->size_transferred = file_handle->size_transferred;
    }

    */
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
        memset(file_handle, 0, sizeof(FILE_TRANSFER));


        file_handle->friend_number = friend_number;
        file_handle->file_number = file_number;
        file_handle->status = FILE_TRANSFER_STATUS_PAUSED_THEM;
        //TODO file_handle->sendsize = tox_file_data_size(tox, fid);

        file_handle->file = file;

        file_handle->name = (uint8_t*)strdup((char*)filename);
        file_handle->path = (uint8_t*)strdup((char*)path);
        file_handle->name_length = filename_length;

        file_handle->size = file_size;

        ++friend[friend_number].transfer_count;
        debug("Sending file %d of %d(max) to friend(%d).\n", friend[friend_number].transfer_count, MAX_FILE_TRANSFERS, friend_number);
        // Create a new msg for the UI and save it's pointer
        file_handle->ui_data = message_add_type_file(file_handle);
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
    uint8_t *file_id;
    uint8_t *filename;
    size_t filename_length = 0;
    file_id = malloc(TOX_HASH_LENGTH);
    if(!tox_hash(file_id, image, image_size)){
        debug("FileTransfer:\tUnable to get hash for image!\n");
        return;
    }

    if( image_size < 1024 * 1024 * 4 ) {
        filename_length = sizeof("utox-inline.png") - 1;
        filename = malloc(filename_length);
        memcpy(filename, "utox-inline.png", filename_length);
    } else {
        filename_length = sizeof("utox-image.png") - 1;
        filename = malloc(filename_length);
        memcpy(filename, "utox-image.png", filename_length);
    }

    uint32_t file_number = tox_file_send(tox, friend_number, TOX_FILE_KIND_DATA, image_size, file_id, (const uint8_t*)filename, filename_length, &error);

    if(file_number != -1) {
        FILE_TRANSFER *file_handle = get_file_transfer(friend_number, file_number);
        debug("Going to memset in image new, friend %u, file is image size is%u\n", friend_number, image_size);
        memset(file_handle, 0, sizeof(FILE_TRANSFER));

        file_handle->friend_number = friend_number;
        file_handle->file_number = file_number;
        file_handle->status = FILE_TRANSFER_STATUS_PAUSED_THEM;

        file_handle->kind = TOX_FILE_KIND_DATA;
        file_handle->name = file_id;
        file_handle->name_length = TOX_HASH_LENGTH;
        file_handle->size = image_size;

        file_handle->in_memory = 1;
        file_handle->is_avatar = 0;

        file_handle->memory = malloc(image_size);
        memcpy(file_handle->memory, image, image_size);

        ++friend[friend_number].transfer_count;
        debug("FileTransfer:\tSending image file %d of %d(max) to friend(%d).\n", friend[friend_number].transfer_count, MAX_FILE_TRANSFERS, friend_number);
        // Create a new msg for the UI and save it's pointer
    } else {
        free(image);
        debug("tox_file_send() failed for image\n");
        return;
    }
    return;
}

int outgoing_file_send_avatar(Tox *tox, uint32_t friend_number, uint8_t *avatar, size_t avatar_size){

    debug("FileTransfer:\tStarting outgoing AVATAR file to friend %u.\n", friend_number);

    if(friend[friend_number].transfer_count >= MAX_FILE_TRANSFERS) {
        debug("FileTransfer:\tMaximum outgoing file sending limit reached(%u/%u) for friend(%u). ABORTING!\n",
                                            friend[friend_number].transfer_count, MAX_FILE_TRANSFERS, friend_number);
        return 1;
    }

    if(!avatar) {
        debug("FileTransfer:\tUnable to use *avatar!\n");
        return 1;
    }


    TOX_ERR_FILE_SEND error;
    uint8_t *file_id;
    file_id = malloc(TOX_HASH_LENGTH);
    if(!tox_hash(file_id, avatar, avatar_size)){
        debug("FileTransfer:\tUnable to get hash for avatar!\n");
        return 1;
    }
    int file_number = tox_file_send(tox, friend_number, TOX_FILE_KIND_AVATAR, avatar_size, file_id, NULL, 0, &error);

    if(file_number != -1) {
        FILE_TRANSFER *file_handle = get_file_transfer(friend_number, file_number);
        debug("Going to memset in avatar new, friend %u, file is avatar size is%u\n", friend_number, avatar_size);
        memset(file_handle, 0, sizeof(FILE_TRANSFER));

        file_handle->friend_number = friend_number;
        file_handle->file_number = file_number;
        file_handle->status = FILE_TRANSFER_STATUS_PAUSED_THEM;

        file_handle->kind = TOX_FILE_KIND_AVATAR;
        file_handle->name = file_id;
        file_handle->name_length = TOX_HASH_LENGTH;
        file_handle->size = avatar_size;

        file_handle->in_memory = 1;
        file_handle->is_avatar = 1;

        file_handle->avatar = malloc(avatar_size);
        memcpy(file_handle->avatar, avatar, avatar_size);

        ++friend[friend_number].transfer_count;
        debug("FileTransfer:\tSending avatar file %d of %d(max) to friend(%d).\n", friend[friend_number].transfer_count, MAX_FILE_TRANSFERS, friend_number);
        // Create a new msg for the UI and save it's pointer
    } else {
        debug("tox_file_send() failed for avatar\n");
        return 1;
    }
    return 0;
}

static void outgoing_file_callback_chunk(Tox *tox, uint32_t friend_number, uint32_t file_number, uint64_t position, size_t length, void *UNUSED(user_data)){

    debug("FileTransfer:\tChunk requested for friend_id (%u), and file_id (%u). Start (%u), End (%u).\r", friend_number, file_number, position, length);
    //send a chunk of data size of length with

    FILE_TRANSFER *file_handle = get_file_transfer(friend_number, file_number);

    if(length == 0){
        debug("FileTransfer:\tOutgoing transfer is done (%u & %u)\n", friend_number, file_number);
        utox_complete_file(file_handle);
        return;
    }

    const uint8_t *chunk;
    uint8_t *buffer;
    buffer = malloc(length);
    size_t read_size;

    uint64_t last_bit = position + length;

    if(file_handle->in_memory){
        // Memory
        if(file_handle->is_avatar){
            if(file_handle->size < length){
                debug("FileTransfer:\tAvatar size mismatch!\n");
                free(buffer);
                return;
            }
            memcpy(buffer, file_handle->avatar + position, length);
            read_size = length; /* We hope!! */
        } else {
            if(file_handle->size < length){
                debug("FileTransfer:\tMemory size mismatch!\n");
                free(buffer);
                return;
            }
            memcpy(buffer, file_handle->memory + position, length);
            read_size = length; /* We hope!! */
        }
    } else {
        // File
        FILE *file = file_handle->file;
        if(file){
            fseeko(file, 0, position);
            read_size = fread(buffer, 1, length, file);
        }
    }

    if(read_size != length){
        debug("FileTransfer:\tERROR READING FILE! (%u & %u)\n", friend_number, file_number);
        file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL);
        free(buffer);
        return;
    }

    TOX_ERR_FILE_SEND_CHUNK error;
    chunk = buffer;

    tox_file_send_chunk(tox, friend_number, file_number, position, chunk, length, &error);
    free(buffer);
}

int utox_file_start_write(uint32_t friend_number, uint32_t file_number, void *filepath){
    FILE_TRANSFER *file_handle = get_file_transfer(friend_number, file_number);
    file_handle->file = fopen(filepath, "wb");
    if(!file_handle->file) {
        free(filepath);
        file_handle->status = FILE_TRANSFER_STATUS_BROKEN;
        return -1;
    }
    file_handle->path = filepath;
    file_handle->path_length = strlen(filepath);
    file_handle->status = FILE_TRANSFER_STATUS_ACTIVE;
    return 0;
}

void utox_set_callbacks_for_transfer(Tox *tox){/*
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

