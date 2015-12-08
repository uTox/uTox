#include "main.h"

static void utox_group_topic_cb(Tox *tox, uint32_t groupnumber, uint32_t peer_id, const uint8_t *topic, size_t length,
                                void *UNUSED(user_data))
{
    debug("newGC:\tGroup Title (%u, %u): %.*s\n", groupnumber, peer_id, length, topic);

    length = utf8_validate(topic, length);
    if (!length) {
        return;
    }

    uint8_t *copy_title = malloc(length);
    if (!copy_title) {
        return;
    }

    memcpy(copy_title, topic, length);
    postmessage(GROUP_TOPIC, groupnumber, length, copy_title);
}

static void utox_group_privacy_state_cb(Tox *tox, uint32_t groupnumber, TOX_GROUP_PRIVACY_STATE privacy_state,
                                        void *UNUSED(user_data))
{
    debug("newGC:\tprivacy_state changed, nothing happened\n");
}

static void utox_group_peer_limit_cb(Tox *tox, uint32_t groupnumber, uint32_t peer_limit, void *UNUSED(user_data)) {
    debug("newGC:\tnew peer limit is %u on %u\n", peer_limit, groupnumber);
}

static void utox_group_password_cb(Tox *tox, uint32_t groupnumber, const uint8_t *password, size_t length,
                                   void *UNUSED(user_data))
{
    debug("newGC:\tNew group password on group %u, %.s\n", groupnumber, length, password);
}

static void utox_incoming_group_message_cb(Tox *tox, uint32_t groupnumber, uint32_t peer_id, TOX_MESSAGE_TYPE type,
                    const uint8_t *message, size_t length, void *UNUSED(user_data))
{
    debug("newGC:\tGroup Message (%u, %u)\n", groupnumber, peer_id);

    uint8_t name[TOX_MAX_NAME_LENGTH];
    TOX_ERR_GROUP_PEER_QUERY err = 0;
    size_t peer_name_length = tox_group_peer_get_name_size(tox, groupnumber, peer_id, &err);
    if (peer_name_length == 0 || peer_name_length == -1 || err ) {
        snprintf(name, 9, "<unknown>");
        peer_name_length = 9;
    } else {
        err = 0;
        tox_group_peer_get_name(tox, groupnumber, peer_id, name, &err);
    }

    group_append_mesage(0, 0, groupnumber, length, message, peer_name_length, name);
}

static void utox_incoming_group_private_message_cb(Tox *tox, uint32_t groupnumber, uint32_t peer_id,
                                                    const uint8_t *message, size_t length, void *UNUSED(user_data))
{
    debug("newGC:\tgroup private message\n");
    uint32_t name_length = tox_group_peer_get_name_size(tox, groupnumber, peer_id, NULL);
    uint8_t name[name_length + 5];
    snprintf(name, 5, "<PM> ");
    tox_group_peer_get_name(tox, groupnumber, peer_id, name + 5, NULL);
    group_append_mesage(0, 0, groupnumber, length, message, name_length + 5, name);
}

static void utox_group_invite_cb(Tox *tox, uint32_t friend_number, const uint8_t *invite_data, size_t length,
                                 void *UNUSED(user_data))
{
    debug("newGC:\tGroup Invite (%u)\n", friend_number);

    TOX_ERR_GROUP_INVITE_ACCEPT err = 0;
    uint32_t groupnumber = tox_group_invite_accept(tox, invite_data, length, NULL, 0, &err);
    if (err) {
        debug("newGC:\tJoin on invite failed!\n");
    } else {
        debug("newGC:\tgroup number is %u\n", groupnumber);
    }
}

static void utox_group_peer_join_cb(Tox *tox, uint32_t groupnumber, uint32_t peer_id, void *UNUSED(user_data)) {
    debug("newGC:\tpeer join group %u, peer %u\n", groupnumber, peer_id);

    uint32_t name_length = tox_group_peer_get_name_size(tox, groupnumber, peer_id, NULL);
    uint8_t name[name_length + 21];
    snprintf(name, 21, "New peer has joined: ");
    tox_group_peer_get_name(tox, groupnumber, peer_id, name + 21, NULL);
    group_append_mesage(0, 0, groupnumber, name_length + 21, name, 8, "<SERVER>");

    postmessage(GROUP_PEER_ADD, groupnumber, peer_id, tox);
}


static void utox_group_peer_exit_cb(Tox *tox, uint32_t groupnumber, uint32_t peer_id, const uint8_t *part_message,
                                    size_t length, void *UNUSED(user_data))
{
    debug("newGC:\tpeer exit, peer_id %u, from group %u\n", peer_id, groupnumber);

    uint32_t name_length = tox_group_peer_get_name_size(tox, groupnumber, peer_id, NULL);
    uint8_t name[name_length + 20];
    snprintf(name, 20, "Peer has left chat: ");
    tox_group_peer_get_name(tox, groupnumber, peer_id, name + 20, NULL);
    group_append_mesage(0, 0, groupnumber, name_length + 20, name, 8, "<SERVER>");

    postmessage(GROUP_PEER_DEL, groupnumber, peer_id, tox);
}

static void utox_group_self_join_cb(Tox *tox, uint32_t groupnumber, void *UNUSED(user_data)) {
    debug("newGC:\tGroup joined\n");
    tox_postmessage(TOX_GROUP_JOINED, groupnumber, 0, NULL);
}

static void utox_group_join_fail_cb(Tox *tox, uint32_t groupnumber, TOX_GROUP_JOIN_FAIL fail_type, void *UNUSED(user_data)) {
    debug("newGC:\tgroup join failed!!\n");
    /*typedef enum TOX_GROUP_JOIN_FAIL {

        /**
         * You are using the same nickname as someone who is already in the group.
         * /
        TOX_GROUP_JOIN_FAIL_NAME_TAKEN,

        /**
         * The group peer limit has been reached.
         * /
        TOX_GROUP_JOIN_FAIL_PEER_LIMIT,

        *
         * You have supplied an invalid password.

        TOX_GROUP_JOIN_FAIL_INVALID_PASSWORD,

        /**
         * The join attempt failed due to an unspecified error. This often occurs when the group is
         * not found in the DHT.
         * /
        TOX_GROUP_JOIN_FAIL_UNKNOWN,

    } TOX_GROUP_JOIN_FAIL; */
}


static void utox_group_moderation_cb(Tox *tox, uint32_t groupnumber, uint32_t source_peer_number,
                                     uint32_t target_peer_number, TOX_GROUP_MOD_EVENT mod_type, void *user_data) {
    debug("newGC:\tSome moderation happened, god knows what...\n");
}

void group_free(GROUPCHAT *g) {
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

_Bool group_append_mesage(_Bool author, uint8_t type, uint32_t groupnumber,
                          size_t msg_length, const uint8_t *message,
                          size_t name_length, const uint8_t *name)
{

    msg_length  = utf8_validate(message, msg_length); /* TODO, if it's not utf8, this function may break */
    name_length = utf8_validate(   name, name_length);

    MESSAGE *msg = malloc(sizeof(MESSAGE) + msg_length + name_length);

    if (!msg) {
        return 1;
    }

    msg->author      = author;
    msg->msg_type    = type;
    msg->length      = msg_length;
    msg->name_length = name_length;
    memcpy(msg->msg, message, msg_length);
    memcpy(&msg->msg[msg_length], name, name_length);

    postmessage(GROUP_MESSAGE, groupnumber, 0, msg);

    return 0;
}


void utox_set_callbacks_for_groupchats(Tox *tox) {
    tox_callback_group_topic(tox, utox_group_topic_cb, NULL);
    tox_callback_group_privacy_state(tox, utox_group_privacy_state_cb, NULL);
    tox_callback_group_peer_limit(tox, utox_group_peer_limit_cb, NULL);
    tox_callback_group_password(tox, utox_group_password_cb, NULL);
    tox_callback_group_message(tox, utox_incoming_group_message_cb, NULL);
    tox_callback_group_private_message(tox, utox_incoming_group_private_message_cb, NULL);
    // tox_callback_group_custom_packet(tox, BLERG, NULL);
    tox_callback_group_invite(tox, utox_group_invite_cb, NULL);
    tox_callback_group_peer_join(tox, utox_group_peer_join_cb, NULL);
    tox_callback_group_peer_exit(tox, utox_group_peer_exit_cb, NULL);
    tox_callback_group_self_join(tox, utox_group_self_join_cb, NULL); /* called when self has joined correctly */
    tox_callback_group_join_fail(tox, utox_group_join_fail_cb, NULL);
    tox_callback_group_moderation(tox, utox_group_moderation_cb, NULL);
}

