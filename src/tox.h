/* todo: proper system for posting messages to the toxcore thread, comments, better names (?), proper cleanup of a/v and
 * a/v thread*/
/* -proper unpause/pause file transfers, resuming file transfers + what if new file transfer with same id gets created
before the main thread receives the message for the old one?
>= GiB file sizes with FILE_*_PROGRESS on 32bit */

/* details about messages and their (param1, param2, data) values are in the message handlers in tox.c*/
#ifndef UTOX_TOX_H
#define UTOX_TOX_H

#include <tox/tox.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint8_t *UTOX_IMAGE;

typedef struct {
    uint8_t  msg;
    uint32_t param1, param2;
    void *   data;
} TOX_MSG;

typedef enum UTOX_ENC_ERR {
    UTOX_ENC_ERR_NONE,
    UTOX_ENC_ERR_LENGTH,
    UTOX_ENC_ERR_BAD_PASS,
    UTOX_ENC_ERR_BAD_DATA,
    UTOX_ENC_ERR_UNKNOWN
} UTOX_ENC_ERR;

/* toxcore thread messages (sent from the client thread) */
enum {
    /* SHUTDOWNEVERYTHING! */
    TOX_KILL, // 0
    TOX_SAVE,

    /* Change our settings in core */
    TOX_SELF_SET_NAME,
    TOX_SELF_SET_STATUS,
    TOX_SELF_SET_STATE,
    TOX_SELF_CHANGE_NOSPAM,

    TOX_SELF_NEW_DEVICE,

    /* Wooo pixturs */
    TOX_AVATAR_SET,
    TOX_AVATAR_UNSET,

    /* Interact with contacts */
    TOX_FRIEND_NEW,
    TOX_FRIEND_NEW_DEVICE,
    TOX_FRIEND_ACCEPT,
    TOX_FRIEND_DELETE,
    TOX_FRIEND_ONLINE,

    /* Default actions */
    TOX_SEND_MESSAGE,
    TOX_SEND_ACTION, /* Should we deprecate this, now that core uses a single function? */
    TOX_SEND_TYPING,

    /* File Transfers */
    TOX_FILE_ACCEPT,
    TOX_FILE_ACCEPT_AUTO,
    TOX_FILE_SEND_NEW,
    TOX_FILE_SEND_NEW_INLINE,
    TOX_FILE_SEND_NEW_SLASH,

    TOX_FILE_RESUME,
    TOX_FILE_PAUSE,
    TOX_FILE_CANCEL,

    /* Audio/Video Calls */
    TOX_CALL_SEND,
    TOX_CALL_INCOMING,
    TOX_CALL_ANSWER,
    TOX_CALL_PAUSE_AUDIO,
    TOX_CALL_PAUSE_VIDEO,
    TOX_CALL_RESUME_AUDIO,
    TOX_CALL_RESUME_VIDEO,
    TOX_CALL_DISCONNECT,

    TOX_GROUP_CREATE,
    TOX_GROUP_JOIN,
    TOX_GROUP_PART,
    TOX_GROUP_SEND_INVITE,
    TOX_GROUP_SET_TOPIC,
    TOX_GROUP_SEND_MESSAGE,
    TOX_GROUP_SEND_ACTION,
    TOX_GROUP_AUDIO_START,
    TOX_GROUP_AUDIO_END,
};

struct TOX_SEND_INLINE_MSG {
    size_t     image_size;
    UTOX_IMAGE image;
};

/* AV STATUS LIST */
enum {
    UTOX_AV_NONE,
    UTOX_AV_INVITE,
    UTOX_AV_RINGING,
    UTOX_AV_STARTED,
};

typedef enum {
    // tox_thread is not initialized yet
    UTOX_TOX_THREAD_INIT_NONE = 0,
    // tox_thread is initialized successfully
    // this means a tox instance has been created
    UTOX_TOX_THREAD_INIT_SUCCESS = 1,
    // tox_thread is initialized but not successfully
    // this means a tox instance may have not been created
    UTOX_TOX_THREAD_INIT_ERROR = 2,
} UTOX_TOX_THREAD_INIT;

UTOX_TOX_THREAD_INIT tox_thread_init;

/* Inter-thread communication vars. */
TOX_MSG       tox_msg, audio_msg, video_msg, toxav_msg;
volatile bool tox_thread_msg, audio_thread_msg, video_thread_msg;

bool tox_connected;
char proxy_address[256]; /* Magic Number inside toxcore */

void tox_after_load(Tox *tox);

/* toxcore thread
 */
void toxcore_thread(void *args);

/* send a message to the toxcore thread
 */
void postmessage_toxcore(uint8_t msg, uint32_t param1, uint32_t param2, void *data);

void tox_settingschanged(void);

/* convert tox id to string
 *  notes: dest must be (TOX_FRIEND_ADDRESS_SIZE * 2) bytes large, src must be TOX_FRIEND_ADDRESS_SIZE bytes large
 */
void id_to_string(char *dest, uint8_t *src);

#endif
