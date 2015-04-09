#include "main.h"

//static FILE_TRANSFER *file_t[256], **file_tend = file_t;
static FILE_TRANSFER outgoing_transfer[MAX_NUM_FRIENDS][MAX_FILE_TRANSFERS];
static FILE_TRANSFER incoming_transfer[MAX_NUM_FRIENDS][MAX_FILE_TRANSFERS];
static BROKEN_TRANSFER broken_list[MAX_FILE_TRANSFERS] = {{0}};

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

/* The following are internal file status helper functions */
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
}

static int utox_file_alloc_resume(Tox *tox, FILE_TRANSFER *file){
    TOX_ERR_FILE_GET error;
    int i = 0;
    int resume = 0;
    resume = tox_file_get_file_id(tox, file->friend_number, file->file_number, file->file_id, &error);
    if (resume){
        file->resume = 0;
        for(i = 1; i < MAX_FILE_TRANSFERS; i++){
            if(!broken_list[i].used){
                // TODO set an expire time for this.
                broken_list[i].used = 1;
                broken_list[i].incoming = file->incoming;

                broken_list[i].friend_number = file->friend_number;
                broken_list[i].file_number = file->file_number;
                memcpy(broken_list[i].file_id, file->file_id, sizeof(uint8_t) * TOX_FILE_ID_LENGTH);
                if(file->path){
                    memcpy(broken_list[i].file_path, file->path, file->path_length);
                }

                FILE_TRANSFER *data = malloc(sizeof(FILE_TRANSFER));
                memcpy(data, file, sizeof(FILE_TRANSFER));
                broken_list[i].data = data;
                break;
            }
        }
        if(i >= 1 && i <= MAX_FILE_TRANSFERS){
            debug("FileTransfer:\tBroken transfer #%u set; ready to resume file %.*s\n", i, (uint32_t)file->name_length, file->name);
            return i;
        } else {
            debug("FileTransfer:\tUnable to set resume number; number was %i, name was %.*s\n", i, (uint32_t)file->name_length, file->name);
            return 0;
        }
    } else {
        debug("FileTransfer:\tUnable to get file id for incoming file... uTox can't resume file %.*s\n", (uint32_t)file->name_length, file->name);
        return 0;
    }
}

static void utox_file_free_resume(uint8_t i){
    if(i >= 1 && i < MAX_FILE_TRANSFERS){
        memset(&broken_list[i], 0, sizeof(BROKEN_TRANSFER));
        // TODO recurse and free needed allocs
        debug("FileTransfer:\tBroken transfer #%u reset!\n",i);
        return;
    }
    if ( i != 0 || broken_list[i].used) { // We shouldn't be called for a 0; but either way no error needed!
        debug("FileTransfer:\tBroken transfer #%u unable to be reset! This is bad!\n",i);
    }
}

static void utox_build_file_transfer(FILE_TRANSFER *ft, uint32_t friend_number, uint32_t file_number,
    uint64_t file_size, _Bool incoming, _Bool in_memory, _Bool is_avatar, uint8_t kind, const uint8_t *name,
    size_t name_length, const uint8_t *path, size_t path_length, const uint8_t *file_id, Tox *tox);

static FILE_TRANSFER* utox_file_find_existing(uint8_t *file_id){
    if(!file_id){
        return NULL;
    }
    for(int i = 1; i < MAX_FILE_TRANSFERS; i++){
        if(broken_list[i].used && memcmp(broken_list[i].file_id, file_id, TOX_FILE_ID_LENGTH) == 0){
            if(broken_list[i].data){
                debug("FileTransfer:\tExisting found, list number %u\n", i);
                return broken_list[i].data;
            } else {
                debug("FileTransfer:\tExisting found (post-restart), list number %u\n", i);

                FILE_TRANSFER *tmp = malloc(sizeof(*tmp));

                utox_build_file_transfer(tmp, broken_list[i].friend_number, broken_list[i].file_number, 0,
                    broken_list[i].incoming, 0, 0, 3, NULL, 0, broken_list[i].file_path,
                    strlen((const char*)broken_list[i].file_path), broken_list[i].file_id, NULL);
                tmp->resume = i;
                debug("path:: %s\n", tmp->path);
                return tmp;
            }
        }
    }
    return NULL;
}

static void utox_kill_file(FILE_TRANSFER *file, uint8_t us){
    if (file->status == FILE_TRANSFER_STATUS_KILLED) {
        debug("File already killed.\n");
    } else if(file->status == FILE_TRANSFER_STATUS_COMPLETED){
        debug("File already completed.\n");
    } else {
        file->status = FILE_TRANSFER_STATUS_KILLED;
    }

    utox_update_user_file(file);

    if (!file->incoming && friend[file->friend_number].transfer_count) {
        --friend[file->friend_number].transfer_count;
    }
    utox_cleanup_file_transfers(file->friend_number, file->file_number);
    utox_file_free_resume(file->resume);
}

static void utox_break_file(FILE_TRANSFER *file){
    if(file->status == FILE_TRANSFER_STATUS_NONE && file->resume){
        return utox_kill_file(file, 1); /* We don't save unstarted files */
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
        } else {
            debug("FileTransfer:\tTried to run outgoing file from an unknown state! (%u)\n", file->status);
        }
    } else {
        if(file->status == FILE_TRANSFER_STATUS_PAUSED_US){
            // Do nothing;
        } else if(file->status == FILE_TRANSFER_STATUS_PAUSED_BOTH) {
            file->status = FILE_TRANSFER_STATUS_PAUSED_US;
        } else if(file->status == FILE_TRANSFER_STATUS_PAUSED_THEM) {
            file->status = FILE_TRANSFER_STATUS_ACTIVE;
        } else {
            debug("FileTransfer:\tTried to run incoming file from an unknown state! (%u)\n", file->status);
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
            }
        } else {
            if(file->in_memory){

                // TODO, might want to do something here.
            } else { // Is a file
                fclose(file->file);
            }

            if(friend[file->friend_number].transfer_count){
                /* Decrement if > 0 this might be better held inside local_control */
                --friend[file->friend_number].transfer_count;
            }
        }
        file->status = FILE_TRANSFER_STATUS_COMPLETED;
        file->ui_data->path = file->path;
        utox_update_user_file(file);
    } else {
        debug("FileTransfer:\tUnable to complete file in non-active state (file:%u)\n", file->file_number);
    }
    utox_file_free_resume(file->resume);
    utox_cleanup_file_transfers(file->friend_number, file->file_number);
    file->resume = 0; // We don't need to always be resetting this broken number anymore
}

static void utox_restart_file(Tox *tox, BROKEN_TRANSFER broken, uint8_t broken_number){
    debug("FileTransfer:\tRestarting File\n");
    if(broken.data){
        outgoing_file_send_existing(tox, broken.data, broken_number);
    } else {
        debug("FileTransfer:\tWe are post-restart, rebuilding file from %u!\n", broken_number);

        FILE_TRANSFER *tmp = malloc(sizeof(*tmp));

        utox_build_file_transfer(tmp, broken.friend_number, broken.file_number, 0, 0, 0, 0, 3, NULL, 0,
            broken.file_path, strlen((const char*)broken.file_path), broken.file_id, NULL);
        tmp->resume = broken_number;
        debug("path:: %s\n", tmp->path);
        outgoing_file_send_existing(tox, tmp, broken_number);
    }
}

void ft_friend_online(Tox *tox, uint32_t friend_number){
    for(int i = 1; i < MAX_FILE_TRANSFERS; i++){
        if( broken_list[i].used &&
            broken_list[i].friend_number == friend_number &&
            broken_list[i].incoming == 0){
            utox_restart_file(tox, broken_list[i], i);
        }
    }
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
        file->name        = malloc(name_length);
        memcpy(file->name, name, name_length);
        file->name_length = name_length;
    } else {
        file->name        = NULL;
        file->name_length = 0;
    }

    if(path){
        file->path        = malloc(path_length);
        memcpy(file->path, path, path_length);
        file->path_length = path_length;
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
        file->memory = calloc(file_size, sizeof(uint8_t));
    } else if (is_avatar){
        file->avatar = calloc(file_size, sizeof(uint8_t));
    }

    if(!incoming){
        /* Outgoing file */
        utox_pause_file(file, 1);
    }
}

/* Function called by core with a new incoming file. */
static void incoming_file_avatar(Tox *tox, uint32_t friend_number, uint32_t file_number, uint32_t kind, uint64_t file_size, const uint8_t *filename, size_t filename_length, void *UNUSED(user_data)){
    debug("FileTransfer:\tNew Avatar from friend (%u)\n", friend_number);

    /* If 0 unset the avatar... TODO! */
    if(file_size <= 0){
        utox_incoming_avatar(friend_number, NULL, 0);
        debug("FileTransfer:\tAvatar Unset from friend(%u).\n", friend_number);
        file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL);
        return;
    /* If incoming avatar is too big, cancel */
    } else if (file_size > UTOX_AVATAR_MAX_DATA_LENGTH){
        file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL);
        debug("FileTransfer:\tAvatar from friend(%u) rejected, TOO LARGE (%u)\n", friend_number, (uint32_t)file_size);
        return;
    }


    uint8_t file_id[TOX_FILE_ID_LENGTH] = {0};
    tox_file_get_file_id(tox, friend_number, file_number, file_id, 0);
    if((friend[friend_number].avatar.format > 0) && memcmp(friend[friend_number].avatar.hash, file_id, TOX_HASH_LENGTH) == 0) {
        file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL);
        debug("FileTransfer:\tAvatar from friend (%u) rejected: Same as Current\n", friend_number);
        return;
    }

    FILE_TRANSFER *file_handle = get_file_transfer(friend_number, file_number);

    if (!file_handle) {
        tox_file_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL, 0);
        return;
    }

    utox_build_file_transfer(file_handle, friend_number, file_number, file_size, 1, 1, 1, TOX_FILE_KIND_AVATAR, NULL, 0, NULL, 0, file_id, tox);

    file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_RESUME);
}

static void incoming_file_existing(Tox *tox, uint32_t friend_number, uint32_t file_number, uint32_t kind, uint64_t size, const uint8_t *filename, size_t filename_length, void *UNUSED(user_data)){
    //new incoming file
    debug("FileTransfer:\tIncoming Existing file from friend (%u) \n", friend_number);

    uint8_t new_id[TOX_FILE_ID_LENGTH] = {0};
    tox_file_get_file_id(tox, friend_number, file_number, new_id, 0);
    FILE_TRANSFER *file_new = get_file_transfer(friend_number, file_number);
    FILE_TRANSFER *file_existing = utox_file_find_existing(new_id);

    FILE *file;
    TOX_ERR_FILE_SEEK error = 0;

    if (!file_new) {
        tox_file_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL, 0);
        return;
    }

    if(file_existing){
        uint8_t old_broken_number = file_existing->resume;
        if( file_existing->friend_number == friend_number &&
            file_existing->size == size &&
            memcmp(file_existing->name, filename, filename_length) == 0 ){
            // DO STUFF

            utox_build_file_transfer(file_new, friend_number, file_number, size, 1, 0, 0, TOX_FILE_KIND_EXISTING,
                filename, filename_length, NULL, 0, new_id, tox);

            file_new->size_transferred = file_existing->size_transferred;
            file_new->file = file_existing->file;
            file_new->ui_data = file_existing->ui_data;
            utox_file_free_resume(old_broken_number);
            file_new->resume = utox_file_alloc_resume(tox, file_new);

            tox_file_seek(tox, friend_number, file_number, file_existing->size_transferred, &error);
            if(error){
                debug("FileTransfer:\tError seeking new file.");
            }
            file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_RESUME);
        } else {
            debug("FileTransfer:\tSomething didn't match, does that file still exist?\n");
            file_new->path = file_existing->path;
            file = fopen((char*)broken_list[old_broken_number].file_path, "rb");
            uint64_t file_size = 0;
            if(file){
                debug("FileTransfer:\tCool file exists, let try to restart it.\n");
                fseeko(file, 0, SEEK_END);
                file_size = ftello(file);
                fseeko(file, 0, SEEK_SET);
                fclose(file);
                if(file_size >= size){
                    // This is a case we don't want to touch... Just abort.
                    debug("FileTransfer:\tResume size mismatch, aborting!");
                } else {
                    file = fopen((const char*)file_new->path, "wb");
                    if(file){
                        utox_build_file_transfer(file_new, friend_number, file_number, size, 1, 0, 0,
                            TOX_FILE_KIND_DATA, filename, filename_length, file_new->path, file_existing->path_length, new_id, tox);

                        file_new->file = file;
                        file_new->size_transferred = file_size;
                        file_new->ui_data = message_add_type_file(file_new);
                        file_new->resume = utox_file_alloc_resume(tox, file_new);
                        debug("Resume %i\n", file_new->resume);

                        tox_file_seek(tox, friend_number, file_number, file_new->size_transferred, &error);
                        postmessage(FRIEND_FILE_NEW, 0, 0, file_new);
                        file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_RESUME);
                    } else {
                        debug("FileTransfer:\tFile opened for reading, but unable to write.\n");
                        file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL);
                    }
                }
            } else {
                debug("FileTransfer:\tUnable to open that file; treating as new.\n");
                utox_build_file_transfer(file_new, friend_number, file_number, size, 1, 0, 0, TOX_FILE_KIND_DATA,
                    filename, filename_length, NULL, 0, new_id, tox);

                file_new->ui_data = message_add_type_file(file_new);
                file_new->resume = utox_file_alloc_resume(tox, file_new);

                postmessage(FRIEND_FILE_NEW, 0, 0, file_new);
            }
            utox_file_free_resume(old_broken_number);
        }
    } else {
        debug("FileTransfer:\tWe don't know anything about this file, so for safety we're just going to reject it!.\n");
        file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL);
    }
}
/* Function called by core with a new incoming file. */
static void incoming_file_callback_request(Tox *tox, uint32_t friend_number, uint32_t file_number, uint32_t kind, uint64_t file_size, const uint8_t *filename, size_t filename_length, void *user_data){

    if(kind == TOX_FILE_KIND_AVATAR){
        incoming_file_avatar(tox, friend_number, file_number, kind, file_size, filename, filename_length, user_data);
        return;
    } else if (kind == TOX_FILE_KIND_EXISTING){
        incoming_file_existing(tox, friend_number, file_number, kind, file_size, filename, filename_length, user_data);
        return;
    } else {
        debug("FileTransfer:\tNew incoming file from friend (%u) file number (%u)\nFileTransfer:\t\tfilename: %s\n", friend_number, file_number, filename);
    }

    FILE_TRANSFER *file_handle = get_file_transfer(friend_number, file_number);

    if (!file_handle) {
        tox_file_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL, 0);
        return;
    }

    if(file_size < 1024 * 1024 * 4 && filename_length == (sizeof("utox-inline.png") - 1) &&
                                    memcmp(filename, "utox-inline.png", filename_length) == 0) {

        utox_build_file_transfer(file_handle, friend_number, file_number, file_size, 1, 1, 0, TOX_FILE_KIND_DATA,
                                filename, filename_length, NULL, 0, NULL, tox);

        file_transfer_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_RESUME);
    } else {
        utox_build_file_transfer(file_handle, friend_number, file_number, file_size, 1, 0, 0, TOX_FILE_KIND_DATA,
                                filename, filename_length, NULL, 0, NULL, tox);

        file_handle->ui_data = message_add_type_file(file_handle);
        file_handle->resume = utox_file_alloc_resume(tox, file_handle);

        postmessage(FRIEND_FILE_NEW, 0, 0, file_handle);
    }
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
    if(file_handle->resume){
        broken_list[file_handle->resume].data->size_transferred = file_handle->size_transferred;
        broken_list[file_handle->resume].data->file = file_handle->file;
    }

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
        file_handle->resume = utox_file_alloc_resume(tox, file_handle);

        // TODO move this to utox_file_run(ish)
        ++friend[friend_number].transfer_count;

        debug("Sending file %d of %d(max) to friend(%d).\n", friend[friend_number].transfer_count, MAX_FILE_TRANSFERS, friend_number);

        // Create a new msg for the UI and save it's pointer
        postmessage(FRIEND_FILE_NEW, 0, 0, file_handle);
    } else {
        debug("tox_file_send() failed\n");
    }
}

void outgoing_file_send_existing(Tox *tox, FILE_TRANSFER *broken_data, uint8_t broken_number){
    debug("FileTransfer:\tRestarting outgoing file to friend %u. (filename, %s)\n", broken_data->friend_number, broken_data->path);

    if(friend[broken_data->friend_number].transfer_count >= MAX_FILE_TRANSFERS) {
        debug("FileTransfer:\tMaximum outgoing file sending limit reached(%u/%u) for friend(%u). ABORTING!\n",
            friend[broken_data->friend_number].transfer_count, MAX_FILE_TRANSFERS, broken_data->friend_number);
        utox_file_free_resume(broken_data->resume);
        return;
    }

    if(!broken_data->file) {
        debug("FileTransfer:\tUnable to open file for reading, trying to re-access!\n");
        if(broken_data->path_length){
            FILE *file = fopen((const char*)broken_data->path, "rb");
            if(file){
                broken_data->file = file;
                fseeko(file, 0, SEEK_END);
                broken_data->size = ftello(file);
                fseeko(file, 0, SEEK_SET);
            } else {
                debug("FileTransfer:\tUnable to re-access file, must fail!\n");
                file_transfer_local_control(tox, broken_data->friend_number, broken_data->file_number, TOX_FILE_CONTROL_CANCEL);
                return;
            }
        }
    }

    TOX_ERR_FILE_SEND error;
    const uint8_t *file_id = broken_data->file_id;
    uint8_t *p = broken_data->path, *name = broken_data->path;
    while(*p != '\0') {
        if(*p == '/' || *p == '\\') {
            name = p + 1;
        }
        p++;
    }

    int new_file_number = tox_file_send(tox, broken_data->friend_number, TOX_FILE_KIND_EXISTING, broken_data->size, file_id, name, p - name, &error);
    if(new_file_number != -1) {
        FILE_TRANSFER *file_handle = get_file_transfer(broken_data->friend_number, new_file_number);

        utox_build_file_transfer(file_handle, broken_data->friend_number, new_file_number, broken_data->size, 0, 0, 0,
            TOX_FILE_KIND_EXISTING, name, p - name, broken_data->path, broken_data->path_length, file_id, tox);

        file_handle->file = broken_data->file;
        if(broken_data->ui_data){
            file_handle->ui_data = broken_data->ui_data;
        } else {
            file_handle->ui_data = message_add_type_file(file_handle);
        }
        utox_file_free_resume(broken_number);
        file_handle->resume = utox_file_alloc_resume(tox, file_handle);
        ++friend[file_handle->friend_number].transfer_count;
        debug("Resending file %d of %d(max) to friend(%d).\n", friend[file_handle->friend_number].transfer_count, MAX_FILE_TRANSFERS, file_handle->friend_number);
        utox_update_user_file(file_handle);
    } else {
        debug("tox_file_send() failed\n");
    }
    free(broken_data);
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
        debug("FileTransfer:\t\tSize (%lu), Position (%lu), Length(%zu), Read_size (%zu), Size_transferred (%zu).\n",
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
        file_handle->status = FILE_TRANSFER_STATUS_BROKEN;
        return -1;
    }
    file_handle->path = filepath;
    file_handle->path_length = strlen(filepath);
    memcpy(broken_list[file_handle->resume].file_path, file_handle->path, file_handle->path_length);
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

int utox_file_start_temp_write(uint32_t friend_number, uint32_t file_number){/*
    FILE_TRANSFER *file_handle = get_file_transfer(friend_number, file_number);
    uint8_t path[UTOX_FILE_NAME_LENGTH];
    size_t path_length;

    // Subdir for each friend? TODO?
    path_length = datapath_subdir(path, FILE_TRANSFER_TEMP_PATH);
    memcpy( (path + path_length), file_handle->name, file_handle->name_length);
    debug("temppath:\t%.*s\n", (uint32_t)(path_length + file_handle->name_length), path);

    file_handle->tmp_file = fopen((const char*)path, "wb");

    if(!file_handle->tmp_file) {
        file_handle->status = FILE_TRANSFER_STATUS_BROKEN;
        debug("nope\n");
        return -1;
    }

    file_handle->in_tmp_loc = 1;
    file_handle->tmp_path = path;
    file_handle->tmp_path_length = strlen((const char*)path);
*/
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
    if(transfer->path)
        free(transfer->path);
    if(transfer->memory)
        free(transfer->memory);
    if(transfer->avatar)
        free(transfer->avatar);
}

void utox_file_save_active(void){
    uint8_t path[UTOX_FILE_NAME_LENGTH], *p;
    FILE *file;

    p = path + datapath(path);
    strcpy((char*)p, "utox_files.data");

    file = fopen((char*)path, "wb");
    if(!file) {
        debug("SaveFile:\tUnable to open uTox Save file for File Transfers\n");
        return;
    }

    debug("SaveFile:\tWriting uTox Save file for File Transfers \n");
    fwrite(broken_list, (sizeof(BROKEN_TRANSFER) * MAX_FILE_TRANSFERS), 1, file);
    fclose(file);

}

void utox_file_load_active(void){
    uint8_t path[UTOX_FILE_NAME_LENGTH], *p;
    uint32_t size_read;

    p = path + datapath(path);
    strcpy((char*)p, "utox_files.data");

    BROKEN_TRANSFER *saved = calloc(MAX_FILE_TRANSFERS, sizeof(BROKEN_TRANSFER));
    saved = file_raw((char*)path, &size_read);
    if(saved && size_read == (sizeof(BROKEN_TRANSFER) * MAX_FILE_TRANSFERS)){
        memcpy(broken_list, saved, (sizeof(BROKEN_TRANSFER) * MAX_FILE_TRANSFERS));
    } else {
        debug("SaveFile:\tUnable to load uTox Save file for File Transfers\n");
    }
    free(saved);

    for(int i = 0 ; i < MAX_FILE_TRANSFERS ; i++){
        broken_list[i].data = NULL;
    }
}
