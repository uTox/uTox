#include "main.h"
#include "tox_bootstrap.h"

typedef struct {
    uint8_t msg;
    uint16_t param1, param2;
    void *data;
} TOX_MSG;

static TOX_MSG tox_msg, audio_msg, video_msg;
static volatile _Bool tox_thread_msg, audio_thread_msg, video_thread_msg;

/* Writes log filename for fid to dest. returns length written */
static int log_file_name(uint8_t *dest, size_t size_dest, Tox *tox, int fid)
{
    if (size_dest < TOX_CLIENT_ID_SIZE * 2 + sizeof(".txt"))
        return -1;

    uint8_t client_id[TOX_CLIENT_ID_SIZE];
    tox_get_client_id(tox, fid, client_id);
    cid_to_string(dest, client_id); dest += TOX_CLIENT_ID_SIZE * 2;
    memcpy((char*)dest, ".txt", sizeof(".txt"));

    return TOX_CLIENT_ID_SIZE * 2 + sizeof(".txt");
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

    uint8_t path[512], *p;
    uint8_t name[TOX_MAX_NAME_LENGTH];
    int namelen;
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
            namelen = tox_get_self_name(tox, name);
        } else if((namelen = tox_get_name(tox, fid, name)) == -1) {
            //error reading name
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
    uint8_t path[512], *p;
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

        struct tm *ti;
        time_t rawtime = header.time;
        ti = localtime(&rawtime);

        msg->time = ti->tm_hour * 60 + ti->tm_min;

        m->data[m->n++] = msg;

        debug("loaded backlog: %d: %.*s\n", fid, msg->length, msg->msg);
    }

    fclose(file);
}

static void tox_thread_message(Tox *tox, ToxAv *av, uint64_t time, uint8_t msg, uint16_t param1, uint16_t param2, void *data);

void tox_postmessage(uint8_t msg, uint16_t param1, uint16_t param2, void *data)
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

void toxvideo_postmessage(uint8_t msg, uint16_t param1, uint16_t param2, void *data)
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
        tox_bootstrap_from_address(tox, d->address, d->port, d->key);
        i++;
        j++;
    }
}

static void set_callbacks(Tox *tox)
{
    tox_callback_friend_request(tox, callback_friend_request, NULL);
    tox_callback_friend_message(tox, callback_friend_message, NULL);
    tox_callback_friend_action(tox, callback_friend_action, NULL);
    tox_callback_name_change(tox, callback_name_change, NULL);
    tox_callback_status_message(tox, callback_status_message, NULL);
    tox_callback_user_status(tox, callback_user_status, NULL);
    tox_callback_typing_change(tox, callback_typing_change, NULL);
    tox_callback_read_receipt(tox, callback_read_receipt, NULL);
    tox_callback_connection_status(tox, callback_connection_status, NULL);

    tox_callback_group_invite(tox, callback_group_invite, NULL);
    tox_callback_group_message(tox, callback_group_message, NULL);
    tox_callback_group_action(tox, callback_group_action, NULL);
    tox_callback_group_namelist_change(tox, callback_group_namelist_change, NULL);

    utox_set_callbacks_for_transfer(tox);
}

static _Bool load_save(Tox *tox)
{
    {
        uint8_t path[512], *p;
        uint32_t size;

        p = path + datapath(path);
        strcpy((char*)p, "tox_save");

        void *data = file_raw((char*)path, &size);
        if(!data) {
            p = path + datapath_old(path);
            strcpy((char*)p, "tox_save");
            data = file_raw((char*)path, &size);
            if (!data) {
                data = file_raw("tox_save", &size);
                if(!data) {
                    return 0;
                }
            }
        }

        tox_load(tox, data, size);
        free(data);
    }

    friends = tox_count_friendlist(tox);

    uint32_t i = 0;
    while(i != friends) {
        int size;
        FRIEND *f = &friend[i];
        uint8_t name[TOX_MAX_NAME_LENGTH];

        f->msg.scroll = 1.0;

        tox_get_client_id(tox, i, f->cid);

        size = tox_get_name(tox, i, name);

        friend_setname(f, name, size);

        size = tox_get_status_message_size(tox, i);
        f->status_message = malloc(size);
        tox_get_status_message(tox, i, f->status_message, size);
        f->status_length = size;

        log_read(tox, i);

        i++;
    }

    self.name_length = tox_get_self_name(tox, self.name);
    self.statusmsg_length = tox_get_self_status_message_size(tox);
    self.statusmsg = malloc(self.statusmsg_length);
    tox_get_self_status_message(tox, self.statusmsg, self.statusmsg_length);
    self.status = tox_get_self_user_status(tox);


    return 1;
}

static void load_defaults(Tox *tox)
{
    uint8_t *name = (uint8_t*)DEFAULT_NAME, *status = (uint8_t*)DEFAULT_STATUS;
    uint16_t name_len = sizeof(DEFAULT_NAME) - 1, status_len = sizeof(DEFAULT_STATUS) - 1;

    tox_set_name(tox, name, name_len);
    tox_set_status_message(tox, status, status_len);

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
    uint8_t path_tmp[512], path_real[512], *p;
    FILE *file;

    size = tox_size(tox);
    data = malloc(size);
    tox_save(tox, data);

    p = path_real + datapath(path_real);
    memcpy(p, "tox_save", sizeof("tox_save"));

    unsigned int path_len = (p - path_real) + sizeof("tox_save");
    memcpy(path_tmp, path_real, path_len);
    memcpy(path_tmp + (path_len - 1), ".tmp", sizeof(".tmp"));

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
        }
    }

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
        if(!tox_set_user_is_typing(tox, typing_state.friendnumber, is_typing)){
            // Successfully sent. Mark new state.
            typing_state.sent_value = is_typing;
            debug("Sent typing state to friend (%d): %d\n", typing_state.friendnumber, typing_state.sent_value);
        }
    }
}

void tox_thread(void *UNUSED(args))
{
    Tox *tox;
    ToxAv *av;
    uint8_t id[TOX_FRIEND_ADDRESS_SIZE];

TOP:;
    debug("new tox object ipv6: %u no_udp: %u proxy: %u %s %u\n", options.ipv6enabled, options.udp_disabled, options.proxy_enabled, options.proxy_address, options.proxy_port);
    if((tox = tox_new(&options)) == NULL) {
        debug("trying without proxy\n");
        if(!options.proxy_enabled || (options.proxy_enabled = 0, (tox = tox_new(&options)) == NULL)) {
            debug("trying without ipv6\n");
            if(!options.ipv6enabled || (options.ipv6enabled = 0, (tox = tox_new(&options)) == NULL)) {
                debug("tox_new() failed\n");
                exit(1);
            }
            dropdown_ipv6.selected = dropdown_ipv6.over = 1;
        }
        dropdown_proxy.selected = dropdown_proxy.over = 0;
    }

    if(!load_save(tox)) {
        debug("No save file, using defaults\n");
        load_defaults(tox);
    }

    edit_setstr(&edit_name, self.name, self.name_length);
    edit_setstr(&edit_status, self.statusmsg, self.statusmsg_length);

    tox_get_address(tox, id);
    id_to_string(self.id, id);

    debug("Tox ID: %.*s\n", (int)sizeof(self.id), self.id);

    set_callbacks(tox);

    do_bootstrap(tox);

    av = toxav_new(tox, MAX_CALLS);

    set_av_callbacks(av);


    global_av = av;
    tox_thread_init = 1;

    thread(audio_thread, av);
    thread(video_thread, av);

    _Bool connected = 0, reconfig;
    uint64_t last_save = get_time(), time;
    while(1) {
        tox_do(tox);

        if(tox_isconnected(tox) != connected) {
            connected = !connected;
            postmessage(DHT_CONNECTED, connected, 0, NULL);

            debug("Connected to DHT: %u\n", connected);
        }

        time = get_time();

        if(time - last_save >= (uint64_t)10 * 1000 * 1000 * 1000) {
            last_save = time;

            if(!connected) {
                do_bootstrap(tox);
            }

            write_save(tox);
        }

        if(tox_thread_msg) {
            TOX_MSG *msg = &tox_msg;
            if(!msg->msg) {
                reconfig = msg->param1;
                tox_thread_msg = 0;
                break;
            }
            tox_thread_message(tox, av, time, msg->msg, msg->param1, msg->param2, msg->data);
            tox_thread_msg = 0;
        }

        utox_thread_work_for_transfers(tox, time);
        utox_thread_work_for_typing_notifications(tox, time);

        uint32_t interval = tox_do_interval(tox);
        yieldcpu((interval > 20) ? 20 : interval);
    }

    write_save(tox);

    while(audio_thread_init || video_thread_init) {
        yieldcpu(1);
    }

    debug("av_thread exit, tox thread ending\n");

    toxav_kill(av);
    tox_kill(tox);

    if(reconfig) {
        goto TOP;
    }

    tox_thread_init = 0;
}

static void tox_thread_message(Tox *tox, ToxAv *av, uint64_t time, uint8_t msg, uint16_t param1, uint16_t param2, void *data)
{
    switch(msg) {
    case TOX_SETNAME: {
        /* param1: name length
         * data: name
         */
        tox_set_name(tox, data, param1);
        break;
    }

    case TOX_SETSTATUSMSG: {
        /* param1: status length
         * data: status message
         */
        tox_set_status_message(tox, data, param1);
        break;
    }

    case TOX_SETSTATUS: {
        /* param1: status
         */
        tox_set_user_status(tox, param1);
        break;
    }

    case TOX_ADDFRIEND: {
        /* param1: length of message
         * data: friend id + message
         */
        int r;

        if(!param1) {
            STRING* default_add_msg = SPTR(DEFAULT_FRIEND_REQUEST_MESSAGE);
            r = tox_add_friend(tox, data, default_add_msg->str, default_add_msg->length);
        } else {
            r = tox_add_friend(tox, data, data + TOX_FRIEND_ADDRESS_SIZE, param1);
        }

        if(r < 0) {
            uint8_t addf_error;
            switch(r) {
            case TOX_FAERR_TOOLONG:
                addf_error = ADDF_TOOLONG; break;
            case TOX_FAERR_NOMESSAGE:
                addf_error = ADDF_NOMESSAGE; break;
            case TOX_FAERR_OWNKEY:
                addf_error = ADDF_OWNKEY; break;
            case TOX_FAERR_ALREADYSENT:
                addf_error = ADDF_ALREADYSENT; break;
            case TOX_FAERR_BADCHECKSUM:
                addf_error = ADDF_BADCHECKSUM; break;
            case TOX_FAERR_SETNEWNOSPAM:
                addf_error = ADDF_SETNEWNOSPAM; break;
            case TOX_FAERR_NOMEM:
                addf_error = ADDF_NOMEM; break;
            case TOX_FAERR_UNKNOWN:
            default:
                addf_error = ADDF_UNKNOWN; break;
            }
            postmessage(FRIEND_ADD, 1, addf_error, data);
        } else {
            postmessage(FRIEND_ADD, 0, r, data);
        }
        break;
    }

    case TOX_DELFRIEND: {
        /* param1: friend #
         */
        tox_del_friend(tox, param1);
        postmessage(FRIEND_DEL, 0, 0, data);
        break;
    }

    case TOX_ACCEPTFRIEND: {
        /* data: FRIENDREQ
         */
        FRIENDREQ *req = data;
        int r = tox_add_friend_norequest(tox, req->id);
        postmessage(FRIEND_ACCEPT, (r < 0), (r < 0) ? 0 : r, req);
        break;
    }

    case TOX_SENDMESSAGE: {
        /* param1: friend #
         * param2: message length
         * data: message
         */
        log_write(tox, param1, data, param2, 1, LOG_FILE_MSG_TYPE_TEXT);

        void *p = data;
        while(param2 > TOX_MAX_MESSAGE_LENGTH) {
            uint16_t len = TOX_MAX_MESSAGE_LENGTH - utf8_unlen(p + TOX_MAX_MESSAGE_LENGTH);
            tox_send_message(tox, param1, p, len);
            param2 -= len;
            p += len;
        }

        tox_send_message(tox, param1, p, param2);
        free(data);
        break;
    }

    case TOX_SENDACTION: {
        /* param1: friend #
         * param2: message length
         * data: message
         */

        log_write(tox, param1, data, param2, 1, LOG_FILE_MSG_TYPE_ACTION);

        void *p = data;
        while(param2 > TOX_MAX_MESSAGE_LENGTH) {
            uint16_t len = TOX_MAX_MESSAGE_LENGTH - utf8_unlen(p + TOX_MAX_MESSAGE_LENGTH);
            tox_send_action(tox, param1, p, len);
            param2 -= len;
            p += len;
        }

        tox_send_action(tox, param1, p, param2);
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
            tox_set_user_is_typing(tox, typing_state.friendnumber, 0);
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
        settings.call_type = TypeVideo;
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
        settings.call_type = TypeVideo;
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
            settings.call_type = TypeVideo;
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

    case TOX_NEWGROUP: {
        /*
         */
        int g = tox_add_groupchat(tox);
        if(g != -1) {
            postmessage(GROUP_ADD, g, 0, NULL);
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

    case TOX_SENDFILES: {
        /* param1: friend #
         * param2: offset of first file name in data
         * data: file names
         */

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
                }

                utox_transfer_start_file(tox, param1, name, s, p - s);
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
                utox_transfer_start_file(tox, param1, data, data + param2, strlen(data) - param2);
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
                    utox_transfer_start_file(tox, param1, data, name, len - 1);
                }
            }
        }

        free(data);

        break;
    }

    case TOX_SEND_INLINE: {
        /* param1: friend id
           data: pointer to a TOX_SEND_INLINE_MSG struct
         */
        struct TOX_SEND_INLINE_MSG *tsim = data;
        utox_transfer_start_memory(tox, param1, tsim->image->png_data, tsim->image_size);
        free(tsim);

        break;
    }

    case TOX_ACCEPTFILE: {
        /* param1: friend #
         * param2: file #
         * data: path to write file
         */
        FILE_T *ft = &friend[param1].incoming[param2];
        ft->data = fopen(data, "wb");
        if(!ft->data) {
            free(data);
            break;
        }

        ft->path = data;
        ft->status = FT_SEND;

        tox_file_send_control(tox, param1, 1, param2, TOX_FILECONTROL_ACCEPT, NULL, 0);

        postmessage(FRIEND_FILE_IN_STATUS, param1, param2, (void*)FILE_OK);
        break;
    }

    case TOX_FILE_IN_CANCEL:
    {
        /* param1: friend #
         * param2: file #
         */
        FILE_T *ft = &friend[param1].incoming[param2];
        if(ft->data) {
            if(ft->inline_png) {
                free(ft->data);
            } else {
                fclose(ft->data);
                free(ft->path);
            }
        }

        ft->status = FT_NONE;
        tox_file_send_control(tox, param1, 1, param2, TOX_FILECONTROL_KILL, NULL, 0);
        postmessage(FRIEND_FILE_IN_STATUS, param1, param2, (void*)FILE_KILLED);
        break;
    }

    case TOX_FILE_OUT_CANCEL:
    {
        /* param1: friend #
         * param2: file #
         */
        FILE_T *ft = &friend[param1].outgoing[param2];
        ft->status = FT_KILL;
        tox_file_send_control(tox, param1, 0, param2, TOX_FILECONTROL_KILL, NULL, 0);
        postmessage(FRIEND_FILE_OUT_STATUS, param1, param2, (void*)FILE_KILLED);
        break;
    }

    case TOX_FILE_IN_PAUSE:
    {
        /* param1: friend #
         * param2: file #
         */
        tox_file_send_control(tox, param1, 1, param2, TOX_FILECONTROL_PAUSE, NULL, 0);
        postmessage(FRIEND_FILE_IN_STATUS, param1, param2, (void*)FILE_PAUSED);
        break;
    }

    case TOX_FILE_OUT_PAUSE:
    {
        /* param1: friend #
         * param2: file #
         */
        FILE_T *ft = &friend[param1].outgoing[param2];
        ft->status = FT_PAUSE;
        tox_file_send_control(tox, param1, 0, param2, TOX_FILECONTROL_PAUSE, NULL, 0);
        postmessage(FRIEND_FILE_OUT_STATUS, param1, param2, (void*)FILE_PAUSED);
        break;
    }

    case TOX_FILE_IN_RESUME:
    {
        /* param1: friend #
         * param2: file #
         */
        tox_file_send_control(tox, param1, 1, param2, TOX_FILECONTROL_ACCEPT, NULL, 0);
        postmessage(FRIEND_FILE_IN_STATUS, param1, param2, (void*)FILE_OK);
        break;
    }

    case TOX_FILE_OUT_RESUME:
    {
        /* param1: friend #
         * param2: file #
         */
        FILE_T *ft = &friend[param1].outgoing[param2];
        ft->status = FT_SEND;
        tox_file_send_control(tox, param1, 0, param2, TOX_FILECONTROL_ACCEPT, NULL, 0);
        postmessage(FRIEND_FILE_OUT_STATUS, param1, param2, (void*)FILE_OK);
        break;
    }

    }
}

static void file_notify(FRIEND *f, MSG_FILE *msg)
{
    STRING *str;

    switch(msg->status) {
    case FILE_PENDING:
        str = SPTR(TRANSFER_NEW); break;
    case FILE_OK:
        str = SPTR(TRANSFER_STARTED); break;
    case FILE_PAUSED:
        str = SPTR(TRANSFER___); break;
    case FILE_PAUSED_OTHER:
        str = SPTR(TRANSFER_PAUSED); break;
    case FILE_KILLED:
        str = SPTR(TRANSFER_CANCELLED); break;
    case FILE_DONE:
        str = SPTR(TRANSFER_COMPLETE); break;
    case FILE_BROKEN:
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
        str = SPTR(CALL_INVITED); break;
    case CALL_RINGING:
    case CALL_RINGING_VIDEO:
        str = SPTR(CALL_RINGING); break;
    case CALL_OK:
    case CALL_OK_VIDEO:
        str = SPTR(CALL_STARTED); break;
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
            friend_addid(data, edit_addmsg.data, edit_addmsg.length);
        } else {
            addfriend_status = ADDF_BADNAME;
        }
        free(data);

        redraw();
        break;
    }

    case OPEN_FILES: {
        tox_postmessage(TOX_SENDFILES, param1, param2, data);
        break;
    }

    case SAVE_FILE: {
        tox_postmessage(TOX_ACCEPTFILE, param1, param2, data);
        break;
    }

    case NEW_AUDIO_IN_DEVICE: {
        if(UI_STRING_ID_INVALID == param1) {
            list_dropdown_add_hardcoded(&dropdown_audio_in, data, data);
        } else {
            list_dropdown_add_localized(&dropdown_audio_in, param1, data);
        }
        break;
    }

    case NEW_AUDIO_OUT_DEVICE: {
        list_dropdown_add_hardcoded(&dropdown_audio_out, data, data);
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
        break;
    }

    case FRIEND_ADD: {
        /* confirmation that friend has been added to friend list (add) */
        if(param1) {
            /* friend was not added */
            addfriend_status = param2;
        } else {
            /* friend was added */
            edit_addid.length = 0;
            edit_addmsg.length = 0;

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
        friend_free(data);
        friends--;
        break;
    }

    case FRIEND_MESSAGE: {
        friend_addmessage(&friend[param1], data);
        redraw();
        break;
    }

#define updatefriend(fp) redraw();//list_draw(); if(sitem && fp == sitem->data) {ui_drawmain();}
#define updategroup(gp) redraw();//list_draw(); if(sitem && gp == sitem->data) {ui_drawmain();}

    case FRIEND_NAME: {
        FRIEND *f = &friend[param1];
        friend_setname(f, data, param2);
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
        f->online = param2;
        if(!f->online) {
            friend_set_typing(f, 0);
        }
        updatefriend(f);
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

    case FRIEND_FILE_IN_NEW:
    case FRIEND_FILE_IN_NEW_INLINE: {
        FRIEND *f = &friend[param1];
        FILE_T *ft = &f->incoming[param2];
        _Bool inline_png = (tox_message_id == FRIEND_FILE_IN_NEW_INLINE);

        MSG_FILE *msg = malloc(sizeof(MSG_FILE));
        msg->author = 0;
        msg->msg_type = MSG_TYPE_FILE;
        msg->filenumber = param2;
        msg->status = (inline_png ? FILE_OK : FILE_PENDING);
        msg->name_length = (ft->name_length > sizeof(msg->name)) ? sizeof(msg->name) : ft->name_length;
        msg->size = ft->total;
        msg->progress = 0;
        msg->speed = 0;
        msg->inline_png = inline_png;
        msg->path = NULL;
        memcpy(msg->name, ft->name, msg->name_length);

        friend_addmessage(f, msg);
        ft->chatdata = msg;

        file_notify(f, msg);

        updatefriend(f);
        break;
    }

    case FRIEND_FILE_OUT_NEW:
    case FRIEND_FILE_OUT_NEW_INLINE: {
        FRIEND *f = &friend[param1];
        FILE_T *ft = &f->outgoing[param2];
        _Bool inline_png = (tox_message_id == FRIEND_FILE_OUT_NEW_INLINE);

        MSG_FILE *msg = malloc(sizeof(MSG_FILE));
        msg->author = 1;
        msg->msg_type = MSG_TYPE_FILE;
        msg->filenumber = param2;
        msg->status = FILE_PENDING;
        msg->name_length = (ft->name_length >= sizeof(msg->name)) ? sizeof(msg->name) - 1 : ft->name_length;
        msg->size = ft->total;
        msg->progress = 0;
        msg->speed = 0;
        msg->inline_png = inline_png;
        msg->path = NULL;
        memcpy(msg->name, ft->name, msg->name_length);
        msg->name[msg->name_length] = 0;

        friend_addmessage(f, msg);
        ft->chatdata = msg;

        updatefriend(f);
        break;
    }

    case FRIEND_FILE_IN_STATUS: {
        FRIEND *f = &friend[param1];
        FILE_T *ft = &f->incoming[param2];

        MSG_FILE *msg = ft->chatdata;
        msg->status = (size_t)data;

        file_notify(f, msg);

        updatefriend(f);
        break;
    }

    case FRIEND_FILE_OUT_STATUS: {
        FRIEND *f = &friend[param1];
        FILE_T *ft = &f->outgoing[param2];

        MSG_FILE *msg = ft->chatdata;
        msg->status = (size_t)data;

        file_notify(f, msg);

        updatefriend(f);
        break;
    }

    case FRIEND_FILE_IN_DONE: {
        FRIEND *f = &friend[param1];
        FILE_T *ft = &f->incoming[param2];

        MSG_FILE *msg = ft->chatdata;
        msg->status = FILE_DONE;
        msg->path = data;

        file_notify(f, msg);

        updatefriend(f);
        break;
    }

    case FRIEND_FILE_IN_DONE_INLINE: {
        FRIEND *f = &friend[param1];
        FILE_T *ft = &f->incoming[param2];

        MSG_FILE *msg = ft->chatdata;
        msg->status = FILE_DONE;
        msg->path = data;

        friend_recvimage(f, data, msg->size);

        file_notify(f, msg);

        updatefriend(f);
        break;
    }

    case FRIEND_FILE_OUT_DONE: {
        FRIEND *f = &friend[param1];
        FILE_T *ft = &f->outgoing[param2];

        MSG_FILE *msg = ft->chatdata;
        msg->status = FILE_DONE;
        msg->path = data;

        file_notify(f, msg);

        updatefriend(f);
        break;
    }

    case FRIEND_FILE_IN_PROGRESS: {
        FRIEND *f = &friend[param1];
        FILE_T *ft = &f->incoming[param2];
        FILE_PROGRESS *p = data;

        MSG_FILE *msg = ft->chatdata;
        msg->progress = p->bytes;
        msg->speed = p->speed;

        free(p);

        updatefriend(f);
        break;
    }

    case FRIEND_FILE_OUT_PROGRESS: {
        FRIEND *f = &friend[param1];
        FILE_T *ft = &f->outgoing[param2];
        FILE_PROGRESS *p = data;

        MSG_FILE *msg = ft->chatdata;
        msg->progress = p->bytes;
        msg->speed = p->speed;

        free(p);

        updatefriend(f);
        break;
    }

    case GROUP_ADD: {
        GROUPCHAT *g = &group[param1];
        g->name_length = snprintf((char*)g->name, sizeof(g->name), "Groupchat #%u", param1);
        g->topic_length = sizeof("Drag friends to invite them") - 1;
        memcpy(g->topic, "Drag friends to invite them", sizeof("Drag friends to invite them") - 1);
        g->msg.scroll = 1.0;
        list_addgroup(g);
        redraw();
        break;
    }

    case GROUP_MESSAGE: {
        GROUPCHAT *g = &group[param1];

        message_add(&messages_group, data, &g->msg);

        if(sitem && g == sitem->data) {
            redraw();//ui_drawmain();
        }

        break;
    }

    case GROUP_PEER_DEL: {
        GROUPCHAT *g = &group[param1];

        if(g->peername[param2]) {
            free(g->peername[param2]);
            g->peername[param2] = NULL;
        }

        g->peers--;
        g->peername[param2] = g->peername[g->peers];
        g->peername[g->peers] = NULL;
        g->topic_length = snprintf((char*)g->topic, sizeof(g->topic), "%u users in chat", g->peers);

        updategroup(g);

        break;
    }

    case GROUP_PEER_ADD:
    case GROUP_PEER_NAME: {
        GROUPCHAT *g = &group[param1];

        if(g->peername[param2]) {
            free(g->peername[param2]);
        }

        if(tox_message_id == GROUP_PEER_ADD) {
            uint8_t *n = malloc(10);
            n[0] = 9;
            memcpy(n + 1, "<unknown>", 9);
            data = n;
            g->peers++;
        }

        g->peername[param2] = data;

        g->topic_length = snprintf((char*)g->topic, sizeof(g->topic), "%u users in chat", g->peers);

        updategroup(g);

        break;
    }
    }
}
