#include "friend.h"

#include "avatar.h"
#include "chatlog.h"
#include "debug.h"
#include "filesys.h"
#include "flist.h"
#include "macros.h"
#include "self.h"
#include "settings.h"
#include "text.h"
#include "tox.h"
#include "utox.h"

#include "av/audio.h"

#include "layout/friend.h"  // TODO, remove this and sent the name differently

#include "native/image.h"
#include "native/notify.h"

#include "ui/edit.h"        // friend_set_name()

#include <string.h>

static FRIEND *friend = NULL;

FRIEND *get_friend(uint32_t friend_number) {
    if (friend_number >= self.friend_list_size) { //friend doesnt exist if true
        LOG_WARN("Friend", "Friend number (%u) out of bounds.", friend_number);
        return NULL;
    }

    return &friend[friend_number];
}

static FRIEND *friend_make(uint32_t friend_number) {
    if (friend_number >= self.friend_list_size) {
        LOG_INFO("Friend", "Reallocating friend array to %u. Current size: %u", (friend_number + 1), self.friend_list_size);
        FRIEND *tmp = realloc(friend, sizeof(FRIEND) * (friend_number + 1));
        if (!tmp) {
            LOG_ERR("Friend", "Could not reallocate friends array.");
            return NULL;
        }

        friend = tmp;

        self.friend_list_size = friend_number + 1;

    }

    // TODO should we memset(0); before return?
    return &friend[friend_number];
}

static FREQUEST *frequests = NULL;
static uint16_t frequest_list_size = 0;

FREQUEST *get_frequest(uint16_t frequest_number) {
    if (frequest_number >= frequest_list_size) { //frequest doesnt exist if true
        LOG_ERR("Friend", "Request number out of bounds.");
        return NULL;
    }

    return &frequests[frequest_number];
}

static FREQUEST *frequest_make(uint16_t frequest_number) {
    if (frequest_number >= frequest_list_size) {
        LOG_INFO("Friend", "Reallocating frequest array to %u. Current size: %u", (frequest_number + 1), frequest_list_size);
        FREQUEST *tmp = realloc(frequests, sizeof(FREQUEST) * (frequest_number + 1));
        if (!tmp) {
            LOG_ERR("Friend", "Could not reallocate frequests array.");
            return NULL;
        }

        frequests = tmp;
        frequest_list_size = frequest_number + 1;
    }

    // TODO should we memset(0); before return?
    return &frequests[frequest_number];
}

uint16_t friend_request_new(const uint8_t *id, const uint8_t *msg, size_t length) {
    uint16_t curr_num = frequest_list_size;
    FREQUEST *r = frequest_make(frequest_list_size); // TODO search for empty request slots
    if (!r) {
        LOG_ERR("Friend", "Unable to get space for Friend Request.");
        return UINT16_MAX;
    }

    r->number = curr_num;
    memcpy(r->bin_id, id, TOX_ADDRESS_SIZE);
    r->msg = malloc(length + 1);
    if (!r->msg) {
        LOG_ERR("Friend", "Unable to get space for friend request message.");
        return UINT16_MAX;
    }
    memcpy(r->msg, msg, length);
    r->msg[length] = 0; // Toxcore doesn't promise null term on strings
    r->length = length;

    return curr_num;
}

void friend_request_free(uint16_t number) {
    FREQUEST *r = get_frequest(number);
    if (!r) {
        LOG_ERR("Friend", "Unable to free a missing request.");
        return;
    }

    free(r->msg);

    // TODO this needs a test
    if (r->number >= frequest_list_size -1) {
        FREQUEST *tmp = realloc(frequests, sizeof(FREQUEST) * (frequest_list_size - 1));
        if (tmp) {
            frequests = tmp;
            --frequest_list_size;
        }
    }
}

/* TODO incoming friends "leaks" */

void free_friends(void) {
    for (uint32_t i = 0; i < self.friend_list_count; i++){
        FRIEND *f = get_friend(i);
        if (!f) {
            LOG_WARN("Friend", "Could not get friend %u. Skipping", i);
            continue;
        }
        friend_free(f);
    }

    if (friend) {
        free(friend);
    }
}

void utox_write_metadata(FRIEND *f) {
    /* Create path */
    char dest[UTOX_FILE_NAME_LENGTH];
    snprintf(dest, UTOX_FILE_NAME_LENGTH, "%.*s.fmetadata", TOX_PUBLIC_KEY_SIZE * 2, f->id_str);

    FILE *file = utox_get_file(dest, NULL, UTOX_FILE_OPTS_WRITE);
    if (!file) {
        LOG_ERR("Friend", "Unable to get file to write metadata for friend %u", f->number);
        return;
    }

    FRIEND_META_DATA metadata = { 0 };
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
        if (f->alias && f->alias_length) {
            memcpy(data + sizeof(metadata), f->alias, metadata.alias_length);
        }

        fwrite(data, total_size, 1, file);
        free(data);
    }

    fclose(file);
}

static void friend_meta_data_read(FRIEND *f) {
    /* Will need to be rewritten if anything is added to friend's meta data */
    char path[UTOX_FILE_NAME_LENGTH];

    snprintf(path, UTOX_FILE_NAME_LENGTH, "%.*s.fmetadata", TOX_PUBLIC_KEY_SIZE * 2,  f->id_str);

    size_t size = 0;
    FILE *file = utox_get_file(path, &size, UTOX_FILE_OPTS_READ);

    if (!file) {
        LOG_TRACE("Friend", "Meta Data not found %s", path);
        return;
    }

    if (size < sizeof(FRIEND_META_DATA)) {
        LOG_ERR("Metadata", "Stored metadata is incomplete.");
        fclose(file);
        return;
    }

    FRIEND_META_DATA *metadata = calloc(1, size);
    if (!metadata) {
        LOG_ERR("Metadata", "Could not allocate memory for metadata." );
        fclose(file);
        return;
    }

    bool read_meta = fread(metadata, size, 1, file);
    fclose(file);

    if (!read_meta) {
        LOG_ERR("Metadata", "Failed to read metadata from disk.");
        free(metadata);
        return;
    }

    if (metadata->version != 0) {
        LOG_ERR("Metadata", "WARNING! This version of utox does not support this metadata file version." );
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
    LOG_INFO("Friend", "Initializing friend: %u", friend_number);
    FRIEND *f = friend_make(friend_number); // get friend pointer
    if (!f) {
        LOG_ERR("Friend", "Could not create init friend %u", friend_number);
        return;
    }
    self.friend_list_count++;

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
    int size = tox_friend_get_name_size(tox, friend_number, 0);
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

    f->avatar = calloc(1, sizeof(AVATAR));
    avatar_init(f->id_str, f->avatar);

    MESSAGES *m = &f->msg;
    messages_init(m, friend_number);

    // Get the chat backlog
    messages_read_from_log(friend_number);

    // Load the meta data, if it exists.
    friend_meta_data_read(f);
}

void utox_friend_list_init(Tox *tox) {
    LOG_INFO("Friend", "Initializing friend list.");

    self.friend_list_size = tox_self_get_friend_list_size(tox);

    friend = calloc(self.friend_list_size, sizeof(FRIEND));
    if (!friend) {
        LOG_FATAL_ERR(EXIT_MALLOC, "Friend", "Could not allocate friend list with size: %u", self.friend_list_size);
    }

    for (uint32_t i = 0; i < self.friend_list_size; ++i) {
        utox_friend_init(tox, i);
    }
    LOG_INFO("Friend", "Friendlist sucessfully initialized with %u friends.", self.friend_list_size);
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
        if (flist_get_type()== ITEM_FRIEND) {
            FRIEND *selected = flist_get_friend();
            if (!selected) {
                LOG_ERR("Friend", "Unable to get selected friend.");
                return;
            }
            if (selected && f->number == selected->number) {
                maybe_i18nal_string_set_plain(&edit_friend_alias.empty_str, f->name, f->name_length);
            }
        }
    }

    flist_update_shown_list();
}

void friend_set_alias(FRIEND *f, uint8_t *alias, uint16_t length) {
    if (length > 0) {
        if (!alias) {
            LOG_ERR("Friend Alias", "Got alias length, but no alias.");
            return;
        }

        LOG_TRACE("Friend", "New Alias set for friend %s." , f->name);
    } else {
        LOG_TRACE("Friend", "Alias for friend %s unset." , f->name);
    }

    free(f->alias);
    if (length == 0) {
        f->alias        = NULL;
        f->alias_length = 0;
    } else {
        f->alias = calloc(1, length + 1);
        if (!f->alias) {
            LOG_ERR("Friend", "Unable to malloc for alias set for friend %s.");
            return;
        }

        memcpy(f->alias, alias, length);
        f->alias_length = length;
    }
}

void friend_sendimage(FRIEND *f, NATIVE_IMAGE *native_image, uint16_t width, uint16_t height, UTOX_IMAGE png_image,
                      size_t png_size) {
    struct TOX_SEND_INLINE_MSG *tsim = malloc(sizeof(struct TOX_SEND_INLINE_MSG));
    if (!tsim) {
        LOG_ERR("Friend", "Unable to malloc for inline image.");
        return;
    }

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

    postmessage_utox(FRIEND_MESSAGE, f->number, 0, NULL);
    notify(title, title_length, msg, msg_length, f, 0);

    if (flist_get_friend() != f) {
        f->unread_msg = true;
    }

    if (flist_get_friend() != f || !have_focus) {
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
    char *data = malloc(TOX_ADDRESS_SIZE + msg_length);
    if (!data) {
        LOG_ERR("Friend", "Unable to malloc for friend request.");
        return;
    }

    memcpy(data, id, TOX_ADDRESS_SIZE);
    memcpy(data + TOX_ADDRESS_SIZE, msg, msg_length);

    postmessage_toxcore(TOX_FRIEND_NEW, msg_length, 0, data);
}

void friend_add(char *name, uint16_t length, char *msg, uint16_t msg_length) {
    if (!length) {
        addfriend_status = ADDF_NONAME;
        return;
    }

    uint8_t  name_cleaned[length];
    uint16_t length_cleaned = 0;

    for (unsigned int i = 0; i < length; ++i) {
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
        addfriend_status = ADDF_BADNAME;
    }
}

void friend_history_clear(FRIEND *f) {
    if (!f) {
        LOG_ERR("FList", "Unable to clear history for missing friend.");
        return;
    }
    messages_clear_all(&f->msg);
    utox_remove_friend_chatlog(f->id_str);
}

void friend_free(FRIEND *f) {
    LOG_INFO("Friend", "Freeing friend: %u", f->number);
    for (uint16_t i = 0; i < f->edit_history_length; ++i) {
        free(f->edit_history[i]);
        f->edit_history[i] = NULL;
    }
    free(f->edit_history);

    free(f->name);
    free(f->status_message);
    free(f->typed);
    free(f->avatar);

    for (uint32_t i = 0; i < f->msg.number; ++i) {
        MSG_HEADER *msg = f->msg.data[i];
        message_free(msg);
    }
    free(f->msg.data);

    if (f->call_state_self) {
        // postmessage_audio(AUDIO_END, f->number, 0, NULL);
        /* TODO end a video call too!
        if(f->calling == CALL_OK_VIDEO) {
            postmessage_video(VIDEO_CALL_END, f->number, 0, NULL);
        }*/
    }

    memset(f, 0, sizeof(FRIEND));
    self.friend_list_count--;
}

FRIEND *find_friend_by_name(uint8_t *name) {
    for (size_t i = 0; i < self.friend_list_count; i++) {
        FRIEND *f = get_friend(i);
        if (!f) {
            LOG_ERR("Friend", "Could not get friend %u", i);
            continue;
        }

        if ((f->alias && memcmp(f->alias, name, MIN(f->alias_length, strlen((char *)name))) == 0)
            || memcmp(f->name, name, MIN(f->name_length, strlen((char *)name))) == 0) {
            return f;
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
