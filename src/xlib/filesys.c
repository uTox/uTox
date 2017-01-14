#include "main.h"

#include "gtk.h"

#include "../chatlog.h"
#include "../logging_native.h"

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
            debug("NATIVE:\tSave directory name too long\n");
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
            debug("NATIVE:\t%sUnable to move file!\n", atomic_path);
            return 0;
        }
        return 1;
    } else {
        debug("NATIVE:\tUnable to open %s to write save\n", path);
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
        debug("NATIVE:\tLoad directory name too long\n");
        return 0;
    } else {
        snprintf(path + strlen(path), UTOX_FILE_NAME_LENGTH - strlen(path), "%s", name);
    }

    FILE *file = fopen(path, "rb");
    if (!file) {
        // debug("NATIVE:\tUnable to open/read %s\n", path);
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
            debug("NATIVE:\tRead error on %s\n", path);
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

void native_export_chatlog_init(uint32_t friend_number) {
    if (libgtk) {
        ugtk_save_chatlog(friend_number);
    } else {
        char name[UTOX_MAX_NAME_LENGTH + sizeof(".txt")];
        snprintf((char *)name, sizeof(name), "%.*s.txt", (int)friend[friend_number].name_length,
                 friend[friend_number].name);

        FILE *file = fopen((char *)name, "wb");
        if (file) {
            utox_export_chatlog(get_friend(friend_number)->id_str, file);
        }
    }
}

bool native_remove_file(const uint8_t *name, size_t length) {
    char path[UTOX_FILE_NAME_LENGTH] = { 0 };

    if (settings.portable_mode) {
        snprintf((char *)path, UTOX_FILE_NAME_LENGTH, "./tox/");
    } else {
        snprintf((char *)path, UTOX_FILE_NAME_LENGTH, "%s/.config/tox/", getenv("HOME"));
    }

    if (strlen((const char *)path) + length >= UTOX_FILE_NAME_LENGTH) {
        debug("NATIVE:\tFile/directory name too long, unable to remove\n");
        return false;
    } else {
        snprintf((char *)path + strlen((const char *)path), UTOX_FILE_NAME_LENGTH - strlen((const char *)path), "%.*s",
                 (int)length, (char *)name);
    }

    if (remove((const char *)path)) {
        debug_error("NATIVE:\tUnable to delete file!\n\t\t%s\n", path);
        return false;
    } else {
        debug_info("NATIVE:\tFile deleted!\n");
        debug("NATIVE:\t\t%s\n", path);
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
        debug("Native:\t file is null.\n");
        return;
    }

    uint8_t *path = malloc(file->name_length + 1);
    if (path == NULL) {
        debug_error("Native:\tCould not allocate memory.\n");
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

    debug_notice("Native:\tAuto Accept Directory: \"%s\"\n", path);
    postmessage_toxcore(TOX_FILE_ACCEPT, fid, file->file_number, path);
}

// TODO: This function has the worst name.
void file_save_inline(FILE_TRANSFER *file) {
    if (libgtk) {
        ugtk_file_save_inline(file);
    } else {
        // fall back to working dir inline.png
        FILE *fp = fopen("inline.png", "wb");
        if (fp) {
            fwrite(file->path, 1, file->target_size, fp);
            fclose(fp);

            snprintf((char *)file->path, UTOX_FILE_NAME_LENGTH, "inline.png");
        }
    }
}

int file_lock(FILE *file, uint64_t start, size_t length) {
    int          result = -1;
    struct flock fl;
    fl.l_type   = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start  = start;
    fl.l_len    = length;

    result = fcntl(fileno(file), F_SETLK, &fl);
    if (result == -1) {
        return 0;
    }
    return 1;
}

int file_unlock(FILE *file, uint64_t start, size_t length) {
    int          result = -1;
    struct flock fl;
    fl.l_type   = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start  = start;
    fl.l_len    = length;

    result = fcntl(fileno(file), F_SETLK, &fl);
    if (result == -1) {
        return 0;
    }
    return 1;
}
