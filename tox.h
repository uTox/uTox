/* todo: proper system for posting messages to the toxcore thread, comments, better names (?), proper cleanup of a/v and a/v thread*/
/* -proper unpause/pause file transfers, resuming file transfers + what if new file transfer with same id gets created before the main thread receives the message for the old one?
>= GiB file sizes with FILE_*_PROGRESS on 32bit */

/* details about messages and their (param1, param2, data) values are in the message handlers in tox.c*/

/* toxcore thread messages (sent from the client thread)
 */
enum {
    TOX_KILL,

    TOX_SETNAME,
    TOX_SETAVATAR,
    TOX_UNSETAVATAR,
    TOX_SETSTATUSMSG,
    TOX_SETSTATUS,
    TOX_ADDFRIEND,
    TOX_DELFRIEND,
    TOX_ACCEPTFRIEND,
    TOX_SENDMESSAGE,
    TOX_SENDACTION, //10
    TOX_SENDMESSAGEGROUP,
    TOX_SENDACTIONGROUP,
    TOX_SET_TYPING,
    TOX_CALL,
    TOX_CALL_VIDEO,
    TOX_CALL_VIDEO_ON,
    TOX_CALL_VIDEO_OFF,
    TOX_CANCELCALL,
    TOX_ACCEPTCALL,
    TOX_HANGUP, //20
    TOX_NEWGROUP,
    TOX_LEAVEGROUP,
    TOX_GROUPINVITE,
    TOX_GROUPCHANGETOPIC,
    TOX_GROUP_AUDIO_START,
    TOX_GROUP_AUDIO_END,

    TOX_SEND_NEW_FILE,
    TOX_SEND_NEW_INLINE,
    TOX_ACCEPTFILE, //30
    TOX_FILE_START_TEMP,
    TOX_FILE_INCOMING_RESUME,
    TOX_FILE_INCOMING_PAUSE,
    TOX_FILE_INCOMING_CANCEL,
    TOX_FILE_OUTGOING_RESUME,
    TOX_FILE_OUTGOING_PAUSE,
    TOX_FILE_OUTGOING_CANCEL,
    TOX_FRIEND_ONLINE,
};

struct TOX_SEND_INLINE_MSG {
    size_t image_size;
    UTOX_PNG_IMAGE image;
};

/* toxav thread messages (sent from the client thread)
 */
enum
{
    AUDIO_KILL,
    AUDIO_SET_INPUT,
    AUDIO_SET_OUTPUT,
    AUDIO_PREVIEW_START,
    AUDIO_PREVIEW_END,
    AUDIO_CALL_START,
    AUDIO_CALL_END,
    AUDIO_PLAY_RINGTONE,
    AUDIO_STOP_RINGTONE,
    GROUP_AUDIO_CALL_START,
    GROUP_AUDIO_CALL_END,
};

enum
{
    VIDEO_KILL,
    VIDEO_SET,
    VIDEO_PREVIEW_START,
    VIDEO_PREVIEW_END,
    VIDEO_CALL_START,
    VIDEO_CALL_END,
};


enum
{
    TOXAV_KILL,
};
/* client thread messages (recieved by the client thread)
 */
enum {
    /* general messages */
    TOX_DONE,
    DHT_CONNECTED,
    DNS_RESULT,

    SET_AVATAR,

    SEND_FILES,
    SAVE_FILE,
    FILE_START_TEMP,
    FILE_ABORT_TEMP,

    NEW_AUDIO_IN_DEVICE,
    NEW_AUDIO_OUT_DEVICE,
    NEW_VIDEO_DEVICE,

    /* friend related */
    FRIEND_REQUEST,
    FRIEND_ACCEPT,
    FRIEND_ADD,
    FRIEND_DEL,
    FRIEND_MESSAGE,
    FRIEND_NAME,
    FRIEND_SETAVATAR,
    FRIEND_UNSETAVATAR,
    FRIEND_STATUS_MESSAGE,
    FRIEND_STATUS,
    FRIEND_TYPING,
    FRIEND_ONLINE,

    /* friend a/v */
    FRIEND_CALL_STATUS,
    FRIEND_CALL_VIDEO,
    FRIEND_CALL_MEDIACHANGE,
    FRIEND_CALL_START_VIDEO,
    FRIEND_CALL_STOP_VIDEO,
    FRIEND_VIDEO_FRAME,
    PREVIEW_FRAME,
    PREVIEW_FRAME_NEW,

    /* friend file */
    FRIEND_FILE_NEW,
    FRIEND_FILE_UPDATE,

    /* group */
    GROUP_ADD,
    GROUP_MESSAGE,
    GROUP_PEER_ADD,
    GROUP_PEER_DEL,
    GROUP_PEER_NAME,
    GROUP_TITLE,
    GROUP_AUDIO_START,
    GROUP_AUDIO_END,
    GROUP_UPDATE,

    TOOLTIP_SHOW,
};

enum
{
    CALL_NONE,
    CALL_INVITED,
    CALL_RINGING,
    CALL_OK,
    CALL_NONE_VIDEO, //not used
    CALL_INVITED_VIDEO,
    CALL_RINGING_VIDEO,
    CALL_OK_VIDEO,
};
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
