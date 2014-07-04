/* todo: proper system for posting messages to the toxcore thread, comments, better names (?), proper cleanup of a/v and a/v thread*/
/* -proper unpause/pause file transfers, resuming file transfers + what if new file transfer with same id gets created before the main thread receives the message for the old one?
>= GiB file sizes with FILE_*_PROGRESS on 32bit */

/* details about messages and their (param1, param2, data) values are in the message handlers in tox.c*/

/* toxcore thread messages (sent from the client thread)
 */
enum {
    TOX_KILL,

    TOX_SETNAME,
    TOX_SETSTATUSMSG,
    TOX_SETSTATUS,
    TOX_ADDFRIEND,
    TOX_DELFRIEND,
    TOX_ACCEPTFRIEND,
    TOX_SENDMESSAGE,
    TOX_SENDMESSAGEGROUP,
    TOX_CALL,
    TOX_CALL_VIDEO,
    TOX_ACCEPTCALL,
    TOX_HANGUP,
    TOX_NEWGROUP,
    TOX_LEAVEGROUP,
    TOX_GROUPINVITE,

    TOX_SENDFILES,
    TOX_ACCEPTFILE,
    TOX_FILE_IN_CANCEL,
    TOX_FILE_OUT_CANCEL,
    TOX_FILE_IN_PAUSE,
    TOX_FILE_OUT_PAUSE,
    TOX_FILE_IN_RESUME,
    TOX_FILE_OUT_RESUME,
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

/* client thread messages (recieved by the client thread)
 */
enum {
    /* general messages */
    TOX_DONE,
    DHT_CONNECTED,
    DNS_RESULT,

    OPEN_FILES,
    SAVE_FILE,

    NEW_AUDIO_IN_DEVICE,
    NEW_AUDIO_OUT_DEVICE,
    NEW_VIDEO_DEVICE,

    /* friend related */
    FRIEND_REQUEST,
    FRIEND_ACCEPT,
    FRIEND_ADD,
    FRIEND_MESSAGE,
    FRIEND_NAME,
    FRIEND_STATUS_MESSAGE,
    FRIEND_STATUS,
    FRIEND_TYPING,
    FRIEND_ONLINE,

    /* friend a/v */
    FRIEND_CALL_STATUS,
    FRIEND_CALL_START_VIDEO,
    FRIEND_VIDEO_FRAME,
    PREVIEW_FRAME,
    PREVIEW_FRAME_NEW,

    /* friend file */
    FRIEND_FILE_IN_NEW,
    FRIEND_FILE_OUT_NEW,
    FRIEND_FILE_IN_STATUS,
    FRIEND_FILE_OUT_STATUS,
    FRIEND_FILE_IN_DONE,
    FRIEND_FILE_OUT_DONE,
    FRIEND_FILE_IN_PROGRESS,
    FRIEND_FILE_OUT_PROGRESS,

    /* group */
    GROUP_ADD,
    GROUP_MESSAGE,
    GROUP_PEER_ADD,
    GROUP_PEER_DEL,
    GROUP_PEER_NAME,
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
void tox_postmessage(uint8_t msg, uint16_t param1, uint16_t param2, void *data);

/* send a message to the audio thread
 */
void toxaudio_postmessage(uint8_t msg, uint16_t param1, uint16_t param2, void *data);

/* send a message to the video thread
 */
void toxvideo_postmessage(uint8_t msg, uint16_t param1, uint16_t param2, void *data);

/* read a message sent from the toxcore thread (sent with postmessage())
 */
void tox_message(uint8_t msg, uint16_t param1, uint16_t param2, void *data);
