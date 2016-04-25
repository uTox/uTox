#include "main.h"

void group_init(GROUPCHAT *g, uint32_t group_number, _Bool av_group) {
    if (!g->peer) {
        g->peer = calloc(MAX_GROUP_PEERS, sizeof(void));
    }

    g->name_length = snprintf((char*)g->name, sizeof(g->name), "Groupchat #%u", group_number);
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

    g->av_group                 = av_group;

    list_addgroup(g);
    roster_select_last();
}

void group_add_message(GROUPCHAT *g, int peer_id, const uint8_t *message, size_t length) {
    MESSAGES *m = &g->msg;

    MSG_TEXT *msg   = calloc(1, sizeof(MSG_TEXT) + length);
    time(&msg->time);
    msg->author     = (g->our_peer_number == peer_id ? 1 : 0);
    msg->msg_type   = MSG_TYPE_TEXT;
    msg->length     = length;
    msg->author_id  = peer_id;
    memcpy(msg->msg, message, length);

    message_add_group(m, msg);
}

void group_peer_add(GROUPCHAT *g, uint32_t peer_id, const uint8_t *name, size_t length, _Bool our_peer_number) {
    if (!g->peer) {
        g->peer = calloc(MAX_GROUP_PEERS, sizeof(void));
        // debug("Groupchat:\tUnable to add peer to NULL group\n");
    }

    GROUP_PEER *peer = (void*)g->peer[peer_id];

    if (peer) {
        free(peer);
    }

    peer = calloc(1, sizeof(*peer) + sizeof(void) * length);
    memcpy(peer->name, "<unknown>", length);
    peer->name_length = length;
    peer->name_color  = rand() % UINT32_MAX;

    g->peer_count++;
    g->peer[peer_id] = peer;
}

void group_peer_name_change(GROUPCHAT *g, uint32_t peer_id, const uint8_t *name, size_t length) {
    if (!g->peer) {
        debug("Groupchat:\tUnable to add peer to NULL group\n");
        return;
    }

    GROUP_PEER *peer = g->peer[peer_id];

    if (peer) {
        peer = realloc(peer, sizeof(GROUP_PEER) + sizeof(void) * length);
    } else {
        peer = calloc(1, sizeof(GROUP_PEER) + sizeof(void) * length);
    }

    if (peer) {
        peer->name_length = length;
        memcpy(peer->name, name, length);
    } else {
        debug("Fatal error:\t couldn't alloc for group peer name!\n");
        exit(40);
    }
}

void group_free(GROUPCHAT *g) {
    uint16_t i = 0;
    while(i != g->edit_history_length) {
        free(g->edit_history[i]);
        i++;
    }
    free(g->edit_history);

    uint32_t j = 0;
    while (j < g->peer_count) {
        if (g->peer[j]) {
            free(g->peer[j]);
        }
        j++;
    }

    uint32_t k = 0;
    while (k < g->msg.number) {
        free(g->msg.data[k]);
        k++;
    }

    free(g->msg.data);

    memset(g, 0, sizeof(GROUPCHAT));
}
