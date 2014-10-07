typedef struct
{
    uint64_t bytes;
    uint32_t speed;
} FILE_PROGRESS;

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

static void startft(Tox *tox, uint32_t fid, uint8_t *path, uint8_t *name, uint16_t name_length)
{
    debug("Sending: %s\n", path);

    FILE *file = fopen((char*)path, "rb");
    if(!file) {
        return;
    }

    uint64_t size;
    fseeko(file, 0, SEEK_END);
    size = ftello(file);
    fseeko(file, 0, SEEK_SET);

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

static void resetft(Tox *UNUSED(tox), FILE_T *ft, uint64_t start)
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


// Tox file callbacks

static void callback_file_send_request(Tox *tox, int32_t fid, uint8_t filenumber, uint64_t filesize, const uint8_t *filename, uint16_t filename_length, void *UNUSED(userdata))
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

static void callback_file_control(Tox *tox, int32_t fid, uint8_t receive_send, uint8_t filenumber, uint8_t control, const uint8_t *data, uint16_t length, void *UNUSED(userdata))
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
                    postmessage(FRIEND_FILE_OUT_DONE, fid, filenumber, ft->path);
                } else {
                    postmessage(FRIEND_FILE_OUT_DONE, fid, filenumber, ft->data);
                }

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



