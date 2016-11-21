#include <inttypes.h>

#include "friend.h"
#include "groups.h"
#include "main.h"
#include "tox.h"
#include "util.h"

static void callback_friend_request(Tox *UNUSED(tox), const uint8_t *id, const uint8_t *msg, size_t length,
                                    void *UNUSED(userdata)) {
    length = utf8_validate(msg, length);

    FRIENDREQ *req = malloc(sizeof(FRIENDREQ) + length);

    req->length = length;
    memcpy(req->id, id, sizeof(req->id));
    memcpy(req->msg, msg, length);

    postmessage(FRIEND_INCOMING_REQUEST, 0, 0, req);
}

static void callback_friend_message(Tox *UNUSED(tox), uint32_t friend_number, TOX_MESSAGE_TYPE type,
                                    const uint8_t *message, size_t length, void *UNUSED(userdata)) {
    /* send message to UI */
    switch (type) {
        case TOX_MESSAGE_TYPE_NORMAL: {
            message_add_type_text(&friend[friend_number].msg, 0, (char *)message, length, 1, 0);
            debug("Friend(%u) Standard Message: %.*s\n", friend_number, (int)length, message);
            break;
        }

        case TOX_MESSAGE_TYPE_ACTION: {
            message_add_type_action(&friend[friend_number].msg, 0, (char *)message, length, 1, 0);
            debug("Friend(%u) Action Message: %.*s\n", friend_number, (int)length, message);
            break;
        }

        default: { debug("Message from Friend(%u) of unsupported type: %.*s\n", friend_number, (int)length, message); }
    }
    friend_notify_msg(&friend[friend_number], message, length);
    postmessage(FRIEND_MESSAGE, friend_number, 0, NULL);
}

static void callback_name_change(Tox *UNUSED(tox), uint32_t fid, const uint8_t *newname, size_t length,
                                 void *UNUSED(userdata)) {
    length     = utf8_validate(newname, length);
    void *data = malloc(length);
    memcpy(data, newname, length);
    postmessage(FRIEND_NAME, fid, length, data);
    debug_info("Friend-%u Name:\t%.*s\n", fid, (int)length, newname);
}

static void callback_status_message(Tox *UNUSED(tox), uint32_t fid, const uint8_t *newstatus, size_t length,
                                    void *UNUSED(userdata)) {
    length     = utf8_validate(newstatus, length);
    void *data = malloc(length);
    memcpy(data, newstatus, length);
    postmessage(FRIEND_STATUS_MESSAGE, fid, length, data);
    debug_info("Friend-%u Status Message:\t%.*s\n", fid, (int)length, newstatus);
}

static void callback_user_status(Tox *UNUSED(tox), uint32_t fid, TOX_USER_STATUS status, void *UNUSED(userdata)) {
    postmessage(FRIEND_STATE, fid, status, NULL);
    debug_info("Friend-%u State:\t%u\n", fid, status);
}

static void callback_typing_change(Tox *UNUSED(tox), uint32_t fid, bool is_typing, void *UNUSED(userdata)) {
    postmessage(FRIEND_TYPING, fid, is_typing, NULL);
    debug_info("Friend-%u Typing:\t%u\n", fid, is_typing);
}

static void callback_read_receipt(Tox *UNUSED(tox), uint32_t fid, uint32_t receipt, void *UNUSED(userdata)) {
    messages_clear_receipt(&friend[fid].msg, receipt);
    debug_info("Friend-%u Receipt:\t%u\n", fid, receipt);
}

static void callback_connection_status(Tox *tox, uint32_t fid, TOX_CONNECTION status, void *UNUSED(userdata)) {
    if (friend[fid].online && !status) {
        ft_friend_offline(tox, fid);
        if (friend[fid].call_state_self || friend[fid].call_state_friend) {
            utox_av_local_disconnect(NULL, fid); /* TODO HACK, toxav doesn't supply a toxav_get_toxav_from_otx() yet. */
        }
    } else if (!friend[fid].online && !!status) {
        ft_friend_online(tox, fid);
        /* resend avatar info (in case it changed) */
        /* Avatars must be sent LAST or they will clobber existing file transfers! */
        avatar_on_friend_online(tox, fid);
        friend_notify_status(&friend[fid], friend[fid].status_message, friend[fid].status_length, "online");
    }
    postmessage(FRIEND_ONLINE, fid, !!status, NULL);

    if (status == TOX_CONNECTION_UDP) {
        debug_info("Friend-%u:\tOnline (UDP)\n", fid);
    } else if (status == TOX_CONNECTION_TCP) {
        debug_info("Friend-%u:\tOnline (TCP)\n", fid);
    } else {
        debug_info("Friend-%u:\tOffline\n", fid);
        friend_notify_status(&friend[fid], NULL, 0, "offline");
    }
}

void utox_set_callbacks_friends(Tox *tox) {
    tox_callback_friend_request(tox, callback_friend_request);
    tox_callback_friend_message(tox, callback_friend_message);
    tox_callback_friend_name(tox, callback_name_change);
    tox_callback_friend_status_message(tox, callback_status_message);
    tox_callback_friend_status(tox, callback_user_status);
    tox_callback_friend_typing(tox, callback_typing_change);
    tox_callback_friend_read_receipt(tox, callback_read_receipt);
    tox_callback_friend_connection_status(tox, callback_connection_status);
}

void callback_av_group_audio(Tox *tox, int groupnumber, int peernumber, const int16_t *pcm, unsigned int samples,
                             uint8_t channels, unsigned int sample_rate, void *userdata);

static void callback_group_invite(Tox *tox, uint32_t fid, TOX_CONFERENCE_TYPE type, const uint8_t *data, size_t length,
                                  void *UNUSED(userdata)) {
    int gid = -1;
    if (type == TOX_CONFERENCE_TYPE_TEXT) {
        gid = tox_conference_join(tox, fid, data, length, NULL);
        group_init(&group[gid], gid, 0);
    } else if (type == TOX_CONFERENCE_TYPE_AV) {
        // TODO FIX THIS AFTER NEW GROUP API IS RELEASED
        // gid = toxav_join_av_groupchat(tox, fid, data, length, &callback_av_group_audio, NULL);
    }

    if (gid != -1) {
        postmessage(GROUP_ADD, gid, 0, tox);
    }

    debug("Group Invite (%i,f:%i) type %u\n", gid, fid, type);
}

static void callback_group_message(Tox *UNUSED(tox), uint32_t gid, uint32_t pid, TOX_MESSAGE_TYPE type,
                                   const uint8_t *message, size_t length, void *UNUSED(userdata)) {
    GROUPCHAT *g = &group[gid];

    switch (type) {
        case TOX_MESSAGE_TYPE_ACTION: {
            debug("Group Action (%u, %u): %.*s\n", gid, pid, (int)length, message);
            group_add_message(g, pid, message, length, MSG_TYPE_ACTION_TEXT);
            break;
        }
        case TOX_MESSAGE_TYPE_NORMAL: {
            debug_notice("Group Message (%u, %u): %.*s\n", gid, pid, (int)length, message);
            group_add_message(g, pid, message, length, MSG_TYPE_TEXT);
            break;
        }
    }
    group_notify_msg(g, message, length);
    postmessage(GROUP_MESSAGE, gid, pid, NULL);
}

static void callback_group_namelist_change(Tox *tox, uint32_t gid, uint32_t pid, TOX_CONFERENCE_STATE_CHANGE change,
                                           void *UNUSED(userdata)) {
    GROUPCHAT *g = &group[gid];

    switch (change) {
        case TOX_CONFERENCE_STATE_CHANGE_PEER_JOIN: {
            if (g->peer) {
                g->peer = realloc(g->peer, sizeof(void *) * (g->peer_count + 2));
            } else {
                g->peer = calloc(g->peer_count + 2, sizeof(void *));
            }
            debug("Group:\tAdd (%u, %u)\n", gid, pid);
            bool is_us = 0;
            if (tox_conference_peer_number_is_ours(tox, gid, pid, 0)) {
                g->our_peer_number = pid;
                is_us              = 1;
            }

            uint8_t pkey[TOX_PUBLIC_KEY_SIZE];
            tox_conference_peer_get_public_key(tox, gid, pid, pkey, NULL);
            uint64_t pkey_to_number = 0;
            int      key_i          = 0;
            for (; key_i < TOX_PUBLIC_KEY_SIZE; ++key_i) {
                pkey_to_number += pkey[key_i];
            }
            srand(pkey_to_number);
            uint32_t name_color = 0;
            name_color          = RGB(rand(), rand(), rand());

            group_peer_add(g, pid, is_us, name_color);

            postmessage(GROUP_PEER_ADD, gid, pid, NULL);
            break;
        }

        case TOX_CONFERENCE_STATE_CHANGE_PEER_NAME_CHANGE: {
            debug("Group:\tPeer name change (%u, %u)\n", gid, pid);

            if (g->peer) {
                if (!g->peer[pid]) {
                    debug("Tox Group:\tERROR, can't sent a name, for non-existant peer!\n");
                    break;
                }
            } else {
                debug("Tox Group:\tERROR, can't sent a name, for non-existant Group!\n");
            }

            uint8_t name[TOX_MAX_NAME_LENGTH];
            size_t  len = tox_conference_peer_get_name_size(tox, gid, pid, NULL);
            tox_conference_peer_get_name(tox, gid, pid, name, NULL);
            len         = utf8_validate(name, len);
            group_peer_name_change(g, pid, name, len);

            postmessage(GROUP_PEER_NAME, gid, pid, NULL);
            break;
        }

        case TOX_CONFERENCE_STATE_CHANGE_PEER_EXIT: {
            debug("Group:\tPeer Quit (%u, %u)\n", gid, pid);
            group_add_message(g, pid, (const uint8_t *)"<- has Quit!", 12, MSG_TYPE_NOTICE);

            pthread_mutex_lock(&messages_lock); /* make sure that messages has posted before we continue */

            group_reset_peerlist(g);

            uint32_t number_peers = tox_conference_peer_count(tox, gid, NULL);

            g->peer = calloc(number_peers, sizeof(void *));

            if (!g->peer) {
                debug("Group:\tToxcore is very broken, but we couldn't alloc here.");
                exit(44);
            }

            /* I'm about to break some uTox style here, because I'm expecting
             * the API to change soon, and I just can't when it's this broken */
            for (uint32_t i = 0; i < number_peers; ++i) {
                uint8_t     tmp[TOX_MAX_NAME_LENGTH];
                size_t      len  = tox_conference_peer_get_name_size(tox, gid, i, NULL);
                tox_conference_peer_get_name(tox, gid, i, tmp, NULL);
                GROUP_PEER *peer = calloc(1, len * sizeof(void *) + sizeof(*peer));
                if (!peer) {
                    debug("Group:\tToxcore is very broken, but we couldn't calloc here.");
                    exit(45);
                }
                /* name and id number (it's worthless, but it's needed */
                memcpy(peer->name, tmp, len);
                peer->name_length = len;
                peer->id          = i;
                /* get static random color */
                uint8_t pkey[TOX_PUBLIC_KEY_SIZE];
                tox_conference_peer_get_public_key(tox, gid, i, pkey, NULL);
                uint64_t pkey_to_number = 0;
                for (int key_i = 0; key_i < TOX_PUBLIC_KEY_SIZE; ++key_i) {
                    pkey_to_number += pkey[key_i];
                }
                /* uTox doesnt' really use this for too much so lets fuck with the random seed.
                 * If you know crypto, and cringe, I know me too... you can blame @irungentoo */
                srand(pkey_to_number);
                peer->name_color = RGB(rand(), rand(), rand());
                g->peer[i]       = peer;
            }
            g->peer_count = number_peers;

            postmessage(GROUP_PEER_DEL, gid, pid, NULL);
            pthread_mutex_unlock(&messages_lock); /* make sure that messages has posted before we continue */
            break;
        }
    }
}

static void callback_group_topic(Tox *UNUSED(tox), uint32_t gid, uint32_t pid, const uint8_t *title, size_t length,
                                 void *UNUSED(userdata)) {
    length = utf8_validate(title, length);
    if (!length)
        return;

    uint8_t *copy_title = malloc(length);
    if (!copy_title)
        return;

    memcpy(copy_title, title, length);
    postmessage(GROUP_TOPIC, gid, length, copy_title);

    debug("Group Title (%u, %u): %.*s\n", gid, pid, (int)length, title);
}

void utox_set_callbacks_groups(Tox *tox) {
    tox_callback_conference_invite(tox, callback_group_invite);
    tox_callback_conference_message(tox, callback_group_message);
    tox_callback_conference_namelist_change(tox, callback_group_namelist_change);
    tox_callback_conference_title(tox, callback_group_topic);
}

#ifdef ENABLE_MULTIDEVICE
static void callback_friend_list_change(Tox *tox, void *user_data) {
    debug_error("friend list change, updating roster\n");

    flist_dump_contacts();
    utox_friend_list_init(tox);
    flist_reload_contacts();
}

static void callback_mdev_self_name(Tox *tox, uint32_t dev_num, const uint8_t *name, size_t length,
                                    void *UNUSED(userdata)) {

    debug_info("Name changed on remote device %u\n", dev_num);

    memcpy(self.name, name, length);
    self.name_length = length;

    edit_setstr(&edit_name, self.name, self.name_length);

    postmessage(REDRAW, 0, 0, NULL);
}

typedef void tox_mdev_self_status_message_cb(Tox *tox, uint32_t device_number, const uint8_t *status_message,
                                             size_t len, void *user_data);

static void callback_mdev_self_status_msg(Tox *tox, uint32_t dev_num, const uint8_t *smsg, size_t length,
                                          void *UNUSED(userdata)) {

    debug_info("Status Message changed on remote device %u\n", dev_num);

    memcpy(self.statusmsg, smsg, length);
    self.statusmsg_length = length;

    edit_setstr(&edit_status, self.statusmsg, self.statusmsg_length);

    postmessage(REDRAW, 0, 0, NULL);
}

static void callback_mdev_self_state(Tox *tox, uint32_t device_number, TOX_USER_STATUS state, void *user_data) {
    self.status = state;
}


static void callback_device_sent_message(Tox *tox, uint32_t sending_device, uint32_t target_friend,
                                         TOX_MESSAGE_TYPE type, uint8_t *msg, size_t msg_length) {
    debug("Message sent from other device %u\n\t\t%.*s\n", sending_device, (uint32_t)msg_length, msg);

    switch (type) {
        case TOX_MESSAGE_TYPE_NORMAL: {
            message_add_type_text(&friend[target_friend].msg, 1, msg, msg_length, 1, 0);
            break;
        }

        case TOX_MESSAGE_TYPE_ACTION: {
            message_add_type_action(&friend[target_friend].msg, 1, msg, msg_length, 1, 0);
            break;
        }

        default: {
            debug_error("Message from Friend(%u) of unsupported type: %.*s\n", target_friend, (uint32_t)msg_length, msg);
        }
    }
    friend_notify_msg(&friend[target_friend], msg, msg_length);
    postmessage(FRIEND_MESSAGE, target_friend, 0, NULL);
}

void utox_set_callbacks_mdevice(Tox *tox) {
    tox_callback_friend_list_change(tox, callback_friend_list_change, NULL);

    tox_callback_mdev_self_status_message(tox, callback_mdev_self_status_msg, NULL);
    tox_callback_mdev_self_name(tox, callback_mdev_self_name, NULL);
    tox_callback_mdev_self_state(tox, callback_mdev_self_state, NULL);

    tox_callback_mdev_sent_message(tox, callback_device_sent_message, NULL);
}
#endif
