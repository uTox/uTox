#include "tox_callbacks.h"

#include "avatar.h"
#include "debug.h"
#include "file_transfers.h"
#include "friend.h"
#include "group_invite.h"
#include "groups.h"
#include "macros.h"
#include "settings.h"
#include "text.h"
#include "ui.h"
#include "utox.h"

#include "av/audio.h"
#include "av/utox_av.h"


#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void callback_friend_request(Tox *UNUSED(tox), const uint8_t *id, const uint8_t *msg, size_t length,
                                    void *UNUSED(userdata)) {

    if (settings.block_friend_requests) {
        LOG_WARN("Tox Callbacks", "Friend request ignored."); // TODO move to friend.c
        return;
    }

    length = utf8_validate(msg, length);

    uint16_t r_number = friend_request_new(id, msg, length);

    postmessage_utox(FRIEND_INCOMING_REQUEST, r_number, 0, NULL);
    postmessage_audio(UTOXAUDIO_PLAY_NOTIFICATION, NOTIFY_TONE_FRIEND_REQUEST, 0, NULL);
}

static void callback_friend_message(Tox *UNUSED(tox), uint32_t friend_number, TOX_MESSAGE_TYPE type,
                                    const uint8_t *message, size_t length, void *UNUSED(userdata)) {
    /* send message to UI */
    FRIEND *f = get_friend(friend_number);
    if (!f) {
        LOG_ERR("Tox Callbacks", "Could not get friend with number: %u", friend_number);
        return;
    }

    switch (type) {
        case TOX_MESSAGE_TYPE_NORMAL: {
            message_add_type_text(&f->msg, 0, (char *)message, length, 1, 0);
            LOG_INFO("Tox Callbacks", "Friend\t%u\t--\tStandard Message: %.*s" , friend_number, (int)length, message);
            break;
        }

        case TOX_MESSAGE_TYPE_ACTION: {
            message_add_type_action(&f->msg, 0, (char *)message, length, 1, 0);
            LOG_INFO("Tox Callbacks", "Friend\t%u\t--\tAction Message: %.*s" , friend_number, (int)length, message);
            break;
        }

        default: {
            LOG_ERR("Tox Callbacks", "Friend\t%u\t--\tUnsupported message type: %.*s" , friend_number, (int)length, message);
            break;
        }
    }
    friend_notify_msg(f, (char *)message, length);
}

static void callback_name_change(Tox *UNUSED(tox), uint32_t fid, const uint8_t *newname, size_t length,
                                 void *UNUSED(userdata)) {
    length     = utf8_validate(newname, length);
    void *data = malloc(length);
    memcpy(data, newname, length);
    postmessage_utox(FRIEND_NAME, fid, length, data);
    LOG_INFO("Tox Callbacks", "Friend\t%u\t--\tName:\t%.*s", fid, (int)length, newname);
}

static void callback_status_message(Tox *UNUSED(tox), uint32_t fid, const uint8_t *newstatus, size_t length,
                                    void *UNUSED(userdata)) {
    length     = utf8_validate(newstatus, length);
    void *data = malloc(length);
    memcpy(data, newstatus, length);
    postmessage_utox(FRIEND_STATUS_MESSAGE, fid, length, data);
    LOG_INFO("Tox Callbacks", "Friend\t%u\t--\tStatus Message:\t%.*s", fid, (int)length, newstatus);
}

static void callback_user_status(Tox *UNUSED(tox), uint32_t fid, TOX_USER_STATUS status, void *UNUSED(userdata)) {
    postmessage_utox(FRIEND_STATE, fid, status, NULL);
    LOG_INFO("Tox Callbacks", "Friend\t%u\t--\tState:\t%u", fid, status);
}

static void callback_typing_change(Tox *UNUSED(tox), uint32_t fid, bool is_typing, void *UNUSED(userdata)) {
    postmessage_utox(FRIEND_TYPING, fid, is_typing, NULL);
    LOG_DEBUG("Tox Callbacks", "Friend\t%u\t--\tTyping:\t%u", fid, is_typing);
}

static void callback_read_receipt(Tox *UNUSED(tox), uint32_t fid, uint32_t receipt, void *UNUSED(userdata)) {
    FRIEND *f = get_friend(fid);
    if (!f) {
        LOG_ERR("Tox Callbacks", "Could not get friend with number: %u", fid);
        return;
    }

    messages_clear_receipt(&f->msg, receipt);
    LOG_INFO("Tox Callbacks", "Friend\t%u\t--\tReceipt:\t%u", fid, receipt);
}

static void callback_connection_status(Tox *tox, uint32_t fid, TOX_CONNECTION status, void *UNUSED(userdata)) {
    FRIEND *f = get_friend(fid);
    if (!f) {
        LOG_ERR("Tox Callbacks", "Could not get friend with number: %u", fid);
        return;
    }

    if (f->online && !status) {
        ft_friend_offline(tox, fid);
        if (f->call_state_self || f->call_state_friend) {
            utox_av_local_disconnect(NULL, fid); /* TODO HACK, toxav doesn't supply a toxav_get_toxav_from_tox() yet. */
        }
    } else if (!f->online && !!status) {
        ft_friend_online(tox, fid);
        /* resend avatar info (in case it changed) */
        /* Avatars must be sent LAST or they will clobber existing file transfers! */
        avatar_on_friend_online(tox, fid);
        friend_notify_status(f, (uint8_t *)f->status_message, f->status_length, S(STATUS_ONLINE));
    }
    postmessage_utox(FRIEND_ONLINE, fid, !!status, NULL);

    if (status == TOX_CONNECTION_UDP) {
        LOG_INFO("Tox Callbacks", "Friend\t%u\t--\tOnline (UDP)", fid);
    } else if (status == TOX_CONNECTION_TCP) {
        LOG_INFO("Tox Callbacks", "Friend\t%u\t--\tOnline (TCP)", fid);
    } else {
        LOG_INFO("Tox Callbacks", "Friend\t%u\t--\tOffline", fid);
        friend_notify_status(f, NULL, 0, S(STATUS_OFFLINE));
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

static void callback_group_invite(Tox *UNUSED(tox), uint32_t friend_number, TOX_CONFERENCE_TYPE type,
                                  const uint8_t *cookie, size_t length, void *UNUSED(userdata))
{
    const uint8_t request_id = group_invite_new(friend_number,
                                                cookie, length,
                                                type == TOX_CONFERENCE_TYPE_AV);

    postmessage_utox(GROUP_INCOMING_REQUEST, request_id, 0, NULL);
    postmessage_audio(UTOXAUDIO_PLAY_NOTIFICATION, NOTIFY_TONE_FRIEND_REQUEST, 0, NULL);
}

static void callback_group_message(Tox *UNUSED(tox), uint32_t gid, uint32_t pid, TOX_MESSAGE_TYPE type,
                                   const uint8_t *message, size_t length, void *UNUSED(userdata)) {
    GROUPCHAT *g = get_group(gid);

    switch (type) {
        case TOX_MESSAGE_TYPE_ACTION: {
            LOG_TRACE("Tox Callbacks", "Group Action (%u, %u): %.*s" , gid, pid, (int)length, message);
            group_add_message(g, pid, message, length, MSG_TYPE_ACTION_TEXT);
            break;
        }
        case TOX_MESSAGE_TYPE_NORMAL: {
            LOG_INFO("Tox Callbacks", "Group Message (%u, %u): %.*s", gid, pid, (int)length, message);
            group_add_message(g, pid, message, length, MSG_TYPE_TEXT);
            break;
        }
    }
    group_notify_msg(g, (const char *)message, length);
    postmessage_utox(GROUP_MESSAGE, gid, pid, NULL);
}

static void callback_group_namelist_change(Tox *tox, uint32_t gid, uint32_t pid, TOX_CONFERENCE_STATE_CHANGE change,
                                           void *UNUSED(userdata)) {
    LOG_ERR("Group callback", "gid %u pid %u change %u", gid, pid, change);
    GROUPCHAT *g = get_group(gid);
    if (!g) {
        LOG_ERR("Tox Callbacks", "Invalid group");
    }

    switch (change) {
        case TOX_CONFERENCE_STATE_CHANGE_PEER_JOIN: {
            LOG_DEBUG("Group", "Add (%u, %u)" , gid, pid);

            if (g->peer) {
                g->peer = realloc(g->peer, sizeof(void *) * (g->peer_count + 2));
            } else {
                g->peer = calloc(g->peer_count + 2, sizeof(void *));
            }
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
            uint32_t name_color = RGB(rand(), rand(), rand());

            group_peer_add(g, pid, is_us, name_color);

            postmessage_utox(GROUP_PEER_ADD, gid, pid, NULL);
            break;
        }

        case TOX_CONFERENCE_STATE_CHANGE_PEER_NAME_CHANGE: {
            LOG_DEBUG("Tox Callbacks", "Group:\tPeer name change (%u, %u)" , gid, pid);

            if (g->peer) {
                if (!g->peer[pid]) {
                    LOG_ERR("Tox Callbacks", "Tox Group:\tERROR, can't sent a name, for non-existant peer!" );
                    break;
                }
            } else {
                // TODO can't happen
                LOG_ERR("Tox Callbacks", "Tox Group:\tERROR, can't sent a name, for non-existant Group!" );
            }

            uint8_t name[TOX_MAX_NAME_LENGTH];
            size_t len = tox_conference_peer_get_name_size(tox, gid, pid, NULL);
            tox_conference_peer_get_name(tox, gid, pid, name, NULL);
            len = utf8_validate(name, len);
            group_peer_name_change(g, pid, name, len);

            postmessage_utox(GROUP_PEER_NAME, gid, pid, NULL);
            break;
        }

        case TOX_CONFERENCE_STATE_CHANGE_PEER_EXIT: {
            LOG_DEBUG("Group", "Peer Quit (%u, %u)" , gid, pid);
            group_add_message(g, pid, (const uint8_t *)S(GROUP_MESSAGE_QUIT), SLEN(GROUP_MESSAGE_QUIT), MSG_TYPE_NOTICE);

            pthread_mutex_lock(&messages_lock); /* make sure that messages has posted before we continue */

            group_reset_peerlist(g);

            uint32_t number_peers = tox_conference_peer_count(tox, gid, NULL);

            g->peer = calloc(number_peers, sizeof(void *));

            if (!g->peer) {
                LOG_FATAL_ERR(EXIT_MALLOC, "Tox Callbacks", "Group:\tToxcore is very broken, but we couldn't alloc here.");
            }

            /* I'm about to break some uTox style here, because I'm expecting
             * the API to change soon, and I just can't when it's this broken */
            for (uint32_t i = 0; i < number_peers; ++i) {
                uint8_t     tmp[TOX_MAX_NAME_LENGTH];
                size_t      len  = tox_conference_peer_get_name_size(tox, gid, i, NULL);
                tox_conference_peer_get_name(tox, gid, i, tmp, NULL);
                GROUP_PEER *peer = calloc(1, len * sizeof(void *) + sizeof(*peer));
                if (!peer) {
                    LOG_FATAL_ERR(EXIT_MALLOC, "Group", "Toxcore is very broken, but we couldn't calloc here.");
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

            postmessage_utox(GROUP_PEER_DEL, gid, pid, NULL);
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
    postmessage_utox(GROUP_TOPIC, gid, length, copy_title);

    LOG_TRACE("Tox Callbacks", "Group Title (%u, %u): %.*s" , gid, pid, (int)length, title);
}

void utox_set_callbacks_groups(Tox *tox) {
    tox_callback_conference_invite(tox, callback_group_invite);
    tox_callback_conference_message(tox, callback_group_message);
    tox_callback_conference_namelist_change(tox, callback_group_namelist_change);
    tox_callback_conference_title(tox, callback_group_topic);
}

#ifdef ENABLE_MULTIDEVICE
static void callback_friend_list_change(Tox *tox, void *user_data) {
    LOG_ERR("Tox Callbacks", "friend list change, updating roster");

    flist_dump_contacts();
    utox_friend_list_init(tox);
    flist_reload_contacts();
}

static void callback_mdev_self_name(Tox *tox, uint32_t dev_num, const uint8_t *name, size_t length,
                                    void *UNUSED(userdata)) {

    LOG_TRACE("Tox Callbacks", "Name changed on remote device %u", dev_num);

    memcpy(self.name, name, length);
    self.name_length = length;

    edit_setstr(&edit_name, self.name, self.name_length);

    postmessage_utox(REDRAW, 0, 0, NULL);
}

typedef void tox_mdev_self_status_message_cb(Tox *tox, uint32_t device_number, const uint8_t *status_message,
                                             size_t len, void *user_data);

static void callback_mdev_self_status_msg(Tox *tox, uint32_t dev_num, const uint8_t *smsg, size_t length,
                                          void *UNUSED(userdata)) {

    LOG_TRACE("Tox Callbacks", "Status Message changed on remote device %u", dev_num);

    memcpy(self.statusmsg, smsg, length);
    self.statusmsg_length = length;

    edit_setstr(&edit_status, self.statusmsg, self.statusmsg_length);

    postmessage_utox(REDRAW, 0, 0, NULL);
}

static void callback_mdev_self_state(Tox *tox, uint32_t device_number, TOX_USER_STATUS state, void *user_data) {
    self.status = state;
}

static void callback_device_sent_message(Tox *tox, uint32_t sending_device, uint32_t target_friend,
                                         TOX_MESSAGE_TYPE type, uint8_t *msg, size_t msg_length) {
    LOG_TRACE("Tox Callbacks", "Message sent from other device %u\n\t\t%.*s" , sending_device, (uint32_t)msg_length, msg);

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
            LOG_ERR("Tox Callbacks", "Message from Friend\t%u\t--\tof unsupported type: %.*s", target_friend, (uint32_t)msg_length, msg);
        }
    }
    friend_notify_msg(&friend[target_friend], msg, msg_length);
    postmessage_utox(FRIEND_MESSAGE_UPDATE, target_friend, 0, NULL);
}

void utox_set_callbacks_mdevice(Tox *tox) {
    tox_callback_friend_list_change(tox, callback_friend_list_change, NULL);

    tox_callback_mdev_self_status_message(tox, callback_mdev_self_status_msg, NULL);
    tox_callback_mdev_self_name(tox, callback_mdev_self_name, NULL);
    tox_callback_mdev_self_state(tox, callback_mdev_self_state, NULL);

    tox_callback_mdev_sent_message(tox, callback_device_sent_message, NULL);
}
#endif
