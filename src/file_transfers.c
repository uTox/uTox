#include "file_transfers.h"

#include "avatar.h"
#include "friend.h"
#include "debug.h"
#include "macros.h"
#include "self.h"
#include "settings.h"
#include "text.h"
#include "tox.h"
#include "utox.h"

#include "native/filesys.h"
#include "native/image.h"
#include "native/thread.h"
#include "native/time.h"

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define MAX_INCOMING_COUNT 32

#define MAX_INLINE_FILESIZE (1024 * 1024 * 4)

static void fid_to_string(char *dest, uint8_t *src) {
    to_hex(dest, src, TOX_FILE_ID_LENGTH);
}

// Accepts a file number and returns indicating whether it's an incoming one or not.
static bool is_incoming_ft(uint32_t file_number) {
    // This is Toxcore magic.
    return file_number >= (1 << 16);
}

// Accepts a Toxcore incoming file number and returns a normal one.
static uint32_t detox_incoming_file_number(uint32_t file_number) {
    return (file_number >> 16) - 1;
}

static FILE_TRANSFER *get_file_transfer(uint32_t friend_number, uint32_t file_number) {
    FRIEND *f = get_friend(friend_number);
    if (!f) {
        return NULL;
    }

    if (is_incoming_ft(file_number)) {
        file_number = detox_incoming_file_number(file_number);
        if (f->ft_incoming_size && f->ft_incoming_size >= file_number) {
            return &f->ft_incoming[file_number];
        }
    } else if (f->ft_outgoing_size && f->ft_outgoing_size >= file_number) {
        return &f->ft_outgoing[file_number];
    }

    return NULL;
}

static FILE_TRANSFER *make_file_transfer(uint32_t friend_number, uint32_t file_number) {
    FRIEND *f = get_friend(friend_number);
    if (!f) {
        return NULL;
    }

    if (is_incoming_ft(file_number)) {
        file_number = detox_incoming_file_number(file_number);
        if (f->ft_incoming_size <= file_number) {
            LOG_TRACE("FileTransfer", "Realloc incoming %u|%u" , friend_number, file_number + 1);

            FILE_TRANSFER *new_ftlist = realloc(f->ft_incoming, sizeof(FILE_TRANSFER) * (file_number + 1));
            if (!new_ftlist) {
                LOG_ERR("FileTransfer", "Unable to allocate memory for new incoming file transfer.");
                return NULL;
            }

            f->ft_incoming = new_ftlist;
            f->ft_incoming_size = file_number + 1;
        }

        return &f->ft_incoming[file_number];
    }

    if (f->ft_outgoing_size <= file_number) {
        LOG_TRACE("FileTransfer", "Realloc outgoing %u|%u" , friend_number, file_number + 1);

        FILE_TRANSFER *new_ftlist = realloc(f->ft_outgoing, sizeof(FILE_TRANSFER) * (file_number + 1));
        if (!new_ftlist) {
            LOG_ERR("FileTransfer", "Unable to allocate memory for new outgoing file transfer.");
            return NULL;
        }

        f->ft_outgoing = new_ftlist;
        f->ft_outgoing_size = file_number + 1;
    }

    return &f->ft_outgoing[file_number];
}

/* Calculate the transfer speed for the UI. */
static void calculate_speed(FILE_TRANSFER *file) {
    if (file->speed > file->num_packets * 20 * 1371) {
        ++file->num_packets;
        return;
    }

    file->num_packets = 0;

    uint64_t time = get_time();
    if (!file->last_check_time) {
        file->last_check_time = time;
        return;
    }

    // TODO replace magic number with something real. (grayhatter> I think it's cpu clock ticks)
    if (time - file->last_check_time >= 1000 * 1000 * 100) {
        file->speed = (((double)(file->current_size - file->last_check_transferred) * 1000.0 * 1000.0 * 1000.0)
                       / (double)(time - file->last_check_time))
                      + 0.5;
        file->last_check_time        = time;
        file->last_check_transferred = file->current_size;
    }

    FILE_TRANSFER *msg = calloc(1, sizeof(FILE_TRANSFER));
    if (!msg) {
        LOG_ERR("FileTransfer", "Unable to malloc for internal message. (This is bad!)");
        return;
    }

    *msg = *file;
    postmessage_utox(FILE_STATUS_UPDATE, file->status, 0, msg);
}

static void ft_decon(uint32_t friend_number, uint32_t file_number) {
    LOG_INFO("FileTransfer", "Cleaning up file transfers! (%u & %u)" , friend_number, file_number);
    FILE_TRANSFER *ft = get_file_transfer(friend_number, file_number);
    if (!ft) {
        LOG_ERR("FileTransfer", "Can't decon a FT that doesn't exist!");
        return;
    }

    if (ft->in_use) {
        while (ft->decon_wait) {
            yieldcpu(10);
        }

        if (ft->incoming) {
            get_friend(friend_number)->ft_incoming_active_count--;
        } else {
            get_friend(friend_number)->ft_outgoing_active_count--;
        }

        if (ft->name) {
            free(ft->name);
        }

        if (ft->in_memory) {
            // free(ft->via.memory)?
        } else if (ft->avatar) {
            // free(ft->via.avatar)?
        } else if (ft->via.file) {
            fclose(ft->via.file);
        }
    }
    /* When decon is called we always want to reset the struct. */
    memset(ft, 0, sizeof(FILE_TRANSFER));
}

static bool resumeable_name(FILE_TRANSFER *ft, char *name) {
    if (ft->incoming) {
        char hex[TOX_HASH_LENGTH * 2];
        fid_to_string(hex, ft->data_hash);
        snprintf(name, UTOX_FILE_NAME_LENGTH, "%.*s.ftinfo", TOX_HASH_LENGTH * 2, hex);

        uint8_t blank_id[TOX_HASH_LENGTH] = { 0 };
        if (memcmp(ft->data_hash, blank_id, TOX_HASH_LENGTH) == 0) {
            LOG_ERR("FileTransfer", "Unable to use current data hash for incoming file.\n"
                        "\t\tuTox can't resume file %.*s\n"
                        "\t\tHash is %.*s\n",
                        (uint32_t)ft->name_length, ft->name,
                        TOX_HASH_LENGTH * 2, name);
            return false;
        }
    } else {
        snprintf(name, UTOX_FILE_NAME_LENGTH, "%.*s%02i.ftoutfo",
                 TOX_PUBLIC_KEY_SIZE * 2, get_friend(ft->friend_number)->id_str,
                    ft->file_number % 100);
    }

    return true;
}

static bool ft_update_resumable(FILE_TRANSFER *ft) {
    if (!ft->resume_file) {
        LOG_ERR("FileTransfer", "Unable to save filetransfer info. Got NULL file pointer.");
        return false;
    }

    // This file pointer has a tendency of being both invalid and non-null so we use fstat to check it.
    struct stat buffer;
    if (fstat(fileno(ft->resume_file), &buffer) != 0) {
        LOG_ERR("FileTransfer", "Unable to save file info. Invalid filepointer.");
        return false;
    }

    fseeko(ft->resume_file, SEEK_SET, 0);
    if (fwrite(ft, sizeof(FILE_TRANSFER), 1, ft->resume_file) != 1) {
        LOG_ERR("FileTransfer", "Unable to save file info... uTox can't resume file %.*s",
                ft->name_length, ft->name);
        return false;
    }

    fflush(ft->resume_file);
    return true;
}

/* Create the file transfer resume info file. */
static bool ft_init_resumable(FILE_TRANSFER *ft) {
    char name[UTOX_FILE_NAME_LENGTH];
    if (!resumeable_name(ft, name)) {
        return false;
    }

    ft->resume_file = utox_get_file(name, NULL, UTOX_FILE_OPTS_WRITE | UTOX_FILE_OPTS_MKDIR);
    if (!ft->resume_file) {
        return false;
    }

    LOG_INFO("FileTransfer", ".ftinfo for file %.*s set; ready to resume!" , (uint32_t)ft->name_length, ft->name);
    return ft_update_resumable(ft);
}

/* Free/Remove/Unlink the file transfer resume info file. */
static void ft_decon_resumable(FILE_TRANSFER *ft) {
    char name[UTOX_FILE_NAME_LENGTH];
    if (!resumeable_name(ft, name)) {
        return;
    }

    LOG_INFO("FileTransfer", "Going to decon file %s." , name);
    FILE *file = utox_get_file(name, NULL, UTOX_FILE_OPTS_READ | UTOX_FILE_OPTS_WRITE);
    if (!file) {
        return;
    }

    fclose(file);
    utox_get_file(name, NULL, UTOX_FILE_OPTS_DELETE);
}

static bool ft_find_resumeable(FILE_TRANSFER *ft) {
    char resume_name[UTOX_FILE_NAME_LENGTH];
    if (!resumeable_name(ft, resume_name)) {
        return false;
    }

    size_t size = 0;
    FILE *resume_disk = utox_get_file(resume_name, &size, UTOX_FILE_OPTS_READ);

    if (!resume_disk) {
        if (ft->incoming) {
            LOG_INFO("FileTransfer", "Unable to load saved info... uTox can't resume file %.*s",
                     (uint32_t)ft->name_length, ft->name);
        }
        ft->status = 0;
        return false;
    }

    if (size != sizeof(FILE_TRANSFER)) {
        LOG_ERR("FileTransfer", "Unable to resume this file, size mismatch");
        fclose(resume_disk);
        return false;
    }

    FILE_TRANSFER resume_file;
    bool read_resumeable = fread(&resume_file, size, 1, resume_disk);
    fclose(resume_disk);

    if (!read_resumeable) {
        LOG_ERR("FileTransfer", "Failed to read resumeable file.");
        return false;
    }

    if (!resume_file.resumeable
        || !resume_file.in_use
        || resume_file.in_memory
        || resume_file.avatar
        || resume_file.inline_img) {
        return false;
    }

    memcpy(ft, &resume_file, sizeof(FILE_TRANSFER));

    ft->name_length = 0;
    uint8_t *p = ft->path + strlen((char *)ft->path);
    while (*--p != '/' && *p != '\\') {
        ++ft->name_length;
    }
    ++p;
    ++ft->name_length;

    ft->name = calloc(1, ft->name_length + 1);
    if (!ft->name) {
        LOG_FATAL_ERR(EXIT_MALLOC, "FileTransfer", "Could not alloc for file name (%uB)",
                      ft->name_length + 1);
    }
    snprintf((char *)ft->name, ft->name_length + 1, "%s", p);

    ft->via.file = NULL;
    ft->resume_file = NULL;
    ft->ui_data = NULL;

    return true;
}

/* Cancel active file. */
static void kill_file(FILE_TRANSFER *file) {
    switch (file->status) {
        case FILE_TRANSFER_STATUS_KILLED: {
            LOG_WARN("FileTransfer", "File already killed.");
            return;
        }

        case FILE_TRANSFER_STATUS_COMPLETED: {
            LOG_WARN("FileTransfer", "File already completed.");
            return;
        }

        default: {
            break;
        }
    }

    file->status = FILE_TRANSFER_STATUS_KILLED;
    postmessage_utox(FILE_STATUS_DONE, file->status, 0, file->ui_data);

    if (file->resumeable) {
        ft_decon_resumable(file);
    }

    ft_decon(file->friend_number, file->file_number);
}

/* Break active file, (when a friend goes offline). */
static void break_file(FILE_TRANSFER *file) {
    if (!file) {
        return;
    }

    switch (file->status) {
        case FILE_TRANSFER_STATUS_NONE: {
            return kill_file(file);
        }

        case FILE_TRANSFER_STATUS_COMPLETED:
        case FILE_TRANSFER_STATUS_KILLED: {
            // We don't break files that are already broken.
            return;
        }

        default: {
            break;
        }
    }

    file->status = FILE_TRANSFER_STATUS_BROKEN;
    postmessage_utox(FILE_STATUS_DONE, file->status, 0, file->ui_data);

    if (file->resumeable) {
        ft_update_resumable(file);
    }

    ft_decon(file->friend_number, file->file_number);
}

/* Pause active file. */
static void utox_pause_file(FILE_TRANSFER *file, bool us) {
    switch (file->status) {
        case FILE_TRANSFER_STATUS_NONE: {
            if (!file->incoming) {
                // New transfers start as paused them
                file->status = FILE_TRANSFER_STATUS_PAUSED_THEM;
            } else {
                LOG_TRACE("FileTransfer", "We can't pause an unaccepted file!");
            }
            break;
        }

        case FILE_TRANSFER_STATUS_ACTIVE: {
            if (us) {
                LOG_TRACE("FileTransfer", "File now paused by us.");
                file->status = FILE_TRANSFER_STATUS_PAUSED_US;
            } else {
                LOG_TRACE("FileTransfer", "File now paused by them.");
                file->status = FILE_TRANSFER_STATUS_PAUSED_THEM;
            }
            break;
        }

        case FILE_TRANSFER_STATUS_PAUSED_US:
        case FILE_TRANSFER_STATUS_PAUSED_BOTH:
        case FILE_TRANSFER_STATUS_PAUSED_THEM: {
            if (us) {
                if (file->status == FILE_TRANSFER_STATUS_PAUSED_US) {
                    LOG_TRACE("FileTransfer", "File already paused by us!");
                } else if (file->status == FILE_TRANSFER_STATUS_PAUSED_THEM) {
                    file->status = FILE_TRANSFER_STATUS_PAUSED_BOTH;
                    LOG_TRACE("FileTransfer", "File now paused by both!");
                } else if (file->status == FILE_TRANSFER_STATUS_PAUSED_BOTH) {
                    LOG_TRACE("FileTransfer", "File already paused by both!");
                } else {
                    file->status = FILE_TRANSFER_STATUS_PAUSED_US;
                }
            } else {
                if (file->status == FILE_TRANSFER_STATUS_PAUSED_US) {
                    file->status = FILE_TRANSFER_STATUS_PAUSED_BOTH;
                    LOG_TRACE("FileTransfer", "File now paused by both!");
                } else if (file->status == FILE_TRANSFER_STATUS_PAUSED_THEM) {
                    LOG_TRACE("FileTransfer", "File was already paused by them!");
                } else if (file->status == FILE_TRANSFER_STATUS_PAUSED_BOTH) {
                    LOG_TRACE("FileTransfer", "File already paused by both!");
                } else {
                    file->status = FILE_TRANSFER_STATUS_PAUSED_THEM;
                }
            }
            break;
        }

        case FILE_TRANSFER_STATUS_BROKEN: {
            LOG_TRACE("FileTransfer", "Can't pause a broken file;");
            break;
        }

        case FILE_TRANSFER_STATUS_COMPLETED: {
            LOG_TRACE("FileTransfer", "Can't pause a completed file;");
            break;
        }

        case FILE_TRANSFER_STATUS_KILLED: {
            LOG_TRACE("FileTransfer", "Can't pause a killed file;");
            break;
        }
    }

    FILE_TRANSFER *msg = calloc(1, sizeof(FILE_TRANSFER));
    if (!msg) {
        LOG_ERR("FileTransfer", "Unable to malloc for internal message. (This is bad!)");
        return;
    }

    *msg = *file;
    postmessage_utox(FILE_STATUS_UPDATE, file->status, 0, msg);
    // TODO free not freed data.
}

/* Start/Resume active file. */
static void run_file_local(FILE_TRANSFER *file) {
    switch (file->status) {
        case FILE_TRANSFER_STATUS_NONE: {
            file->status = FILE_TRANSFER_STATUS_ACTIVE;
            if (!file->resumeable && file->incoming) {
                file->resumeable = ft_init_resumable(file);
            }
            break;
        }

        case FILE_TRANSFER_STATUS_PAUSED_US: {
            file->status = FILE_TRANSFER_STATUS_ACTIVE;
            break;
        }

        case FILE_TRANSFER_STATUS_PAUSED_BOTH: {
            file->status = FILE_TRANSFER_STATUS_PAUSED_THEM;
            break;
        }

        case FILE_TRANSFER_STATUS_ACTIVE:
        case FILE_TRANSFER_STATUS_PAUSED_THEM: {
            return;
        }

        case FILE_TRANSFER_STATUS_BROKEN:
        case FILE_TRANSFER_STATUS_COMPLETED:
        case FILE_TRANSFER_STATUS_KILLED: {
            LOG_ERR("FileTransfer", "We tried to run file from an unknown state! (%u)" , file->status);
            return;
        }
    }

    FILE_TRANSFER *msg = calloc(1, sizeof(FILE_TRANSFER));
    if (!msg) {
        LOG_ERR("FileTransfer", "Unable to malloc for internal message. (This is bad!)");
        return;
    }

    *msg = *file;
    postmessage_utox(FILE_STATUS_UPDATE, file->status, 0, msg);
}

static void run_file_remote(FILE_TRANSFER *file) {
    switch (file->status) {
        case FILE_TRANSFER_STATUS_PAUSED_US: {
            break;
        }

        case FILE_TRANSFER_STATUS_PAUSED_BOTH: {
            file->status = FILE_TRANSFER_STATUS_PAUSED_US;
            break;
        }

        case FILE_TRANSFER_STATUS_PAUSED_THEM:
        case FILE_TRANSFER_STATUS_BROKEN: {
            file->status = FILE_TRANSFER_STATUS_ACTIVE;
            break;
        }

        default: {
            LOG_ERR("FileTransfer", "They tried to run file from an unknown state! (%u)" , file->status);
            break;
        }
    }

    FILE_TRANSFER *msg = calloc(1, sizeof(FILE_TRANSFER));
    if (!msg) {
        LOG_ERR("FileTransfer", "Unable to malloc for internal message. (This is bad!)");
        return;
    }

    *msg = *file;
    postmessage_utox(FILE_STATUS_UPDATE, file->status, 0, msg);
}

static void decode_inline_png(uint32_t friend_id, uint8_t *data, uint64_t size) {
    // TODO: start a new thread and decode the png in it.
    uint16_t width, height;
    // TODO: move the decode out of file_transfers.c
    NATIVE_IMAGE *native_image = utox_image_to_native((UTOX_IMAGE)data, size, &width, &height, 0);
    if (NATIVE_IMAGE_IS_VALID(native_image)) {
        uint8_t *msg = malloc(sizeof(uint16_t) * 2 + sizeof(NATIVE_IMAGE *));
        if (!msg) {
            LOG_ERR("decode_inline_png", "Unable to malloc for inline data.");
            free(native_image);
            return;
        }

        memcpy(msg, &width, sizeof(uint16_t));
        memcpy(msg + sizeof(uint16_t), &height, sizeof(uint16_t));
        memcpy(msg + sizeof(uint16_t) * 2, &native_image, sizeof(NATIVE_IMAGE *));

        postmessage_utox(FILE_INCOMING_NEW_INLINE, friend_id, 0, msg);
    }
}

/* Complete active file, (when the whole file transfer is successful). */
static void utox_complete_file(FILE_TRANSFER *file) {
    FILE_TRANSFER *msg = calloc(1, sizeof(FILE_TRANSFER));
    if (!msg) {
        LOG_ERR("FileTransfer", "Unable to malloc for internal message. (This is bad!)");
        return;
    }

    *msg = *file;
    postmessage_utox(FILE_STATUS_UPDATE, file->status, 0, msg);

    if (file->status == FILE_TRANSFER_STATUS_ACTIVE) {
        file->status = FILE_TRANSFER_STATUS_COMPLETED;
        if (file->incoming) {
            if (file->inline_img) {
                decode_inline_png(file->friend_number, file->via.memory, file->current_size);
                postmessage_utox(FILE_INCOMING_NEW_INLINE_DONE, file->friend_number, 0, file);
            } else if (file->avatar) {
                postmessage_utox(FRIEND_AVATAR_SET, file->friend_number, file->current_size, file->via.avatar);
            }
        }
        file->decon_wait = true;
        postmessage_utox(FILE_STATUS_UPDATE_DATA, file->status, 0, file);
    } else {
        LOG_ERR("FileTransfer", "Unable to complete file in non-active state (file:%u)" , file->file_number);
    }
    LOG_NOTE("FileTransfer", "File transfer is done (%u & %u)" , file->friend_number, file->file_number);
    postmessage_utox(FILE_STATUS_DONE, file->status, 0, file->ui_data);

    if (file->resumeable) {
        ft_decon_resumable(file);
    }

    ft_decon(file->friend_number, file->file_number);
}

/* Friend has come online, restart our outgoing transfers to this friend. */
void ft_friend_online(Tox *tox, uint32_t friend_number) {
    for (uint16_t i = 0; i < MAX_FILE_TRANSFERS; i++) {
        FILE_TRANSFER *file = calloc(1, sizeof(FILE_TRANSFER));
        if (!file) {
            LOG_FATAL_ERR(EXIT_MALLOC, "FileTransfer", "Could not alloc file transfer struct");
        }
        file->friend_number = friend_number;
        file->file_number   = i;
        file->incoming      = false;
        ft_find_resumeable(file);
        if (file->path[0]) {
            /* If we got a path from utox_file_load we should try to resume! */
            file->via.file = fopen((char *)file->path, "rb+");
            ft_send_file(tox, friend_number, file->via.file, file->path, strlen((char *)file->path), file->data_hash);
        }

        char name[UTOX_FILE_NAME_LENGTH];
        if (resumeable_name(file, name)) {
            LOG_INFO("FileTransfer", "Loading outgoing %u, deleting origin file.", i);
            utox_get_file(name, NULL, UTOX_FILE_OPTS_DELETE);
        }

        free(file);
    }
}

/* Friend has gone offline, break our outgoing transfers to this friend. */
void ft_friend_offline(Tox *UNUSED(tox), uint32_t friend_number) {
    LOG_NOTE("FileTransfer", "Friend %u has gone offline, breaking transfers" , friend_number);

    FRIEND *f = get_friend(friend_number);
    if (!f) {
        return;
    }

    for (uint16_t i = 0; i < f->ft_outgoing_size; ++i) {
        break_file(&f->ft_outgoing[i]);
    }

    for (uint16_t i = 0; i < f->ft_incoming_size; ++i) {
        break_file(&f->ft_incoming[i]);
    }
}

/* Local command callback to change a file status. */
void ft_local_control(Tox *tox, uint32_t friend_number, uint32_t file_number, TOX_FILE_CONTROL control) {
    FILE_TRANSFER *info  = get_file_transfer(friend_number, file_number);
    if (!info) {
        LOG_ERR("FileTransfer", "We know nothing of this file. This is probably an error. Friend(%u) FileNum(%u)",
                friend_number, file_number);
        return;
    }

    TOX_ERR_FILE_CONTROL error = 0;
    switch (control) {
        case TOX_FILE_CONTROL_RESUME: {
            if (info->status != FILE_TRANSFER_STATUS_ACTIVE) {
                if (get_friend(friend_number)->ft_outgoing_size < MAX_FILE_TRANSFERS) {
                    if (tox_file_control(tox, friend_number, file_number, control, &error)) {
                        LOG_INFO("FileTransfer", "We just resumed file (%u & %u)" , friend_number, file_number);
                    } else {
                        LOG_INFO("FileTransfer", "Toxcore doesn't like us! (%u & %u)" , friend_number, file_number);
                    }
                } else {
                    LOG_INFO("FileTransfer", "Can't start file, max file transfer limit reached! (%u & %u)",
                          friend_number, file_number);
                }
            } else {
                LOG_INFO("FileTransfer", "File already active (%u & %u)" , friend_number, file_number);
            }
            run_file_local(info);
            break;
        }

        case TOX_FILE_CONTROL_PAUSE: {
            if (info->status != FILE_TRANSFER_STATUS_PAUSED_US && info->status != FILE_TRANSFER_STATUS_PAUSED_BOTH) {
                if (tox_file_control(tox, friend_number, file_number, control, &error)) {
                    LOG_INFO("FileTransfer", "We just paused file (%u & %u)" , friend_number, file_number);
                } else {
                    LOG_INFO("FileTransfer", "Toxcore doesn't like us! (%u & %u)" , friend_number, file_number);
                }
            } else {
                LOG_INFO("FileTransfer", "File already paused (%u & %u)" , friend_number, file_number);
            }
            utox_pause_file(info, true);
            break;
        }

        case TOX_FILE_CONTROL_CANCEL: {
            if (info->status != FILE_TRANSFER_STATUS_KILLED) {
                if (tox_file_control(tox, friend_number, file_number, control, &error)) {
                    LOG_INFO("FileTransfer", "We just killed file (%u & %u)" , friend_number, file_number);
                } else {
                    LOG_INFO("FileTransfer", "Toxcore doesn't like us! (%u & %u)" , friend_number, file_number);
                }
            } else {
                LOG_INFO("FileTransfer", "File already killed (%u & %u)" , friend_number, file_number);
            }
            kill_file(info);
            break;
        }
    }
    /* Do something with the error! */
    switch (error) {
        case TOX_ERR_FILE_CONTROL_OK: {
            // Everything's fine.
            break;
        }

        case TOX_ERR_FILE_CONTROL_FRIEND_NOT_FOUND: {
            LOG_ERR("FileTransfer", "Unable to send command, Friend (%u) doesn't exist!" , info->friend_number);
            break;
        }

        case TOX_ERR_FILE_CONTROL_FRIEND_NOT_CONNECTED: {
            LOG_ERR("FileTransfer", "Unable to send command, Friend (%u) offline!" , info->friend_number);
            break;
        }

        case TOX_ERR_FILE_CONTROL_NOT_FOUND: {
            LOG_ERR("FileTransfer", "Unable to send command, ft (%u) doesn't exist!" , info->friend_number);
            break;
        }

        case TOX_ERR_FILE_CONTROL_DENIED: {
            LOG_ERR("FileTransfer", "Unable to send command, ft (%u) paused by other party." , info->friend_number);
            break;
        }

        default: {
            LOG_ERR("FileTransfer", "FileTransfer:\tThere was an error(%u) sending the command."
                        "You probably want to see to that!\n", error);
            break;
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
            LOG_TRACE("FileTransfer", "Friend (%i) has resumed file (%i)" , friend_number, file_number);
            run_file_remote(ft);
            break;
        }

        case TOX_FILE_CONTROL_PAUSE: {
            LOG_TRACE("FileTransfer", "Friend (%i) has paused file (%i)" , friend_number, file_number);
            utox_pause_file(ft, false);
            break;
        }

        case TOX_FILE_CONTROL_CANCEL: {
            if (ft->avatar) {
                LOG_TRACE("FileTransfer", "Friend (%i) rejected avatar" , friend_number);
            } else {
                LOG_TRACE("FileTransfer", "Friend (%i) has canceled file (%i)" , friend_number, file_number);
            }
            kill_file(ft);
            break;
        }
    }
}

static void incoming_avatar(Tox *tox, uint32_t friend_number, uint32_t file_number, uint64_t size) {
    LOG_TRACE("FileTransfer", "Incoming avatar from friend %u." , friend_number);

    FRIEND *f = get_friend(friend_number);
    if (!f) {
        LOG_ERR("FileTransfer", "This friend doesn't exist... This is bad!");
        return;
    }

    if (size == 0) {
        LOG_TRACE("FileTransfer", "Avatar from friend %u deleted." , friend_number);
        postmessage_utox(FRIEND_AVATAR_UNSET, friend_number, 0, NULL);
        ft_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL);
        return;
    } else if (size > UTOX_AVATAR_MAX_DATA_LENGTH) {
        LOG_TRACE("FileTransfer", "Avatar from friend(%u) rejected. (Too Large %lu)" , friend_number, size);
        ft_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL);
        return;
    }

    uint8_t file_id[TOX_FILE_ID_LENGTH] = { 0 };
    tox_file_get_file_id(tox, friend_number, file_number, file_id, 0);

    /* Verify this is a new avatar */
    if (f->avatar->format && memcmp(f->avatar->hash, file_id, TOX_HASH_LENGTH) == 0) {
        LOG_TRACE("FileTransfer", "Avatar from friend (%u) rejected: Same as Current" , friend_number);
        ft_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL);
        return;
    }

    FILE_TRANSFER *ft = make_file_transfer(friend_number, file_number);
    if (!ft) {
        LOG_ERR("FileTransfer", "Unable to malloc ft to accept incoming avatar!");
        tox_file_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL, NULL);
        return;
    }

    f->ft_incoming_active_count++;

    memset(ft, 0, sizeof(FILE_TRANSFER));
    ft->in_use = true;

    ft->friend_number = friend_number;
    ft->file_number   = file_number;
    ft->target_size   = size;

    tox_file_get_file_id(tox, friend_number, file_number, ft->data_hash, NULL);

    ft->incoming  = true;
    ft->avatar    = true;

    ft->via.avatar = calloc(1, size);
    if (!ft->via.avatar) {
        LOG_ERR("FileTransfer", "Unable to malloc for incoming avatar");
        ft_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL);
        return;
    }

    ft->status = FILE_TRANSFER_STATUS_PAUSED_US;
    ft_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_RESUME);
}

static void incoming_inline_image(Tox *tox, uint32_t friend_number, uint32_t file_number, size_t size) {
    LOG_INFO("FileTransfer", "Getting an incoming inline image");

    FRIEND *f = get_friend(friend_number);
    if (!f) {
        LOG_ERR("FileTransfer", "This friend doesn't exist... This is bad!");
        return;
    }

    FILE_TRANSFER *ft = make_file_transfer(friend_number, file_number);
    if (!ft) {
        LOG_ERR("FileTransfer", "Unable to malloc ft to accept incoming inline image!");
        tox_file_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL, NULL);
        return;
    }

    f->ft_incoming_active_count++;

    memset(ft, 0, sizeof(FILE_TRANSFER));
    ft->in_use      = true;

    ft->incoming    = true;
    ft->in_memory   = true;
    ft->inline_img  = true;

    ft->friend_number = friend_number;
    ft->file_number = file_number;

    ft->target_size = size;

    ft->via.memory = calloc(1, size);
    if (!ft->via.memory) {
        LOG_ERR("FileTransfer", "Unable to malloc enough memory for incoming inline image of size %lu" , size);
        ft_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL);
        return;
    }

    LOG_NOTE("FileTransfer", "Starting incoming inline image of size %lu" , size);
    ft_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_RESUME);

    ft->name = (uint8_t *)strdup("utox-inline.png");
    ft->name_length = strlen("utox-inline.png");
    if (!ft->name) {
        LOG_ERR("FileTransfer", "Error, couldn't allocate memory for ft->name.");
        ft->name_length = 0;
    }
}

/* Function called by core with a new file send request from a friend. */
static void incoming_file_callback_request(Tox *tox, uint32_t friend_number, uint32_t file_number, uint32_t kind,
                                           uint64_t size, const uint8_t *name, size_t name_length,
                                           void *UNUSED(user_data))
{
    LOG_NOTE("FileTransfer", "New incoming file transfer request from friend %u" , friend_number);

    FRIEND *f = get_friend(friend_number);
    if (f->ft_incoming_active_count >= MAX_INCOMING_COUNT) {
        LOG_ERR("FileTransfer", "Too many incoming file transfers from friend %u", friend_number);
        /* ft_local_control is preferred, but in this case it can't access the ft struct. */
        tox_file_control(tox, friend_number, file_number, TOX_FILE_CANCEL, NULL);
        return;
    }

    if (kind == TOX_FILE_KIND_AVATAR) {
        return incoming_avatar(tox, friend_number, file_number, size);
    }

    if (settings.accept_inline_images
        && size < MAX_INLINE_FILESIZE
        && name_length == (sizeof("utox-inline.png") - 1)
        && memcmp(name, "utox-inline.png", name_length) == 0)
    {
        return incoming_inline_image(tox, friend_number, file_number, size);
    }

    FILE_TRANSFER *ft = make_file_transfer(friend_number, file_number);
    if (!ft) {
        LOG_ERR("FileTransfer", "Unable to get memory handle for transfer, canceling friend/file number (%u/%u)",
              friend_number, file_number);
        tox_file_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL, 0);
        return;
    }
    f->ft_incoming_active_count++;

    memset(ft, 0, sizeof(FILE_TRANSFER));
    ft->in_use = true;

    // Preload some data needed by ft_find_resumeable
    ft->friend_number = friend_number;
    ft->file_number   = file_number;
    ft->incoming      = true;
    tox_file_get_file_id(tox, friend_number, file_number, ft->data_hash, NULL);
    ft->name = calloc(1, name_length + 1);
    if (!ft->name) {
        LOG_FATAL_ERR(EXIT_MALLOC, "FileTransfer", "Could not allocate space for file name (%uB)",
                      name_length + 1);
    }
    snprintf((char *)ft->name, name_length + 1, "%.*s", (int)name_length, name);
    ft->name_length = name_length;

    /* access the correct memory location for this file */
    /* Load saved information about this file */
    if (ft_find_resumeable(ft)) {
        LOG_NOTE("FileTransfer", "Incoming Existing file from friend (%u) " , friend_number);
        FILE *file = fopen((const char *)ft->path, "rb+");
        if (file) {
            LOG_INFO("FileTransfer", "Cool file exists, let try to restart it.");
            ft->in_use        = true;
            ft->in_memory     = false;
            ft->avatar        = false;
            ft->friend_number = friend_number;
            ft->file_number   = file_number;
            ft->target_size   = size;
            ft->via.file      = file;

            FILE_TRANSFER *msg = calloc(1, sizeof(FILE_TRANSFER));
            if (!msg) {
                LOG_ERR("FileTransfer", "Unable to malloc for internal message. (This is bad!)");
                return;
            }
            *msg = *ft;
            postmessage_utox(FILE_SEND_NEW, friend_number, file_number, msg);
            TOX_ERR_FILE_SEEK error = 0;
            tox_file_seek(tox, friend_number, file_number, ft->current_size, &error);
            if (error) {
                LOG_ERR("FileTransfer", "seek error %i" , error);
                // TODO UI error here as well;
                ft_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL);
                return;
            }
            LOG_INFO("FileTransfer", "seek & resume");
            ft->status = FILE_TRANSFER_STATUS_NONE;
            ft_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_RESUME);
            ft->resumeable = ft_init_resumable(ft);
            return;
        }
        LOG_ERR("FileTransfer", "Unable to open file suggested by resume!");
        // This is fine-ish, we'll just fallback to new incoming file.
    }

    ft->friend_number = friend_number;
    ft->file_number   = file_number;

    ft->target_size = size;

    ft->resumeable = ft_init_resumable(ft);

    FILE_TRANSFER *msg = calloc(1, sizeof(FILE_TRANSFER));
    if (!msg) {
        LOG_ERR("FileTransfer", "Unable to malloc for internal message. (This is bad!)");
        return;
    }

    *msg = *ft;
    postmessage_utox(FILE_INCOMING_NEW, friend_number, detox_incoming_file_number(file_number), msg);
    /* The file doesn't exist on disk where we expected, let's prompt the user to accept it as a new file */
    LOG_NOTE("FileTransfer", "New incoming file from friend (%u) file number (%u)\nFileTransfer:\t\tfilename: %s",
          friend_number, file_number, name);
    /* Auto accept if it's a utox-inline image, with the correct size */
}

/* Called by toxcore to deliver the next chunk of incoming data. */
static void incoming_file_callback_chunk(Tox *tox, uint32_t friend_number, uint32_t file_number,
                                         uint64_t position, const uint8_t *data, size_t length, void *UNUSED(user_data))
{
    LOG_INFO("FileTransfer", "Incoming chunk friend(%u), file(%u), start(%lu), end(%lu), \n",
            friend_number, file_number, position, length);

    FILE_TRANSFER *ft = get_file_transfer(friend_number, file_number);
    if (!ft || !ft->in_use) {
        LOG_ERR("FileTransfer", "ERROR incoming chunk for an out of use file transfer!");
        return;
    }

    if (length == 0) {
        utox_complete_file(ft);
        return;
    }

    if (ft->inline_img && ft->via.memory) {
        if (position == 0) {
            uint8_t png_header[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a };
            if (memcmp(data, png_header, 8) != 0) {
                // this isn't a png header, just die
                LOG_ERR("FileTransfer", "Friend %u sent an inline image thats' not a PNG" , friend_number);
                ft_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL);
                return;
            }
        }
        memcpy(ft->via.memory + position, data, length);
    } else if (ft->avatar && ft->via.avatar) {
        memcpy(ft->via.avatar + position, data, length);
    } else if (ft->via.file) {
        uint8_t count = 10;
        while (!file_lock(ft->via.file, position, length)) {
            LOG_ERR("FileTransfer", "Can't get lock, sleeping...");
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
            LOG_ERR("FileTransfer", "\n\nFileTransfer:\tERROR WRITING DATA TO FILE! (%u & %u)\n\n", friend_number, file_number);
            ft_local_control(tox, friend_number, file_number, TOX_FILE_CANCEL);
            return;
        }
        calculate_speed(ft);
    } else {
        LOG_TRACE("FileTransfer", "File Handle failed!");
        ft_local_control(tox, friend_number, file_number, TOX_FILE_CANCEL);
        return;
    }

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
    if (!tox || !self.png_data) {
        LOG_ERR("FileTransfer", "Can't send an avatar without data");
        return UINT32_MAX;
    }
    LOG_NOTE("FileTransfer", "Starting avatar to friend %u." , friend_number);

    // TODO send the unset avatar command.

    FRIEND *f = get_friend(friend_number);
    if (!f) {
        LOG_ERR("FileTransfer", "Unable to get friend %u to send avatar.", friend_number);
        return UINT32_MAX;
    }

    if (f->ft_outgoing_active_count > MAX_FILE_TRANSFERS) {
        LOG_ERR("FileTransfer", "Can't send this avatar too many in progress...");
        return UINT32_MAX;
    }

    /* While It's not ideal, we don't make sure we can alloc the FILE_TRANSFER until
     * we get the file number from toxcore. This could happen, but I assume it'll be
     * rare enough. Either way, it'll be noisy if it fails so here's to hoping! */
    uint8_t hash[TOX_HASH_LENGTH];
    tox_hash(hash, self.png_data, self.png_size);

    TOX_ERR_FILE_SEND error = 0;
    uint32_t file_number = tox_file_send(tox, friend_number, TOX_FILE_KIND_AVATAR,
                                         self.png_size, hash, NULL, 0, &error);
    if (error || file_number == UINT32_MAX) {
        LOG_ERR("FileTransfer", "tox_file_send() failed error code %u", error);
        return UINT32_MAX;
    };

    FILE_TRANSFER *ft = make_file_transfer(friend_number, file_number);
    if (!ft) {
        // This is the noisy case noted above.
        LOG_ERR("FileTransfer", "Unable to malloc to actually send avatar!");
        tox_file_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL, NULL);
        return UINT32_MAX;
    }
    /* All errors handled */
    ++f->ft_outgoing_active_count;

    memset(ft, 0, sizeof(FILE_TRANSFER));
    ft->in_use = true;

    ft->incoming = false;
    ft->avatar = true;
    ft->friend_number = friend_number;
    ft->file_number = file_number;
    memcpy(ft->data_hash, hash, TOX_HASH_LENGTH);
    ft->target_size = self.png_size;
    ft->status = FILE_TRANSFER_STATUS_PAUSED_THEM;

    LOG_INFO("FileTransfer", "File transfer #%u sent to friend %u", ft->file_number, ft->friend_number);
    return file_number;
}

uint32_t ft_send_file(Tox *tox, uint32_t friend_number, FILE *file, uint8_t *path, size_t path_length, uint8_t *hash) {
    if (!tox || !file) {
        LOG_ERR("FileTransfer", "Can't send a file without data");
        return UINT32_MAX;
    }
    LOG_TRACE("FileTransfer", "Starting FILE to friend %u." , friend_number);

    FRIEND *f = get_friend(friend_number);
    if (!f) {
        LOG_ERR("FileTransfer", "Unable to get friend %u to send file.", friend_number);
        return UINT32_MAX;
    }

    if (f->ft_outgoing_active_count > MAX_FILE_TRANSFERS) {
        LOG_ERR("FileTransfer", "Can't send this file too many in progress...");
        return UINT32_MAX;
    }

    fseeko(file, 0, SEEK_END);
    size_t size = ftello(file);

    const uint8_t *name = path + path_length;
    size_t name_length = 0;
    while (*--name != '/' && *name != '\\') { // TODO remove widows style path support from uTox.
        ++name_length;
    }
    ++name;

    TOX_ERR_FILE_SEND error = 0;
    uint32_t file_number = tox_file_send(tox, friend_number, TOX_FILE_KIND_DATA, size, hash, name, name_length, &error);
    if (error || file_number == UINT32_MAX) {
        switch (error) {
            case TOX_ERR_FILE_SEND_NULL: {
                LOG_ERR("FileTransfer", "Error, Toxcore reports NULL"); break;
            }
            case TOX_ERR_FILE_SEND_FRIEND_NOT_FOUND: {
                LOG_ERR("FileTransfer", "Error, friend Not found."); break;
            }
            case TOX_ERR_FILE_SEND_FRIEND_NOT_CONNECTED: {
                LOG_ERR("FileTransfer", "Error, friend not connected."); break;
            }
            case TOX_ERR_FILE_SEND_NAME_TOO_LONG: {
                LOG_ERR("FileTransfer", "Error, name too long '%s'" , name); break;
            }
            case TOX_ERR_FILE_SEND_TOO_MANY: {
                LOG_ERR("FileTransfer", "Error, too many files in progress"); break;
            }
            case TOX_ERR_FILE_SEND_OK: { break; }
        }
        LOG_ERR("FileTransfer", "tox_file_send() failed error code %u", error);
        return UINT32_MAX;
    }

    FILE_TRANSFER *ft = make_file_transfer(friend_number, file_number);
    if (!ft) {
        // This is the noisy case noted above.
        LOG_ERR("FileTransfer", "Unable to malloc to actually send file!");
        tox_file_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL, NULL);
        return UINT32_MAX;
    }
    ++f->ft_outgoing_active_count;

    memset(ft, 0, sizeof(FILE_TRANSFER));
    ft->in_use = true;

    ft->incoming      = false;
    ft->friend_number = friend_number;
    ft->file_number   = file_number;

    ft->target_size = size;

    ft->name = calloc(1, name_length + 1);
    if (!ft->name) {
        LOG_ERR("FileTransfer", "Error, couldn't allocate memory for ft->name.");
        --f->ft_outgoing_active_count;
        return UINT32_MAX;
    }
    ft->name_length = name_length;
    snprintf((char *)ft->name, name_length + 1, "%.*s", (int)name_length, name);

    snprintf((char *)ft->path, UTOX_FILE_NAME_LENGTH, "%.*s", (int)path_length, path);

    ft->via.file = file;
    tox_file_get_file_id(tox, friend_number, file_number, ft->data_hash, NULL);
    ft->resumeable = ft_init_resumable(ft);

    ft->status = FILE_TRANSFER_STATUS_PAUSED_THEM;

    FILE_TRANSFER *msg = calloc(1, sizeof(FILE_TRANSFER));
    if (!msg) {
        LOG_ERR("FileTransfer", "Unable to malloc for internal message. (This is bad!)");
        return UINT32_MAX;
    }
    *msg = *ft;
    postmessage_utox(FILE_SEND_NEW, friend_number, file_number, msg);
    return file_number;
}

/* Returns file number on success, UINT32_MAX on failure. */
uint32_t ft_send_data(Tox *tox, uint32_t friend_number, uint8_t *data, size_t size, uint8_t *name, size_t name_length) {
    if (!tox || !data || !name) {
        LOG_ERR("FileTransfer", "Can't send data to friend without data");
        return UINT32_MAX;
    }

    LOG_INFO("FileTransfer", "Starting raw data transfer to friend %u." , friend_number);

    // TODO send the unset avatar command.

    FRIEND *f = get_friend(friend_number);
    if (!f) {
        LOG_ERR("FileTransfer", "Unable to get friend %u to send raw data.", friend_number);
        return UINT32_MAX;
    }

    if (f->ft_outgoing_active_count >= MAX_FILE_TRANSFERS) {
        LOG_ERR("FileTransfer", "Can't send raw data too many in progress...");
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
        LOG_ERR("FileTransfer", "tox_file_send() failed error code %u", error);
        return UINT32_MAX;
    };

    FILE_TRANSFER *ft = make_file_transfer(friend_number, file_number);
    if (!ft) {
        // This is the noisy case noted above.
        LOG_ERR("FileTransfer", "Unable to malloc to actually send data!");
        tox_file_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL, NULL);
        return UINT32_MAX;
    }

    ++f->ft_outgoing_active_count;

    memset(ft, 0, sizeof(FILE_TRANSFER));
    ft->in_use     = true;

    ft->incoming   = false;
    ft->in_memory  = true;
    ft->inline_img = true;

    ft->name = calloc(1, name_length + 1);
    if (!ft->name) {
        LOG_ERR("FileTransfer", "Error, couldn't allocate memory for ft->name.");
        --f->ft_outgoing_active_count;
        return UINT32_MAX;
    }

    ft->name_length = name_length;
    snprintf((char *)ft->name, name_length + 1, "%.*s", (int)name_length, name);

    ft->friend_number = friend_number;
    ft->file_number = file_number;

    memcpy(ft->data_hash, hash, TOX_HASH_LENGTH);

    ft->via.memory = data;
    ft->target_size = size;
    ft->status = FILE_TRANSFER_STATUS_PAUSED_THEM;


    FILE_TRANSFER *msg = calloc(1, sizeof(FILE_TRANSFER));
    if (!msg) {
        LOG_ERR("FileTransfer", "Unable to malloc for internal message. (This is bad!)");
        return UINT32_MAX;
    }

    *msg = *ft;
    postmessage_utox(FILE_SEND_NEW, friend_number, file_number, msg);
    LOG_INFO("FileTransfer", "Inline image sent to friend. FT %u, Friend %u", ft->file_number, ft->friend_number);

    return file_number;
}

bool ft_set_ui_data(uint32_t friend_number, uint32_t file_number, MSG_HEADER *ui_data) {
    FILE_TRANSFER *file = get_file_transfer(friend_number, file_number);
    if (!file) {
        LOG_WARN("FileTransfer", "Unable to set ui_data for unknown file number %u with friend %u",
                 file_number, friend_number);
        return false;
    }

    file->ui_data = ui_data;
    return true;
}

static void outgoing_file_callback_chunk(Tox *tox, uint32_t friend_number, uint32_t file_number, uint64_t position,
                                         size_t length, void *UNUSED(user_data))
{
    LOG_INFO("FileTransfer", "Chunk requested for friend_id (%u), and file_id (%u). Start (%lu), End (%zu).\r",
            friend_number, file_number, position, length);

    FILE_TRANSFER *ft = get_file_transfer(friend_number, file_number);
    if (!ft) {
        LOG_ERR("FileTransfer", "Unabele to get file transfer (%u & %u)", friend_number, file_number);
        return;
    }

    if (length == 0) {
        LOG_NOTE("FileTransfer", "Outgoing transfer is done (%u & %u)", friend_number, file_number);
        utox_complete_file(ft);
        return;
    }

    if (position + length > ft->target_size) {
        LOG_ERR("FileTransfer", "Outing transfer size mismatch!");
        return;
    }

    TOX_ERR_FILE_SEND_CHUNK error = 0;
    if (ft->in_memory) {
        if (!ft->via.memory) {
            LOG_ERR("FileTransfer", "ERROR READING FROM MEMORY! (%u & %u)", friend_number, file_number);
            return;
        }

        tox_file_send_chunk(tox, friend_number, file_number, position, ft->via.memory + position, length, &error);
        if (error) {
            LOG_ERR("FileTransfer", "Outgoing chunk error on memory (%u)", error);
        }

        calculate_speed(ft);
    } else if (ft->avatar) {
        if (!self.png_data) {
            LOG_ERR("FileTransfer", "ERROR READING FROM AVATAR! (%u & %u)", friend_number, file_number);
            return;
        }

        tox_file_send_chunk(tox, friend_number, file_number, position, self.png_data + position, length, &error);
        if (error) {
            LOG_ERR("FileTransfer", "Outgoing chunk error on avatar (%u)", error);
        }
    } else { // File
        if (ft->via.file) {
            uint8_t buffer[length];
            fseeko(ft->via.file, position, SEEK_SET);
            if (fread(buffer, length, 1, ft->via.file) != 1) {
                LOG_ERR("FileTransfer", "ERROR READING FILE! (%u & %u)", friend_number, file_number);
                LOG_INFO("FileTransfer", "Size (%lu), Position (%lu), Length(%lu), size_transferred (%lu).",
                         ft->target_size, position, length, ft->current_size);
                ft_local_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL);
                return;
            }

            tox_file_send_chunk(tox, friend_number, file_number, position, buffer, length, &error);
            if (error) {
                LOG_ERR("FileTransfer", "Outgoing chunk error on file (%u)", error);
            }
        }
        calculate_speed(ft);
    }

    ft->current_size += length;
}

bool utox_file_start_write(uint32_t friend_number, uint32_t file_number, const char *file) {
    FILE_TRANSFER *ft = get_file_transfer(friend_number, file_number);
    if (!ft || !file) {
        LOG_ERR("FileTransfer", "FileTransfer:\tUnable to grab a file to start the write friend %u, file %u.",
                    friend_number, file_number);
        return false;
    }

    snprintf((char *)ft->path, UTOX_FILE_NAME_LENGTH, "%s", file);

    ft->via.file = utox_get_file_simple((char *)ft->path, UTOX_FILE_OPTS_WRITE);
    if (!ft->via.file) {
        LOG_ERR("FileTransfer", "The file we're supposed to write to couldn't be opened\n\t\t\"%s\"", ft->path);
        break_file(ft);
        return false;
    }

    return true;
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
