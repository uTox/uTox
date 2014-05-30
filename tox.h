/* todo: proper system for posting messages to the toxcore thread, comments, better names (?) */

/* toxcore thread messages
 */
enum {
    TOX_SETNAME,
    TOX_SETSTATUSMSG,
    TOX_SETSTATUS,
    TOX_ADDFRIEND,
    TOX_DELFRIEND,
    TOX_ACCEPTFRIEND,
    TOX_SENDMESSAGE,
    TOX_SENDMESSAGEGROUP,
    TOX_CALL,
    TOX_ACCEPTCALL,
    TOX_HANGUP,
    TOX_NEWGROUP,
    TOX_LEAVEGROUP,
    TOX_GROUPINVITE,
};

/* client thread messages
 */
enum {
    FRIEND_REQUEST,
    FRIEND_ACCEPT,
    FRIEND_ADD,
    FRIEND_MESSAGE,
    FRIEND_NAME,
    FRIEND_STATUS_MESSAGE,
    FRIEND_STATUS,
    FRIEND_TYPING,
    FRIEND_ONLINE,

    FRIEND_CALL_INVITE,
    FRIEND_CALL_START,
    FRIEND_CALL_RING,
    FRIEND_CALL_END,

    GROUP_ADD,
    GROUP_MESSAGE,
    GROUP_PEER_ADD,
    GROUP_PEER_DEL,
    GROUP_PEER_NAME

};

/* toxcore thread
 */
void tox_thread(void *args);

/* send a message to the toxcore thread
 */
void tox_postmessage(uint8_t msg, uint16_t param1, uint16_t param2, void *data);

/* read a message sent from the toxcore thread (sent with postmessage())
 */
void tox_message(uint8_t msg, uint16_t param1, uint16_t param2, void *data);
