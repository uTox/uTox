#include "main.h"

#include "gtk.h"

#include "../chatlog.h"
#include "../debug.h"
#include "../file_transfers.h"
#include "../filesys.h"
#include "../friend.h"
#include "../groups.h"
#include "../settings.h"
#include "../tox.h"

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#if 0 // commented because this function is deprecated, but I'm not ready to delete all this code yet
/** Takes data from µTox and saves it, just how the OS likes it saved! */
size_t native_save_data(const uint8_t *name, size_t name_length, const uint8_t *data, size_t length, bool append) {
    char path[UTOX_FILE_NAME_LENGTH]        = { 0 };
    char atomic_path[UTOX_FILE_NAME_LENGTH] = { 0 };

    FILE *file;

    size_t offset = 0;

    if (settings.portable_mode) {
        snprintf(path, UTOX_FILE_NAME_LENGTH, "./tox/");
    } else {
        snprintf(path, UTOX_FILE_NAME_LENGTH, "%s/.config/tox/", getenv("HOME"));
    }

    mkdir(path, 0700);

    snprintf(path + strlen(path), UTOX_FILE_NAME_LENGTH - strlen(path), "%s", name);

    if (append) {
        file = fopen(path, "ab");
    } else {
        if (strlen(path) + name_length >= UTOX_FILE_NAME_LENGTH - strlen(".atomic")) {
            LOG_TRACE("Filesys", "Save directory name too long" );
            return 0;
        } else {
            snprintf(atomic_path, UTOX_FILE_NAME_LENGTH, "%s.atomic", path);
        }
        file = fopen(atomic_path, "wb");
    }

    if (file) {
        offset = ftello(file);
        fwrite(data, length, 1, file);
        fclose(file);

        if (append) {
            return offset;
        }

        if (rename(atomic_path, path)) {
            /* Consider backing up this file instead of overwriting it. */
            LOG_TRACE("Filesys", "%sUnable to move file!" , atomic_path);
            return 0;
        }
        return 1;
    } else {
        LOG_TRACE("Filesys", "Unable to open %s to write save" , path);
        return 0;
    }

    return 0;
}
#endif

/** Takes data from µTox and loads it up! */
uint8_t *native_load_data(const uint8_t *name, size_t name_length, size_t *out_size) {
    char path[UTOX_FILE_NAME_LENGTH] = { 0 };

    if (settings.portable_mode) {
        snprintf((char *)path, UTOX_FILE_NAME_LENGTH, "./tox/");
    } else {
        snprintf((char *)path, UTOX_FILE_NAME_LENGTH, "%s/.config/tox/", getenv("HOME"));
    }

    if (strlen(path) + name_length >= UTOX_FILE_NAME_LENGTH) {
        LOG_TRACE("Filesys", "Load directory name too long" );
        return 0;
    } else {
        snprintf(path + strlen(path), UTOX_FILE_NAME_LENGTH - strlen(path), "%s", name);
    }

    FILE *file = fopen(path, "rb");
    if (!file) {
        // LOG_TRACE("Filesys", "Unable to open/read %s" , path);
        if (out_size) {
            *out_size = 0;
        }
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);

    uint8_t *data = calloc(size + 1, 1); // needed for the ending null byte
    if (!data) {
        fclose(file);
        if (out_size) {
            *out_size = 0;
        }
        return NULL;
    } else {
        fseek(file, 0, SEEK_SET);

        if (fread(data, size, 1, file) != 1) {
            LOG_TRACE("Filesys", "Read error on %s" , path);
            fclose(file);
            free(data);
            if (out_size) {
                *out_size = 0;
            }
            return NULL;
        }

        fclose(file);
    }

    if (out_size) {
        *out_size = size;
    }
    return data;
}

void native_export_chatlog_init(uint32_t chat_number, bool is_chat) {
    if (libgtk) {
        ugtk_save_chatlog(chat_number);
    } else {
        FRIEND *f = NULL;
        GROUPCHAT *g = NULL;

        if (is_chat) {
            g = get_group(chat_number);
            if (!g) {
                LOG_ERR("Filesys", "Could not get friend with number: %u", chat_number);
                return;
            }
        } else {
            f = get_friend(chat_number);
            if (!f) {
                LOG_ERR("Filesys", "Could not get friend with number: %u", chat_number);
                return;
            }
        }

        char name[TOX_MAX_NAME_LENGTH + sizeof(".txt")];
        snprintf((char *)name, sizeof(name), "%.*s.txt",
             (int)(is_chat ? g->name_length : f->name_length),
             is_chat ? g->name : f->name);

        FILE *file = fopen((char *)name, "wb");
        if (file) {
            utox_export_chatlog(is_chat ? g->id_str : f->id_str, file);
        }
    }
}

bool native_remove_file(const uint8_t *name, size_t length, bool portable_mode) {
    char path[UTOX_FILE_NAME_LENGTH] = { 0 };

    if (portable_mode) {
        snprintf((char *)path, UTOX_FILE_NAME_LENGTH, "./tox/");
    } else {
        snprintf((char *)path, UTOX_FILE_NAME_LENGTH, "%s/.config/tox/", getenv("HOME"));
    }

    if (strlen((const char *)path) + length >= UTOX_FILE_NAME_LENGTH) {
        LOG_DEBUG("Filesys", "File/directory name too long, unable to remove" );
        return false;
    } else {
        snprintf((char *)path + strlen((const char *)path), UTOX_FILE_NAME_LENGTH - strlen((const char *)path), "%.*s",
                 (int)length, (char *)name);
    }

    if (remove((const char *)path)) {
        LOG_ERR("NATIVE", "Unable to delete file!\n\t\t%s" , path);
        return false;
    } else {
        LOG_INFO("NATIVE", "File deleted!" );
        LOG_DEBUG("Filesys", "\t%s" , path);
    }
    return true;
}

void native_select_dir_ft(uint32_t fid, uint32_t file_number, FILE_TRANSFER *file) {
    if (libgtk) {
        ugtk_native_select_dir_ft(fid, file);
    } else {
        // fall back to working dir
        char *path = malloc(file->name_length + 1);
        memcpy(path, file->name, file->name_length);
        path[file->name_length] = 0;

        postmessage_toxcore(TOX_FILE_ACCEPT, fid, file_number, path);
    }
}

void native_autoselect_dir_ft(uint32_t fid, FILE_TRANSFER *file) {
    if (file == NULL){
        LOG_TRACE("Native", " file is null." );
        return;
    }

    uint8_t *path = malloc(file->name_length + 1);
    if (path == NULL) {
        LOG_ERR("Native", "Could not allocate memory.");
        return;
    }

    if (settings.portable_mode) {
        snprintf((char *)path, UTOX_FILE_NAME_LENGTH, "./tox/Tox_Auto_Accept/");
        native_create_dir(path);
        snprintf((char *)path, UTOX_FILE_NAME_LENGTH, "./tox/Tox_Auto_Accept/%.*s", (int)file->name_length, file->name);
    } else {
        memcpy(path, file->name, file->name_length);
        path[file->name_length] = 0;
    }

    LOG_NOTE("Native", "Auto Accept Directory: \"%s\"" , path);
    postmessage_toxcore(TOX_FILE_ACCEPT, fid, file->file_number, path);
}

// TODO: This function has the worst name.
void file_save_inline_image_png(MSG_HEADER *msg) {
    if (libgtk) {
        ugtk_file_save_inline(msg);
    } else {
        // fall back to working dir inline.png
        FILE *fp = fopen("inline.png", "wb");
        if (fp) {
            fwrite(msg->via.ft.data, 1, msg->via.ft.data_size, fp);
            fclose(fp);

            snprintf((char *)msg->via.ft.path, UTOX_FILE_NAME_LENGTH, "inline.png");
            msg->via.ft.inline_png = false;
        }
    }
}

bool native_save_image_png(char *name, uint8_t *image, int image_size) {
    if (libgtk) {
        FILE_IMAGE *file_image = calloc(1, sizeof(FILE_IMAGE));
        if (!file_image) {
            LOG_ERR("Native", "Could not allocate memory.");
            return false;
        }

        file_image->name       = name;
        file_image->data       = image;
        file_image->data_size  = image_size;

        ugtk_file_save_image_png(file_image);
        return true;
    }

    char path[TOX_MAX_NAME_LENGTH + sizeof(".png")] = { 0 };
    snprintf(path, sizeof(path), "%s.png", name);

    FILE *file = fopen(path, "wb");
    if (!file) {
        LOG_ERR("Native", "Could not open file %s for write.", path);
        return false;
    }

    fwrite(image, image_size, 1, file);
    fclose(file);
    return true;
}

int file_lock(FILE *file, uint64_t start, size_t length) {
    struct flock fl;
    fl.l_type   = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start  = start;
    fl.l_len    = length;

    int result = fcntl(fileno(file), F_SETLK, &fl);
    if (result == -1) {
        return 0;
    }

    return 1;
}

int file_unlock(FILE *file, uint64_t start, size_t length) {
    struct flock fl;
    fl.l_type   = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start  = start;
    fl.l_len    = length;

    int result = fcntl(fileno(file), F_SETLK, &fl);
    if (result == -1) {
        return 0;
    }

    return 1;
}
