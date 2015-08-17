#include "main.h"

void toxav_postmessage(uint8_t msg, uint32_t param1, uint32_t param2, void *data) {
    while(toxav_thread_msg) {
        yieldcpu(1);
    }

    toxav_msg.msg = msg;
    toxav_msg.param1 = param1;
    toxav_msg.param2 = param2;
    toxav_msg.data = data;

    toxav_thread_msg = 1;
}

void toxav_thread(void *args) {
    ToxAV *av = args;

    toxav_thread_init = 1;

    debug("Toxav thread init\n");
    while (1) {
        if(toxav_thread_msg) {
            TOX_MSG *msg = &toxav_msg;
            if(msg->msg == TOXAV_KILL) {
                break;
            }
            /*
            switch(m->msg) {
            }*/
            toxav_thread_msg = 0;
        }

        toxav_iterate(av);
        yieldcpu(toxav_iteration_interval(av));
    }

    debug("Toxav:\tClean thread exit!\n");
    toxav_thread_msg = 0;
    toxav_thread_init = 0;
}

static void utox_av_incoming_call(ToxAV *av, uint32_t friend_number, bool audio, bool video, void *UNUSED(userdata)) {
    debug("A/V Invite (%u)\n", friend_number);
    FRIEND *f = &friend[friend_number];

    f->call_state_self = 0;
    f->call_state_friend = ( audio << 2 | video << 3 | audio << 4 | video << 5 );
    debug("uTox AV:\tcall friend (%u) state for incoming call: %i\n", friend_number, f->call_state_friend);
    postmessage(AV_CALL_INCOMING, friend_number, video, NULL);
    toxaudio_postmessage(AUDIO_PLAY_RINGTONE, friend_number, 0, NULL);
}

static void utox_av_remote_disconnect(ToxAV *av, int32_t friend_number) {
    TOXAV_ERR_CALL_CONTROL error = 0;
    if (error) {
        debug("unhanded error in utox_av_end\n");
    }
    if (UTOX_SENDING_VIDEO(friend_number) | UTOX_ACCEPTING_VIDEO(friend_number)) {
        tox_postmessage(TOX_CALL_DISCONNECT, friend_number, 1, NULL);
    } else {
        tox_postmessage(TOX_CALL_DISCONNECT, friend_number, 0, NULL);
    }
}

void utox_av_local_disconnect(ToxAV *av, int32_t friend_number) {
    TOXAV_ERR_CALL_CONTROL error = 0;
    toxav_call_control(av, friend_number, TOXAV_CALL_CONTROL_CANCEL, &error);
    if (error) {
        debug("unhanded error in utox_av_end (%i)\n", error);
    }
    FRIEND *f = &friend[friend_number];
    f->call_state_self = 0;
    f->call_state_friend = 0;
    postmessage(AV_CALL_DISCONNECTED, friend_number, 0, NULL);
}

/** responds to a audio frame call back from toxav
 *
 * Moving this here might break Android, if you know this commit compiles and runs on android, remove this line!
 */
static void utox_av_incoming_frame_a(ToxAV *av, uint32_t friend_number, const int16_t *pcm, size_t sample_count,
                                    uint8_t channels, uint32_t sample_rate, void *userdata) {
    // debug("Incoming audio frame for friend %u \n", friend_number);
    #ifdef NATIVE_ANDROID_AUDIO
    audio_play(friend_number, pcm, sample_count, channels);
    #else
    sourceplaybuffer(friend_number, pcm, sample_count, channels, sample_rate);
    #endif
}

static void utox_av_incoming_frame_v(ToxAV *toxAV, uint32_t friend_number, uint16_t width, uint16_t height,
                                        const uint8_t *y, const uint8_t *u, const uint8_t *v,
                                        int32_t ystride, int32_t ustride, int32_t vstride, void *user_data) {
    /* copy the vpx_image */
    /* 4 bits for the H*W, then a pixel for each color * size */
    FRIEND *f = &friend[friend_number];
    f->video_width = width;
    f->video_height = height;

    utox_frame_pkg *frame = malloc(sizeof(*frame));
    frame->w = width;
    frame->h = height;
    frame->img = malloc(width * height * 4);

    yuv420tobgr(width, height, y, u, v, ystride, ustride, vstride, frame->img);

    postmessage(AV_VIDEO_FRAME, friend_number + 1, 0, (void*)frame);
}

/** respond to a Audio Video state change call back from toxav */
static void utox_callback_av_change_state(ToxAV *av, uint32_t friend_number, uint32_t state, void *userdata) {
    if ( state == 1 ) {
        // handle error
        debug("ToxAV:\tChange state with an error, this should never happen. Please send bug report!\n");
        utox_av_remote_disconnect(av, friend_number);
        return;
    } else if ( state == 2 ) {
        debug("ToxAV:\tCall ended with friend_number %u.\n", friend_number);
        utox_av_remote_disconnect(av, friend_number);
    }

    int state_audio = (state | (TOXAV_FRIEND_CALL_STATE_SENDING_A | TOXAV_FRIEND_CALL_STATE_ACCEPTING_A));
    int state_video = (state | (TOXAV_FRIEND_CALL_STATE_SENDING_V | TOXAV_FRIEND_CALL_STATE_ACCEPTING_V));

    if ((friend[friend_number].call_state_friend ^ state_audio)) {
        debug("Audio state change %i for %u\n", state, friend_number);
        friend[friend_number].call_state_friend = state;
        // do change
        // start audio
        // stop audio
    }

    if ((friend[friend_number].call_state_friend ^ state_video)) {
        debug("Video state change %i for %u\n", state, friend_number);
        friend[friend_number].call_state_friend = state;
        // start video
        // stop video
    }
}

static void utox_incoming_rate_change_audio(ToxAV *toxAV, uint32_t friend_number, bool stable, uint32_t bit_rate, void *user_data) {
    debug("ToxAV:\tIncoming audio rate change, please debug me!\n\tIncoming audio rate change, please debug me!\n\tIncoming audio rate change, please debug me!\n");
    return;
}

static void utox_incoming_rate_change_video(ToxAV *toxAV, uint32_t friend_number, bool stable, uint32_t bit_rate, void *user_data) {
    debug("ToxAV:\tIncoming Video rate change, please debug me!\n\tIncoming Video rate change, please debug me!\n\tIncoming Video rate change, please debug me!\n");
    return;
}

void set_av_callbacks(ToxAV *av) {
    /* Friend update callbacks */
    toxav_callback_call(av, &utox_av_incoming_call, NULL);
    toxav_callback_call_state(av, &utox_callback_av_change_state, NULL);

    /* Incoming data callbacks */
    toxav_callback_audio_receive_frame(av, &utox_av_incoming_frame_a, NULL);
    toxav_callback_video_receive_frame(av, &utox_av_incoming_frame_v, NULL);

    /* Data type change callbacks. */
    toxav_callback_audio_bit_rate_status(av, &utox_incoming_rate_change_audio, NULL);
    toxav_callback_video_bit_rate_status(av, &utox_incoming_rate_change_video, NULL);
}
// TODO

/**
 * Set the audio bit rate to be used in subsequent audio frames. If the passed
 * bit rate is the same as the current bit rate this function will return true
 * without calling a callback. If there is an active non forceful setup with the
 * passed audio bit rate and the new set request is forceful, the bit rate is
 * forcefully set and the previous non forceful request is cancelled. The active
 * non forceful setup will be canceled in favour of new non forceful setup.
 *
 * @param friend_number The friend number of the friend for which to set the
 * audio bit rate.
 * @param audio_bit_rate The new audio bit rate in Kb/sec. Set to 0 to disable
 * audio sending.
 * @param force True if the bit rate change is forceful.
 *
bool toxav_audio_bit_rate_set(ToxAV *toxAV, uint32_t friend_number, uint32_t audio_bit_rate, bool force, TOXAV_ERR_SET_BIT_RATE *error);
 */

/**
 * Set the video bit rate to be used in subsequent video frames. If the passed
 * bit rate is the same as the current bit rate this function will return true
 * without calling a callback. If there is an active non forceful setup with the
 * passed video bit rate and the new set request is forceful, the bit rate is
 * forcefully set and the previous non forceful request is cancelled. The active
 * non forceful setup will be canceled in favour of new non forceful setup.
 *
 * @param friend_number The friend number of the friend for which to set the
 * video bit rate.
 * @param audio_bit_rate The new video bit rate in Kb/sec. Set to 0 to disable
 * video sending.
 * @param force True if the bit rate change is forceful.
 *
bool toxav_video_bit_rate_set(ToxAV *toxAV, uint32_t friend_number, uint32_t audio_bit_rate, bool force, TOXAV_ERR_SET_BIT_RATE *error);
 */

