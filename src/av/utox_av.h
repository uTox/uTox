/* toxav thread messages (sent from the client thread to the audio or video thread) */
#ifndef UTOX_AV_H
#define UTOX_AV_H

#include <tox/toxav.h> // if it weren't for TOXAV_CALL_CONTROL we could move this to the .c

#include <stdint.h>

#define UTOX_MAX_CALLS 16
// UTOX_MAX_VIDEO_CALLS is never used. Remove?
#define UTOX_MAX_VIDEO_CALLS 32
/* utox av thread commands */
enum {
    UTOXAV_KILL,

    UTOXAV_INCOMING_CALL_PENDING,
    UTOXAV_INCOMING_CALL_ANSWER,
    UTOXAV_INCOMING_CALL_REJECT,

    UTOXAV_OUTGOING_CALL_PENDING,
    UTOXAV_OUTGOING_CALL_ACCEPTED,
    UTOXAV_OUTGOING_CALL_REJECTED,

    UTOXAV_CALL_END,

    UTOXAV_GROUPCALL_START,
    UTOXAV_GROUPCALL_END,

    UTOXAV_START_AUDIO,
    UTOXAV_STOP_AUDIO,

    UTOXAV_START_VIDEO,
    UTOXAV_STOP_VIDEO,

    UTOXAV_SET_AUDIO_IN,
    UTOXAV_SET_AUDIO_OUT,

    UTOXAV_SET_VIDEO_IN,
    UTOXAV_SET_VIDEO_OUT,
};

enum {
    // kill the video thread
    UTOXVIDEO_KILL,
    /*    UTOXVIDEO_RECORD_START,
    UTOXVIDEO_RECORD_STOP,
    UTOXVIDEO_SET,
    UTOXVIDEO_PREVIEW_START,
    UTOXVIDEO_PREVIEW_STOP,
*/
};

/* send a message to the toxav thread
 */
void postmessage_utoxav(uint8_t msg, uint32_t param1, uint32_t param2, void *data);

void utox_av_ctrl_thread(void *args);

void utox_av_local_disconnect(ToxAV *av, int32_t friend_number);

void utox_av_local_call_control(ToxAV *av, uint32_t friend_number, TOXAV_CALL_CONTROL control);

void set_av_callbacks(ToxAV *av);

// void callback_av_group_audio(Tox *tox, int groupnumber, int peernumber, const int16_t *pcm, unsigned int samples,
//                                     uint8_t channels, unsigned int sample_rate, void *userdata);
// void group_av_peer_add(GROUPCHAT *g, int peernumber);
// void group_av_peer_remove(GROUPCHAT *g, int peernumber);

#endif
