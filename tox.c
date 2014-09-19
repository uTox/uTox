#include "main.h"
#include "tox_bootstrap.h"

typedef struct {
    uint8_t msg;
    uint16_t param1, param2;
    void *data;
} TOX_MSG;

typedef struct
{
    uint64_t bytes;
    uint32_t speed;
} FILE_PROGRESS;

static TOX_MSG tox_msg, audio_msg, video_msg;
static volatile _Bool tox_thread_msg, audio_thread_msg, video_thread_msg;

static FILE_T *file_t[256], **file_tend = file_t;

void log_write(Tox *tox, int fid, const uint8_t *message, uint16_t length, _Bool self)
{
    if(!logging_enabled) {
        return;
    }

    uint8_t path[512], *p;
    uint8_t name[TOX_MAX_NAME_LENGTH];
    int namelen;
    uint8_t client_id[TOX_CLIENT_ID_SIZE];
    FILE *file;

    p = path + datapath(path);
    tox_get_client_id(tox, fid, client_id);
    cid_to_string(p, client_id); p += TOX_CLIENT_ID_SIZE * 2;
    strcpy((char*)p, ".txt");

    file = fopen((char*)path, "ab");
    if(file) {
        time_t rawtime;
        time(&rawtime);

        if(self) {
            namelen = tox_get_self_name(tox, name);
        } else if((namelen = tox_get_name(tox, fid, name)) == -1) {
            //error reading name
            namelen = 0;
        }

        struct {
            uint64_t time;
            uint16_t namelen, length;
            uint8_t flags, zeroes[3];
        } header = {
            .time = rawtime,
            .namelen = namelen,
            .length = length,
            .flags = self,
        };

        fwrite(&header, sizeof(header), 1, file);
        fwrite(name, namelen, 1, file);
        fwrite(message, length, 1, file);
        fclose(file);
    }
}

void log_read(Tox *tox, int fid)
{
    uint8_t path[512], *p, *pp, *end;
    uint8_t client_id[TOX_CLIENT_ID_SIZE];
    uint32_t size, i;

    p = path + datapath(path);
    tox_get_client_id(tox, fid, client_id);
    cid_to_string(p, client_id); p += TOX_CLIENT_ID_SIZE * 2;
    strcpy((char*)p, ".txt");

    p = pp = file_raw((char*)path, &size);
    if(!p) {
        return;
    }

    end = p + size;

    /* todo: some checks to avoid crashes with corrupted log files */
    /* first find the last 128 messages in the log */
    i = 0;
    while(p < end) {
        uint16_t namelen, length;
        memcpy(&namelen, p + 8, 2);
        memcpy(&length, p + 10, 2);
        p += 16 + namelen + length;;

        if(++i > 128) {
            memcpy(&namelen, pp + 8, 2);
            memcpy(&length, pp + 10, 2);
            pp += 16 + namelen + length;
        }
    }

    if(i > 128) {
        i = 128;
    }

    MSG_DATA *m = &friend[fid].msg;
    m->data = malloc(sizeof(void*) * i);
    m->n = i;
    i = 0;

    /* add the messages */
    p = pp;
    while(p < end) {
        uint64_t time;
        uint16_t namelen, length;
        uint8_t flags;
        memcpy(&time, p, 8);
        memcpy(&namelen, p + 8, 2);
        memcpy(&length, p + 10, 2);
        flags = p[12];
        p += 16;

        MESSAGE *msg = malloc(sizeof(MESSAGE) + length);
        msg->flags = flags;
        msg->length = length;
        memcpy(msg->msg, p + namelen, length);

        struct tm *ti;
        time_t rawtime = time;
        ti = localtime(&rawtime);

        msg->time = ti->tm_hour * 60 + ti->tm_min;

        m->data[i++] = msg;

        debug("loaded backlog: %.*s: %.*s\n", namelen, p, length, p + namelen);
        p += namelen + length;
    }
}

static void fillbuffer(FILE_T *ft)
{
    if(ft->inline_png) {
        ft->buffer += ft->sendsize;

        if(ft->total - ft->bytes <= ft->sendsize) {
            ft->buffer_bytes = ft->total - ft->bytes;
            ft->bytes = ft->total;
            ft->finish = 1;
            return;
        }

        ft->buffer_bytes = ft->sendsize;
        ft->bytes += ft->sendsize;
        return;
    }

    ft->buffer_bytes = fread(ft->buffer, 1, ft->sendsize, ft->data);
    ft->bytes += ft->buffer_bytes;
    ft->finish = (ft->bytes >= ft->total) || (feof((FILE*)ft->data) != 0);
}

static void startft(Tox *tox, uint32_t fid, uint8_t *path, uint8_t *name, uint16_t name_length)
{
    debug("Sending: %s\n", path);

    FILE *file = fopen((char*)path, "rb");
    if(!file) {
        return;
    }

    uint64_t size;
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fseek(file, 0, SEEK_SET);

    int filenumber = tox_new_file_sender(tox, fid, size, name, name_length);
    if(filenumber != -1) {
        FILE_T *ft = &friend[fid].outgoing[filenumber];
        memset(ft, 0, sizeof(FILE_T));

        *file_tend++ = ft;

        ft->fid = fid;
        ft->filenumber = filenumber;

        name_length = name_length > sizeof(ft->name) ? sizeof(ft->name) : name_length;
        ft->name_length = name_length;

        ft->status = FT_PENDING;
        ft->sendsize = tox_file_data_size(tox, fid);

        memcpy(ft->name, name, name_length);
        ft->total = size;

        ft->path = (uint8_t*)strdup((char*)path);

        ft->data = file;
        ft->buffer = malloc(ft->sendsize);
        fillbuffer(ft);

        postmessage(FRIEND_FILE_OUT_NEW, fid, filenumber, NULL);
    } else {
        fclose(file);
        debug("tox_new_file_sender() failed\n");
    }
}

static void startft_inline(Tox *tox, uint16_t fid, void *pngdata)
{
    uint32_t size;
    memcpy(&size, pngdata, 4);

    int filenumber = tox_new_file_sender(tox, fid, size, (uint8_t*)"inline.png", sizeof("inline.png") - 1);
    if(filenumber != -1) {
        if(filenumber > countof(friend[0].outgoing)) {
            tox_file_send_control(tox, fid, 0, filenumber, TOX_FILECONTROL_KILL, NULL, 0);
            return;
        }

        FILE_T *ft = &friend[fid].outgoing[filenumber];
        memset(ft, 0, sizeof(FILE_T));

        *file_tend++ = ft;

        ft->fid = fid;
        ft->filenumber = filenumber;
        ft->inline_png = 1;

        ft->status = FT_PENDING;
        ft->sendsize = tox_file_data_size(tox, fid);

        memcpy(ft->name, "inline.png", sizeof("inline.png") - 1);
        ft->name_length = sizeof("inline.png") - 1;
        ft->total = size;

        ft->data = pngdata;
        ft->buffer = ft->data + 4;

        if(ft->total <= ft->sendsize) {
            ft->bytes = ft->total;
            ft->buffer_bytes = ft->total;
            ft->finish = 1;
        } else {
            ft->bytes = ft->sendsize;
            ft->buffer_bytes = ft->sendsize;
        }


        postmessage(FRIEND_FILE_OUT_NEW_INLINE, fid, filenumber, NULL);
    } else {
        free(pngdata);
    }
}

static void resetft(Tox *tox, FILE_T *ft, uint64_t start)
{
    if(start >= ft->total) {
        return;
    }

    if(ft->inline_png) {
        ft->buffer = ft->data + 4;
    } else {
        fseek(ft->data, start, SEEK_SET);
        fillbuffer(ft);
    }

    if(ft->status == FT_NONE) {
        *file_tend++ = ft;
    }

    ft->bytes = start;
    ft->status = FT_SEND;
}

static void tox_thread_message(Tox *tox, ToxAv *av, uint8_t msg, uint16_t param1, uint16_t param2, void *data);

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

static void callback_file_send_request(Tox *tox, int32_t fid, uint8_t filenumber, uint64_t filesize, const uint8_t *filename, uint16_t filename_length, void *userdata)
{
    if(filenumber > countof(friend[0].incoming)) {
        tox_file_send_control(tox, fid, 1, filenumber, TOX_FILECONTROL_KILL, NULL, 0);
        return;
    }

    FILE_T *ft = &friend[fid].incoming[filenumber];
    memset(ft, 0, sizeof(FILE_T));

    ft->fid = fid;
    ft->filenumber = filenumber;
    ft->total = filesize;

    filename_length = filename_length > sizeof(ft->name) ? sizeof(ft->name) : filename_length;
    filename_length = utf8_validate(filename, filename_length);

    ft->name_length = filename_length;
    memcpy(ft->name, filename, filename_length);
    debug("File Request %.*s\n", filename_length, filename);

    if(filesize < 1024 * 1024 *4 && filename_length == sizeof("inline.png") - 1 && memcmp(filename, "inline.png", filename_length) == 0) {
        ft->inline_png = 1;
        ft->status = FT_SEND;

        ft->data = malloc(filesize);

        tox_file_send_control(tox, fid, 1, filenumber, TOX_FILECONTROL_ACCEPT, NULL, 0);

        //postmessage(FRIEND_FILE_IN_STATUS, fid, filenumber, (void*)FILE_OK);
        postmessage(FRIEND_FILE_IN_NEW_INLINE, fid, filenumber, NULL);
    } else {
        postmessage(FRIEND_FILE_IN_NEW, fid, filenumber, NULL);
    }
}

static void callback_file_control(Tox *tox, int32_t fid, uint8_t receive_send, uint8_t filenumber, uint8_t control, const uint8_t *data, uint16_t length, void *userdata)
{
    FILE_T *ft = (receive_send) ? &friend[fid].outgoing[filenumber] : &friend[fid].incoming[filenumber];

    switch(control) {
    case TOX_FILECONTROL_ACCEPT: {
        ft->status = FT_SEND;
        debug("FileAccepted %u\n", filenumber);
        postmessage(FRIEND_FILE_IN_STATUS + receive_send, fid, filenumber, (void*)FILE_OK);
        break;
    }

    case TOX_FILECONTROL_KILL: {
        ft->status = receive_send ? FT_KILL : FT_NONE;
        if(!receive_send) {
            if(ft->data) {
                if(ft->inline_png) {
                    free(ft->data);
                } else {
                    fclose(ft->data);
                    free(ft->path);
                }
            }
            postmessage(FRIEND_FILE_IN_STATUS, fid, filenumber, (void*)FILE_KILLED);
        }

        break;
    }

    case TOX_FILECONTROL_PAUSE: {
        if(ft->status == FT_SEND) {
            ft->status = FT_PAUSE;
            postmessage(FRIEND_FILE_IN_STATUS + receive_send, fid, filenumber, (void*)FILE_PAUSED_OTHER);
        } else if(ft->status == FT_PAUSE) {
            ft->status = FT_SEND;
            postmessage(FRIEND_FILE_IN_STATUS + receive_send, fid, filenumber, (void*)FILE_OK);
        }

        break;
    }

    case TOX_FILECONTROL_FINISHED: {
        if(!receive_send) {
            ft->status = FT_NONE;
            if(ft->inline_png) {
                postmessage(FRIEND_FILE_IN_DONE_INLINE, fid, filenumber, ft->data);
            } else {
                fclose(ft->data);
                postmessage(FRIEND_FILE_IN_DONE, fid, filenumber, ft->path);
            }

            tox_file_send_control(tox, fid, 1, filenumber, TOX_FILECONTROL_FINISHED, NULL, 0);

        } else {
            if(ft->status == FT_NONE) {
                if(!ft->inline_png) {
                    fclose(ft->data);
                    free(ft->buffer);
                }
                postmessage(FRIEND_FILE_OUT_DONE, fid, filenumber, ft->path);
            }
        }
        break;
    }

    case TOX_FILECONTROL_RESUME_BROKEN: {
        if(receive_send && length == 8) {
            resetft(tox, ft, *(uint64_t*)data);
            tox_file_send_control(tox, fid, 0, filenumber, TOX_FILECONTROL_ACCEPT, NULL, 0);
            postmessage(FRIEND_FILE_IN_STATUS + receive_send, fid, filenumber, (void*)FILE_OK);
        }

        break;
    }

    }
    debug("File Control\n");
}

static void callback_file_data(Tox *tox, int32_t fid, uint8_t filenumber, const uint8_t *data, uint16_t length, void *userdata)
{
    FILE_T *ft = &friend[fid].incoming[filenumber];
    if(ft->inline_png) {
        memcpy(ft->data + ft->bytes, data, length);
    } else {
        fwrite(data, 1, length, ft->data);
    }
    ft->bytes += length;

    uint64_t time = get_time();
    if(time - ft->lastupdate >= 1000 * 1000 * 50) {
        FILE_PROGRESS *p = malloc(sizeof(FILE_PROGRESS));
        p->bytes = ft->bytes;
        p->speed = ((ft->bytes - ft->lastprogress) * 1000 * 1000 * 1000) / (time - ft->lastupdate);

        postmessage(FRIEND_FILE_IN_PROGRESS, fid, filenumber, p);

        ft->lastupdate = time;
        ft->lastprogress = ft->bytes;
    }
}

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

    tox_callback_file_send_request(tox, callback_file_send_request, NULL);
    tox_callback_file_control(tox, callback_file_control, NULL);
    tox_callback_file_data(tox, callback_file_data, NULL);
}

static _Bool load_save(Tox *tox)
{
    uint8_t path[512], *p;
    uint32_t size;

    p = path + datapath(path);
    strcpy((char*)p, "tox_save");

    void *data = file_raw((char*)path, &size);
    if(!data) {
        data = file_raw("tox_save", &size);
        if(!data) {
            return 0;
        }
    }
    int r = tox_load(tox, data, size);
    free(data);

    if(r != 0) {
        return 0;
    }

    friends = tox_count_friendlist(tox);

    uint32_t i = 0;
    while(i != friends) {
        int size;
        FRIEND *f = &friend[i];
        uint8_t name[128];

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
    uint8_t path[512], *p;
    FILE *file;

    size = tox_size(tox);
    data = malloc(size);
    tox_save(tox, data);

    p = path + datapath(path);
    strcpy((char*)p, "tox_save");

    file = fopen((char*)path, "wb");
    if(file) {
        fwrite(data, size, 1, file);
        fclose(file);
        debug("Saved data\n");
    }

    free(data);
}

void tox_settingschanged(void)
{
    //free everything
    tox_connected = 0;
    list_freeall();

    free(dropdown_audio_in.drop);
    dropdown_audio_in.drop = NULL;
    dropdown_audio_in.dropcount = 0;
    dropdown_audio_in.over = 0;
    dropdown_audio_in.selected = 0;

    free(dropdown_audio_out.drop);
    dropdown_audio_out.drop = NULL;
    dropdown_audio_out.dropcount = 0;
    dropdown_audio_out.over = 0;
    dropdown_audio_out.selected = 0;

    free(dropdown_video.drop);
    dropdown_video.drop = NULL;
    dropdown_video.dropcount = 0;
    dropdown_video.over = 0;
    dropdown_video.selected = 0;

    dropdown_add(&dropdown_video, (uint8_t*)"None", NULL);
    dropdown_add(&dropdown_video, (uint8_t*)"Desktop", (void*)1);


    tox_thread_init = 0;

    toxaudio_postmessage(AUDIO_KILL, 0, 0, NULL);
    toxvideo_postmessage(VIDEO_KILL, 0, 0, NULL);
    tox_postmessage(0, 1, 0, NULL);

    while(!tox_thread_init) {
        yieldcpu(1);
    }

    list_start();
}

void tox_thread(void *args)
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
            tox_thread_message(tox, av, msg->msg, msg->param1, msg->param2, msg->data);
            tox_thread_msg = 0;
        }

        FILE_T **p;
        for(p = file_t; p < file_tend; p++) {
            FILE_T *ft = *p;
            if(!ft) {
                continue;
            }

            switch(ft->status) {
            case FT_SEND: {
                while(tox_file_send_data(tox, ft->fid, ft->filenumber, ft->buffer, ft->buffer_bytes) != -1) {
                    if(time - ft->lastupdate >= 1000 * 1000 * 50 || ft->bytes - ft->lastprogress >= 1024 * 1024) {
                        FILE_PROGRESS *p = malloc(sizeof(FILE_PROGRESS));
                        p->bytes = ft->bytes;
                        p->speed = (time - ft->lastupdate) == 0 ? 0 : ((ft->bytes - ft->lastprogress) * 1000 * 1000 * 1000) / (time - ft->lastupdate);

                        postmessage(FRIEND_FILE_OUT_PROGRESS, ft->fid, ft->filenumber, p);

                        ft->lastupdate = time;
                        ft->lastprogress = ft->bytes;
                    }
                    if(ft->finish) {
                        tox_file_send_control(tox, ft->fid, 0, ft->filenumber, TOX_FILECONTROL_FINISHED, NULL, 0);

                        memmove(p, p + 1, ((void*)file_tend - (void*)(p + 1)));
                        p--;
                        file_tend--;
                        ft->status = FT_NONE;
                        break;
                    }

                    fillbuffer(ft);
                }
                break;
            }

            case FT_KILL: {
                if(ft->inline_png) {
                    free(ft->data);
                } else {
                    fclose(ft->data);
                    free(ft->buffer);
                }

                memmove(p, p + 1, ((void*)file_tend - (void*)(p + 1)));
                p--;
                file_tend--;
                postmessage(FRIEND_FILE_OUT_STATUS, ft->fid, ft->filenumber, (void*)FILE_KILLED);
                break;
            }

            case FT_NONE: {
                memmove(p, p + 1, ((void*)file_tend - (void*)(p + 1)));
                p--;
                file_tend--;
                break;
            }

            }
        }

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

static void tox_thread_message(Tox *tox, ToxAv *av, uint8_t msg, uint16_t param1, uint16_t param2, void *data)
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
            r = tox_add_friend(tox, data, (uint8_t*)DEFAULT_ADD_MESSAGE, sizeof(DEFAULT_ADD_MESSAGE) - 1);
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
        log_write(tox, param1, data, param2, 1);

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

    case TOX_SENDMESSAGEGROUP: {
        /* param1: group #
         * param2: message length
         * data: message
         */
        tox_group_message_send(tox, param1, data, param2);
        free(data);
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

                startft(tox, param1, name, s, p - s);
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
                startft(tox, param1, data, data + param2, strlen(data) - param2);
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
                    startft(tox, param1, data, name, len - 1);
                }
            }
        }

        free(data);

        break;
    }

    case TOX_SEND_INLINE: {
        /* param1: friend id
           data: pointer to pngdata
         */
        startft_inline(tox, param1, data);
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

void tox_message(uint8_t msg, uint16_t param1, uint16_t param2, void *data)
{
    switch(msg) {
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
        dropdown_add(&dropdown_audio_in, data, param2 ? (void*)(size_t)(param2 - 1) : data);
        break;
    }

    case NEW_AUDIO_OUT_DEVICE: {
        dropdown_add(&dropdown_audio_out, data, data);
        break;
    }

    case NEW_VIDEO_DEVICE: {
        dropdown_video.selected = dropdown_video.over = dropdown_video.dropcount;
        dropdown_add(&dropdown_video, data + sizeof(void*), *(void**)data);
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
            video_frame(0, data, param1, param2, msg == PREVIEW_FRAME_NEW);
        }
        free(data);
        break;
    }

    case FRIEND_FILE_IN_NEW: {
        FRIEND *f = &friend[param1];
        FILE_T *ft = &f->incoming[param2];

        MSG_FILE *msg = malloc(sizeof(MSG_FILE));
        msg->flags = 6;
        msg->filenumber = param2;
        msg->status = FILE_PENDING;
        msg->name_length = (ft->name_length > sizeof(msg->name)) ? sizeof(msg->name) : ft->name_length;
        msg->size = ft->total;
        msg->progress = 0;
        msg->speed = 0;
        msg->inline_png = 0;
        msg->path = NULL;
        memcpy(msg->name, ft->name, msg->name_length);

        friend_addmessage(f, msg);
        ft->chatdata = msg;

        file_notify(f, msg);

        updatefriend(f);
        break;
    }

    case FRIEND_FILE_IN_NEW_INLINE: {
        FRIEND *f = &friend[param1];
        FILE_T *ft = &f->incoming[param2];

        MSG_FILE *msg = malloc(sizeof(MSG_FILE));
        msg->flags = 6;
        msg->filenumber = param2;
        msg->status = FILE_OK;
        msg->name_length = (ft->name_length > sizeof(msg->name)) ? sizeof(msg->name) : ft->name_length;
        msg->size = ft->total;
        msg->progress = 0;
        msg->speed = 0;
        msg->inline_png = 1;
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
        _Bool inline_png = (msg == FRIEND_FILE_OUT_NEW_INLINE);

        MSG_FILE *msg = malloc(sizeof(MSG_FILE));
        msg->flags = 7;
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
        g->name_length = sprintf((char*)g->name, "Groupchat #%u", param1);
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
            g->peers--;
        }

        g->topic_length = sprintf((char*)g->topic, "%u users in chat", g->peers);

        updategroup(g);

        break;
    }

    case GROUP_PEER_ADD:
    case GROUP_PEER_NAME: {
        GROUPCHAT *g = &group[param1];

        if(g->peername[param2]) {
            free(g->peername[param2]);
        } else {
            g->peers++;
        }

        if(msg == GROUP_PEER_ADD) {
            uint8_t *n = malloc(10);
            n[0] = 9;
            memcpy(n + 1, "<unknown>", 9);
            data = n;
        }

        g->peername[param2] = data;

        g->topic_length = sprintf((char*)g->topic, "%u users in chat", g->peers);

        updategroup(g);

        break;
    }
    }
}
