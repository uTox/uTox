#include "chatlog.h"

#include "filesys.h"
#include "debug.h"
#include "main_native.h"
#include "messages.h"
#include "text.h"

#include <stdint.h>

static FILE* chatlog_get_file(char hex[TOX_PUBLIC_KEY_SIZE * 2], bool append) {
    uint8_t name[TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".new.txt")];
    snprintf((char *)name, sizeof(name), "%.*s.new.txt", TOX_PUBLIC_KEY_SIZE * 2, hex);

    FILE *file;
    if (append) {
        file = native_get_file(name, NULL, UTOX_FILE_OPTS_READ | UTOX_FILE_OPTS_WRITE | UTOX_FILE_OPTS_MKDIR);
        if (!file) {
            return NULL;
        }

        fseek(file, 0, SEEK_END);
    } else {
        file = native_get_file((uint8_t *)name, NULL, UTOX_FILE_OPTS_READ);
    }

    return file;
}

size_t utox_save_chatlog(char hex[TOX_PUBLIC_KEY_SIZE * 2], uint8_t *data, size_t length) {
    FILE *fp = chatlog_get_file(hex, true);
    if (fp == NULL) {
        debug_error("uTox:\tError getting a file handle for this chatlog!\n");
        return 0;
    }
    // Seek to the beginning of the file first because grayhatter has had issues with this on Windows.
    // (and he really doesn't want uTox eating people's chat logs)
    fseeko(fp, 0, SEEK_SET);
    fseeko(fp, 0, SEEK_END);
    off_t offset = ftello(fp);
    fwrite(data, length, 1, fp);
    fclose(fp);

    return offset;
}

static size_t utox_count_chatlog(char hex[TOX_PUBLIC_KEY_SIZE * 2]) {
    FILE *file = chatlog_get_file(hex, false);

    if (!file) {
        return 0;
    }

    LOG_FILE_MSG_HEADER header;
    size_t records_count = 0;

    while (fread(&header, sizeof(header), 1, file) == 1) {
        fseeko(file, header.author_length + header.msg_length + 1, SEEK_CUR);
        records_count++;
    }


    if (ferror(file) || !feof(file)) {
        /* TODO: consider removing or truncating the log file.
         * If !feof() this means that the file has an incomplete record,
         * which would prevent it from loading forever, even though
         * new records will keep being appended as usual. */
        debug_error("Log read trying to count history for friend %.*s\n", TOX_PUBLIC_KEY_SIZE * 2, hex);
        fclose(file);
        return 0;
    }

    fclose(file);
    return records_count;
}

/* TODO create fxn that will try to recover a corrupt chat history.
 *
 * In the majority of bug reports the corrupt message is often the first, so in
 * theory we should be able to trim the start of the chatlog up to and including
 * the first \n char. We may have to do so multiple times, but once we find the
 * first valid message everything else should "work" */
MSG_HEADER **utox_load_chatlog(char hex[TOX_PUBLIC_KEY_SIZE * 2], size_t *size, uint32_t count, uint32_t skip) {
    /* Becasue every platform is different, we have to ask them to open the file for us.
     * However once we have it, every platform does the same thing, this should prevent issues
     * from occuring on a single platform. */

    size_t records_count = utox_count_chatlog(hex);
    if (skip >= records_count) {
        if (skip > 0) {
            debug_error("Log read:\tError, skipped all records\n");
        } else {
            debug("Log read:\tNo log exists.");
        }
        return NULL;
    }

    FILE *file = chatlog_get_file(hex, false);
    if (!file) {
        debug("Log read:\tUnable to access file provided.\n");
        return NULL;
    }

    if (count > (records_count - skip)) {
        count = records_count - skip;
    }

    MSG_HEADER **data = calloc(count + 1, sizeof(MSG_HEADER));
    MSG_HEADER **start = data;

    if (!data) {
        debug_error("Log read:\tCouldn't allocate memory for log entries.");
        fclose(file);
        return NULL;
    }

    size_t start_at = records_count - count - skip;
    size_t actual_count = 0;

    size_t file_offset = 0;

    LOG_FILE_MSG_HEADER header;
    while (fread(&header, sizeof(header), 1, file) == 1) {
        if (start_at) {
            fseeko(file, header.author_length, SEEK_CUR); /* Skip the recorded author */
            fseeko(file, header.msg_length, SEEK_CUR);    /* Skip the message */
            fseeko(file, 1, SEEK_CUR);                    /* Skip the newline char */
            start_at--;
            file_offset = ftello(file);
            continue;
        }

        if (count) {
            /* we have to skip the author name for now, it's left here for group chats support in the future */
            fseeko(file, header.author_length, SEEK_CUR);
            if (header.msg_length > 1 << 16) {
                debug_error("Can't malloc that much, you'll probably have to move or delete your"
                            " history for this peer.\n\t\tFriend number %.*s, count %u,"
                            " actual_count %lu, start at %lu, error size %lu.\n",
                            TOX_PUBLIC_KEY_SIZE * 2, hex, count, actual_count, start_at, header.msg_length);
                if (size) {
                    *size = 0;
                }

                fclose(file);
                return start;
            }
            MSG_HEADER *msg = calloc(1, sizeof(MSG_HEADER));
            if (!msg) {
                debug_error("Chatlog:\tUnable to malloc... sorry!\n");
                fclose(file);
                return NULL;
            }
            msg->our_msg    = header.author;

            /* TEMP Fix to recover logs from v0.8.* */
            if (header.log_version == 0) {
                msg->receipt_time = 1;
            } else {
                msg->receipt_time = header.receipt;
            }

            msg->time          = header.time;
            msg->msg_type      = header.msg_type;
            msg->disk_offset   = file_offset;

            msg->via.txt.length        = header.msg_length;
            msg->via.txt.msg = calloc(1, msg->via.txt.length);
            if (!msg->via.txt.msg) {
                debug_error("Chatlog:\tUnable to malloc for via.txt.msg... sorry!\n");
                free(msg);
                fclose(file);
                return NULL;
            }

            msg->via.txt.author_length = header.author_length;
            // TODO: msg->via.txt.author used to be allocated but left empty. Commented out for now.
            // msg->via.txt.author = calloc(1, msg->via.txt.author_length);
            // if (!msg->via.txt.author) {
            //     debug_error("Chatlog:\tUnable to malloc for via.txt.author... sorry!\n");
            //     free(msg->via.txt.msg);
            //     free(msg);
            //     fclose(file);
            //     return NULL;
            // }

            if (fread(msg->via.txt.msg, msg->via.txt.length, 1, file) != 1) {
                debug_error("Log read:\tError reading record %u of length %u at offset %lu: stopping.\n",
                            count, msg->via.txt.length, msg->disk_offset);
                // free(msg->via.txt.author);
                free(msg->via.txt.msg);
                free(msg);
                break;
            }
            msg->via.txt.length = utf8_validate((uint8_t *)msg->via.txt.msg, msg->via.txt.length);
            *data++ = msg;
            --count;
            ++actual_count;
            fseeko(file, 1, SEEK_CUR); /* seek an extra \n char */
            file_offset = ftello(file);
        }
    }

    fclose(file);

    if (size) {
        *size = actual_count;
    }
    return start;
}

bool utox_update_chatlog(char hex[TOX_PUBLIC_KEY_SIZE * 2], size_t offset, uint8_t *data, size_t length) {
    FILE *file = chatlog_get_file(hex, true);

    if (!file) {
        debug_error("History:\tUnable to access file provided.\n");
        return false;
    }

    if (fseeko(file, offset, SEEK_SET)) {
        debug_error("History:\tUnable to seek to position %lu in file provided.", offset);
        return false;
    }

    fwrite(data, length, 1, file);
    fclose(file);

    return true;
}

bool utox_remove_friend_chatlog(char hex[TOX_PUBLIC_KEY_SIZE * 2]) {
    char name[TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".new.txt")];

    snprintf(name, sizeof(name), "%.*s.new.txt", TOX_PUBLIC_KEY_SIZE * 2, hex);

    return utox_remove_file((uint8_t*)name, sizeof(name));
}

void utox_export_chatlog_init(uint32_t friend_number) {
    native_export_chatlog_init(friend_number);
}

void utox_export_chatlog(char hex[TOX_PUBLIC_KEY_SIZE * 2], FILE *dest_file) {
    if (!dest_file) {
        return;
    }

    FILE *file = chatlog_get_file(hex, false);

    LOG_FILE_MSG_HEADER header;
    while (1 == fread(&header, sizeof(header), 1, file)) {
        char c;
        /* Write Author */
        fwrite("<", 1, 1, dest_file);
        for (size_t i = 0; i < header.author_length; ++i) {
            c = fgetc(file);
            if (c != EOF) {
                fputc(c, dest_file);
            } else {
                continue;
            }
        }
        fwrite(">", 1, 1, dest_file);

        /* Write text */
        fwrite(" ", 1, 1, dest_file);
        for (size_t i = 0; i < header.msg_length; ++i) {
            c = fgetc(file);
            if (c != EOF) {
                fputc(c, dest_file);
            } else {
                continue;
            }
        }
        c = fgetc(file); /* the newline char */
        fputc(c, dest_file);
    }

    fclose(file);
    fclose(dest_file);
}
