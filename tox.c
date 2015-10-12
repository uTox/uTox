#include "main.h"
#include "tox_bootstrap.h"

struct Tox_Options options = {.proxy_host = proxy_address};

static volatile _Bool save_needed = 1;

/* Writes log filename for fid to dest. returns length written */
static int log_file_name(uint8_t *dest, size_t size_dest, Tox *tox, int fid) {
    if (size_dest < TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".txt"))
        return -1;

    uint8_t client_id[TOX_PUBLIC_KEY_SIZE];
    tox_friend_get_public_key(tox, fid, client_id, 0);
    cid_to_string(dest, client_id); dest += TOX_PUBLIC_KEY_SIZE * 2;
    memcpy((char*)dest, ".txt", sizeof(".txt"));

    return TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".txt");
}

/* Writes friend meta data filename for fid to dest. returns length written */
static int friend_meta_data_file(uint8_t *dest, size_t size_dest, Tox *tox, int fid) {
    if (size_dest < TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".fmetadata")){
        return -1;
    }

    uint8_t client_id[TOX_PUBLIC_KEY_SIZE];
    tox_friend_get_public_key(tox, fid, client_id, 0);
    cid_to_string(dest, client_id); dest += TOX_PUBLIC_KEY_SIZE * 2;
    memcpy((char*)dest, ".fmetadata", sizeof(".fmetadata"));

    return TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".fmetadata");
}

enum {
  LOG_FILE_MSG_TYPE_TEXT = 0,
  LOG_FILE_MSG_TYPE_ACTION = 1,
};

typedef struct {
    uint64_t time;
    uint16_t namelen, length;
    uint8_t flags;
    uint8_t msg_type;
    uint8_t zeroes[2];
} LOG_FILE_MSG_HEADER;

void log_write(Tox *tox, int fid, const uint8_t *message, uint16_t length, _Bool author, uint8_t msg_type)
{
    if(!logging_enabled) {
        return;
    }

    uint8_t path[UTOX_FILE_NAME_LENGTH], *p;
    uint8_t name[TOX_MAX_NAME_LENGTH];
    size_t namelen;
    FILE *file;

    p = path + datapath(path);

    int len = log_file_name(p, sizeof(path) - (p - path), tox, fid);
    if (len == -1)
        return;

    p += len;

    file = fopen((char*)path, "ab");
    if (file) {
        time_t rawtime;
        time(&rawtime);

        if (author) {
            namelen = tox_self_get_name_size(tox);
            tox_self_get_name(tox, name);
        } else {
            namelen = tox_friend_get_name_size(tox, fid, 0);
            tox_friend_get_name(tox, fid, name, 0);

        }

        if (namelen > TOX_MAX_NAME_LENGTH) {
            namelen = 0;
        }

        LOG_FILE_MSG_HEADER header = {
            .time = rawtime,
            .namelen = namelen,
            .length = length,
            .flags = author,
            .msg_type = msg_type,
        };

        fwrite(&header, sizeof(header), 1, file);
        fwrite(name, namelen, 1, file);
        fwrite(message, length, 1, file);
        fclose(file);
    }
}

void log_read(Tox *tox, int fid)
{
    uint8_t path[UTOX_FILE_NAME_LENGTH], *p;
    FILE *file;

    p = path + datapath(path);

    int len = log_file_name(p, sizeof(path) - (p - path), tox, fid);
    if (len == -1) {
        debug("Error getting log file name for friend %d\n", fid);
        return;
    }

    file = fopen((char*)path, "rb");
    if(!file) {
        debug("File not found (%s)\n", path);
        p = path + datapath_old(path);

        len = log_file_name(p, sizeof(path) - (p - path), tox, fid);
        if (len == -1) {
            debug("Error getting log file name for friend %d\n", fid);
            return;
        }

        file = fopen((char*) path, "rb");
        if (!file) {
            debug("File not found (%s)\n", path);
            return;
        }
    }

    LOG_FILE_MSG_HEADER header;
    off_t rewinds[MAX_BACKLOG_MESSAGES] = {};
    size_t records_count = 0;

    /* todo: some checks to avoid crashes with corrupted log files */
    /* first find the last MAX_BACKLOG_MESSAGES messages in the log */
    while(1 == fread(&header, sizeof(LOG_FILE_MSG_HEADER), 1, file)) {
        fseeko(file, header.namelen + header.length, SEEK_CUR);

        rewinds[records_count % countof(rewinds)] =
                (off_t) sizeof(LOG_FILE_MSG_HEADER) + header.namelen + header.length;
        records_count++;
    }

    if(ferror(file) || !feof(file)) {
        // TODO: consider removing or truncating the log file.
        // If !feof() this means that the file has an incomplete record,
        // which would prevent it from loading forever, even though
        // new records will keep being appended as usual.
        debug("Log read error (%s)\n", path);
        fclose(file);
        return;
    }

    // Backtrack to read last MAX_BACKLOG_MESSAGES in full.
    off_t rewind = 0;
    MSG_IDX i;
    for(i = 0; (i < records_count) && (i < countof(rewinds)); i++) {
        rewind += rewinds[i];
    }
    fseeko(file, -rewind, SEEK_CUR);

    MSG_DATA *m = &friend[fid].msg;
    m->data = malloc(sizeof(void*) * i);
    m->n = 0;

    /* add the messages */
    while((0 < i) && (1 == fread(&header, sizeof(LOG_FILE_MSG_HEADER), 1, file))) {
        i--;

        // Skip unused friend name recorded at the time.
        fseeko(file, header.namelen, SEEK_CUR);

        MESSAGE *msg = NULL;
        switch(header.msg_type) {
        case LOG_FILE_MSG_TYPE_ACTION: {
            msg = malloc(sizeof(MESSAGE) + header.length);
            msg->msg_type = MSG_TYPE_ACTION_TEXT;
            break;
        }
        case LOG_FILE_MSG_TYPE_TEXT: {
            msg = malloc(sizeof(MESSAGE) + header.length);
            msg->msg_type = MSG_TYPE_TEXT;
            break;
        }
        default: {
            debug("Unknown backlog message type(%d), skipping.\n", (int)header.msg_type);
            fseeko(file, header.length, SEEK_CUR);
            continue;
        }
        }

        // Read text message.
        msg->author = header.flags & 1;
        msg->length = header.length;

        if(1 != fread(msg->msg, msg->length, 1, file)) {
            debug("Log read error (%s)\n", path);
            fclose(file);
            return;
        }

        msg->length = utf8_validate(msg->msg, msg->length);

        struct tm *ti;
        time_t rawtime = header.time;
        ti = localtime(&rawtime);

        msg->time = ti->tm_hour * 60 + ti->tm_min;

        m->data[m->n++] = msg;

        // debug("loaded backlog: %d: %.*s\n", fid, msg->length, msg->msg);
    }

    fclose(file);
}

void friend_meta_data_read(Tox *tox, int friend_id) {
    /* Will need to be rewritten if anything is added to friend's meta data */
    uint8_t path[UTOX_FILE_NAME_LENGTH], *p;
    p = path + datapath(path);

    int len = friend_meta_data_file(p, sizeof(path) - (p - path), tox, friend_id);
    if (len == -1) {
        debug("Error getting meta data file name for friend %d\n", friend_id);
        return;
    }

    uint32_t size;
    void *mdata = file_raw((char*)path, &size);
    if (!mdata) {
        debug("Meta Data not found (%s)\n", path);
        return;
    }
    FRIEND_META_DATA *metadata = calloc(1, sizeof(*metadata));

    if (size < sizeof(*metadata)) {
        debug("Meta Data was incomplete\n");
        return;
    }

    memcpy(metadata, mdata, sizeof(*metadata));
    if (metadata->alias_length) {
        friend_set_alias(&friend[friend_id], mdata + sizeof(size_t), metadata->alias_length);
    } else {
        friend_set_alias(&friend[friend_id], NULL, 0); /* uTox depends on this being 0/NULL if there's no alias. */
    }
    free(metadata);
    free(mdata);
}

static void tox_thread_message(Tox *tox, ToxAV *av, uint64_t time, uint8_t msg, uint32_t param1, uint32_t param2, void *data);

void tox_postmessage(uint8_t msg, uint32_t param1, uint32_t param2, void *data) {
    while(tox_thread_msg) {
        yieldcpu(1);
    }

    tox_msg.msg = msg;
    tox_msg.param1 = param1;
    tox_msg.param2 = param2;
    tox_msg.data = data;

    tox_thread_msg = 1;
}

#include "tox_callbacks.h"

/* bootstrap to dht with bootstrap_nodes */
static void do_bootstrap(Tox *tox)
{
    static unsigned int j = 0;

    if (j == 0)
        j = rand();

    int i = 0;
    while(i < 4) {
        struct bootstrap_node *d = &bootstrap_nodes[j % countof(bootstrap_nodes)];
        tox_bootstrap(tox, d->address, d->port, d->key, 0);
        tox_add_tcp_relay(tox, d->address, d->port, d->key, 0);
        i++;
        j++;
    }
}

static void set_callbacks(Tox *tox)
{
    tox_callback_friend_request(tox, callback_friend_request, NULL);
    tox_callback_friend_message(tox, callback_friend_message, NULL);
    tox_callback_friend_name(tox, callback_name_change, NULL);
    tox_callback_friend_status_message(tox, callback_status_message, NULL);
    tox_callback_friend_status(tox, callback_user_status, NULL);
    tox_callback_friend_typing(tox, callback_typing_change, NULL);
    tox_callback_friend_read_receipt(tox, callback_read_receipt, NULL);
    tox_callback_friend_connection_status(tox, callback_connection_status, NULL);

    tox_callback_group_invite(tox, callback_group_invite, NULL);
    tox_callback_group_message(tox, callback_group_message, NULL);
    tox_callback_group_action(tox, callback_group_action, NULL);
    tox_callback_group_namelist_change(tox, callback_group_namelist_change, NULL);
    tox_callback_group_title(tox, callback_group_title, NULL);

    utox_set_callbacks_for_transfer(tox);
}

static size_t load_save(uint8_t **out_data){
    uint8_t path[UTOX_FILE_NAME_LENGTH], *p, *data;
    uint32_t size;

    do{ /* Try the STS compliant save location */
        p = path + datapath(path);
        strcpy((char*)p, "tox_save.tox");
        data = file_raw((char*)path, &size);
        if(data) break; /* We have data, were done here! */
        /* Try filename missing the .tox extension */
        p = path + datapath(path);
        strcpy((char*)p, "tox_save");
        data = file_raw((char*)path, &size);
        if(data) break;
        /* That didn't work, do we have a backup? */
        p = path + datapath(path);
        strcpy((char*)p, "tox_save.tmp");
        data = file_raw((char*)path, &size);
        if(data) break;
        /* No backup huh? Is it in an old location we support? */
        p = path + datapath_old(path);
        strcpy((char*)p, "tox_save");
        data = file_raw((char*)path, &size);
        if(data) break;
        /* Well, lets try the current directory... */
        data = file_raw("tox_save", &size);
        if(!data) return 0; /* F***it I give up! */
    } while(0); /* Only once! */

    *out_data = data;
    return size;
}

static void tox_after_load(Tox *tox)
{
    friends = tox_self_get_friend_list_size(tox);

    uint32_t i = 0;
    while(i != friends) {
        utox_friend_init(tox, i);
        i++;
    }

    self.name_length = tox_self_get_name_size(tox);
    tox_self_get_name(tox, self.name);
    self.statusmsg_length = tox_self_get_status_message_size(tox);
    self.statusmsg = malloc(self.statusmsg_length);
    tox_self_get_status_message(tox, self.statusmsg);
    self.status = tox_self_get_status(tox);
}

static void load_defaults(Tox *tox)
{
    uint8_t *name = (uint8_t*)DEFAULT_NAME, *status = (uint8_t*)DEFAULT_STATUS;
    uint16_t name_len = sizeof(DEFAULT_NAME) - 1, status_len = sizeof(DEFAULT_STATUS) - 1;

    tox_self_set_name(tox, name, name_len, 0);
    tox_self_set_status_message(tox, status, status_len, 0);

    self.name_length = name_len;
    memcpy(self.name, name, name_len);
    self.statusmsg_length = status_len;
    self.statusmsg = malloc(status_len);
    memcpy(self.statusmsg, status, status_len);
}

static void write_save(Tox *tox) {
    void *data;
    uint32_t size;
    uint8_t path_tmp[UTOX_FILE_NAME_LENGTH], path_real[UTOX_FILE_NAME_LENGTH], *p;
    FILE *file;

    size = tox_get_savedata_size(tox);
    data = malloc(size);
    tox_get_savedata(tox, data);

    p = path_real + datapath(path_real);
    memcpy(p, "tox_save.tox", sizeof("tox_save.tox"));

    unsigned int path_len = (p - path_real) + sizeof("tox_save.tox");
    memcpy(path_tmp, path_real, path_len);
    memcpy(path_tmp + (path_len - 1), ".tmp", sizeof(".tmp"));

    debug("Writing tox_save to: '%s': ", (char*)path_tmp);
    file = fopen((char*)path_tmp, "wb");
    if(file) {
        fwrite(data, size, 1, file);
        flush_file(file);
        fclose(file);
        if (rename((char*)path_tmp, (char*)path_real) != 0) {
            debug("\nFailed to rename file %s to %s; deleting and trying again: ", path_tmp, path_real);
            remove((const char *)path_real);
            if (rename((char*)path_tmp, (char*)path_real) != 0) {
                debug("\nSaving Failed!!\n");
            } else {
                debug("Saved data\n");
            }
        } else {
            debug("Saved data! Trying to chmod: ");
            int ch = ch_mod(path_real);
            if(!ch){
                debug("success!\n");
            } else {
                debug("failed!\n");
            }
        }
    } else {
        debug("no data saved...\n");
    }

    save_needed = 0;
    free(data);
}

void tox_settingschanged(void)
{
    //free everything
    tox_connected = 0;
    list_freeall();

    list_dropdown_clear(&dropdown_audio_in);
    list_dropdown_clear(&dropdown_audio_out);
    list_dropdown_clear(&dropdown_video);

    tox_thread_init = 0;

    toxaudio_postmessage(AUDIO_KILL, 0, 0, NULL);
    toxvideo_postmessage(VIDEO_KILL, 0, 0, NULL);
    toxav_postmessage(TOXAV_KILL, 0, 0, NULL);

    // send the reconfig message!
    tox_postmessage(0, 1, 0, NULL);

    while(!tox_thread_init) {
        yieldcpu(1);
    }

    list_start();
}

#define UTOX_TYPING_NOTIFICATION_TIMEOUT (1ul*1000*1000*1000)

static struct {
    Tox *tox;
    uint16_t friendnumber;
    uint64_t time;
    _Bool sent_value;
} typing_state = {
        .tox = NULL,
        .friendnumber = 0,
        .time = 0,
        .sent_value = 0,
};

static void utox_thread_work_for_typing_notifications(Tox *tox, uint64_t time) {
    if(typing_state.tox != tox) {
        // Guard against Tox engine restarts.
        return;
    }

    _Bool is_typing = (time < typing_state.time + UTOX_TYPING_NOTIFICATION_TIMEOUT);
    if(typing_state.sent_value ^ is_typing) {
        // Need to send an update.
        if(tox_self_set_typing(tox, typing_state.friendnumber, is_typing, 0)){
            // Successfully sent. Mark new state.
            typing_state.sent_value = is_typing;
            debug("Sent typing state to friend (%d): %d\n", typing_state.friendnumber, typing_state.sent_value);
        }
    }
}

/** void tox_thread(void)
 *
 * Main tox function, starts a new toxcore for utox to use, and then spawns its
 * threads.
 *
 * Accepts and returns nothing.
 */
void tox_thread(void *UNUSED(args)) {
    Tox *tox;
    ToxAV *av;
    uint8_t id[TOX_FRIEND_ADDRESS_SIZE];

    _Bool reconfig;

    do {
        uint8_t *save_data = NULL;
        size_t save_size = load_save(&save_data);
        // Create main connection
        TOX_ERR_NEW tox_new_err;
        if (save_data && save_size) {
            options.savedata_type = TOX_SAVEDATA_TYPE_TOX_SAVE;
            options.savedata_data = save_data;
            options.savedata_length = save_size;
        }

        debug("new tox object ipv6: %u udp: %u proxy: %u %s %u\n", options.ipv6_enabled, options.udp_enabled, options.proxy_type, options.proxy_host, options.proxy_port);
        if((tox = tox_new(&options, &tox_new_err)) == NULL) {
            debug("trying without proxy, err %u\n", tox_new_err);
            if(!options.proxy_type || (options.proxy_type = TOX_PROXY_TYPE_NONE, (tox = tox_new(&options, &tox_new_err)) == NULL)) {
                debug("trying without ipv6, err %u\n", tox_new_err);
                if(!options.ipv6_enabled || (options.ipv6_enabled = 0, (tox = tox_new(&options, &tox_new_err)) == NULL)) {
                    debug("tox_new() failed %u\n", tox_new_err);
                    exit(1);
                }
                dropdown_ipv6.selected = dropdown_ipv6.over = 1;
            }
            dropdown_proxy.selected = dropdown_proxy.over = 0;
        }

        if (save_size) {
            tox_after_load(tox);
            free(save_data);
        } else {
            debug("No save file, using defaults\n");
            load_defaults(tox);
        }

        // Set local info for self
        edit_setstr(&edit_name, self.name, self.name_length);
        edit_setstr(&edit_status, self.statusmsg, self.statusmsg_length);

        // Get tox id, and gets the hex version for utox
        tox_self_get_address(tox, id);
        memcpy(self.id_binary, id, TOX_FRIEND_ADDRESS_SIZE);
        id_to_string(self.id_buffer, id);
        self.id_buffer_length = TOX_FRIEND_ADDRESS_SIZE * 2;
        debug("Tox ID: %.*s\n", (int)self.id_buffer_length, self.id_buffer);

        {
            uint8_t avatar_data[UTOX_AVATAR_MAX_DATA_LENGTH];
            uint32_t avatar_size;

            uint8_t hex_id[TOX_FRIEND_ADDRESS_SIZE * 2];
            id_to_string(hex_id, self.id_binary);
            if (init_avatar(&self.avatar, hex_id, avatar_data, &avatar_size)) {
                self.avatar_data = malloc(avatar_size);
                if (self.avatar_data) {
                    memcpy(self.avatar_data, avatar_data, avatar_size);
                    self.avatar_size = avatar_size;
                    self.avatar_format = UTOX_AVATAR_FORMAT_PNG;
                    char_t hash_string[TOX_HASH_LENGTH * 2];
                    hash_to_string(hash_string, self.avatar.hash);
                    debug("Tox Avatar Hash: %.*s\n", (int)sizeof(hash_string), hash_string);
                }
            }
        }

        // Give toxcore the functions to call
        set_callbacks(tox);

        // Connect to bootstrapped nodes in "tox_bootstrap.h"
        do_bootstrap(tox);

        // Start the tox av session.
        TOXAV_ERR_NEW toxav_error;
        av = toxav_new(tox, &toxav_error);

        // Give toxcore the av functions to call
        set_av_callbacks(av);

        global_av = av;
        tox_thread_init = 1;

        // Start the treads
        thread(audio_thread, av);
        thread(video_thread, av);
        thread(toxav_thread, av);

        //
        _Bool connected = 0;
        uint64_t last_save = get_time(), time;
        while(1) {
            // Put toxcore to work
            tox_iterate(tox);

            // Check currents connection
            if(!!tox_self_get_connection_status(tox) != connected) {
                connected = !connected;
                postmessage(DHT_CONNECTED, connected, 0, NULL);
            }

            time = get_time();
            // Wait 1million ticks then reconnect if needed and write save
            if(time - last_save >= (uint64_t)10 * 1000 * 1000 * 1000) {
                last_save = time;

                if(!connected) {
                    do_bootstrap(tox);
                }
                //save every 10mill.
                if (save_needed || (time - last_save >= (uint64_t)100 * 1000 * 1000 * 1000)){
                    // Save tox data
                    write_save(tox);
                }
            }

            // If there's a message, load it, and send to the tox message thread
            if (tox_thread_msg) {
                TOX_MSG *msg = &tox_msg;
                // If msg->msg is 0, reconfig if needed and break from tox_do
                if (!msg->msg) {
                    reconfig = msg->param1;
                    tox_thread_msg = 0;
                    break;
                }
                tox_thread_message(tox, av, time, msg->msg, msg->param1, msg->param2, msg->data);
                tox_thread_msg = 0;
            }

            if (!dont_send_typing_notes){
                // Thread active transfers and check if friend is typing
                utox_thread_work_for_typing_notifications(tox, time);
            }

            // Ask toxcore how many ms to wait, then wait at the most 20ms
            uint32_t interval = tox_iteration_interval(tox);
            yieldcpu((interval > 20) ? 20 : interval);
        }

        write_save(tox);

        // Wait for all a/v threads to return 0
        while(audio_thread_init || video_thread_init || toxav_thread_init) {
            yieldcpu(1);
        }

        // Stop av threads, and toxcore.
        debug("av_thread exit, tox thread ending\n");
        toxav_kill(av);
        tox_kill(tox);

    } while(reconfig);

    tox_thread_init = 0;
}

static void tox_thread_message(Tox *tox, ToxAV *av, uint64_t time, uint8_t msg, uint32_t param1, uint32_t param2, void *data) {
    switch(msg) {
        /* Change Self in core */
        case TOX_SELF_SET_NAME: {
            /* param1: name length
             * data: name
             */
            tox_self_set_name(tox, data, param1, 0);
            save_needed = 1;
            break;
        }
        case TOX_SELF_SET_STATUS: {
            /* param1: status length
             * data: status message
             */
            tox_self_set_status_message(tox, data, param1, 0);
            save_needed = 1;
            break;
        }
        case TOX_SELF_SET_STATE: {
            /* param1: status
             */
            tox_self_set_status(tox, param1);
            save_needed = 1;
            break;
        }

        /* Avatar status */
        case TOX_AVATAR_SET: {
            /* param1: avatar format
             * param2: length of avatar data
             * data: raw avatar data (PNG)
             */

            if (self.avatar_data) {
                free(self.avatar_data);
            }

            self.avatar_data = data;
            self.avatar_size = param2;
            self.avatar_format = param1;
            utox_avatar_update_friends(tox);
            save_needed = 1;
            break;
        }
        case TOX_AVATAR_UNSET: {
            free(self.avatar_data);
            self.avatar_data = NULL;
            self.avatar_size = 0;
            self.avatar_format = 0;
            utox_avatar_update_friends(tox);
            save_needed = 1;
            break;
        }

        /* Interact with contacts */
        case TOX_FRIEND_NEW: {
            /* param1: length of message
             * data: friend id + message
             */
            uint32_t fid;
            TOX_ERR_FRIEND_ADD f_err;

            if(!param1) {
                STRING* default_add_msg = SPTR(DEFAULT_FRIEND_REQUEST_MESSAGE);
                fid = tox_friend_add(tox, data, default_add_msg->str, default_add_msg->length, &f_err);
            } else {
                fid = tox_friend_add(tox, data, data + TOX_FRIEND_ADDRESS_SIZE, param1, &f_err);
            }

            if(f_err != TOX_ERR_FRIEND_ADD_OK) {
                uint8_t addf_error;
                switch(f_err) {
                case TOX_ERR_FRIEND_ADD_TOO_LONG:
                    addf_error = ADDF_TOOLONG; break;
                case TOX_ERR_FRIEND_ADD_NO_MESSAGE:
                    addf_error = ADDF_NOMESSAGE; break;
                case TOX_ERR_FRIEND_ADD_OWN_KEY:
                    addf_error = ADDF_OWNKEY; break;
                case TOX_ERR_FRIEND_ADD_ALREADY_SENT:
                    addf_error = ADDF_ALREADYSENT; break;
                case TOX_ERR_FRIEND_ADD_BAD_CHECKSUM:
                    addf_error = ADDF_BADCHECKSUM; break;
                case TOX_ERR_FRIEND_ADD_SET_NEW_NOSPAM:
                    addf_error = ADDF_SETNEWNOSPAM; break;
                case TOX_ERR_FRIEND_ADD_MALLOC:
                    addf_error = ADDF_NOMEM; break;
                default:
                    addf_error = ADDF_UNKNOWN; break;
                }
                postmessage(FRIEND_SEND_REQUEST, 1, addf_error, data);
            } else {
                utox_friend_init(tox, fid);
                postmessage(FRIEND_SEND_REQUEST, 0, fid, data);
            }
            save_needed = 1;
            break;
        }
        case TOX_FRIEND_ACCEPT: {
            /* data: FRIENDREQ
             */
            FRIENDREQ *req = data;
            TOX_ERR_FRIEND_ADD f_err;
            uint32_t fid = tox_friend_add_norequest(tox, req->id, &f_err);
            utox_friend_init(tox, fid);
            postmessage(FRIEND_ACCEPT_REQUEST, (f_err != TOX_ERR_FRIEND_ADD_OK), (f_err != TOX_ERR_FRIEND_ADD_OK) ? 0 : fid, req);
            save_needed = 1;
            break;
        }
        case TOX_FRIEND_DELETE: {
            /* param1: friend #
             */
            tox_friend_delete(tox, param1, 0);
            postmessage(FRIEND_REMOVE, 0, 0, data);
            save_needed = 1;
            break;
        }
        case TOX_FRIEND_ONLINE: {
            if(!param2) {
                ft_friend_offline(tox, param1);
                utox_av_local_disconnect(av, param1);
            } else {
                if (!friend[param1].online) {
                    ft_friend_online(tox, param1);
                    /* resend avatar info (in case it changed) */
                    /* Avatars must be sent LAST or they will clobber existing file transfers! */
                    avatar_on_friend_online(tox, param1);
                    friend[param1].online = 1;
                }
            }
            break;
        }

        /* Default actions */
        case TOX_SEND_MESSAGE:
        case TOX_SEND_ACTION: {
            /* param1: friend #
             * param2: message length
             * data: message
             */
            void *p = data;
            TOX_MESSAGE_TYPE type;
            if(msg == TOX_SEND_ACTION){
                type = TOX_MESSAGE_TYPE_ACTION;
            } else {
                type = TOX_MESSAGE_TYPE_NORMAL;
            }
            while(param2 > TOX_MAX_MESSAGE_LENGTH) {
                uint16_t len = TOX_MAX_MESSAGE_LENGTH - utf8_unlen(p + TOX_MAX_MESSAGE_LENGTH);
                tox_friend_send_message(tox, param1, type, p, len, 0);
                param2 -= len;
                p += len;
            }
            // Send last or only message
            tox_friend_send_message(tox, param1, type, p, param2, 0);

            /* write message to friend to logfile */
            log_write(tox, param1, data, param2, 1, LOG_FILE_MSG_TYPE_TEXT);

            free(data);
            break;
        }
        case TOX_SEND_TYPING: {
            /* param1: friend #
             */

            // Check if user has switched to another friend window chat.
            // Take care not to react on obsolete data from old Tox instance.
            _Bool need_resetting = (typing_state.tox == tox) &&
                (typing_state.friendnumber != param1) &&
                (typing_state.sent_value);

            if(need_resetting) {
                // Tell previous friend that he's betrayed.
                tox_self_set_typing(tox, typing_state.friendnumber, 0, 0);
                // Mark that new friend doesn't know that we're typing yet.
                typing_state.sent_value = 0;
            }

            // Mark us as typing to this friend at the moment.
            // utox_thread_work_for_typing_notifications() will
            // send a notification if it deems necessary.
            typing_state.tox = tox;
            typing_state.friendnumber = param1;
            typing_state.time = time;

            //debug("Set typing state for friend (%d): %d\n", typing_state.friendnumber, typing_state.sent_value);
            break;
        }

        /* File transfers are so in right now. */
        case TOX_FILE_SEND_NEW:
        case TOX_FILE_SEND_NEW_SLASH:{
            /* param1: friend #
             * param2: offset of first file name in data
             * data: file names
             */

            /* If friend doesn't exist, don't send file. */
            if (param1 >= MAX_NUM_FRIENDS) {
                break;
            }

            if(param2 == 0xFFFF) {
                //paths with line breaks
                uint8_t *name = data, *p = data, *s = name;
                while(*p) {
                    _Bool end = 1;
                    while(*p) {
                        if(*p == '\n') {
                            *p = 0;
                            end = 0;
                            break;
                        }

                        if(*p == '/' || *p == '\\') {
                            s = p + 1;
                        }
                        p++;
                    }

                    if(strcmp2(name, "file://") == 0) {
                        name += 7;
                    }                   /* tox, friend, path, filename, filename_length */
                    outgoing_file_send(tox, param1, name,        s,           p - s, TOX_FILE_KIND_DATA);
                    p++;
                    s = name = p;

                    if(end) {
                        break;
                    }
                }
            } else {
                //windows path list
                uint8_t *name = data;
                _Bool multifile = (name[param2 - 1] == 0);
                if(!multifile) {
                                        /* tox, Friend, path, filename,      filename_length */
                    outgoing_file_send(tox, param1, name, name + param2, strlen((const char*)(name + param2)), TOX_FILE_KIND_DATA);
                } else {
                    uint8_t *p = name + param2;
                    name += param2 - 1;
                    if(*(name - 1) != '\\') {
                        *name++ = '\\';
                    }
                    while(*p) {
                        int len = strlen((char*)p) + 1;
                        memmove(name, p, len);
                        p += len;
                        outgoing_file_send(tox, param1, data, name, len -1, TOX_FILE_KIND_DATA);
                    }
                }
            }

            if(msg != TOX_FILE_SEND_NEW_SLASH){
                free(data);
            }

            break;
        }
        case TOX_FILE_SEND_NEW_INLINE: {
            /* param1: friend id
               data: pointer to a TOX_SEND_INLINE_MSG struct
             */
            struct TOX_SEND_INLINE_MSG *tsim = data;
            outgoing_file_send(tox, param1, NULL, tsim->image->png_data, tsim->image_size, TOX_FILE_KIND_DATA);
            free(tsim);

            break;
        }
        case TOX_FILE_ACCEPT: {
            /* param1: friend #
             * param2: file #
             * data: path to write file */
            if (utox_file_start_write(param1, param2, data) == 0) {
            /*                          tox, friend#, file#,        START_FILE */
                file_transfer_local_control(tox, param1, param2, TOX_FILE_CONTROL_RESUME);
            } else {
                file_transfer_local_control(tox, param1, param2, TOX_FILE_CONTROL_CANCEL);
            }
            break;
            free(data);
        }
        case TOX_FILE_RESUME:{
            /* param1: friend #
             * param2: file #           tox, friend#, file#,       RESUME_FILE */
            file_transfer_local_control(tox, param1, param2, TOX_FILE_CONTROL_RESUME);
            break;
        }
        case TOX_FILE_PAUSE:{
            /* param1: friend #
             * param2: file #           tox, friend#, file#,        PAUSE_FILE */
            file_transfer_local_control(tox, param1, param2, TOX_FILE_CONTROL_PAUSE);
            break;
        }
        case TOX_FILE_CANCEL:{
            /* param1: friend #
             * param2: file #           tox, friend#, file#,        CANCEL_FILE */
            file_transfer_local_control(tox, param1, param2, TOX_FILE_CONTROL_CANCEL);
            break;
        }

        /* Audio & Video */
        case TOX_CALL_SEND: {
            /* param1: friend #
             */

            /* Set the video bitrate, if we're starting a video call. */
            int v_bitrate = 0;
            if (param2) {
                v_bitrate = UTOX_DEFAULT_VIDEO_BITRATE;
                debug("Tox:\tSending video call to friend %u\n", param1);
            } else {
                v_bitrate = 0;
                debug("Tox:\tSending call to friend %u\n", param1);
            }

            TOXAV_ERR_CALL error = 0;
            toxav_call(av, param1, UTOX_DEFAULT_AUDIO_BITRATE, v_bitrate, &error);
            if (error) {
                switch(error) {
                    case TOXAV_ERR_CALL_MALLOC: {
                        debug("Tox:\tError making call to friend %u; Unable to malloc for this call.\n", param1);
                        break;
                    }
                    case TOXAV_ERR_CALL_FRIEND_ALREADY_IN_CALL: {
                        /* This shouldn't happen, but just in case toxav gets a call before uTox gets this message */
                        debug("Tox:\tError making call to friend %u; Already in call.\n", param1);
                        debug("Tox:\tForwarding and accepting call!\n");
                        tox_postmessage(TOX_CALL_ANSWER, param1, param2, data);
                        break;
                    }
                    default: {
                        /* Un-handled errors
                        TOXAV_ERR_CALL_SYNC,
                        TOXAV_ERR_CALL_FRIEND_NOT_FOUND,
                        TOXAV_ERR_CALL_FRIEND_NOT_CONNECTED,
                        TOXAV_ERR_CALL_FRIEND_ALREADY_IN_CALL,
                        TOXAV_ERR_CALL_INVALID_BIT_RATE,*/
                        debug("Error making call to %u, error num is %i.\n", param1, error);
                        break;
                    }
                }
            } else {
                friend[param1].call_state_self = ( TOXAV_FRIEND_CALL_STATE_SENDING_A | TOXAV_FRIEND_CALL_STATE_ACCEPTING_A );
                toxaudio_postmessage(AUDIO_START, param1, 0, NULL); // TODO, do we really want this to be HERE?
                debug("uToxAV:\tCall is ringing\n");
                postmessage(AV_CALL_RINGING, param1, 0, NULL);

                if (param2) {
                    toxvideo_postmessage(VIDEO_START, param1, 0, NULL);
                    friend[param1].call_state_self |= (TOXAV_FRIEND_CALL_STATE_SENDING_V | TOXAV_FRIEND_CALL_STATE_ACCEPTING_V);
                }
            }
            break;
        }
        case TOX_CALL_INCOMING:{
            break;
        }
        case TOX_CALL_ANSWER: {
            /* param1: Friend_number #
             * param2: Accept Video? #
             */
            FRIEND *f = &friend[param1];
            TOXAV_ERR_ANSWER error = 0;
            int v_bitrate = 0;

            if (param2) {
                v_bitrate = UTOX_DEFAULT_VIDEO_BITRATE;
                debug("uTox:\tAnswering video call.\n");
            } else {
                v_bitrate = 0;
                debug("uTox:\tAnswering audio call.\n");
            }

            toxaudio_postmessage(AUDIO_STOP_RINGTONE, param1, 0, NULL);
            toxaudio_postmessage(AUDIO_START, param1, 0, NULL);

            if (param2) {
                toxvideo_postmessage(VIDEO_START, param1, 0, NULL);
                friend[param1].call_state_self |= (TOXAV_FRIEND_CALL_STATE_SENDING_V | TOXAV_FRIEND_CALL_STATE_ACCEPTING_V);
            }
            toxav_answer(av, param1, UTOX_DEFAULT_AUDIO_BITRATE, v_bitrate, &error);

            if (error) {
                debug("uTox:\tError trying to toxav_answer error (%i)\n", error);
            } else {
                f->call_state_self = ( TOXAV_FRIEND_CALL_STATE_SENDING_A | TOXAV_FRIEND_CALL_STATE_ACCEPTING_A );
            }
            postmessage(AV_CALL_ACCEPTED, param1, 0, NULL);
            break;
        }
        case TOX_CALL_PAUSE_AUDIO: {
            /* param1: friend #
             * param2: call #
             */
            debug("TODO bug, please report!!\n");
            break;
        }
        case TOX_CALL_PAUSE_VIDEO: {
            /* param1: friend #
             * param2: call #
             */
            debug("TODO bug, please report!!\n");
            break;
        }
        case TOX_CALL_DISCONNECT: {
            /* param1: friend_number
             */
            if (friend[param1].call_state_self || friend[param1].call_state_friend) {
                utox_av_local_disconnect(av, param1);
            }
            break;
        }

        /* Groups are broken while we await the new GCs getting merged. */
        case TOX_GROUP_CREATE: {
            int g = -1;
            if (param1) {
                // TODO FIX THIS AFTER NEW GROUP API
                // g = toxav_add_av_groupchat(tox, &callback_av_group_audio, NULL);
            } else {
                g = tox_add_groupchat(tox);
            }
            if(g != -1) {
                postmessage(GROUP_ADD, g, 0, tox);
            }
            save_needed = 1;
            break;
        }
        case TOX_GROUP_EXIT: {
            /* param1: group #
             */
            tox_del_groupchat(tox, param1);
            save_needed = 1;
            break;
        }
        case TOX_GROUP_SEND_INVITE: {
            /* param1: group #
             * param2: friend #
             */
            tox_invite_friend(tox, param2, param1);
            save_needed = 1;
            break;
        }
        case TOX_GROUP_SET_TOPIC: {
            /* param1: group #
             * param2: topic length
             * data: topic
             */
            tox_group_set_title(tox, param1, data, param2);
            postmessage(GROUP_TITLE, param1, param2, data);
            save_needed = 1;
            break;
        }
        case TOX_GROUP_SEND_MESSAGE: {
            /* param1: group #
             * param2: message length
             * data: message
             */
            tox_group_message_send(tox, param1, data, param2);
            free(data);
            break;
        }
        case TOX_GROUP_SEND_ACTION: {
            /* param1: group #
             * param2: message length
             * data: message
             */
            tox_group_action_send(tox, param1, data, param2);
            free(data);
            break;
        }
        case TOX_GROUP_AUDIO_START:{
            /* param1: group #
             */
            postmessage(GROUP_AUDIO_START, param1, 0, NULL);
            break;
        }
        case TOX_GROUP_AUDIO_END:{
            /* param1: group #
             */
            postmessage(GROUP_AUDIO_END, param1, 0, NULL);
            break;
        }
    } // End of switch.
}

/** Translates status code to text then sends back to the user */
static void file_notify(FRIEND *f, MSG_FILE *msg) {
    STRING *str;
    switch(msg->status) {
        case FILE_TRANSFER_STATUS_NONE: {
            str = SPTR(TRANSFER_NEW); break;
        }
        case FILE_TRANSFER_STATUS_ACTIVE: {
            str = SPTR(TRANSFER_STARTED); break;
        }
        case FILE_TRANSFER_STATUS_PAUSED_BOTH: {
            str = SPTR(TRANSFER___); break;
        }
        case FILE_TRANSFER_STATUS_PAUSED_US:
        case FILE_TRANSFER_STATUS_PAUSED_THEM: {
            str = SPTR(TRANSFER_PAUSED); break;
        }
        case FILE_TRANSFER_STATUS_KILLED: {
            str = SPTR(TRANSFER_CANCELLED); break;
        }
        case FILE_TRANSFER_STATUS_COMPLETED: {
            str = SPTR(TRANSFER_COMPLETE); break;
        }
        case FILE_TRANSFER_STATUS_BROKEN:
        default: { //render unknown status as "transfer broken"
            str = SPTR(TRANSFER_BROKEN); break;
        }
    }

    friend_notify(f, str->str, str->length, msg->name, msg->name_length);
}

static void call_notify(FRIEND *f, uint8_t status) {
    STRING *str;
    switch(status) {
        case UTOX_AV_INVITE: {
            str = SPTR(CALL_INVITED);
            break;
        }
        case UTOX_AV_RINGING: {
            str = SPTR(CALL_RINGING);
            break;
        }
        case UTOX_AV_STARTED: {
            str = SPTR(CALL_STARTED);
            break;
        }
        default: {//render unknown status as "call canceled"
            str = SPTR(CALL_CANCELLED);
            break;
        }
    }

    friend_notify(f, str->str, str->length, (uint8_t*)"", 0);
    friend_addmessage_notify(f, str->str, str->length);
}

void tox_message(uint8_t tox_message_id, uint16_t param1, uint16_t param2, void *data) {
    switch(tox_message_id) {
        /* General core and networking messages */
        case TOX_DONE: {
            /* Does nothing. */
            break;
        }
        case DHT_CONNECTED: {
            /* param1: connection status (1 = connected, 0 = disconnected) */
            tox_connected = param1;
            if (tox_connected) {
                debug("uTox:\tConnected to DHT!\n");
            } else {
                debug("uTox:\tDisconnected from DHT!\n");
            }
            redraw();
            break;
        }
        case DNS_RESULT: {
            /* param1: result (0 = failure, 1 = success)
             * data: resolved tox id (if successful)
             */
            if (param1) {
                friend_addid(data, edit_add_msg.data, edit_add_msg.length);
            } else {
                addfriend_status = ADDF_BADNAME;
            }
            free(data);
            redraw();
            break;
        }

        /* OS interaction/integration messages */
        case AUDIO_IN_DEVICE: {
            /* param1: string
             * param2: default device?
             * data: device identifier.
             */
            if(UI_STRING_ID_INVALID == param1) {
                list_dropdown_add_hardcoded(&dropdown_audio_in, data, data);
            } else {
                list_dropdown_add_localized(&dropdown_audio_in, param1, data);
            }

            if (loaded_audio_in_device == (uint16_t)~0 && param2) {
                loaded_audio_in_device = (dropdown_audio_in.dropcount - 1);
            }

            if (loaded_audio_in_device != 0 && (dropdown_audio_in.dropcount - 1) == loaded_audio_in_device) {
                toxaudio_postmessage(AUDIO_SET_INPUT, 0, 0, data);
                dropdown_audio_in.selected = loaded_audio_in_device;
                loaded_audio_in_device = 0;
            }
            break;
        }
        case AUDIO_OUT_DEVICE: {
            list_dropdown_add_hardcoded(&dropdown_audio_out, data, data);

            if (loaded_audio_out_device != 0 && (dropdown_audio_out.dropcount - 1) == loaded_audio_out_device) {
                toxaudio_postmessage(AUDIO_SET_OUTPUT, 0, 0, data);
                dropdown_audio_out.selected = loaded_audio_out_device;
                loaded_audio_out_device = 0;
            }

            break;
        }
        case VIDEO_IN_DEVICE: {
            if(UI_STRING_ID_INVALID == param1) {
                // Device name is a hardcoded string.
                // data is a pointer to a buffer, that contains device handle pointer,
                // followed by device name string.
                list_dropdown_add_hardcoded(&dropdown_video, data + sizeof(void*), *(void**)data);
            } else {
                // Device name is localized with param1 containing UI_STRING_ID.
                // data is device handle pointer.
                list_dropdown_add_localized(&dropdown_video, param1, data);
            }
            //param2 == true, if this device will be chosen by video detecting code.
            if(param2) {
                dropdown_video.selected = dropdown_video.over = (dropdown_video.dropcount - 1);
            }
            break;
        }

        /* Client/User Interface messages. */
        case REDRAW:{
            redraw();
            break;
        }
        case TOOLTIP_SHOW: {
            tooltip_show();
            redraw();
            break;
        }
        case SELF_AVATAR_SET: {
            /* param1: size of data
             * data: png data
             */
            self_set_and_save_avatar(data, param1);
            free(data);
            redraw();
            break;
        }

        /* File transfer messages */
        case FILE_SEND_NEW: {
            FILE_TRANSFER *file_handle = data;
            FRIEND *f = &friend[file_handle->friend_number];

            friend_addmessage(f, file_handle->ui_data);
            file_notify(f, file_handle->ui_data);
            redraw();
            free(file_handle);
            break;
        }
        case FILE_INCOMING_NEW: {
            FILE_TRANSFER *file_handle = data;
            FRIEND *f = &friend[file_handle->friend_number];

            friend_addmessage(f, file_handle->ui_data);
            file_notify(f, file_handle->ui_data);
            redraw();
            free(file_handle);
            break;
        }
        case FILE_INCOMING_ACCEPT: {
            tox_postmessage(TOX_FILE_ACCEPT, param1, param2 << 16, data);
            break;
        }
        case FILE_UPDATE_STATUS:{
            FILE_TRANSFER *file = data;
            MSG_FILE *msg = file->ui_data;
            if(!msg){//TODO shove on ui thread
                free(file);
                return;
            }

            FRIEND *f = &friend[file->friend_number];

            _Bool f_notify = 0;
            if (msg->status != file->status) {
                f_notify = 1;
                msg->status = file->status;
            }
            msg->filenumber = file->file_number;
            msg->progress = file->size_transferred;
            msg->speed = file->speed;
            if(file->in_memory){
                msg->path = file->memory;
            } else {
                msg->path = file->path;
            }
            if (f_notify) {
                file_notify(f, msg);
            }

            redraw();
            free(file);
            break;
        }
        case FILE_INLINE_IMAGE: {
            FRIEND *f = &friend[param1];
            uint16_t width, height;
            uint8_t *image;
            memcpy(&width, data, sizeof(uint16_t));
            memcpy(&height, data + sizeof(uint16_t), sizeof(uint16_t));
            memcpy(&image, data + sizeof(uint16_t) * 2, sizeof(uint8_t *));
            free(data);
            friend_recvimage(f, (UTOX_NATIVE_IMAGE*)image, width, height);
            redraw();
            break;
        }

        /* Friend interaction messages. */
            /* Handshake */
        case FRIEND_ONLINE: {
            FRIEND *f = &friend[param1];

            if (f->online == param2) {
                break;
            }

            f->online = param2;
            if(!f->online) {
                friend_set_typing(f, 0);
            }
            redraw();
            break;
        }
        case FRIEND_NAME: {
            FRIEND *f = &friend[param1];
            friend_setname(f, data, param2);
            redraw();
            free(data);
            break;
        }
        case FRIEND_STATUS_MESSAGE: {
            FRIEND *f = &friend[param1];
            free(f->status_message);
            f->status_length = param2;
            f->status_message = data;
            redraw();
            break;
        }
        case FRIEND_STATE: {
            FRIEND *f = &friend[param1];
            f->status = param2;
            redraw();
            break;
        }
        case FRIEND_AVATAR_SET: {
            /* param1: friend id
               param2: png size
               data: png data
            */
            /* Work now done by file callback */
            uint8_t *avatar = data;
            size_t size = param2;

            FRIEND *f = &friend[param1];
            char_t cid[TOX_PUBLIC_KEY_SIZE * 2];
            cid_to_string(cid, (char_t*)f->cid);
            set_avatar(&f->avatar, avatar, size);
            save_avatar(cid, avatar, size);

            free(avatar);
            redraw();
            break;
        }
        case FRIEND_AVATAR_UNSET: {
            FRIEND *f = &friend[param1];
            unset_avatar(&f->avatar);
            // remove avatar from disk
            char_t cid[TOX_PUBLIC_KEY_SIZE * 2];
            cid_to_string(cid, f->cid);
            delete_saved_avatar(cid);

            redraw();
            break;
        }
        /* Interactions */
        case FRIEND_TYPING: {
            FRIEND *f = &friend[param1];
            friend_set_typing(f, param2);
            redraw();
            break;
        }
        case FRIEND_MESSAGE: {
            friend_addmessage(&friend[param1], data);
            redraw();
            break;
        }
        /* Adding and deleting */
        case FRIEND_INCOMING_REQUEST: {
            /* data: pointer to FRIENDREQ structure
             */
            list_addfriendreq(data);
            redraw();
            break;
        }
        case FRIEND_ACCEPT_REQUEST: {
            /* confirmation that friend has been added to friend list (accept) */
            if(!param1) {
                FRIEND *f = &friend[param2];
                FRIENDREQ *req = data;
                friends++;
                list_addfriend2(f, req);
                redraw();
            }

            free(data);
            break;
        }
        case FRIEND_SEND_REQUEST: {
            /* confirmation that friend has been added to friend list (add) */
            if(param1) {
                /* friend was not added */
                addfriend_status = param2;
            } else {
                /* friend was added */
                edit_add_id.length = 0;
                edit_add_msg.length = 0;

                FRIEND *f = &friend[param2];
                friends++;
                memcpy(f->cid, data, sizeof(f->cid));
                list_addfriend(f);

                addfriend_status = ADDF_SENT;
            }
            free(data);
            redraw();
            break;
        }
        case FRIEND_REMOVE: {
            FRIEND *f = data;
            // commented out incase you have multiple clients in the same data dir and remove one as friend from the other
            //   (it would remove his avatar locally too otherwise)
            //char_t cid[TOX_PUBLIC_KEY_SIZE * 2];
            //cid_to_string(cid, f->cid);
            //delete_saved_avatar(cid);
            friend_free(f);
            friends--;
            break;
        }

        case AV_CALL_INCOMING: {
            call_notify(&friend[param1], UTOX_AV_INVITE);
            redraw();
            break;
        }
        case AV_CALL_RINGING: {
            call_notify(&friend[param1], UTOX_AV_RINGING);
            redraw();
            break;
        }
        case AV_CALL_ACCEPTED: {
            call_notify(&friend[param1], UTOX_AV_STARTED);
            redraw();
            break;
        }
        case AV_CALL_DISCONNECTED: {
            call_notify(&friend[param1], UTOX_AV_NONE);
            redraw();
            break;
        }
        case AV_VIDEO_FRAME: {
            /* param1: video handle to send frame to (friend id or 0 for preview)
               param2: unused
               data: packaged frame data */

            utox_frame_pkg *frame = data;
            video_frame(param1, frame->img, frame->w, frame->h, 0); //TODO re-enable the resize option, disabled for reasons
            free(frame->img);
            free(data);
            redraw();
            break;
        }

        /* Commented out until new group chats...
        case GROUP_ADD: {
            GROUPCHAT *g = &group[param1];
            g->name_length = snprintf((char*)g->name, sizeof(g->name), "Groupchat #%u", param1);
            if (g->name_length >= sizeof(g->name)) {
                g->name_length = sizeof(g->name) - 1;
            }
            g->topic_length = sizeof("Drag friends to invite them") - 1;
            memcpy(g->topic, "Drag friends to invite them", sizeof("Drag friends to invite them") - 1);
            g->msg.scroll = 1.0;
            g->type = tox_group_get_type(data, param1);
            list_addgroup(g);
            redraw();
            break;
        }

        case GROUP_MESSAGE: {
            GROUPCHAT *g = &group[param1];

            message_add(&messages_group, data, &g->msg);

            if(selected_item && g == selected_item->data) {
                redraw();//ui_drawmain();
            }

            break;
        }

        case GROUP_PEER_DEL: {
            GROUPCHAT *g = &group[param1];

            if (param2 > MAX_GROUP_PEERS) //TODO: dynamic arrays.
                break;

            if(g->peername[param2]) {
                free(g->peername[param2]);
                g->peername[param2] = NULL;
            }

            g->peers--;
            g->peername[param2] = g->peername[g->peers];
            g->peername[g->peers] = NULL;

            if (g->type == TOX_GROUPCHAT_TYPE_AV) {
                g->last_recv_audio[param2] = g->last_recv_audio[g->peers];
                g->last_recv_audio[g->peers] = 0;
                // REMOVED UNTIL AFTER NEW GCs group_av_peer_remove(g, param2);
                g->source[param2] = g->source[g->peers];
            }

            if (g->peers == g->our_peer_number) {
                g->our_peer_number = param2;
            }

            g->topic_length = snprintf((char*)g->topic, sizeof(g->topic), "%u users in chat", g->peers);
            if (g->topic_length >= sizeof(g->topic)) {
                g->topic_length = sizeof(g->topic) - 1;
            }

            updategroup(g);

            break;
        }

        case GROUP_PEER_ADD:
        case GROUP_PEER_NAME: {
            GROUPCHAT *g = &group[param1];

            if (param2 > MAX_GROUP_PEERS) //TODO: dynamic arrays.
                break;

            if(g->peername[param2]) {
                free(g->peername[param2]);
            }

            if(tox_message_id == GROUP_PEER_ADD) {
                if (g->type == TOX_GROUPCHAT_TYPE_AV) {
                    // REMOVED UNTIL AFTER NEW GCs group_av_peer_add(g, param2);
                }

                if (tox_group_peernumber_is_ours(data, param1, param2)) {
                    g->our_peer_number = param2;
                }

                uint8_t *n = malloc(10);
                n[0] = 9;
                memcpy(n + 1, "<unknown>", 9);
                data = n;
                g->peers++;

            }

            g->peername[param2] = data;

            g->topic_length = snprintf((char*)g->topic, sizeof(g->topic), "%u users in chat", g->peers);
            if (g->topic_length >= sizeof(g->topic)) {
                g->topic_length = sizeof(g->topic) - 1;
            }

            updategroup(g);

            break;
        }

        case GROUP_TITLE: {
            GROUPCHAT *g = &group[param1];

            if (param2 > sizeof(g->name)) {
                memcpy(g->name, data, sizeof(g->name));
                g->name_length = sizeof(g->name);
            } else {
                memcpy(g->name, data, param2);
                g->name_length = param2;
            }

            free(data);
            updategroup(g);
            break;
        }

        case GROUP_AUDIO_START: {
            GROUPCHAT *g = &group[param1];

            if (g->type == TOX_GROUPCHAT_TYPE_AV) {
                g->audio_calling = 1;
                toxaudio_postmessage(GROUP_AUDIO_CALL_START, param1, 0, NULL);
                updategroup(g);
            }
            break;
        }
        case GROUP_AUDIO_END: {
            GROUPCHAT *g = &group[param1];

            if (g->type == TOX_GROUPCHAT_TYPE_AV) {
                g->audio_calling = 0;
                toxaudio_postmessage(GROUP_AUDIO_CALL_END, param1, 0, NULL);
                updategroup(g);
            }
            break;
        }

        case GROUP_UPDATE: {
            //GROUPCHAT *g = &group[param1];
            updategroup(g);

            break;
        }*/
    }
}
