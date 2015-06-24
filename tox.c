#include "main.h"
#include "tox_bootstrap.h"

struct Tox_Options options = {.proxy_host = proxy_address};

typedef struct {
    uint8_t msg;
    uint32_t param1, param2;
    void *data;
} TOX_MSG;

static TOX_MSG tox_msg, audio_msg, video_msg, toxav_msg;
static volatile _Bool tox_thread_msg, audio_thread_msg, video_thread_msg, toxav_thread_msg;
static volatile _Bool save_needed = 1;

/* Writes log filename for fid to dest. returns length written */
static int log_file_name(uint8_t *dest, size_t size_dest, Tox *tox, int fid)
{
    if (size_dest < TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".txt"))
        return -1;

    uint8_t client_id[TOX_PUBLIC_KEY_SIZE];
    tox_friend_get_public_key(tox, fid, client_id, 0);
    cid_to_string(dest, client_id); dest += TOX_PUBLIC_KEY_SIZE * 2;
    memcpy((char*)dest, ".txt", sizeof(".txt"));

    return TOX_PUBLIC_KEY_SIZE * 2 + sizeof(".txt");
}

/* Writes friend meta data filename for fid to dest. returns length written */
static int friend_meta_data_file(uint8_t *dest, size_t size_dest, Tox *tox, int fid)
{
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
    if(file) {
        time_t rawtime;
        time(&rawtime);

        if(author) {
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

void friend_meta_data_read(Tox *tox, int friend_id)
{
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
}

static void tox_thread_message(Tox *tox, ToxAv *av, uint64_t time, uint8_t msg, uint32_t param1, uint32_t param2, void *data);

void tox_postmessage(uint8_t msg, uint32_t param1, uint32_t param2, void *data)
{
    while(tox_thread_msg) {
        yieldcpu(1);
    }

    tox_msg.msg = msg;
    tox_msg.param1 = param1;
    tox_msg.param2 = param2;
    tox_msg.data = data;

    tox_thread_msg = 1;
}

void toxvideo_postmessage(uint8_t msg, uint32_t param1, uint32_t param2, void *data)
{
    while(video_thread_msg) {
        yieldcpu(1);
    }

    video_msg.msg = msg;
    video_msg.param1 = param1;
    video_msg.param2 = param2;
    video_msg.data = data;

    video_thread_msg = 1;
}

void toxav_postmessage(uint8_t msg, uint32_t param1, uint32_t param2, void *data)
{
    while(toxav_thread_msg) {
        yieldcpu(1);
    }

    toxav_msg.msg = msg;
    toxav_msg.param1 = param1;
    toxav_msg.param2 = param2;
    toxav_msg.data = data;

    toxav_thread_msg = 1;
}

#include "tox_callbacks.h"
#include "tox_av.h"

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

/* tries to load avatar from disk for given client id string and set avatar based on saved png data
 *  avatar is avatar to initialize. Will be unset if no file is found on disk or if file is corrupt or too large,
 *      otherwise will be set to avatar found on disk
 *  id is cid string of whose avatar to find(see also load_avatar in avatar.h)
 *  if png_data_out is not NULL, the png data loaded from disk will be copied to it.
 *      if it is not null, it should be at least UTOX_AVATAR_MAX_DATA_LENGTH bytes long
 *  if png_size_out is not null, the size of the png data will be stored in it
 *
 *  returns: 1 on successful loading, 0 on failure
 */
static _Bool init_avatar(AVATAR *avatar, const char_t *id, uint8_t *png_data_out, uint32_t *png_size_out) {
    unset_avatar(avatar);
    uint8_t avatar_data[UTOX_AVATAR_MAX_DATA_LENGTH];
    uint32_t size;
    if (load_avatar(id, avatar_data, &size)) {
        if (set_avatar(avatar, avatar_data, size)) {
            if (png_data_out) {
                memcpy(png_data_out, avatar_data, size);
            }
            if (png_size_out) {
                *png_size_out = size;
            }

            return 1;
        }
    }
    return 0;
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
        int size;
        FRIEND *f = &friend[i];
        uint8_t name[TOX_MAX_NAME_LENGTH];

        f->msg.scroll = 1.0;

        tox_friend_get_public_key(tox, i, f->cid, 0);

        size = tox_friend_get_name_size(tox, i, 0);
        tox_friend_get_name(tox, i, name, 0);

        friend_setname(f, name, size);

        size = tox_friend_get_status_message_size(tox, i, 0);
        f->status_message = malloc(size);
        tox_friend_get_status_message(tox, i, f->status_message, 0);
        f->status_length = size;

        char_t cid[TOX_PUBLIC_KEY_SIZE * 2];
        cid_to_string(cid, f->cid);
        init_avatar(&f->avatar, cid, NULL, NULL);

        log_read(tox, i);

        friend_meta_data_read(tox, i);
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

static void write_save(Tox *tox)
{
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


    debug("Writing tox_save to: %s\n", (char*)path_tmp);
    file = fopen((char*)path_tmp, "wb");
    if(file) {
        fwrite(data, size, 1, file);
        flush_file(file);
        fclose(file);
        if (rename((char*)path_tmp, (char*)path_real) != 0) {
            debug("Failed to rename file. %s to %s deleting and trying again\n", path_tmp, path_real);
            remove((const char *)path_real);
            if (rename((char*)path_tmp, (char*)path_real) != 0) {
                debug("Saving Failed\n");
            } else {
                debug("Saved data\n");
            }
        } else {
            debug("Saved data\n");
            int ch = ch_mod(path_real);
            if(!ch){
                debug("CHMOD: success\n");
            } else {
                debug("CHMOD: failure\n");
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

static void utox_thread_work_for_typing_notifications(Tox *tox, uint64_t time)
{
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
void tox_thread(void *UNUSED(args))
{
    Tox *tox;
    ToxAv *av;
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
        av = toxav_new(tox, MAX_CALLS);

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

                debug("Connected to DHT: %u\n", connected);
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

static void tox_thread_message(Tox *tox, ToxAv *av, uint64_t time, uint8_t msg, uint32_t param1, uint32_t param2, void *data)
{
    switch(msg) {
    case TOX_SETNAME: {
        /* param1: name length
         * data: name
         */
        tox_self_set_name(tox, data, param1, 0);
        break;
    }

    case TOX_SETAVATAR: {
        /*
         * param1: avatar format
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
        break;
    }

    case TOX_UNSETAVATAR: {
        free(self.avatar_data);
        self.avatar_data = NULL;
        self.avatar_size = 0;
        self.avatar_format = 0;
        utox_avatar_update_friends(tox);
        break;
    }

    case TOX_SETSTATUSMSG: {
        /* param1: status length
         * data: status message
         */
        tox_self_set_status_message(tox, data, param1, 0);
        break;
    }

    case TOX_SETSTATUS: {
        /* param1: status
         */
        tox_self_set_status(tox, param1);
        break;
    }

    case TOX_ADDFRIEND: {
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
            postmessage(FRIEND_ADD, 1, addf_error, data);
        } else {
            postmessage(FRIEND_ADD, 0, fid, data);
        }
        break;
    }

    case TOX_DELFRIEND: {
        /* param1: friend #
         */
        tox_friend_delete(tox, param1, 0);
        postmessage(FRIEND_DEL, 0, 0, data);
        break;
    }

    case TOX_ACCEPTFRIEND: {
        /* data: FRIENDREQ
         */
        FRIENDREQ *req = data;
        TOX_ERR_FRIEND_ADD f_err;
        uint32_t fid = tox_friend_add_norequest(tox, req->id, &f_err);
        postmessage(FRIEND_ACCEPT, (f_err != TOX_ERR_FRIEND_ADD_OK), (f_err != TOX_ERR_FRIEND_ADD_OK) ? 0 : fid, req);
        break;
    }

    case TOX_SENDMESSAGE:
    case TOX_SENDACTION: {
        /* param1: friend #
         * param2: message length
         * data: message
         */

        void *p = data;
        TOX_MESSAGE_TYPE type;
        if(msg == TOX_SENDACTION){
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

    case TOX_SENDMESSAGEGROUP: {
        /* param1: group #
         * param2: message length
         * data: message
         */
        tox_group_message_send(tox, param1, data, param2);
        free(data);
        break;
    }

    case TOX_SENDACTIONGROUP: {
        /* param1: group #
         * param2: message length
         * data: message
         */
        tox_group_action_send(tox, param1, data, param2);
        free(data);
        break;
    }

    case TOX_SET_TYPING: {
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

    case TOX_CALL: {
        /* param1: friend #
         */
        int32_t id;
        toxav_call(av, &id, param1, &av_DefaultSettings, 10);

        postmessage(FRIEND_CALL_STATUS, param1, id, (void*)CALL_RINGING);
        break;
    }

    case TOX_CALL_VIDEO: {
        /* param1: friend #
         */
        ToxAvCSettings settings = av_DefaultSettings;
        settings.call_type = av_TypeVideo;
        settings.max_video_width = max_video_width;
        settings.max_video_height = max_video_height;

        int32_t id;
        toxav_call(av, &id, param1, &settings, 10);

        postmessage(FRIEND_CALL_STATUS, param1, id, (void*)CALL_RINGING_VIDEO);
        break;
    }

    case TOX_CALL_VIDEO_ON: {
        /* param1: friend #
         * param2: call #
         */
        ToxAvCSettings settings = av_DefaultSettings;
        settings.call_type = av_TypeVideo;
        settings.max_video_width = max_video_width;
        settings.max_video_height = max_video_height;

        toxav_change_settings(av, param2, &settings);
        postmessage(FRIEND_CALL_START_VIDEO, param1, param2, NULL);
        break;
    }

    case TOX_CALL_VIDEO_OFF: {
        /* param1: friend #
         * param2: call #
         */
        toxav_change_settings(av, param2, &av_DefaultSettings);
        postmessage(FRIEND_CALL_STOP_VIDEO, param1, param2, NULL);
        break;
    }

    case TOX_ACCEPTCALL: {
        /* param1: call #
         */
        ToxAvCSettings settings = av_DefaultSettings;
        if(param2) {
            settings.call_type = av_TypeVideo;
            settings.max_video_width = max_video_width;
            settings.max_video_height = max_video_height;
        }

        toxav_answer(av, param1, &settings);
        break;
    }

    case TOX_HANGUP: {
        /* param1: call #
         */
        toxav_hangup(av, param1);
        break;
    }

    case TOX_CANCELCALL: {
        /* param1: call #
         * param2: friend #
         */
        toxav_cancel(av, param1, param2, "Call canceled by friend");
        postmessage(FRIEND_CALL_STATUS, param2, param1, (void*)(size_t)CALL_NONE);
        break;
    }

    case TOX_NEWGROUP: {
        /*
         */
        int g = -1;
        if (param1) {
            g = toxav_add_av_groupchat(tox, &callback_av_group_audio, NULL);
        } else {
            g = tox_add_groupchat(tox);
        }
        if(g != -1) {
            postmessage(GROUP_ADD, g, 0, tox);
        }

        break;
    }

    case TOX_LEAVEGROUP: {
        /* param1: group #
         */
        tox_del_groupchat(tox, param1);
        break;
    }

    case TOX_GROUPINVITE: {
        /* param1: group #
         * param2: friend #
         */
        tox_invite_friend(tox, param2, param1);
        break;
    }

    case TOX_GROUPCHANGETOPIC: {
        /* param1: group #
         * param2: topic length
         * data: topic
         */
        tox_group_set_title(tox, param1, data, param2);
        postmessage(GROUP_TITLE, param1, param2, data);
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

    case TOX_SEND_NEW_FILE: {
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

        free(data);

        break;
    }

    case TOX_SEND_NEW_INLINE: {
        /* param1: friend id
           data: pointer to a TOX_SEND_INLINE_MSG struct
         */
        struct TOX_SEND_INLINE_MSG *tsim = data;
        outgoing_file_send(tox, param1, NULL, tsim->image->png_data, tsim->image_size, TOX_FILE_KIND_DATA);
        free(tsim);

        break;
    }

    case TOX_FILE_START_TEMP:{
        /* param1: friend #
         * param2: file # */
        break;
        /* This call currently does nothing, eventually we'd like to start downloading a file before we have its dir */
    }


    case TOX_ACCEPTFILE: {
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

    case TOX_FILE_INCOMING_RESUME:
    case TOX_FILE_OUTGOING_RESUME:{
        /* param1: friend #
         * param2: file #           tox, friend#, file#,       RESUME_FILE */
        file_transfer_local_control(tox, param1, param2, TOX_FILE_CONTROL_RESUME);
        break;
    }
    case TOX_FILE_INCOMING_PAUSE:
    case TOX_FILE_OUTGOING_PAUSE:{
        /* param1: friend #
         * param2: file #           tox, friend#, file#,        PAUSE_FILE */
        file_transfer_local_control(tox, param1, param2, TOX_FILE_CONTROL_PAUSE);
        break;
    }
    case TOX_FILE_INCOMING_CANCEL:
    case TOX_FILE_OUTGOING_CANCEL:{
        /* param1: friend #
         * param2: file #           tox, friend#, file#,        CANCEL_FILE */
        file_transfer_local_control(tox, param1, param2, TOX_FILE_CONTROL_CANCEL);
        break;
    }

    case TOX_FRIEND_ONLINE: {
        if(!param2) {
            ft_friend_offline(tox, param1);
        } else {
            ft_friend_online(tox, param1);
            /* resend avatar info (in case it changed) */
            /* Avatars must be sent LAST or they will clobber existing file transfers! */
            avatar_on_friend_online(tox, param1);
        }
        break;
    }

    }
    save_needed = 1;
}

/** Translates status code to text then sends back to the user */
static void file_notify(FRIEND *f, MSG_FILE *msg)
{
    STRING *str;

    switch(msg->status) {
    case FILE_TRANSFER_STATUS_NONE:
        str = SPTR(TRANSFER_NEW); break;
    case FILE_TRANSFER_STATUS_ACTIVE:
        str = SPTR(TRANSFER_STARTED); break;
    case FILE_TRANSFER_STATUS_PAUSED_BOTH:
        str = SPTR(TRANSFER___); break;
    case FILE_TRANSFER_STATUS_PAUSED_US:
    case FILE_TRANSFER_STATUS_PAUSED_THEM:
        str = SPTR(TRANSFER_PAUSED); break;
    case FILE_TRANSFER_STATUS_KILLED:
        str = SPTR(TRANSFER_CANCELLED); break;
    case FILE_TRANSFER_STATUS_COMPLETED:
        str = SPTR(TRANSFER_COMPLETE); break;
    case FILE_TRANSFER_STATUS_BROKEN:
    default: //render unknown status as "transfer broken"
        str = SPTR(TRANSFER_BROKEN); break;
    }

    friend_notify(f, str->str, str->length, msg->name, msg->name_length);
}

static void call_notify(FRIEND *f, uint8_t status)
{
    STRING *str;

    switch(status) {
    case CALL_INVITED:
    case CALL_INVITED_VIDEO:
        str = SPTR(CALL_INVITED);
        break;
    case CALL_RINGING:
    case CALL_RINGING_VIDEO:
        str = SPTR(CALL_RINGING); break;
    case CALL_OK:
    case CALL_OK_VIDEO:
        str = SPTR(CALL_STARTED);
        break;
    case CALL_NONE:
    default: //render unknown status as "call cancelled"
        str = SPTR(CALL_CANCELLED); break;
    }

    friend_notify(f, str->str, str->length, (uint8_t*)"", 0);
    friend_addmessage_notify(f, str->str, str->length);
}

void tox_message(uint8_t tox_message_id, uint16_t param1, uint16_t param2, void *data)
{
    switch(tox_message_id) {
    case DHT_CONNECTED: {
        /* param1: connection status (1 = connected, 0 = disconnected)
         */
        tox_connected = param1;
        redraw();
        break;
    }

    case DNS_RESULT: {
        /* param1: result (0 = failure, 1 = success)
         * data: resolved tox id (if successful)
         */
        if(param1) {
            friend_addid(data, edit_add_msg.data, edit_add_msg.length);
        } else {
            addfriend_status = ADDF_BADNAME;
        }
        free(data);

        redraw();
        break;
    }

    case SET_AVATAR: {
        /* param1: size of data
         * data: png data
         */
        self_set_and_save_avatar(data, param1);
        free(data);
        redraw();
        break;
    }

    case SEND_FILES: {
        tox_postmessage(TOX_SEND_NEW_FILE, param1, param2, data);
        break;
    }

    case SAVE_FILE: {
        tox_postmessage(TOX_ACCEPTFILE, param1, param2 << 16, data);
        break;
    }

    case FILE_START_TEMP:{
        // Called once the user starts looking for a save location
            // start ft to a temp directory
            // start writing data
        tox_postmessage(TOX_FILE_START_TEMP, param1, param2 << 16, data);
        break;
    }

    case FILE_ABORT_TEMP:{
        // User canceled file saving.
            // stop/pause ft
            // kill the file
            // delete existing data
        // We do want to do something else, but for now this is goodenough™
        tox_postmessage(TOX_FILE_INCOMING_PAUSE, param1, param2 << 16, data);
        break;
    }

    case NEW_AUDIO_IN_DEVICE: {
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

    case NEW_AUDIO_OUT_DEVICE: {
        list_dropdown_add_hardcoded(&dropdown_audio_out, data, data);

        if (loaded_audio_out_device != 0 && (dropdown_audio_out.dropcount - 1) == loaded_audio_out_device) {
            toxaudio_postmessage(AUDIO_SET_OUTPUT, 0, 0, data);
            dropdown_audio_out.selected = loaded_audio_out_device;
            loaded_audio_out_device = 0;
        }

        break;
    }

    case NEW_VIDEO_DEVICE: {
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

    case FRIEND_REQUEST: {
        /* data: pointer to FRIENDREQ structure
         */
        list_addfriendreq(data);
        redraw();
        break;
    }

    case FRIEND_ADD: {
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

            f->msg.scroll = 1.0;

            memcpy(f->cid, data, sizeof(f->cid));

            friend_setname(f, NULL, 0);

            list_addfriend(f);

            addfriend_status = ADDF_SENT;

        }

        free(data);
        redraw();
        break;
    }

    case FRIEND_ACCEPT: {
        /* confirmation that friend has been added to friend list (accept) */
        if(!param1) {
            FRIEND *f = &friend[param2];
            FRIENDREQ *req = data;
            friends++;

            memcpy(f->cid, req->id, sizeof(f->cid));
            friend_setname(f, NULL, 0);

            list_addfriend2(f, req);
            redraw();
        }

        free(data);
        break;
    }

    case FRIEND_DEL: {
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

    case FRIEND_MESSAGE: {
        friend_addmessage(&friend[param1], data);
        redraw();
        break;
    }

    #define updatefriend(fp) redraw();
    #define updategroup(gp) redraw();

    case FRIEND_NAME: {
        FRIEND *f = &friend[param1];
        friend_setname(f, data, param2);
        updatefriend(f);
        free(data);
        break;
    }

    case FRIEND_SETAVATAR: {
        /* param1: friend id
           param2: png size
           data: png data
        Work now done by file callback
        */
        uint8_t *avatar = data;
        size_t size = param2;

        FRIEND *f = &friend[param1];
        char_t cid[TOX_PUBLIC_KEY_SIZE * 2];
        cid_to_string(cid, (char_t*)f->cid);
        set_avatar(&f->avatar, avatar, size);
        save_avatar(cid, avatar, size);

        free(avatar);
        updatefriend(f);
        break;
    }

    case FRIEND_UNSETAVATAR: {
        FRIEND *f = &friend[param1];
        unset_avatar(&f->avatar);

        // remove avatar from disk
        char_t cid[TOX_PUBLIC_KEY_SIZE * 2];
        cid_to_string(cid, f->cid);
        delete_saved_avatar(cid);

        updatefriend(f);
        break;
    }

    case FRIEND_STATUS_MESSAGE: {
        FRIEND *f = &friend[param1];
        free(f->status_message);
        f->status_length = param2;
        f->status_message = data;
        updatefriend(f);
        break;
    }

    case FRIEND_STATUS: {
        FRIEND *f = &friend[param1];
        f->status = param2;
        updatefriend(f);
        break;
    }

    case FRIEND_TYPING: {
        FRIEND *f = &friend[param1];
        friend_set_typing(f, param2);
        updatefriend(f);
        break;
    }

    case FRIEND_ONLINE: {
        FRIEND *f = &friend[param1];

        if (f->online == param2) {
            break;
        }

        f->online = param2;
        if(!f->online) {
            friend_set_typing(f, 0);
        }
        updatefriend(f);
        tox_postmessage(TOX_FRIEND_ONLINE, param1, param2, data);
        break;
    }

    case FRIEND_CALL_STATUS: {
        /* param1: friend id
           param2: call id
           data: integer call status
         */
        FRIEND *f = &friend[param1];
        uint8_t status = (size_t)data;
        if(status == CALL_NONE && (f->calling == CALL_OK || f->calling == CALL_OK_VIDEO)) {
            toxaudio_postmessage(AUDIO_CALL_END, param2, 0, NULL);
            if(f->calling == CALL_OK_VIDEO) {
                toxvideo_postmessage(VIDEO_CALL_END, param2, 0, NULL);
            }

            video_end(param1 + 1);
        }

        f->calling = status;
        f->callid = param2;

        if(status == CALL_OK) {
            toxaudio_postmessage(AUDIO_CALL_START, param2, 0, NULL);
        }

        call_notify(f, status);

        updatefriend(f);
        break;
    }

    case FRIEND_CALL_VIDEO: {
        /* param1: friend id
           param2: call id
         */
        FRIEND *f = &friend[param1];
        f->calling = CALL_OK_VIDEO;
        f->callid = param2;
        updatefriend(f);

        toxvideo_postmessage(VIDEO_CALL_START, param2, 0, NULL);
        toxaudio_postmessage(AUDIO_CALL_START, param2, 0, NULL);

        f->call_width = 640;
        f->call_height = 480;

        video_begin(param1 + 1, f->name, f->name_length, 640, 480);

        call_notify(f, CALL_OK_VIDEO);

        break;
    }

    case FRIEND_CALL_MEDIACHANGE: {
        /* param1: friend id
           param2: call id
           data: zero = audio, nonzero = audio/video
         */
        FRIEND *f = &friend[param1];

        if(!data) {
            video_end(param1 + 1);
        } else {
            f->call_width = 640;
            f->call_height = 480;

            video_begin(param1 + 1, f->name, f->name_length, 640, 480);
        }
        break;
    }

    case FRIEND_CALL_START_VIDEO: {
        /* param1: friend id
           param2: call id
         */
        FRIEND *f = &friend[param1];
        if(f->calling == CALL_OK) {
            f->calling = CALL_OK_VIDEO;
            toxvideo_postmessage(VIDEO_CALL_START, param2, 0, NULL);
            updatefriend(f);
        }
        break;
    }

    case FRIEND_CALL_STOP_VIDEO: {
        /* param1: friend id
           param2: call id
         */
        FRIEND *f = &friend[param1];
        if(f->calling == CALL_OK_VIDEO) {
            f->calling = CALL_OK;
            toxvideo_postmessage(VIDEO_CALL_END, param2, 0, NULL);
            updatefriend(f);
        }
        break;
    }

    case FRIEND_VIDEO_FRAME: {
        /* param1: friend id
           param2: call id
           data: frame data
         */
        uint16_t *image = data;
        FRIEND *f = &friend[param1];

        _Bool b = (image[0] != f->call_width || image[1] != f->call_height);
        if(b) {
            f->call_width = image[0];
            f->call_height = image[1];
        }
        video_frame(param1 + 1, (void*)&image[2], image[0], image[1], b);
        free(image);
        break;
    }

    case PREVIEW_FRAME_NEW:
    case PREVIEW_FRAME: {
        if(video_preview) {
            video_frame(0, data, param1, param2, tox_message_id == PREVIEW_FRAME_NEW);
        }
        free(data);
        break;
    }

    case FRIEND_FILE_NEW: {
        FILE_TRANSFER *file_handle = data;
        FRIEND *f = &friend[file_handle->friend_number];

        friend_addmessage(f, file_handle->ui_data);
        file_notify(f, file_handle->ui_data);
        updatefriend(f);
        free(file_handle);
        break;
    }

    case FRIEND_FILE_UPDATE:{
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

        updatefriend(f);
        free(file);
        break;
    }

    case FRIEND_INLINE_IMAGE: {
        FRIEND *f = &friend[param1];
        uint16_t width, height;
        uint8_t *image;
        memcpy(&width, data, sizeof(uint16_t));
        memcpy(&height, data + sizeof(uint16_t), sizeof(uint16_t));
        memcpy(&image, data + sizeof(uint16_t) * 2, sizeof(uint8_t *));
        free(data);
        friend_recvimage(f, image, width, height);
        redraw();
        break;
    }

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
            group_av_peer_remove(g, param2);
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
                group_av_peer_add(g, param2);
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
    }

    case TOOLTIP_SHOW: {
        tooltip_show();
        redraw();
        break;
    }
    }
}
