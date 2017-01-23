#include "friend.h"

#include "chatlog.h"
#include "dns.h"
#include "filesys.h"
#include "flist.h"
#include "logging_native.h"
#include "main_native.h"
#include "settings.h"
#include "text.h"
#include "tox.h"
#include "utox.h"

#include "av/utox_av.h"
#include "ui/edits.h"
#include "ui/scrollable.h"

#include "main.h" // self, addfriend_status

FRIEND* get_friend(uint32_t friend_number){
    if (friend_number >= 128) {
        return NULL; // artifical limit while we discuss the limmit of friends we want to support
    }

    return &friend[friend_number];
}

void utox_write_metadata(FRIEND *f) {
    /* Create path */
    uint8_t dest[UTOX_FILE_NAME_LENGTH];
    snprintf((char *)dest, UTOX_FILE_NAME_LENGTH, "%.*s.fmetadata", TOX_PUBLIC_KEY_SIZE * 2, f->id_str);

    FILE *file = native_get_file((uint8_t *)dest, NULL, UTOX_FILE_OPTS_WRITE);
    if (file) {

        FRIEND_META_DATA metadata;
        memset(&metadata, 0, sizeof(metadata));
        size_t total_size = sizeof(metadata);

        metadata.version          = METADATA_VERSION;
        metadata.ft_autoaccept    = f->ft_autoaccept;
        metadata.skip_msg_logging = f->skip_msg_logging;

        if (f->alias && f->alias_length) {
            metadata.alias_length = f->alias_length;
            total_size += metadata.alias_length;
        }

        uint8_t *data = calloc(1, total_size);
        if (data) {
            memcpy(data, &metadata, sizeof(metadata));
            memcpy(data + sizeof(metadata), f->alias, metadata.alias_length);

            fwrite(data, total_size, 1, file);
            free(data);
        }
        fclose(file);
    }
}

static void friend_meta_data_read(FRIEND *f) {
    /* Will need to be rewritten if anything is added to friend's meta data */
    uint8_t path[UTOX_FILE_NAME_LENGTH];

    snprintf((char *)path, UTOX_FILE_NAME_LENGTH, "%.*s.fmetadata", TOX_PUBLIC_KEY_SIZE * 2,  f->id_str);

    size_t size = 0;
    FILE *file = native_get_file(path, &size, UTOX_FILE_OPTS_READ);

    if (!file) {
        debug_notice("Meta Data not found (%s)\n", path);
        return;
    }

    FRIEND_META_DATA *metadata = calloc(1, sizeof(*metadata) + size);
    if (metadata == NULL) {
        debug("Metadata:\tCould not allocate memory for metadata.\n");
        fclose(file);
        return;
    }

    fread(metadata, size, 1, file);
    fclose(file);

    /* Compatibility code for original version of meta_data... TODO: Remove in version >= 0.10 */
    if (metadata->version >= 2) { /* Version 2 chosen because alias length (the original value at *
                                   * metadata[0] should be > 2 (hopefully)                        */
        if (size < sizeof(FRIEND_META_DATA_OLD)) {
            debug("Metadata:\tMeta Data was incomplete\n");
            free(metadata);
            return;
        }

        if (((FRIEND_META_DATA_OLD *)metadata)->alias_length) {
            friend_set_alias(f, metadata + sizeof(size_t),
                             ((FRIEND_META_DATA_OLD *)metadata)->alias_length);
        } else {
            friend_set_alias(f, NULL, 0);
        }

        debug("Metadata:\tConverting old metadata file to new!\n");
        utox_write_metadata(f);

        free(metadata);
        return;
    } else if (metadata->version != 0) {
        debug_notice("Metadata:\tWARNING! This version of utox does not support this metadata file version.\n");
        free(metadata);
        return;
    }

    if (size < sizeof(*metadata)) {
        debug_error("Metadata:\tMeta Data was incomplete\n");
        free(metadata);
        return;
    }

    if (metadata->alias_length) {
        friend_set_alias(f, &metadata->data[0], metadata->alias_length);
    } else {
        friend_set_alias(f, NULL, 0); /* uTox expects this to be 0/NULL if there's no alias. */
    }

    f->ft_autoaccept = metadata->ft_autoaccept;

    free(metadata);
    return;
}

void utox_friend_init(Tox *tox, uint32_t friend_number) {
    int size;
    // get friend pointer
    FRIEND *f = &friend[friend_number];
    uint8_t name[TOX_MAX_NAME_LENGTH];

    memset(f, 0, sizeof(FRIEND));

    // Set scroll position to bottom of window.
    f->msg.scroll               = 1.0;
    f->msg.panel.type           = PANEL_MESSAGES;
    f->msg.panel.content_scroll = &scrollbar_friend;
    f->msg.panel.y              = MAIN_TOP;
    f->msg.panel.height         = CHAT_BOX_TOP;
    f->msg.panel.width          = -SCROLL_WIDTH;

    // Get and set the public key for this friend number and set it.
    tox_friend_get_public_key(tox, friend_number, f->cid, 0);
    tox_friend_get_public_key(tox, friend_number, f->id_bin, 0);
    cid_to_string(f->id_str, f->id_bin);

    // Set the friend number we got from toxcore
    f->number = friend_number;

    // Get and set friend name and length
    size = tox_friend_get_name_size(tox, friend_number, 0);
    tox_friend_get_name(tox, friend_number, name, 0);
    // Set the name for utox as well
    friend_setname(f, name, size);

    // Get and set the status message
    size = tox_friend_get_status_message_size(tox, friend_number, 0);
    f->status_message = calloc(1, size);

    tox_friend_get_status_message(tox, friend_number, (uint8_t *)f->status_message, 0);
    f->status_length = size;

    /* TODO; consider error handling these two */
    f->online = tox_friend_get_connection_status(tox, friend_number, NULL);
    f->status = tox_friend_get_status(tox, friend_number, NULL);

    avatar_init(f->id_str, &f->avatar);

    MESSAGES *m = &f->msg;
    messages_init(m, friend_number);

    // Get the chat backlog
    messages_read_from_log(friend_number);

    // Load the meta data, if it exists.
    friend_meta_data_read(f);
}

void utox_friend_list_init(Tox *tox) {
    /* Eventually count should be the literal number of current friends
     * and size will be the capacity. Without dynamic sized friend array
     * we just set both to the number when we init, and hope for the best! */
    self.friend_list_count = self.friend_list_size = tox_self_get_friend_list_size(tox);

    uint32_t i;
    for (i = 0; i < self.friend_list_count; ++i) {
        utox_friend_init(tox, i);
    }
}

void friend_setname(FRIEND *f, uint8_t *name, size_t length) {
    if (f->name && f->name_length) {
        size_t size = sizeof(" is now known as ") + f->name_length + length;

        char *p = calloc(1, size);
        size = snprintf(p, size, "%.*s is now known as %.*s", (int)f->name_length, f->name, (int)length, name);

        if (length != f->name_length || memcmp(f->name, name, (length < f->name_length ? length : f->name_length))) {
            message_add_type_notice(&f->msg, p, size, 1);
        }

        free(f->name);
        free(p);
    }

    if (length == 0) {
        f->name = calloc(1, sizeof(f->cid) * 2 + 1);
        cid_to_string(f->name, f->cid);
        f->name_length = sizeof(f->cid) * 2;
    } else {
        f->name = calloc(1, length + 1);
        memcpy(f->name, name, length);
        f->name_length = length;
    }

    f->name[f->name_length] = 0;

    if (!f->alias_length) {
        if (flist_get_selected()->item == ITEM_FRIEND) {
            FRIEND *selected = flist_get_selected()->data;
            if (selected && f->number == selected->number) {
                maybe_i18nal_string_set_plain(&edit_friend_alias.empty_str, f->name, f->name_length);
            }
        }
    }

    flist_update_shown_list();
}

void friend_set_alias(FRIEND *f, uint8_t *alias, uint16_t length) {
    if (alias && length > 0) {
        debug("New Alias set for friend %s\n", f->name);
    } else {
        debug("Alias for friend %s unset\n", f->name);
    }

    free(f->alias);
    if (length == 0) {
        f->alias        = NULL;
        f->alias_length = 0;
    } else {
        f->alias = malloc(length + 1);
        memcpy(f->alias, alias, length);
        f->alias_length           = length;
        f->alias[f->alias_length] = 0;
    }
}

void friend_sendimage(FRIEND *f, NATIVE_IMAGE *native_image, uint16_t width, uint16_t height, UTOX_IMAGE png_image,
                      size_t png_size) {
    struct TOX_SEND_INLINE_MSG *tsim = malloc(sizeof(struct TOX_SEND_INLINE_MSG));
    tsim->image      = png_image;
    tsim->image_size = png_size;
    postmessage_toxcore(TOX_FILE_SEND_NEW_INLINE, f - friend, 0, tsim);

    message_add_type_image(&f->msg, 1, native_image, width, height, 0);
}

void friend_recvimage(FRIEND *f, NATIVE_IMAGE *native_image, uint16_t width, uint16_t height) {
    if (!NATIVE_IMAGE_IS_VALID(native_image)) {
        return;
    }

    message_add_type_image(&f->msg, 0, native_image, width, height, 0);
}

void friend_notify_msg(FRIEND *f, const char *msg, size_t msg_length) {
    char title[UTOX_FRIEND_NAME_LENGTH(f) + 25];

    size_t title_length = snprintf((char *)title, UTOX_FRIEND_NAME_LENGTH(f) + 25, "uTox new message from %.*s",
                                   (int)UTOX_FRIEND_NAME_LENGTH(f), UTOX_FRIEND_NAME(f));

    postmessage_utox(FRIEND_MESSAGE, 0, 0, NULL);
    notify(title, title_length, msg, msg_length, f, 0);

    if (flist_get_selected()->data != f) {
        f->unread_msg = 1;
        postmessage_audio(UTOXAUDIO_PLAY_NOTIFICATION, NOTIFY_TONE_FRIEND_NEW_MSG, 0, NULL);
    }
}

bool friend_set_online(FRIEND *f, bool online) {
    if (f->online == online) {
        return false;
    }

    f->online = online;
    if (!f->online) {
        friend_set_typing(f, 0);
    }

    flist_update_shown_list();

    return true;
}


void friend_set_typing(FRIEND *f, int typing) {
    f->typing = typing;
}

void friend_addid(uint8_t *id, char *msg, uint16_t msg_length) {
    void *data = malloc(TOX_ADDRESS_SIZE + msg_length * sizeof(char));
    memcpy(data, id, TOX_ADDRESS_SIZE);
    memcpy(data + TOX_ADDRESS_SIZE, msg, msg_length * sizeof(char));

    postmessage_toxcore(TOX_FRIEND_NEW, msg_length, 0, data);
}

void friend_add(char *name, uint16_t length, char *msg, uint16_t msg_length) {
    if (!length) {
        addfriend_status = ADDF_NONAME;
        return;
    }

    uint8_t  name_cleaned[length];
    uint16_t length_cleaned = 0;

    unsigned int i;
    for (i = 0; i < length; ++i) {
        if (name[i] != ' ') {
            name_cleaned[length_cleaned] = name[i];
            ++length_cleaned;
        }
    }

    if (!length_cleaned) {
        addfriend_status = ADDF_NONAME;
        return;
    }

    uint8_t id[TOX_ADDRESS_SIZE];
    if (length_cleaned == TOX_ADDRESS_SIZE * 2 && string_to_id(id, (char *)name_cleaned)) {
        friend_addid(id, msg, msg_length);
    } else {
        /* not a regular id, try DNS discovery */
        addfriend_status = ADDF_DISCOVER;
        dns_request((char *)name_cleaned, length_cleaned);
    }
}

void friend_history_clear(FRIEND *f) {
    messages_clear_all(&f->msg);

    utox_remove_friend_chatlog(f->id_str);
}

void friend_free(FRIEND *f) {
    uint16_t j = 0;
    while (j != f->edit_history_length) {
        free(f->edit_history[j]);
        j++;
    }
    free(f->edit_history);

    free(f->name);
    free(f->status_message);
    free(f->typed);

    uint32_t i = 0;
    while (i < f->msg.number) {
        MSG_HEADER *msg = f->msg.data[i];
        message_free(msg);
        i++;
    }

    free(f->msg.data);

    if (f->call_state_self) {
        // postmessage_audio(AUDIO_END, f->number, 0, NULL);
        /* TODO end a video call too!
        if(f->calling == CALL_OK_VIDEO) {
            postmessage_video(VIDEO_CALL_END, f->number, 0, NULL);
        }*/
    }

    memset(f, 0, sizeof(FRIEND)); //
}

FRIEND *find_friend_by_name(uint8_t *name) {
    for (size_t i = 0; i < self.friend_list_count; i++) {
        if ((friend[i].alias && memcmp(friend[i].alias, name, friend[i].alias_length) == 0)
            || memcmp(friend[i].name, name, friend[i].name_length) == 0) {
            return &friend[i];
        }
    }
    return NULL;
}

void friend_notify_status(FRIEND *f, const uint8_t *msg, size_t msg_length, char *state) {
    if (!settings.status_notifications) {
        return;
    }

    char title[UTOX_FRIEND_NAME_LENGTH(f) + 20];
    size_t  title_length = snprintf((char *)title, UTOX_FRIEND_NAME_LENGTH(f) + 20, "uTox %.*s is now %s.",
                                   (int)UTOX_FRIEND_NAME_LENGTH(f), UTOX_FRIEND_NAME(f), state);

    notify(title, title_length, (char *)msg, msg_length, f, 0);

    /* This function is called before the status is changed. so we have to go by the inverse
     * obviously not ideal, TODO fix later with the friends struct refactor. */
    if (f->online) {
        postmessage_audio(UTOXAUDIO_PLAY_NOTIFICATION, NOTIFY_TONE_FRIEND_OFFLINE, 0, NULL);
    } else {
        postmessage_audio(UTOXAUDIO_PLAY_NOTIFICATION, NOTIFY_TONE_FRIEND_ONLINE, 0, NULL);
    }
}

bool string_to_id(uint8_t *w, char *a) {
    uint8_t *end = w + TOX_ADDRESS_SIZE;
    while (w != end) {
        char c, v;

        c = *a++;
        if (c >= '0' && c <= '9') {
            v = (c - '0') << 4;
        } else if (c >= 'A' && c <= 'F') {
            v = (c - 'A' + 10) << 4;
        } else if (c >= 'a' && c <= 'f') {
            v = (c - 'a' + 10) << 4;
        } else {
            return false;
        }

        c = *a++;
        if (c >= '0' && c <= '9') {
            v |= (c - '0');
        } else if (c >= 'A' && c <= 'F') {
            v |= (c - 'A' + 10);
        } else if (c >= 'a' && c <= 'f') {
            v |= (c - 'a' + 10);
        } else {
            return false;
        }

        *w++ = v;
    }

    return true;
}

void cid_to_string(char *dest, uint8_t *src) {
    to_hex(dest, src, TOX_PUBLIC_KEY_SIZE);
}
