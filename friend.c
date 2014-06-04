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

void friend_addid(uint8_t *id, uint8_t *msg, uint16_t msg_length)
{
    void *data = malloc(TOX_FRIEND_ADDRESS_SIZE + msg_length);
    memcpy(data, id, TOX_FRIEND_ADDRESS_SIZE);
    memcpy(data + TOX_FRIEND_ADDRESS_SIZE, msg, msg_length);

    tox_postmessage(TOX_ADDFRIEND, msg_length, 0, data);
}

void friend_add(uint8_t *name, uint16_t length, uint8_t *msg, uint16_t msg_length)
{
    if(!length) {
        addfriend_status = ADDF_NONAME;
        return;
    }

    uint8_t id[TOX_FRIEND_ADDRESS_SIZE];
    if(length == TOX_FRIEND_ADDRESS_SIZE * 2 && string_to_id(id, edit_addid.data)) {
        friend_addid(id, msg, msg_length);
    } else {
        /* not a regular id, try DNS discovery */
        addfriend_status = ADDF_DISCOVER;
        dns_request(name, length);
    }
}
