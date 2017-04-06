#include "utox_av.h"

#include "audio.h"
#include "video.h"

#include "../debug.h"
#include "../flist.h"
#include "../friend.h"
#include "../inline_video.h"
#include "../macros.h"
#include "../tox.h"
#include "../utox.h"

#include "../native/thread.h"

#include <stdlib.h>

#include "../main.h" // utox_av_ctrl_init, utox_video_thread_init, utox_audio_thread_init


bool toxav_thread_msg = 0;
void postmessage_utoxav(uint8_t msg, uint32_t param1, uint32_t param2, void *data) {
    while (toxav_thread_msg && utox_av_ctrl_init) { /* I'm not convinced this is the best way */
        yieldcpu(1);
    }

    toxav_msg.msg    = msg;
    toxav_msg.param1 = param1;
    toxav_msg.param2 = param2;
    toxav_msg.data   = data;

    toxav_thread_msg = 1;
}

#define VERIFY_AUDIO_IN()                    \
    do {                                     \
        if (call_count) {                    \
            if (!audio_in) {                 \
                utox_audio_in_device_open(); \
                utox_audio_in_listen();      \
                audio_in = 1;                \
            }                                \
        } else {                             \
            utox_audio_in_ignore();          \
            utox_audio_in_device_close();    \
            audio_in = 0;                    \
        }                                    \
        yieldcpu(5);                         \
    } while (0)

void utox_av_ctrl_thread(void *args) {
    ToxAV *av = args;

    utox_av_ctrl_init = 1;
    LOG_TRACE("uToxAv", "Toxav thread init" );

    volatile uint32_t call_count = 0;
    volatile bool     audio_in   = 0;
    // volatile bool video_on  = 0;

    thread(utox_audio_thread, av);
    thread(utox_video_thread, av);

    while (1) {
        if (toxav_thread_msg) {
            TOX_MSG *msg = &toxav_msg;
            if (msg->msg == UTOXAV_KILL) {
                break;
            }

            if (!utox_audio_thread_init || !utox_video_thread_init) {
                yieldcpu(10);
            }

            switch (msg->msg) {
                case UTOXAV_INCOMING_CALL_PENDING: {
                    call_count++;
                    postmessage_audio(UTOXAUDIO_PLAY_RINGTONE, msg->param1, msg->param2, NULL);
                    break;
                }

                case UTOXAV_INCOMING_CALL_ANSWER: {
                    VERIFY_AUDIO_IN();
                    FRIEND *f = get_friend(msg->param1);
                    postmessage_audio(UTOXAUDIO_STOP_RINGTONE, msg->param1, msg->param2, NULL);
                    postmessage_audio(UTOXAUDIO_START_FRIEND, msg->param1, msg->param2, NULL);
                    f->call_state_self = (TOXAV_FRIEND_CALL_STATE_SENDING_A | TOXAV_FRIEND_CALL_STATE_ACCEPTING_A);
                    if (msg->param2) {
                        utox_video_start(0);
                        f->call_state_self |= (TOXAV_FRIEND_CALL_STATE_SENDING_V | TOXAV_FRIEND_CALL_STATE_ACCEPTING_V);
                    }
                    break;
                }

                case UTOXAV_INCOMING_CALL_REJECT: {
                    call_count--;
                    postmessage_audio(UTOXAUDIO_STOP_RINGTONE, msg->param1, msg->param2, NULL);
                    break;
                }

                case UTOXAV_OUTGOING_CALL_PENDING: {
                    call_count++;
                    VERIFY_AUDIO_IN();
                    postmessage_audio(UTOXAUDIO_PLAY_RINGTONE, msg->param1, msg->param2, NULL);
                    FRIEND *f          = get_friend(msg->param1);
                    f->call_state_self = (TOXAV_FRIEND_CALL_STATE_SENDING_A | TOXAV_FRIEND_CALL_STATE_ACCEPTING_A);
                    if (msg->param2) {
                        utox_video_start(0);
                        f->call_state_self |= (TOXAV_FRIEND_CALL_STATE_SENDING_V | TOXAV_FRIEND_CALL_STATE_ACCEPTING_V);
                    }
                    break;
                }

                case UTOXAV_OUTGOING_CALL_ACCEPTED: {
                    postmessage_audio(UTOXAUDIO_START_FRIEND, msg->param1, msg->param2, NULL);
                    LOG_NOTE("uToxAV", "Call accepted by friend" );
                    // intentional fall thorough
                }

                case UTOXAV_OUTGOING_CALL_REJECTED: {
                    postmessage_audio(UTOXAUDIO_STOP_RINGTONE, msg->param1, msg->param2, NULL);
                    break;
                }
                case UTOXAV_CALL_END: {
                    call_count--;
                    FRIEND *f = get_friend(msg->param1);
                    if (f && f->call_state_self
                             & (TOXAV_FRIEND_CALL_STATE_SENDING_V | TOXAV_FRIEND_CALL_STATE_ACCEPTING_V))
                    {
                        utox_video_stop(false);
                    }

                    postmessage_audio(UTOXAUDIO_STOP_FRIEND, msg->param1, msg->param2, NULL);
                    postmessage_audio(UTOXAUDIO_STOP_RINGTONE, msg->param1, msg->param2, NULL);

                    if (msg->param2) {
                        // call_count++;
                        // groups_audio[m->param1] = 1;
                        // if(!record_on) {
                        // device_in = alcopencapture(audio_device);
                        // alccapturestart(device_in);
                        // record_on = 1;
                        LOG_TRACE("uToxAv", "Starting Audio GroupCall" );
                        // }
                    }

                    VERIFY_AUDIO_IN();
                    break;
                }

                case UTOXAV_START_AUDIO: {
                    call_count++;
                    if (msg->param1) { /* Start audio preview */
                        call_count++;
                        VERIFY_AUDIO_IN();
                        LOG_TRACE("uToxAV", "Starting Audio Preview" );
                        postmessage_audio(UTOXAUDIO_START_PREVIEW, 0, 0, NULL);
                    }
                    break;
                }
                case UTOXAV_STOP_AUDIO: {
                    if (!call_count) {
                        LOG_TRACE("uToxAV", "WARNING, trying to stop audio while already closed!\nThis is bad!" );
                        break;
                    }

                    if (msg->param1) {
                        call_count--;
                        LOG_TRACE("uToxAV", "Stopping Audio Preview" );
                        postmessage_audio(UTOXAUDIO_STOP_PREVIEW, 0, 0, NULL);
                    }
                    break;
                }

                case UTOXAV_START_VIDEO: {
                    if (msg->param2) {
                        utox_video_start(1);
                    } else {
                        utox_video_start(0);
                        TOXAV_ERR_BIT_RATE_SET bitrate_err = 0;
                        toxav_bit_rate_set(av, msg->param1, UTOX_DEFAULT_BITRATE_V, 0, &bitrate_err);
                    }
                    break;
                }

                case UTOXAV_STOP_VIDEO: {
                    if (msg->param2) {
                        utox_video_stop(1);
                    } else {
                        utox_video_stop(0);
                        TOXAV_ERR_BIT_RATE_SET bitrate_err = 0;
                        toxav_bit_rate_set(av, msg->param1, -1, 0, &bitrate_err);
                    }
                    postmessage_utox(AV_CLOSE_WINDOW, msg->param1, 0, NULL);
                    break;
                }

                case UTOXAV_SET_AUDIO_IN: {
                    LOG_TRACE("uToxAV", "Set audio in" );
                    if (audio_in) {
                        utox_audio_in_ignore();
                        utox_audio_in_device_close();
                    }

                    utox_audio_in_device_set(msg->data);

                    if (msg->data != utox_audio_in_device_get()) {
                        LOG_TRACE("uToxAV", "Error changing audio in" );
                        audio_in   = 0;
                        call_count = 0;
                        break;
                    }

                    if (audio_in) {
                        utox_audio_in_device_open();
                        utox_audio_in_listen();
                    }
                    break;
                }

                case UTOXAV_SET_AUDIO_OUT: {
                    LOG_TRACE("uToxAV", "Set audio out" );
                    utox_audio_out_device_close();
                    utox_audio_out_device_set(msg->data);
                    utox_audio_out_device_open();
                    break;
                }

                case UTOXAV_SET_VIDEO_IN: {
                    utox_video_change_device(msg->param1);
                    LOG_TRACE("uToxAV", "Changed video input device" );
                    break;
                }

                case UTOXAV_SET_VIDEO_OUT: {
                    break;
                }
            }

            VERIFY_AUDIO_IN();
        }

        toxav_thread_msg = false;
        toxav_iterate(av);
        yieldcpu(toxav_iteration_interval(av));
    }


    postmessage_audio(UTOXAUDIO_KILL, 0, 0, NULL);
    postmessage_video(UTOXVIDEO_KILL, 0, 0, NULL);

    // Wait for all a/v threads to return 0
    while (utox_audio_thread_init || utox_video_thread_init) {
        yieldcpu(1);
    }

    toxav_thread_msg  = false;
    utox_av_ctrl_init = false;

    LOG_NOTE("UTOXAV", "Clean thread exit!");
    return;
}

static void utox_av_incoming_call(ToxAV *UNUSED(av), uint32_t friend_number,
                                  bool audio, bool video, void *UNUSED(userdata))
{
    LOG_TRACE("uToxAV", "A/V Invite (%u)" , friend_number);
    FRIEND *f = get_friend(friend_number);
    if (!f) {
        LOG_ERR("uToxAV", "Unable to get friend %u for A/V invite.", friend_number);
        return;
    }

    f->call_state_self   = 0;
    f->call_state_friend = (audio << 2 | video << 3 | audio << 4 | video << 5);
    LOG_TRACE("uToxAV", "uTox AV:\tcall friend (%u) state for incoming call: %i" , friend_number, f->call_state_friend);
    postmessage_utoxav(UTOXAV_INCOMING_CALL_PENDING, friend_number, 0, NULL);
    postmessage_utox(AV_CALL_INCOMING, friend_number, video, NULL);
}

static void utox_av_remote_disconnect(ToxAV *UNUSED(av), int32_t friend_number) {
    LOG_TRACE("uToxAV", "Remote disconnect from friend %u" , friend_number);
    FRIEND *f = get_friend(friend_number);
    if (!f) {
        LOG_ERR("uToxAV", "Unable to get friend %u for remote disconnect.", friend_number);
        return;
    }

    postmessage_utoxav(UTOXAV_CALL_END, friend_number, 0, NULL);
    f->call_state_self   = 0;
    f->call_state_friend = 0;
    postmessage_utox(AV_CLOSE_WINDOW, friend_number + 1, 0, NULL);
    postmessage_utox(AV_CALL_DISCONNECTED, friend_number, 0, NULL);
}

void utox_av_local_disconnect(ToxAV *av, int32_t friend_number) {
    TOXAV_ERR_CALL_CONTROL error = 0;
    if (av) {
        /* TODO HACK: tox_callbacks doesn't have access to toxav, so it just sets it as NULL, this is bad! */
        toxav_call_control(av, friend_number, TOXAV_CALL_CONTROL_CANCEL, &error);
    }

    switch (error) {
        case TOXAV_ERR_CALL_CONTROL_OK: {
            LOG_NOTE("uToxAV", "ToxAV has disconnected!" );
            break;
        }

        case TOXAV_ERR_CALL_CONTROL_SYNC: {
            LOG_ERR("uToxAV", "ToxAV sync error!");
            break;
        }

        case TOXAV_ERR_CALL_CONTROL_FRIEND_NOT_FOUND: {
            LOG_ERR("uToxAV", "ToxAV friend #%i not found." , friend_number);
            break;
        }

        case TOXAV_ERR_CALL_CONTROL_FRIEND_NOT_IN_CALL: {
            LOG_ERR("uToxAV", "ToxAV no existing call for friend #%i." , friend_number);
            break;
        }

        case TOXAV_ERR_CALL_CONTROL_INVALID_TRANSITION: {
            LOG_NOTE("uToxAV", "Call already paused, or already running." );
            break;
        }
    }
    FRIEND *f = get_friend(friend_number);
    if (!f) {
        LOG_ERR("uToxAV", "Unable to get friend %u for A/V disconnect.", friend_number);
        return;
    }

    f->call_state_self   = 0;
    f->call_state_friend = 0;
    postmessage_utox(AV_CLOSE_WINDOW, friend_number + 1, 0, NULL); /* TODO move all of this into a static function in that
                                                                 file !*/
    postmessage_utox(AV_CALL_DISCONNECTED, friend_number, 0, NULL);
    postmessage_utoxav(UTOXAV_CALL_END, friend_number, 0, NULL);
}

void utox_av_local_call_control(ToxAV *av, uint32_t friend_number, TOXAV_CALL_CONTROL control) {
    TOXAV_ERR_CALL_CONTROL err = 0;
    toxav_call_control(av, friend_number, control, &err);
    if (err) {
        LOG_TRACE("uToxAV", "Local call control error!" );
        return;
    }

    TOXAV_ERR_BIT_RATE_SET bitrate_err = 0;
    FRIEND *f = get_friend(friend_number);
    if (!f) {
        LOG_ERR("uToxAV", "Unable to get friend %u for local call control.", friend_number);
        return;
    }

    switch (control) {
        case TOXAV_CALL_CONTROL_HIDE_VIDEO: {
            toxav_bit_rate_set(av, friend_number, -1, 0, &bitrate_err);
            postmessage_utoxav(UTOXAV_STOP_VIDEO, friend_number, 0, NULL);
            f->call_state_self &= (0xFF ^ TOXAV_FRIEND_CALL_STATE_SENDING_V);
            break;
        }

        case TOXAV_CALL_CONTROL_SHOW_VIDEO: {
            toxav_bit_rate_set(av, friend_number, -1, UTOX_DEFAULT_BITRATE_V, &bitrate_err);
            postmessage_utoxav(UTOXAV_START_VIDEO, friend_number, 0, NULL);
            f->call_state_self |= TOXAV_FRIEND_CALL_STATE_SENDING_V;
            break;
        }

        default: {
            LOG_ERR("uToxAV", "Unhandled local call control");
        }
            // TODO
            // TOXAV_CALL_CONTROL_RESUME,
            // TOXAV_CALL_CONTROL_PAUSE,
            // TOXAV_CALL_CONTROL_CANCEL,
            // TOXAV_CALL_CONTROL_MUTE_AUDIO,
            // TOXAV_CALL_CONTROL_UNMUTE_AUDIO,
    }

    if (bitrate_err) {
        LOG_ERR("uToxAV", "Error setting/changing video bitrate");
    }
}

// responds to a audio frame call back from toxav
static void utox_av_incoming_frame_a(ToxAV *UNUSED(av), uint32_t friend_number, const int16_t *pcm, size_t sample_count,
                                     uint8_t channels, uint32_t sample_rate, void *UNUSED(userdata))
{
    // LOG_TRACE("uToxAv", "Incoming audio frame for friend %u " , friend_number);
#ifdef NATIVE_ANDROID_AUDIO
    audio_play(friend_number, pcm, sample_count, channels);
#else
    sourceplaybuffer(friend_number, pcm, sample_count, channels, sample_rate);
#endif
}

static void utox_av_incoming_frame_v(ToxAV *UNUSED(toxAV), uint32_t friend_number, uint16_t width, uint16_t height,
                                     const uint8_t *y, const uint8_t *u, const uint8_t *v, int32_t ystride,
                                     int32_t ustride, int32_t vstride, void *UNUSED(user_data)) {
    /* copy the vpx_image */
    /* 4 bits for the H*W, then a pixel for each color * size */
    LOG_TRACE("uToxAV", "new video frame from friend %u" , friend_number);
    FRIEND *f       = get_friend(friend_number);
    f->video_width  = width;
    f->video_height = height;
    size_t size     = width * height * 4;

    UTOX_FRAME_PKG *frame = calloc(1, sizeof(UTOX_FRAME_PKG));

    if (!frame) {
        LOG_ERR("uToxAV", "can't malloc for incoming frame");
        return;
    }

    frame->w    = width;
    frame->h    = height;
    frame->size = size;
    frame->img  = malloc(size);
    if (!frame->img) {
        LOG_TRACE("uToxAV", " Could not allocate memory for image." );
        free(frame);
        return;
    }

    yuv420tobgr(width, height, y, u, v, ystride, ustride, vstride, frame->img);
    if (f->video_inline) {
        if (!inline_set_frame(width, height, size, frame->img)) {
            LOG_ERR("uToxAV", " error setting frame for inline video");
        }

        postmessage_utox(AV_INLINE_FRAME, friend_number, 0, NULL);
    } else {
        postmessage_utox(AV_VIDEO_FRAME, friend_number + 1, 0, (void *)frame);
    }

    free(frame);
}

static void utox_audio_friend_accepted(ToxAV *av, uint32_t friend_number, uint32_t state) {
    /* First accepted call back */
    LOG_NOTE("uToxAV", "Friend accepted call" );
    get_friend(friend_number)->call_state_friend = state;
    if (SELF_SEND_VIDEO(friend_number) && !FRIEND_ACCEPTING_VIDEO(friend_number)) {
        utox_av_local_call_control(av, friend_number, TOXAV_CALL_CONTROL_HIDE_VIDEO);
    }
    postmessage_utoxav(UTOXAV_OUTGOING_CALL_ACCEPTED, friend_number, 0, NULL);
    postmessage_utox(AV_CALL_ACCEPTED, friend_number, 0, NULL);
}

/** respond to a Audio Video state change call back from toxav */
static void utox_callback_av_change_state(ToxAV *av, uint32_t friend_number, uint32_t state, void *UNUSED(userdata)) {
    FRIEND *f = get_friend(friend_number);
    if (state == 1) {
        // handle error
        LOG_ERR("uToxAV", "Change state with an error, this should never happen. Please send bug report!");
        utox_av_remote_disconnect(av, friend_number);
        return;
    } else if (state == 2) {
        LOG_NOTE("uToxAV", "Call ended with friend_number %u." , friend_number);
        utox_av_remote_disconnect(av, friend_number);
        message_add_type_notice(&f->msg, "Friend Has Ended the call!", 26, 0); /* TODO localization with S() SLEN() */
        return;
    } else if (!f->call_state_friend) {
        utox_audio_friend_accepted(av, friend_number, state);
    }

    if (get_friend(friend_number)->call_state_friend ^ (state & TOXAV_FRIEND_CALL_STATE_SENDING_A)) {
        if (state & TOXAV_FRIEND_CALL_STATE_SENDING_A) {
            LOG_INFO("uToxAV", "Friend %u is now sending audio." , friend_number);
        } else {
            LOG_INFO("uToxAV", "Friend %u is no longer sending audio." , friend_number);
        }
    }
    if (get_friend(friend_number)->call_state_friend ^ (state & TOXAV_FRIEND_CALL_STATE_SENDING_V)) {
        if (state & TOXAV_FRIEND_CALL_STATE_SENDING_V) {
            LOG_INFO("uToxAV", "Friend %u is now sending video." , friend_number);
        } else {
            LOG_INFO("uToxAV", "Friend %u is no longer sending video." , friend_number);
            flist_reselect_current();
        }
    }
    if (get_friend(friend_number)->call_state_friend ^ (state & TOXAV_FRIEND_CALL_STATE_ACCEPTING_A)) {
        if (state & TOXAV_FRIEND_CALL_STATE_ACCEPTING_A) {
            LOG_INFO("uToxAV", "Friend %u is now accepting audio." , friend_number);
        } else {
            LOG_INFO("uToxAV", "Friend %u is no longer accepting audio." , friend_number);
        }
    }
    if (get_friend(friend_number)->call_state_friend ^ (state & TOXAV_FRIEND_CALL_STATE_ACCEPTING_V)) {
        if (state & TOXAV_FRIEND_CALL_STATE_ACCEPTING_V) {
            LOG_INFO("uToxAV", "Friend %u is now accepting video." , friend_number);
        } else {
            LOG_INFO("uToxAV", "Friend %u is no longer accepting video." , friend_number);
        }
    }

    get_friend(friend_number)->call_state_friend = state;
}

static void utox_incoming_rate_change(ToxAV *AV, uint32_t f_num, uint32_t UNUSED(a_bitrate), uint32_t v_bitrate, void *UNUSED(ud)) {
    /* Just accept what toxav wants the bitrate to be... */
    if (v_bitrate > (uint32_t)UTOX_MIN_BITRATE_VIDEO) {
        TOXAV_ERR_BIT_RATE_SET error = 0;
        toxav_bit_rate_set(AV, f_num, -1, v_bitrate, &error);
        if (error) {
            LOG_ERR("ToxAV", "Setting new Video bitrate has failed with error #%u" , error);
        } else {
            LOG_NOTE("uToxAV", "\tVideo bitrate changed to %u" , v_bitrate);
        }
    } else {
        LOG_NOTE("uToxAV", "\tVideo bitrate unchanged %u is less than %u" , v_bitrate, UTOX_MIN_BITRATE_VIDEO);
    }
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
    toxav_callback_bit_rate_status(av, &utox_incoming_rate_change, NULL);
}
