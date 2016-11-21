// groups.c

#include "groups.h"

#include "flist.h"
#include "main.h"
#include "main_native.h"

void group_init(GROUPCHAT *g, uint32_t group_number, bool av_group) {
    pthread_mutex_lock(&messages_lock); /* make sure that messages has posted before we continue */
    if (!g->peer) {
        g->peer = calloc(MAX_GROUP_PEERS, sizeof(void));
    }

    g->name_length = snprintf((char *)g->name, sizeof(g->name), "Groupchat #%u", group_number);
    if (g->name_length >= sizeof(g->name)) {
        g->name_length = sizeof(g->name) - 1;
    }
    if (av_group) {
        g->topic_length = sizeof("Error creating voice group, not supported yet") - 1;
        strcpy2(g->topic, "Error creating voice group, not supported yet");
    } else {
        g->topic_length = sizeof("Drag friends to invite them") - 1;
        memcpy(g->topic, "Drag friends to invite them", sizeof("Drag friends to invite them") - 1);
    }

    g->msg.scroll               = 1.0;
    g->msg.panel.type           = PANEL_MESSAGES;
    g->msg.panel.content_scroll = &scrollbar_group;
    g->msg.panel.y              = MAIN_TOP;
    g->msg.panel.height         = CHAT_BOX_TOP;
    g->msg.panel.width          = -SCROLL_WIDTH;
    g->msg.is_groupchat         = 1;

    g->number   = group_number;
    g->notify   = settings.group_notifications;
    g->av_group = av_group;
    pthread_mutex_unlock(&messages_lock); /* make sure that messages has posted before we continue */

    flist_addgroup(g);
    flist_select_last();
}

uint32_t group_add_message(GROUPCHAT *g, int peer_id, const uint8_t *message, size_t length, uint8_t m_type) {
    pthread_mutex_lock(&messages_lock); /* make sure that messages has posted before we continue */
    MESSAGES *  m    = &g->msg;
    GROUP_PEER *peer = g->peer[peer_id];
    uint8_t *   nick = peer->name;

    MSG_GROUP *msg     = calloc(1, sizeof(*msg) + (sizeof(void *) * (length + peer->name_length)));
    msg->our_msg       = (g->our_peer_number == peer_id ? 1 : 0);
    msg->msg_type      = m_type;
    msg->length        = length;
    msg->author_id     = peer_id;
    msg->author_length = peer->name_length;
    msg->author_color  = peer->name_color;
    time(&msg->time);

    memcpy(msg->msg, nick, peer->name_length);
    memcpy(msg->msg + peer->name_length, message, length);

    pthread_mutex_unlock(&messages_lock);
    return message_add_group(m, (void *)msg);
}

void group_peer_add(GROUPCHAT *g, uint32_t peer_id, bool our_peer_number, uint32_t name_color) {
    pthread_mutex_lock(&messages_lock); /* make sure that messages has posted before we continue */
    if (!g->peer) {
        g->peer = calloc(MAX_GROUP_PEERS, sizeof(void));
        debug_notice("Groupchat:\tNeeded to calloc peers for this group chat. (%u)\n", peer_id);
    }

    GROUP_PEER *peer = (void *)g->peer[peer_id];

    /* I don't want to comment out this, because it might cause a memleak, but I really have no choice because of
     * how I have to realloc to work around current group chats. TODO & FIXME: this may leak */
    // if (peer) {
    //     free(peer);
    // }

    peer = calloc(1, sizeof(*peer) + sizeof(void) * 10);
    strcpy2(peer->name, "<unknown>");
    peer->name_length = 0;
    peer->name_color  = name_color;
    peer->id          = peer_id;

    g->peer[peer_id] = peer;
    g->peer_count++;
    pthread_mutex_unlock(&messages_lock);
}

void group_peer_del(GROUPCHAT *g, uint32_t peer_id) {
    group_add_message(g, peer_id, (const uint8_t *)"<- has Quit!", 12, MSG_TYPE_NOTICE);

    pthread_mutex_lock(&messages_lock); /* make sure that messages has posted before we continue */

    if (!g->peer) {
        debug("Groupchat:\tUnable to del peer from NULL group\n");
    }

    GROUP_PEER *peer = (void *)g->peer[peer_id];

    if (peer) {
        debug("Freeing peer %u, name %.*s\n", peer_id, (int)peer->name_length, peer->name);
        free(peer);
    } else {
        debug("Groupchat:\tUnable to find peer for deletion\n");
        return;
    }
    g->peer_count--;
    g->peer[peer_id] = NULL;
    pthread_mutex_unlock(&messages_lock);
}

void group_peer_name_change(GROUPCHAT *g, uint32_t peer_id, const uint8_t *name, size_t length) {
    pthread_mutex_lock(&messages_lock); /* make sure that messages has posted before we continue */
    if (!g->peer) {
        debug("Groupchat:\tUnable to add peer to NULL group\n");
        return;
    }

    GROUP_PEER *peer = g->peer[peer_id];

    if (peer && peer->name_length) {
        uint8_t old[TOX_MAX_NAME_LENGTH];
        uint8_t msg[TOX_MAX_NAME_LENGTH];
        size_t  size = 0;

        memcpy(old, peer->name, peer->name_length);
        size = snprintf((void *)msg, TOX_MAX_NAME_LENGTH, "<- has changed their name from %.*s", (int)peer->name_length,
                        old);
        peer = realloc(peer, sizeof(GROUP_PEER) + sizeof(void) * length);

        if (peer) {
            peer->name_length = utf8_validate(name, length);
            memcpy(peer->name, name, length);
            g->peer[peer_id] = peer;
            pthread_mutex_unlock(&messages_lock);
            group_add_message(g, peer_id, msg, size, MSG_TYPE_NOTICE);
            return;
        } else {
            debug("Fatal error:\t couldn't realloc for group peer name!\n");
            exit(40);
        }

    } else if (peer) {
        /* Hopefully, they just joined, becasue that's the UX message we're going with! */
        peer = realloc(peer, sizeof(GROUP_PEER) + sizeof(void) * length);
        if (peer) {
            peer->name_length = utf8_validate(name, length);
            memcpy(peer->name, name, length);
            g->peer[peer_id] = peer;
            pthread_mutex_unlock(&messages_lock);
            group_add_message(g, peer_id, (const uint8_t *)"<- has joined the chat!", 23, MSG_TYPE_NOTICE);
            return;
        }
    } else {
        debug("Fatal error:\t we can't set a name for a null peer! %u\n", peer_id);
        exit(41);
    }
}

void group_reset_peerlist(GROUPCHAT *g) {
    /* ARE YOU KIDDING... WHO THOUGHT THIS API WAS OKAY?! */
    for (size_t i = 0; i < g->peer_count; ++i) {
        if (g->peer[i]) {
            free(g->peer[i]);
        }
    }
    free(g->peer);
}

void group_free(GROUPCHAT *g) {
    for (size_t i = 0; i < g->edit_history_length; ++i) {
        free(g->edit_history[i]);
    }

    free(g->edit_history);

    group_reset_peerlist(g);

    for (size_t i = 0; i < g->msg.number; ++i) {
        message_free((void *)g->msg.data[i]);
    }
    free(g->msg.data);

    memset(g, 0, sizeof(GROUPCHAT));
}


void group_notify_msg(GROUPCHAT *g, const uint8_t *msg, size_t msg_length) {
    if (g->notify == GNOTIFY_NEVER) {
        return;
    }

    if (g->notify == GNOTIFY_HIGHLIGHTS && strstr((const char *)msg, (const char *)self.name) == NULL) {
        return;
    }

    uint8_t title[g->name_length + 25];

    size_t title_length =
        snprintf((char *)title, g->name_length + 25, "uTox new message in %.*s", (int)g->name_length, g->name);

    notify(title, title_length, msg, msg_length, g, 1);

    if (flist_get_selected()->data != g) {
        postmessage_audio(UTOXAUDIO_PLAY_NOTIFICATION, NOTIFY_TONE_FRIEND_NEW_MSG, 0, NULL);
    }
}
