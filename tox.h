/* todo: proper system for posting messages to the toxcore thread, comments, better names (?), proper cleanup of a/v and a/v thread*/
/* -proper unpause/pause file transfers, resuming file transfers + what if new file transfer with same id gets created before the main thread receives the message for the old one?
>= GiB file sizes with FILE_*_PROGRESS on 32bit */

/* details about messages and their (param1, param2, data) values are in the message handlers in tox.c*/

typedef struct {
    uint8_t msg;
    uint32_t param1, param2;
    void *data;
} TOX_MSG;

/* toxcore thread messages (sent from the client thread) */
enum {
    /* SHUTDOWNEVERYTHING! */
    TOX_KILL,

    /* Change our settings in core */
    TOX_SELF_SET_NAME,
    TOX_SELF_SET_STATUS,
    TOX_SELF_SET_STATE,

    /* Wooo pixturs */
    TOX_AVATAR_SET,
    TOX_AVATAR_UNSET, // 5

    /* Interact with contacts */
    TOX_FRIEND_NEW,
    TOX_FRIEND_ACCEPT,
    TOX_FRIEND_DELETE,
    TOX_FRIEND_ONLINE,

    /* Default actions */
    TOX_SEND_MESSAGE, // 10
    TOX_SEND_ACTION, /* Should we deprecate this, now that core uses a single function? */
    TOX_SEND_TYPING,

    /* File Transfers */
    TOX_FILE_ACCEPT,
    TOX_FILE_SEND_NEW,
    TOX_FILE_SEND_NEW_INLINE, // 15
    TOX_FILE_SEND_NEW_SLASH,

    TOX_FILE_RESUME,
    TOX_FILE_PAUSE,
    TOX_FILE_CANCEL,

    /* Audio/Video Calls */
    TOX_CALL_SEND, // 20
    TOX_CALL_INCOMING,
    TOX_CALL_ANSWER,
    TOX_CALL_PAUSE_AUDIO,
    TOX_CALL_PAUSE_VIDEO,
    TOX_CALL_RESUME_AUDIO, // 25
    TOX_CALL_RESUME_VIDEO,
    TOX_CALL_DISCONNECT,

    TOX_GROUP_CREATE,
    TOX_GROUP_JOIN,
    TOX_GROUP_PART, // 30
    TOX_GROUP_SEND_INVITE,
    TOX_GROUP_SET_TOPIC,
    TOX_GROUP_SEND_MESSAGE,
    TOX_GROUP_SEND_ACTION,
    TOX_GROUP_AUDIO_START, // 35
    TOX_GROUP_AUDIO_END,
};

/* uTox client thread messages (received by the client thread) */
enum {
    /* General core and networking messages */
    TOX_DONE, // 0
    DHT_CONNECTED,
    DNS_RESULT,

    /* OS interaction/integration messages*/
    AUDIO_IN_DEVICE,
    AUDIO_OUT_DEVICE,
    VIDEO_IN_DEVICE,

    /* Client/User Interface messages. */
    REDRAW,
    TOOLTIP_SHOW,
    SELF_AVATAR_SET,

    /* File transfer messages */
    FILE_SEND_NEW,
    FILE_INCOMING_NEW, // 10
    FILE_INCOMING_ACCEPT,
    FILE_UPDATE_STATUS,
    FILE_INLINE_IMAGE,

    /* Friend interaction messages. */
    /* Handshake */
    FRIEND_ONLINE,
    FRIEND_NAME,
    FRIEND_STATUS_MESSAGE,
    FRIEND_STATE,
    FRIEND_AVATAR_SET,
    FRIEND_AVATAR_UNSET,
    /* Interactions */
    FRIEND_TYPING, // 20
    FRIEND_MESSAGE,
    /* Adding and deleting */
    FRIEND_INCOMING_REQUEST,
    FRIEND_ACCEPT_REQUEST,
    FRIEND_SEND_REQUEST,
    FRIEND_REMOVE,

    /* Audio & Video calls, */
    AV_CALL_INCOMING,
    AV_CALL_RINGING,
    AV_CALL_ACCEPTED,
    AV_CALL_DISCONNECTED,
    AV_VIDEO_FRAME, // 30
    AV_CLOSE_WINDOW,

    /* Group interactions, commented out for the new groupchats (coming soon maybe?) */
    GROUP_ADD,
    GROUP_MESSAGE,
    GROUP_PEER_ADD,
    GROUP_PEER_DEL,
    GROUP_PEER_NAME,
    GROUP_TOPIC,
    GROUP_AUDIO_START,
    GROUP_AUDIO_END,
    GROUP_UPDATE,
};

struct TOX_SEND_INLINE_MSG {
    size_t image_size;
    UTOX_PNG_IMAGE image;
};

/* AV STATUS LIST */
enum {
    UTOX_AV_NONE,
    UTOX_AV_INVITE,
    UTOX_AV_RINGING,
    UTOX_AV_STARTED,
};

/* Inter-thread communication vars. */
TOX_MSG tox_msg, audio_msg, video_msg, toxav_msg;
volatile _Bool tox_thread_msg, audio_thread_msg, video_thread_msg, toxav_thread_msg;
volatile _Bool save_needed;

/** [log_read description] */
void log_read(Tox *tox, int fid);

/** [friend_meta_data_read description] */
void friend_meta_data_read(Tox *tox, int friend_id);

/** [init_avatar description]
 *
 * TODO move this to avatar.h
 */
//_Bool init_avatar(AVATAR *avatar, const char_t *id, uint8_t *png_data_out, uint32_t *png_size_out);

/* toxcore thread
 */
void tox_thread(void *args);

/* send a message to the toxcore thread
 */
void tox_postmessage(uint8_t msg, uint32_t param1, uint32_t param2, void *data);

/* send a message to the audio thread
 */
void toxaudio_postmessage(uint8_t msg, uint32_t param1, uint32_t param2, void *data);

/* send a message to the video thread
 */
void toxvideo_postmessage(uint8_t msg, uint32_t param1, uint32_t param2, void *data);

/* send a message to the toxav thread
 */
void toxav_postmessage(uint8_t msg, uint32_t param1, uint32_t param2, void *data);

/* read a message sent from the toxcore thread (sent with postmessage())
 */
void tox_message(uint8_t msg, uint16_t param1, uint16_t param2, void *data);

void tox_settingschanged(void);
