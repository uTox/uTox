#include "main.h"
#include "tox_bootstrap.h"

#include <AL/al.h>
#include <AL/alc.h>

#define MAX_CALLS 16

static struct {
    volatile _Bool active, video;
} call[MAX_CALLS];

#define BUF_SIZE (1024 * 1024)

typedef struct {
    uint8_t msg;
    uint16_t param1, param2;
    void *data;
} TOX_MSG;

static TOX_MSG tox_msg;
static volatile _Bool tox_thread_msg;

static ALCdevice *device_out, *device_in;
static ALCcontext *context;
static ALuint source[MAX_CALLS];

static volatile _Bool av_thread_run;

static FILE_T *file_t[256], **file_tend = file_t;

static void fillbuffer(FILE_T *ft)
{
    ft->buffer_bytes = fread(ft->buffer, 1, ft->sendsize, ft->data);
    ft->bytes += ft->buffer_bytes;
    ft->finish = (ft->bytes >= ft->total) || (feof((FILE*)ft->data) != 0);
}

static void sendfile(Tox *tox, uint32_t fid, uint8_t *path, uint8_t *name, uint16_t name_length)
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
        memset(ft, 0, sizeof(FILE));

        *file_tend++ = ft;

        ft->fid = fid;
        ft->filenumber = filenumber;
        ft->name_length = name_length;

        ft->status = FT_PENDING;
        ft->sendsize = tox_file_data_size(tox, fid);

        memcpy(ft->name, name, name_length);
        ft->total = size;

        ft->data = file;
        ft->buffer = malloc(ft->sendsize);
        fillbuffer(ft);

        postmessage(FRIEND_FILE_OUT_NEW, fid, filenumber, NULL);
    } else {
        fclose(file);
        debug("tox_new_file_sender() failed\n");
    }
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

#include "tox_callbacks.h"

static void callback_file_send_request(Tox *tox, int32_t fid, uint8_t filenumber, uint64_t filesize, uint8_t *filename, uint16_t filename_length, void *userdata)
{
    FILE_T *ft = &friend[fid].incoming[filenumber];
    memset(ft, 0, sizeof(FILE));

    ft->fid = fid;
    ft->filenumber = filenumber;
    ft->total = filesize;
    ft->name_length = filename_length;
    memcpy(ft->name, filename, filename_length);
    debug("File Request %.*s\n", filename_length, filename);

    postmessage(FRIEND_FILE_IN_NEW, fid, filenumber, NULL);
}

static void callback_file_control(Tox *tox, int32_t fid, uint8_t receive_send, uint8_t filenumber, uint8_t control, uint8_t *data, uint16_t length, void *userdata)
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
        ft->status = FT_KILL;
        if(!receive_send) {
            if(ft->data) {
                fclose(ft->data);
            }
            postmessage(FRIEND_FILE_IN_STATUS, fid, filenumber, (void*)FILE_KILLED);
        }

        break;
    }

    case TOX_FILECONTROL_PAUSE: {
        ft->status = FT_PAUSE;
        postmessage(FRIEND_FILE_IN_STATUS + receive_send, fid, filenumber, (void*)FILE_PAUSED);
        break;
    }

    case TOX_FILECONTROL_FINISHED: {
        if(!receive_send) {
            ft->status = FT_FINISHED;
            if(ft->data) {
                fclose(ft->data);
            }
            postmessage(FRIEND_FILE_IN_STATUS, fid, filenumber, (void*)FILE_DONE);
        }
        break;
    }
    }
    debug("File Control\n");
}

static void callback_file_data(Tox *tox, int32_t fid, uint8_t filenumber, uint8_t *data, uint16_t length, void *userdata)
{
    //debug("data: %u\n", length);

    FILE_T *ft = &friend[fid].incoming[filenumber];
    fwrite(data, 1, length, ft->data);
    ft->bytes += length;

    uint64_t time = get_time();
    if(time - ft->lastupdate >= 1000 * 1000 * 50) {
        ft->lastupdate = time;
        postmessage(FRIEND_FILE_IN_PROGRESS, fid, filenumber, (void*)ft->bytes);
    }
}

#include "tox_av.h"

/* bootstrap to dht with bootstrap_nodes */
static void do_bootstrap(Tox *tox)
{
    static int j = 0;
    int i = 0;
    while(i < 2) {
        struct bootstrap_node *d = &bootstrap_nodes[j % countof(bootstrap_nodes)];
        tox_bootstrap_from_address(tox, d->address, 0, d->port, d->key);
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
    uint32_t size;
    void *data = file_raw("tox_save", &size);
    if(!data) {
        return 0;
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

        tox_get_client_id(tox, i, f->cid);

        size = tox_get_name(tox, i, name);

        friend_setname(f, name, size);

        size = tox_get_status_message_size(tox, i);
        f->status_message = malloc(size);
        tox_get_status_message(tox, i, f->status_message, size);
        f->status_length = size;

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
    FILE *file;
    void *data;
    uint32_t size;

    size = tox_size(tox);
    data = malloc(size);
    tox_save(tox, data);
    file = fopen("tox_save", "wb");
    if(file) {
        fwrite(data, size, 1, file);
        fclose(file);
        debug("Saved data\n");
    }

    free(data);
}

void tox_thread(void *args)
{
    Tox *tox;
    ToxAv *av;
    uint8_t id[TOX_FRIEND_ADDRESS_SIZE];

    if((tox = tox_new(IPV6_ENABLED)) == NULL) {
        printf("tox_new() failed\n");
        exit(1);
    }

    if(!load_save(tox)) {
        printf("No save file, using defaults\n");
        load_defaults(tox);
    }

    edit_setstr(&edit_name, self.name, self.name_length);
    edit_setstr(&edit_status, self.statusmsg, self.statusmsg_length);

    tox_get_address(tox, id);
    id_to_string(self.id, id);

    printf("Tox ID: %.*s\n", (int)sizeof(self.id), self.id);

    set_callbacks(tox);

    do_bootstrap(tox);

    av = toxav_new(tox, MAX_CALLS);

    tox_thread_init = 1;
    thread(av_thread, av);

    _Bool connected = 0;
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
                        ft->lastupdate = time;
                        ft->lastprogress = ft->bytes;
                        postmessage(FRIEND_FILE_OUT_PROGRESS, ft->fid, ft->filenumber, (void*)ft->bytes);
                    }
                    if(ft->finish) {
                        tox_file_send_control(tox, ft->fid, 0, ft->filenumber, TOX_FILECONTROL_FINISHED, NULL, 0);
                        free(ft->buffer);
                        fclose(ft->data);

                        memmove(p, p + 1, ((void*)file_tend - (void*)(p + 1)));
                        p--;
                        file_tend--;
                        ft->status = FT_FINISHED;
                        postmessage(FRIEND_FILE_OUT_STATUS, ft->fid, ft->filenumber, (void*)FILE_DONE);
                        break;
                    }

                    fillbuffer(ft);
                }
                break;
            }

            case FT_KILL: {
                free(ft->buffer);
                fclose(ft->data);

                memmove(p, p + 1, ((void*)file_tend - (void*)(p + 1)));
                p--;
                file_tend--;
                postmessage(FRIEND_FILE_OUT_STATUS, ft->fid, ft->filenumber, (void*)FILE_KILLED);
            }

            }
        }

        uint32_t interval = tox_do_interval(tox);
        yieldcpu((interval > 20) ? 20 : interval);
    }

    av_thread_run = 0;
    write_save(tox);

    //toxav_kill(av);
    tox_kill(tox);

    /*while(!av_thread_run) {
        yieldcpu();
    }*/

    tox_done = 1;
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

        postmessage(FRIEND_ADD, (r < 0), (r < 0) ? ~r : r, data);
        break;
    }

    case TOX_DELFRIEND: {
        /* param1: friend #
         */
        tox_del_friend(tox, param1);
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
        tox_send_message(tox, param1, data, param2);
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
        toxav_call(av, &id, param1, TypeAudio, 10);

        postmessage(FRIEND_CALL_STATUS, param1, id, (void*)CALL_RINGING);
        break;
    }

    case TOX_CALL_VIDEO: {
        /* param1: friend #
         */
        int32_t id;
        toxav_call(av, &id, param1, TypeVideo, 10);

        postmessage(FRIEND_CALL_STATUS, param1, id, (void*)CALL_RINGING_VIDEO);
        break;
    }

    case TOX_ACCEPTCALL: {
        /* param1: call #
         */
        toxav_answer(av, param1, param2 ? TypeVideo : TypeAudio);
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
                while(*p) {
                    if(*p == '\n') {
                        *p++ = 0;
                        break;
                    }

                    if(*p == '/') {
                        s = p + 1;
                    }
                    p++;
                }

                if(strcmp2(name, "file://") == 0) {
                    name += 7;
                }

                sendfile(tox, param1, name, s, p - s - 1);
                s = name = p;
            }
        } else {
            //windows path list
            uint8_t *name = data;
            _Bool multifile = (name[param2 - 1] == 0);
            if(!multifile) {
                sendfile(tox, param1, data, data + param2, strlen(data) - param2);
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
                    sendfile(tox, param1, data, name, len - 1);
                }
            }
        }

        free(data);

        break;
    }

    case TOX_ACCEPTFILE:
    {
        /* param1: friend #
         * param2: file #
         * data: path to write file
         */
        FILE_T *ft = &friend[param1].incoming[param2];
        ft->data = fopen(data, "wb");
        free(data);
        if(!ft->data) {
            break;
        }

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
            fclose(ft->data);
        }
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
            addfriend_status = param2 + ADDF_TOOLONG;
        } else {
            /* friend was added */
            edit_addid.length = 0;
            edit_addmsg.length = 0;

            FRIEND *f = &friend[param2];
            friends++;

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
        f->typing = param2;
        updatefriend(f);
        break;
    }

    case FRIEND_ONLINE: {
        FRIEND *f = &friend[param1];
        f->online = param2;
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
        if(status == CALL_NONE && f->calling == CALL_OK_VIDEO) {
            video_end(f);
        }

        f->calling = status;
        f->callid = param2;
        updatefriend(f);
        break;
    }

    case FRIEND_CALL_START_VIDEO: {
        /* param1: friend id
           param2: call id
           data: encoded width | height << 16 of video
         */
        FRIEND *f = &friend[param1];
        f->calling = CALL_OK_VIDEO;
        updatefriend(f);

        uint32_t res = (size_t)data;
        video_begin(f, res & 0xFFFF, res >> 16);
        break;
    }

    case FRIEND_VIDEO_FRAME: {
        /* param1: friend id
           param2: call id
           data: frame data
         */
        video_frame(&friend[param1], data);
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
        memcpy(msg->name, ft->name, msg->name_length);

        friend_addmessage(f, msg);
        ft->chatdata = msg;

        updatefriend(f);
        break;
    }

    case FRIEND_FILE_OUT_NEW: {
        FRIEND *f = &friend[param1];
        FILE_T *ft = &f->outgoing[param2];

        MSG_FILE *msg = malloc(sizeof(MSG_FILE));
        msg->flags = 7;
        msg->filenumber = param2;
        msg->status = FILE_PENDING;
        msg->name_length = (ft->name_length > sizeof(msg->name)) ? sizeof(msg->name) : ft->name_length;
        msg->size = ft->total;
        memcpy(msg->name, ft->name, msg->name_length);

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

        updatefriend(f);
        break;
    }

    case FRIEND_FILE_OUT_STATUS: {
        FRIEND *f = &friend[param1];
        FILE_T *ft = &f->outgoing[param2];

        MSG_FILE *msg = ft->chatdata;
        msg->status = (size_t)data;

        updatefriend(f);
        break;
    }

    case FRIEND_FILE_IN_PROGRESS: {
        FRIEND *f = &friend[param1];
        FILE_T *ft = &f->incoming[param2];

        MSG_FILE *msg = ft->chatdata;
        msg->progress = (size_t)data;

        updatefriend(f);
        break;
    }

    case FRIEND_FILE_OUT_PROGRESS: {
        FRIEND *f = &friend[param1];
        FILE_T *ft = &f->outgoing[param2];

        MSG_FILE *msg = ft->chatdata;
        msg->progress = (size_t)data;

        updatefriend(f);
        break;
    }

    case GROUP_ADD: {
        GROUPCHAT *g = &group[param1];
        g->name_length = sprintf((char*)g->name, "Groupchat #%u", param1);
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
