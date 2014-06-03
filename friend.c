#include "main.h"

void friend_setname(FRIEND *f, uint8_t *name, uint16_t length)
{
    free(f->name);
    if(length == 0) {
        f->name = malloc(sizeof(f->cid) * 2);
        cid_to_string(f->name, f->cid);
        f->name_length = sizeof(f->cid) * 2;
    } else {
        f->name = malloc(length);
        memcpy(f->name, name, length);
        f->name_length = length;
    }
}

FILE_T* friend_newincoming(FRIEND *f, uint8_t filenumber)
{
    if(filenumber >= f->nincoming) {
        f->nincoming = filenumber + 1;
        FILE_T *new = realloc(f->incoming, f->nincoming * sizeof(FILE_T));
        if(!new) {
            return NULL;
        }

        f->incoming = new;
    }

    FILE_T *ft = &f->incoming[filenumber];
    memset(ft, 0, sizeof(FILE_T));
    return ft;
}

FILE_T* friend_newoutgoing(FRIEND *f, uint8_t filenumber)
{
    if(filenumber >= f->noutgoing) {
        f->noutgoing = filenumber + 1;
        FILE_T *new = realloc(f->outgoing, f->noutgoing * sizeof(FILE_T));
        if(!new) {
            return NULL;
        }

        f->outgoing = new;
    }

    FILE_T *ft = &f->outgoing[filenumber];
    memset(ft, 0, sizeof(FILE_T));
    return ft;
}

void friend_addid(uint8_t *id, uint8_t *msg, uint16_t msg_length)
{
    void *data = malloc(TOX_FRIEND_ADDRESS_SIZE + msg_length);
    memcpy(data, id, TOX_FRIEND_ADDRESS_SIZE);
    memcpy(data + TOX_FRIEND_ADDRESS_SIZE, msg, msg_length);

    tox_postmessage(TOX_ADDFRIEND, msg_length, 0, data);
}

void friend_add(uint8_t *name, uint16_t length, uint8_t *msg, uint16_t msg_length)
{
    uint8_t id[TOX_FRIEND_ADDRESS_SIZE];
    if(length == TOX_FRIEND_ADDRESS_SIZE * 2 && string_to_id(id, edit_addid.data)) {
        friend_addid(id, msg, msg_length);
    } else {
        /* not a regular id, try DNS discovery */
        addfriend_status = ADDF_DISCOVER;
        dns_request(name, length);
    }
}
