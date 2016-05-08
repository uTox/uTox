#include "main.h"

/** Writes friend meta data filename for fid to dest. returns length written */
static int friend_meta_data_path(uint8_t *dest, size_t size_dest, uint8_t *friend_key, uint32_t friend_num) {
    if (size_dest < TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".fmetadata")){
        return -1;
    }

    cid_to_string(dest, friend_key); dest += TOX_PUBLIC_KEY_SIZE * 2;
    memcpy((char*)dest, ".fmetadata", sizeof(".fmetadata"));

    return TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".fmetadata");
}

static void friend_meta_data_read(Tox *tox, int friend_id) {
    /* Will need to be rewritten if anything is added to friend's meta data */
    uint8_t path[UTOX_FILE_NAME_LENGTH], *p;
    p = path + datapath(path);

    uint8_t client_id[TOX_PUBLIC_KEY_SIZE];
    tox_friend_get_public_key(tox, friend_id, client_id, 0);

    int len = friend_meta_data_path(p, sizeof(path) - (p - path), client_id, friend_id);
    if (len == -1) {
        debug("Metadata:\tError getting meta data file name for friend %d\n", friend_id);
        return;
    }

    uint32_t size;
    void *file_data = file_raw((char*)path, &size);
    if (!file_data) {
        // debug("Meta Data not found (%s)\n", path);
        return;
    }
    FRIEND_META_DATA *metadata = calloc(1, sizeof(*metadata) + size);

    memcpy(metadata, file_data, size);
    /* Compatibility code for original version of meta_data... TODO: Remove in version >= 0.10 */
    if (metadata->version >= 2) { /* Version 2 chosen because alias length (the original value at *
                                   * metadata[0] should be > 2 (hopefully)                        */
        if (size < sizeof(FRIEND_META_DATA_OLD)) {
            debug("Metadata:\tMeta Data was incomplete\n");
            return;
        }

        if (((FRIEND_META_DATA_OLD*)metadata)->alias_length) {
            friend_set_alias(&friend[friend_id], file_data + sizeof(size_t),
                                ((FRIEND_META_DATA_OLD*)metadata)->alias_length);
        } else {
            friend_set_alias(&friend[friend_id], NULL, 0);
        }

        debug("Metadata:\tConverting old metadata file to new!\n");
        utox_write_metadata(&friend[friend_id]);

        free(metadata);
        free(file_data);
        return;
    } else if (metadata->version != 0) {
        debug("Metadata:\tWARNING! This version of utox does not support this metadata file version.\n");
        return;
    }

    if (size < sizeof(*metadata)) {
        debug("Metadata:\tMeta Data was incomplete\n");
        return;
    }

    if (metadata->alias_length) {
        friend_set_alias(&friend[friend_id], &metadata->data[0], metadata->alias_length);
    } else {
        friend_set_alias(&friend[friend_id], NULL, 0); /* uTox expects this to be 0/NULL if there's no alias. */
    }

    friend[friend_id].ft_autoaccept         = metadata->ft_autoaccept;

    free(metadata);
    free(file_data);
    return;
}

void utox_friend_init(Tox *tox, uint32_t friend_number) {
        int size;
        // get friend pointer
        FRIEND *f = &friend[friend_number];
        uint8_t name[TOX_MAX_NAME_LENGTH];

        // Set scroll position to bottom of window.
        f->msg.scroll = 1.0;
        f->msg.panel.type           = PANEL_MESSAGES;
        f->msg.panel.content_scroll = &scrollbar_friend;
        f->msg.panel.y              = MAIN_TOP;
        f->msg.panel.height         = CHAT_BOX_TOP;
        f->msg.panel.width          = -SCROLL_WIDTH;

        // Get and set the public key for this friend number and set it.
        tox_friend_get_public_key(tox, friend_number, f->cid, 0);

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
        tox_friend_get_status_message(tox, friend_number, f->status_message, 0);
        f->status_length = size;

        // Get the hex version of this friends ID
        char_t cid[TOX_PUBLIC_KEY_SIZE * 2];
        cid_to_string(cid, f->cid);
        init_avatar(&f->avatar, cid, NULL, NULL);

        MESSAGES *m = &f->msg;
        messages_init(m, friend_number);

        // Get the chat backlog
        messages_read_from_log(friend_number);

        /* And make sure we load these too */
        log_read_old(tox, friend_number);

        // Load the meta data, if it exists.
        friend_meta_data_read(tox, friend_number);
}

void friend_setname(FRIEND *f, char_t *name, uint16_t length){
    /* TODO: rewrite */
    if (f->name && (length != f->name_length || memcmp(f->name, name, length) != 0)) {

        size_t size = sizeof(" is now known as ") - 1 + f->name_length + length;

        char_t *p = calloc(1, size);
        memcpy(p, f->name, f->name_length);
        memcpy(p + f->name_length, " is now known as ", sizeof(" is now known as ") - 1);
        memcpy(p + f->name_length + sizeof(" is now known as ") - 1, name, length);

        message_add_type_notice(&f->msg, p, size, 1);
    }

    free(f->name);
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
        if (selected_item->data && f->number == ((FRIEND*)selected_item->data)->number) {
            maybe_i18nal_string_set_plain(&edit_friend_alias.empty_str, f->name, f->name_length);
        }
    }

    update_shown_list();
}

void friend_set_alias(FRIEND *f, char_t *alias, uint16_t length){
    if (alias && length > 0) {
        debug("New Alias set for friend %s\n", f->name);
    } else {
        debug("Alias for friend %s unset\n", f->name);
    }

    free(f->alias);
    if(length == 0) {
        f->alias = NULL;
        f->alias_length = 0;
    } else {
        f->alias = malloc(length + 1);
        memcpy(f->alias, alias, length);
        f->alias_length = length;
        f->alias[f->alias_length] = 0;
    }
}

void friend_sendimage(FRIEND *f, UTOX_NATIVE_IMAGE *native_image, uint16_t width, uint16_t height,
                      UTOX_IMAGE png_image, size_t png_size)
{
    message_add_type_image(&f->msg, 1, native_image, width, height, 0);
    redraw();

    struct TOX_SEND_INLINE_MSG *tsim = malloc(sizeof(struct TOX_SEND_INLINE_MSG));
    tsim->image = png_image;
    tsim->image_size = png_size;
    postmessage_toxcore(TOX_FILE_SEND_NEW_INLINE, f - friend, 0, tsim);
}

void friend_recvimage(FRIEND *f, UTOX_NATIVE_IMAGE *native_image, uint16_t width, uint16_t height) {
    if (!UTOX_NATIVE_IMAGE_IS_VALID(native_image)) {
        return;
    }

    message_add_type_image(&f->msg, 0, native_image, width, height, 0);
}

void friend_notify_msg(FRIEND *f, uint8_t *msg, size_t msg_length) {

    uint8_t title[UTOX_FRIEND_NAME_LENGTH(f) + 25];

    size_t title_length = snprintf((char*)title, UTOX_FRIEND_NAME_LENGTH(f) + 25, "uTox new message from %.*s", UTOX_FRIEND_NAME_LENGTH(f), UTOX_FRIEND_NAME(f));

    notify(title, title_length, msg, msg_length, f);

    if (selected_item->data != f) {
        f->unread_msg = 1;
    }
}

_Bool friend_set_online(FRIEND *f, _Bool online) {
    if (f->online == online) {
        return false;
    }

    f->online = online;
    if (!f->online) {
        friend_set_typing(f, 0);
    }

    update_shown_list();

    return true;
}


void friend_set_typing(FRIEND *f, int typing) {
    f->typing = typing;
}

void friend_addid(uint8_t *id, char_t *msg, uint16_t msg_length)
{
    void *data = malloc(TOX_FRIEND_ADDRESS_SIZE + msg_length * sizeof(char_t));
    memcpy(data, id, TOX_FRIEND_ADDRESS_SIZE);
    memcpy(data + TOX_FRIEND_ADDRESS_SIZE, msg, msg_length * sizeof(char_t));

    postmessage_toxcore(TOX_FRIEND_NEW, msg_length, 0, data);
}

void friend_add(char_t *name, uint16_t length, char_t *msg, uint16_t msg_length)
{
    if(!length) {
        addfriend_status = ADDF_NONAME;
        return;
    }

#ifdef EMOJI_IDS
    uint8_t emo_id[TOX_FRIEND_ADDRESS_SIZE];
    if (emoji_string_to_bytes(emo_id, TOX_FRIEND_ADDRESS_SIZE, name, length) == TOX_FRIEND_ADDRESS_SIZE) {
        friend_addid(emo_id, msg, msg_length);
    }
#endif

    uint8_t name_cleaned[length];
    uint16_t length_cleaned = 0;

    unsigned int i;
    for (i = 0; i < length; ++i) {
        if (name[i] != ' ') {
            name_cleaned[length_cleaned] = name[i];
            ++length_cleaned;
        }
    }

    if(!length_cleaned) {
        addfriend_status = ADDF_NONAME;
        return;
    }

    uint8_t id[TOX_FRIEND_ADDRESS_SIZE];
    if(length_cleaned == TOX_FRIEND_ADDRESS_SIZE * 2 && string_to_id(id, name_cleaned)) {
        friend_addid(id, msg, msg_length);
    } else {
        /* not a regular id, try DNS discovery */
        addfriend_status = ADDF_DISCOVER;
        dns_request(name_cleaned, length_cleaned);
    }
}

#define LOGFILE_EXT ".txt"

void friend_history_clear(FRIEND *f)
{
    uint8_t path[UTOX_FILE_NAME_LENGTH], *p;

    messages_clear_all(&f->msg);

    {
        /* We get the file path of the log file */
        p = path + datapath(path);

        if(countof(path) - (p - path) < TOX_PUBLIC_KEY_SIZE * 2 + sizeof(LOGFILE_EXT))
        {
            /* We ensure that we have enough space in the buffer,
               if not we fail */
            debug("error/history_clear: path too long\n");
            return;
        }

        cid_to_string(p, f->cid);
        p += TOX_PUBLIC_KEY_SIZE * 2;
        memcpy((char*)p, LOGFILE_EXT, sizeof(LOGFILE_EXT));
    }

    remove((const char *)path);
}

void friend_free(FRIEND *f)
{
    uint16_t j = 0;
    while(j != f->edit_history_length) {
        free(f->edit_history[j]);
        j++;
    }
    free(f->edit_history);

    free(f->name);
    free(f->status_message);
    free(f->typed);

    uint32_t i = 0;
    while(i < f->msg.number) {
        MSG_TEXT *msg = f->msg.data[i];
        message_free(msg);
        i++;
    }

    free(f->msg.data);

    if(f->call_state_self) {
        // postmessage_audio(AUDIO_END, f->number, 0, NULL);
        /* TODO end a video call too!
        if(f->calling == CALL_OK_VIDEO) {
            postmessage_video(VIDEO_CALL_END, f->number, 0, NULL);
        }*/
    }

    memset(f, 0, sizeof(FRIEND));//
}
