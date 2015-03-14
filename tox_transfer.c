#include "main.h"

static FILE_T *file_t[256], **file_tend = file_t;

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

// Outgoing transfer functions

void utox_transfer_start_file(Tox *tox, uint32_t fid, uint8_t *path, uint8_t *name, uint16_t name_length)
{
    debug("Sending: %s\n", path);

    if (friend[fid].count_outgoing >= MAX_FILE_TRANSFERS || (file_tend - file_t) >= countof(file_t)) {
        debug("Maximum outgoing file sending limit reached(%d/%d) for friend(%d).\n",
                                        friend[fid].count_outgoing, MAX_FILE_TRANSFERS, fid);
        return;
    }

    FILE *file = fopen((char*)path, "rb");
    if(!file) {
        return;
    }

    uint64_t size;
    fseeko(file, 0, SEEK_END);
    size = ftello(file);
    fseeko(file, 0, SEEK_SET);

    int filenumber = -1;//TODO tox_new_file_sender(tox, fid, size, name, name_length);
    if(filenumber != -1) {
        FILE_T *ft = &friend[fid].outgoing[filenumber];
        memset(ft, 0, sizeof(FILE_T));

        *file_tend++ = ft;

        ft->fid = fid;
        ft->filenumber = filenumber;

        name_length = name_length > sizeof(ft->name) ? sizeof(ft->name) : name_length;
        ft->name_length = name_length;

        ft->status = FT_PENDING;
        //TODO ft->sendsize = tox_file_data_size(tox, fid);

        memcpy(ft->name, name, name_length);
        ft->total = size;

        ft->path = (uint8_t*)strdup((char*)path);

        ft->data = file;
        ft->buffer = malloc(ft->sendsize);
        fillbuffer(ft);

        postmessage(FRIEND_FILE_OUT_NEW, fid, filenumber, NULL);
        ++friend[fid].count_outgoing;
        debug("Sending file %d of %d(max) to friend(%d).\n", friend[fid].count_outgoing, MAX_FILE_TRANSFERS, fid);

    } else {
        fclose(file);
        debug("tox_new_file_sender() failed\n");
    }
}

void utox_transfer_start_memory(Tox *tox, uint16_t fid, void *pngdata, size_t size)
{
    if (friend[fid].count_outgoing >= MAX_FILE_TRANSFERS || (file_tend - file_t) >= countof(file_t)) {
        debug("Maximum outgoing file sending limit reached(%d/%d) for friend(%d).\n",
                                        friend[fid].count_outgoing, MAX_FILE_TRANSFERS, fid);
        return;
    }

    int filenumber = -1;//TODO tox_new_file_sender(tox, fid, size, (uint8_t*)"inline.png", sizeof("inline.png") - 1);
    if(filenumber != -1) {
        FILE_T *ft = &friend[fid].outgoing[filenumber];
        memset(ft, 0, sizeof(FILE_T));

        *file_tend++ = ft;

        ft->fid = fid;
        ft->filenumber = filenumber;
        ft->inline_png = 1;

        ft->status = FT_PENDING;
        //TODO ft->sendsize = tox_file_data_size(tox, fid);

        memcpy(ft->name, "inline.png", sizeof("inline.png") - 1);
        ft->name_length = sizeof("inline.png") - 1;
        ft->total = size;

        ft->data = pngdata;
        ft->buffer = ft->data;

        if(ft->total <= ft->sendsize) {
            ft->bytes = ft->total;
            ft->buffer_bytes = ft->total;
            ft->finish = 1;
        } else {
            ft->bytes = ft->sendsize;
            ft->buffer_bytes = ft->sendsize;
        }


        postmessage(FRIEND_FILE_OUT_NEW_INLINE, fid, filenumber, NULL);
        ++friend[fid].count_outgoing;
    } else {
        free(pngdata);
    }
}

static void reset_file_transfer(FILE_T *ft, uint64_t start){
    if(start >= ft->total) {
        debug("Bad data sent when trying to restart file transfer\n");
        return;
    }

    if(ft->inline_png) {
        ft->buffer = ft->data;
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


// Tox file callbacks

static void callback_file_send_request(Tox *tox, int32_t fid, uint8_t filenumber, uint64_t filesize, const uint8_t *filename, uint16_t filename_length, void *UNUSED(userdata))
{/* TODO
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
    }*/
}

static void callback_file_control(Tox *tox, int32_t fid, uint8_t receive_send, uint8_t filenumber, uint8_t control, const uint8_t *data, uint16_t length, void *UNUSED(userdata))
{/*
    FILE_T *ft = (receive_send) ? &friend[fid].outgoing[filenumber] : &friend[fid].incoming[filenumber];

    switch(control) {
    case TOX_FILECONTROL_ACCEPT: {
        ft->status = FT_SEND;
        debug("File Accepted %u for friend (%i)\n", filenumber, fid);
        postmessage(FRIEND_FILE_IN_STATUS + receive_send, fid, filenumber, (void*)FILE_OK);
        break;
    }

    case TOX_FILECONTROL_KILL: {
        debug("File Control (File Killed for friend(%d) filenumber(%d)\n", fid, filenumber);
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
        debug("File Control (File Paused for friend(%d) filenumber(%d)\n", fid, filenumber);
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
        debug("File Control (File Done for friend(%d) filenumber(%d)\n", fid, filenumber);
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
                    postmessage(FRIEND_FILE_OUT_DONE, fid, filenumber, ft->path);
                } else {
                    postmessage(FRIEND_FILE_OUT_DONE, fid, filenumber, ft->data);
                }

            }
        }
        break;
    }

    case TOX_FILECONTROL_RESUME_BROKEN: {
        debug("File Control (File Restarted for friend(%d) filenumber(%d)\n", fid, filenumber);
        if(receive_send && length == 8) {
            reset_file_transfer(ft, *(uint64_t*)data);
            tox_file_send_control(tox, fid, 0, filenumber, TOX_FILECONTROL_ACCEPT, NULL, 0);
            postmessage(FRIEND_FILE_IN_STATUS + receive_send, fid, filenumber, (void*)FILE_OK);
        }
        break;
    }

    }*/
}

static void callback_file_data(Tox *UNUSED(tox), int32_t fid, uint8_t filenumber, const uint8_t *data, uint16_t length, void *UNUSED(userdata))
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

void utox_set_callbacks_for_transfer(Tox *tox)
{/*
    tox_callback_file_send_request(tox, callback_file_send_request, NULL);
    tox_callback_file_control(tox, callback_file_control, NULL);
    tox_callback_file_data(tox, callback_file_data, NULL);
    */
}

// Called from tox_thread main loop to do transfer work.
// Tox API can be called directly.
void utox_thread_work_for_transfers(Tox *tox, uint64_t time)
{
    FILE_T **p;
    for(p = file_t; p < file_tend; p++) {
        FILE_T *ft = *p;
        if(!ft) {
            continue;
        }

        switch(ft->status) {
        case FT_SEND: {
            while(/*tox_file_send_data(tox, ft->fid, ft->filenumber, ft->buffer, ft->buffer_bytes) != -1 TODO*/ 0) {
                if(time - ft->lastupdate >= 1000 * 1000 * 50 || ft->bytes - ft->lastprogress >= 1024 * 1024) {
                    FILE_PROGRESS *fp = malloc(sizeof(FILE_PROGRESS));
                    fp->bytes = ft->bytes;
                    fp->speed = (time - ft->lastupdate) == 0 ? 0 : ((ft->bytes - ft->lastprogress) * 1000 * 1000 * 1000) / (time - ft->lastupdate);

                    postmessage(FRIEND_FILE_OUT_PROGRESS, ft->fid, ft->filenumber, fp);

                    ft->lastupdate = time;
                    ft->lastprogress = ft->bytes;
                }
                if(ft->finish) {
                    //TODO tox_file_send_control(tox, ft->fid, 0, ft->filenumber, TOX_FILECONTROL_FINISHED, NULL, 0);

                    memmove(p, p + 1, ((void*)file_tend - (void*)(p + 1)));
                    p--;
                    file_tend--;
                    --friend[ft->fid].count_outgoing;
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
            --friend[ft->fid].count_outgoing;
            postmessage(FRIEND_FILE_OUT_STATUS, ft->fid, ft->filenumber, (void*)FILE_KILLED);
            break;
        }

        case FT_NONE: {
            memmove(p, p + 1, ((void*)file_tend - (void*)(p + 1)));
            p--;
            file_tend--;
            --friend[ft->fid].count_outgoing;
            break;
        }

        }
    }
}
