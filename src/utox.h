#ifndef UTOX_H
#define UTOX_H

#include <inttypes.h>

/* uTox client thread messages (received by the client thread) */
typedef enum utox_msg_id {
    /* General core and networking messages */
    TOX_DONE, // 0
    DHT_CONNECTED,
    DNS_RESULT,

    /* OS interaction/integration messages*/
    AUDIO_IN_DEVICE,
    AUDIO_OUT_DEVICE,

    /* Client/User Interface messages. */
    REDRAW,
    TOOLTIP_SHOW,
    SELF_AVATAR_SET,
    UPDATE_TRAY,
    PROFILE_DID_LOAD,

    /* File transfer messages */
    FILE_SEND_NEW,
    FILE_INCOMING_NEW,
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
    FRIEND_TYPING,
    FRIEND_MESSAGE,
    FRIEND_MESSAGE_UPDATE,
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
    AV_VIDEO_FRAME,
    AV_INLINE_FRAME,
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
} UTOX_MSG;

void postmessage_utox(UTOX_MSG msg, uint16_t param1, uint16_t param2, void *data);

void utox_message_dispatch(UTOX_MSG utox_msg_id, uint16_t param1, uint16_t param2, void *data);

#endif
