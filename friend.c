#include "main.h"

void friend_setname(FRIEND *f, char_t *name, uint16_t length)
{
    if(f->name && (length != f->name_length || memcmp(f->name, name, length) != 0)) {
        MESSAGE *msg = malloc(sizeof(MESSAGE) + sizeof(" is now known as ") - 1 + f->name_length + length);
        msg->flags = 2;
        msg->length = sizeof(" is now known as ") - 1 + f->name_length + length;
        uint8_t *p = msg->msg;
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

void friend_notify(FRIEND *f, uint8_t *str, uint16_t str_length, uint8_t *msg, uint16_t msg_length)
{
    int len = f->name_length + str_length + 3;
    uint8_t title[len + 1], *p = title;
    memcpy(p, str, str_length); p += str_length;
    *p++ = ' ';
    *p++ = '(';
    memcpy(p, f->name, f->name_length); p += f->name_length;
    *p++ = ')';
    *p = 0;
    notify(title, len, msg, msg_length);
}

void friend_add_str_message(FRIEND *f, char_t *data, uint16_t length)
{
    MESSAGE *msg = malloc(sizeof(MESSAGE) + length);
    msg->flags = 2;
    msg->length = length;
    uint8_t *p = msg->msg;
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

    if(msg->flags < 4) {
        uint8_t m[msg->length + 1];
        memcpy(m, msg->msg, msg->length);
        m[msg->length] = 0;
        notify(f->name, f->name_length, m, msg->length);
    }

    if(sitem->data != f) {
        f->notify = 1;
    }
}

void friend_addid(uint8_t *id, char_t *msg, uint16_t msg_length)
{
    void *data = malloc(TOX_FRIEND_ADDRESS_SIZE + msg_length * sizeof(char_t));
    memcpy(data, id, TOX_FRIEND_ADDRESS_SIZE);
    memcpy(data + TOX_FRIEND_ADDRESS_SIZE, msg, msg_length * sizeof(char_t));

    tox_postmessage(TOX_ADDFRIEND, msg_length, 0, data);
}

void friend_add(char_t *name, uint16_t length, char_t *msg, uint16_t msg_length)
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
