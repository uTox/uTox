#include "main.h"

void friend_setname(FRIEND *f, char_t *name, STRING_IDX length)
{
    if(f->name && (length != f->name_length || memcmp(f->name, name, length) != 0)) {
        MESSAGE *msg = malloc(sizeof(MESSAGE) + sizeof(" is now known as ") - 1 + f->name_length + length);
        msg->author = 0;
        msg->msg_type = MSG_TYPE_ACTION_TEXT;
        msg->length = sizeof(" is now known as ") - 1 + f->name_length + length;
        char_t *p = msg->msg;
        memcpy(p, f->name, f->name_length); p += f->name_length;
        memcpy(p, " is now known as ", sizeof(" is now known as ") - 1); p += sizeof(" is now known as ") - 1;
        memcpy(p, name, length);

        friend_addmessage(f, msg);
    }

    free(f->name);
    if(length == 0) {
        f->name = malloc(sizeof(f->cid) * 2 + 1);
        cid_to_string(f->name, f->cid);
        f->name_length = sizeof(f->cid) * 2;
    } else {
        f->name = malloc(length + 1);
        memcpy(f->name, name, length);
        f->name_length = length;
    }
    f->name[f->name_length] = 0;
}

void friend_sendimage(FRIEND *f, void *data, void *pngdata, uint16_t width, uint16_t height)
{
    MSG_IMG *msg = malloc(sizeof(MSG_IMG));
    msg->author = 1;
    msg->msg_type = MSG_TYPE_IMAGE;
    msg->w = width;
    msg->h = height;
    msg->zoom = 0;
    msg->data = data;
    msg->position = 0.0;

    message_add(&messages_friend, (void*)msg, &f->msg);

    tox_postmessage(TOX_SEND_INLINE, f - friend, 0, pngdata);
}

void friend_recvimage(FRIEND *f, void *pngdata, uint32_t size)
{
    uint16_t width, height;
    void *data = png_to_image(pngdata, &width, &height, size);
    if(!data) {
        return;
    }

    MSG_IMG *msg = malloc(sizeof(MSG_IMG));
    msg->author = 0;
    msg->msg_type = MSG_TYPE_IMAGE;
    msg->w = width;
    msg->h = height;
    msg->zoom = 0;
    msg->data = data;
    msg->position = 0.0;

    message_add(&messages_friend, (void*)msg, &f->msg);
}

void friend_notify(FRIEND *f, char_t *str, STRING_IDX str_length, char_t *msg, STRING_IDX msg_length)
{
    int len = f->name_length + str_length + 3;
    char_t title[len + 1], *p = title;
    memcpy(p, str, str_length); p += str_length;
    *p++ = ' ';
    *p++ = '(';
    memcpy(p, f->name, f->name_length); p += f->name_length;
    *p++ = ')';
    *p = 0;
    notify(title, len, msg, msg_length);
}

void friend_addmessage_notify(FRIEND *f, char_t *data, STRING_IDX length)
{
    MESSAGE *msg = malloc(sizeof(MESSAGE) + length);
    msg->author = 0;
    msg->msg_type = MSG_TYPE_ACTION_TEXT;
    msg->length = length;
    char_t *p = msg->msg;
    memcpy(p, data, length);

    message_add(&messages_friend, msg, &f->msg);

    if(sitem->data != f) {
        f->notify = 1;
    }
}

void friend_addmessage(FRIEND *f, void *data)
{
    MESSAGE *msg = data;

    message_add(&messages_friend, data, &f->msg);

    switch(msg->msg_type) {
    case MSG_TYPE_TEXT:
    case MSG_TYPE_ACTION_TEXT: {
        char_t m[msg->length + 1];
        memcpy(m, msg->msg, msg->length);
        m[msg->length] = 0;
        notify(f->name, f->name_length, m, msg->length);
        break;
    }
    }

    if(sitem->data != f) {
        f->notify = 1;
    }
}

void friend_set_typing(FRIEND *f, int typing) {
    f->typing = typing;
    messages_set_typing(&messages_friend, &f->msg, typing);
}

void friend_addid(uint8_t *id, char_t *msg, STRING_IDX msg_length)
{
    void *data = malloc(TOX_FRIEND_ADDRESS_SIZE + msg_length * sizeof(char_t));
    memcpy(data, id, TOX_FRIEND_ADDRESS_SIZE);
    memcpy(data + TOX_FRIEND_ADDRESS_SIZE, msg, msg_length * sizeof(char_t));

    tox_postmessage(TOX_ADDFRIEND, msg_length, 0, data);
}

void friend_add(char_t *name, STRING_IDX length, char_t *msg, STRING_IDX msg_length)
{
    if(!length) {
        addfriend_status = ADDF_NONAME;
        return;
    }

    uint8_t id[TOX_FRIEND_ADDRESS_SIZE];
    if(length == TOX_FRIEND_ADDRESS_SIZE * 2 && string_to_id(id, name)) {
        friend_addid(id, msg, msg_length);
    } else {
        /* not a regular id, try DNS discovery */
        addfriend_status = ADDF_DISCOVER;
        dns_request(name, length);
    }
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

    MSG_IDX i = 0;
    while(i < f->msg.n) {
        MESSAGE *msg = f->msg.data[i];
        switch(msg->msg_type) {
        case MSG_TYPE_IMAGE: {
            //MSG_IMG *img = (void*)msg;
            //todo: free image
            break;
        }
        case MSG_TYPE_FILE: {
            MSG_FILE *file = (void*)msg;
            free(file->path);
            FILE_T *ft = &f->incoming[file->filenumber];
            if(ft->data) {
                if(ft->inline_png) {
                    free(ft->data);
                } else {
                    fclose(ft->data);
                    free(ft->path);
                }
            }

            if(msg->author) {
                ft->status = FT_NONE;
            }
            break;
        }
        }
        message_free(msg);
        i++;
    }

    free(f->msg.data);

    if(f->calling) {
        toxaudio_postmessage(AUDIO_CALL_END, f->callid, 0, NULL);
        if(f->calling == CALL_OK_VIDEO) {
            toxvideo_postmessage(VIDEO_CALL_END, f->callid, 0, NULL);
        }
    }

    memset(f, 0, sizeof(FRIEND));//
}

void group_free(GROUPCHAT *g)
{
    uint16_t i = 0;
    while(i != g->edit_history_length) {
        free(g->edit_history[i]);
        i++;
    }
    free(g->edit_history);

    char_t **np = g->peername;
    uint32_t j = 0;
    while(j < g->peers) {
        char_t *n = *np++;
        if(n) {
            free(n);
        }
        j++;
    }

    MSG_IDX k = 0;
    while(k < g->msg.n) {
        free(g->msg.data[k]);
        k++;
    }

    free(g->msg.data);

    memset(g, 0, sizeof(GROUPCHAT));//
}
