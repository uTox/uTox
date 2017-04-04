#ifndef FRIEND_H
#define FRIEND_H

#include "messages.h"

#include <tox/tox.h>

typedef struct avatar AVATAR;
typedef struct edit_change EDIT_CHANGE;
typedef struct file_transfer FILE_TRANSFER;
typedef uint8_t *UTOX_IMAGE;
typedef unsigned int ALuint;

typedef struct friend_meta_data {
    uint8_t version;

    uint8_t ft_autoaccept : 1;
    uint8_t ft_autoaccept_path : 1;
    uint8_t skip_msg_logging : 1;
    uint8_t unused : 5;

    uint8_t zero[30];

    size_t alias_length;
    size_t ft_autoaccept_path_length;

    uint8_t data[];
} FRIEND_META_DATA;
#define METADATA_VERSION 0


typedef struct friend_meta_data_old {
    size_t  alias_length;
    uint8_t data[];
} FRIEND_META_DATA_OLD;


typedef struct utox_friend {
    uint8_t cid[TOX_PUBLIC_KEY_SIZE]; // TODO DEPERCATED REMOVE!

    uint8_t id_bin[TOX_PUBLIC_KEY_SIZE];
    char    id_str[TOX_PUBLIC_KEY_SIZE * 2];
    uint8_t number;

    char *name;
    char *alias;
    char *status_message;

    uint8_t *typed;

    size_t name_length;
    size_t alias_length;
    size_t status_length;
    size_t typed_length;

    /* Friend Status */
    uint8_t status;
    bool    online;
    bool    typing;
    bool    video_inline;

    AVATAR *avatar;

    /* Messages */
    bool          skip_msg_logging;
    bool          unread_msg;
    MESSAGES      msg;
    EDIT_CHANGE **edit_history;
    uint16_t      edit_history_cur, edit_history_length;

    /* Audio / Video */
    int32_t  call_state_self, call_state_friend;
    uint16_t video_width, video_height;
    ALuint   audio_dest;

    /* File transfers */
    bool ft_autoaccept;

    FILE_TRANSFER  *ft_incoming;
    uint16_t        ft_incoming_size;
    uint16_t        ft_incoming_active_count;

    FILE_TRANSFER  *ft_outgoing;
    uint16_t        ft_outgoing_size;
    uint16_t        ft_outgoing_active_count;
} FRIEND;

typedef struct utox_friend_request {
    uint16_t number;
    uint8_t  bin_id[TOX_ADDRESS_SIZE];

    char *msg;
    size_t length;
} FREQUEST;


#define UTOX_FRIEND_NAME(f) ((f->alias) ? f->alias : f->name)
#define UTOX_FRIEND_NAME_LENGTH(f) ((f->alias) ? f->alias_length : f->name_length)

/*
 * Gets the friend at position friend_number
 */
FRIEND *get_friend(uint32_t friend_number);

FREQUEST *get_frequest(uint16_t frequest_number);

/* Add a new friend request */
uint16_t friend_request_new(const uint8_t *id, const uint8_t *msg, size_t length);
void friend_request_free(uint16_t number);

/*
 * Frees all of your friends
 */
void free_friends(void);

void utox_friend_init(Tox *tox, uint32_t friend_number);

void utox_friend_list_init(Tox *tox);

void friend_setname(FRIEND *f, uint8_t *name, size_t length);
void friend_set_alias(FRIEND *f, uint8_t *alias, uint16_t length);
void friend_sendimage(FRIEND *f, NATIVE_IMAGE *native_image, uint16_t width, uint16_t height, UTOX_IMAGE png_image,
                      size_t png_size);
void friend_recvimage(FRIEND *f, NATIVE_IMAGE *native_image, uint16_t width, uint16_t height);

void friend_notify_msg(FRIEND *f, const char *msg, size_t msg_length);

/* set friend online status. Returns: true if status changed, false otherwise */
bool friend_set_online(FRIEND *f, bool online);

void friend_set_typing(FRIEND *f, int typing);

void friend_addid(uint8_t *id, char *msg, uint16_t msg_length);
void friend_add(char *name, uint16_t length, char *msg, uint16_t msg_length);

void friend_history_clear(FRIEND *f);

void friend_free(FRIEND *f);

/* Searches for a friend using the specified name */
FRIEND *find_friend_by_name(uint8_t *name);

/* Notifies the user that a friend is online or offline */
void friend_notify_status(FRIEND *f, const uint8_t *msg, size_t msg_length, char *state);

// Saves user meta data to disk
void utox_write_metadata(FRIEND *f);

/** convert string to tox id
 *  on success: returns 1
 *  on failure: returns 0
 *  notes: dest must be TOX_FRIEND_ADDRESS_SIZE bytes large, some data may be written to dest even on failure
 */
bool string_to_id(uint8_t *dest, char *src);

/* same as id_to_string(), but for TOX_PUBLIC_KEY_SIZE
 */
void cid_to_string(char *dest, uint8_t *src);

#endif
