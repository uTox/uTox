#ifndef GROUPS_H
#define GROUPS_H

#include "messages.h"

typedef unsigned int ALuint;
typedef struct edit_change EDIT_CHANGE;

#define UTOX_MAX_GROUP_PEERS 256

/*  UTOX_SAVE limits 8 as the max */
typedef enum {
    GNOTIFY_NEVER,      /* 0: never send notifications, */
    GNOTIFY_HIGHLIGHTS, /* 1: only send when mentioned, */
    GNOTIFY_ALWAYS,     /* 2: always send notifications */
} GNOTIFY_TYPE;

typedef struct group_peer {
    uint32_t id;
    uint32_t name_color;
    size_t name_length;
    uint8_t name[];
} GROUP_PEER;

typedef struct groupchat {
    bool     audio_calling, unread_msg;
    uint32_t our_peer_number;

    uint8_t number;
    uint8_t av_group;

    GNOTIFY_TYPE notify;

    bool   muted;
    ALuint audio_dest;

    char     name[128];
    uint16_t name_length;
    char     topic[256]; /* TODO magic numbers */
    uint16_t topic_length;
    uint16_t typed_length;

    char *typed;

    /* Audio sources */
    unsigned int source[UTOX_MAX_GROUP_PEERS];
    volatile uint64_t
        last_recv_audio[UTOX_MAX_GROUP_PEERS]; /* TODO: thread safety (This should work fine but it isn't very clean.) */

    MESSAGES      msg;
    EDIT_CHANGE **edit_history;
    uint16_t      edit_history_cur, edit_history_length;

    uint32_t peer_count;
    GROUP_PEER **peer;
} GROUPCHAT;

/* Initialize a new groupchat */
void group_init(GROUPCHAT *g, uint32_t group_number, bool av_group);

// Returns the message number on success, returns UINT32_MAX on failure.
uint32_t group_add_message(GROUPCHAT *g, uint32_t peer_id, const uint8_t *message, size_t length, uint8_t m_type);

/* Add a peer to a group */
void group_peer_add(GROUPCHAT *g, uint32_t peer_id, bool our_peer_number, uint32_t name_color);

/* Delete a peer from a group */
void group_peer_del(GROUPCHAT *g, uint32_t peer_id);

/* Updates the peers name */
void group_peer_name_change(GROUPCHAT *g, uint32_t peer_id, const uint8_t *name, size_t length);

/* Frees every peer */
void group_reset_peerlist(GROUPCHAT *g);

/* Frees a group */
void group_free(GROUPCHAT *g);

/* Creates a notification for messages received  */
void group_notify_msg(GROUPCHAT *g, const char *msg, size_t length);

/* Gets the group qt the specified index */
GROUPCHAT *get_group(uint32_t group_number);

/* Free all groups */
void raze_groups(void);

/*
 * Initalize the groupchats array
 * This function should only be called once at startup after tox and the self struct have been setup
 */
void init_groups(void);

/**/
bool group_create(uint32_t group_number, bool av_group);

#endif
