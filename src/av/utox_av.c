#include "utox_av.h"

#include "audio.h"
#include "video.h"

#include "../debug.h"
#include "../flist.h"
#include "../friend.h"
#include "../groups.h"
#include "../inline_video.h"
#include "../macros.h"
#include "../tox.h"
#include "../utox.h"
#include "../ui.h"

#include "../native/audio.h"
#include "../native/thread.h"

#include <stdlib.h>

bool utox_av_ctrl_init = false;

static bool toxav_thread_msg = 0;

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

void utox_av_ctrl_thread(void *UNUSED(args)) {
    ToxAV *av = NULL;

    utox_av_ctrl_init = 1;
    LOG_TRACE("uToxAv", "Toxav thread init" );

    volatile uint32_t call_count = 0;
    volatile bool     audio_in   = 0;
    // volatile bool video_on  = 0;

    while (1) {
        if (toxav_thread_msg) {
            TOX_MSG *msg = &toxav_msg;
            if (msg->msg == UTOXAV_KILL) {
                break;
            } else if (msg->msg == UTOXAV_NEW_TOX_INSTANCE) {
                if (av) { /* toxcore restart */
                    toxav_kill(av);
                    postmessage_audio(UTOXAUDIO_NEW_AV_INSTANCE, 0, 0, msg->data);
                    postmessage_video(UTOXVIDEO_NEW_AV_INSTANCE, 0, 0, msg->data);
                } else {
                    thread(utox_audio_thread, msg->data);
                    thread(utox_video_thread, msg->data);
                }

                av = msg->data;
                set_av_callbacks(av);
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
                    FRIEND *f = get_friend(msg->param1);
                    if (!f) {
                        LOG_ERR("uToxAV", "Could not to get friend when INCOMING_CALL_ANSWER %u", msg->param1);
                        break;
                    }
                    f->call_started = time(NULL);
                    message_add_type_notice(&f->msg, S(CALL_STARTED), SLEN(CALL_STARTED), true);
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
                    call_count++;                    postmessage_audio(UTOXAUDIO_PLAY_RINGTONE, msg->param1, msg->param2, NULL);
                    FRIEND *f          = get_friend(msg->param1);
                    if (!f) {
                        LOG_ERR("uToxAV", "Could not to get friend when OUTGOING_CALL_PENDING %u", msg->param1);
                        break;
                    }
                    f->call_state_self = (TOXAV_FRIEND_CALL_STATE_SENDING_A | TOXAV_FRIEND_CALL_STATE_ACCEPTING_A);
                    if (msg->param2) {
                        utox_video_start(0);
                        f->call_state_self |= (TOXAV_FRIEND_CALL_STATE_SENDING_V | TOXAV_FRIEND_CALL_STATE_ACCEPTING_V);
                    }
                    break;
                }

                case UTOXAV_OUTGOING_CALL_ACCEPTED: {
                    FRIEND *f = get_friend(msg->param1);
                    if (!f) {
                        LOG_ERR("uToxAV", "Could not to get friend when OUTGOING_CALL_ACCEPTED %u", msg->param1);
                        break;
                    }
                    f->call_started = time(NULL);
                    message_add_type_notice(&f->msg, S(CALL_STARTED), SLEN(CALL_STARTED), true);

                    postmessage_audio(UTOXAUDIO_START_FRIEND, msg->param1, msg->param2, NULL);
                    postmessage_audio(UTOXAUDIO_STOP_RINGTONE, msg->param1, msg->param2, NULL);
                    LOG_NOTE("uToxAV", "Call accepted by friend" );
                    break;
                }

                case UTOXAV_OUTGOING_CALL_REJECTED: {
                    postmessage_audio(UTOXAUDIO_STOP_RINGTONE, msg->param1, msg->param2, NULL);
                    break;
                }

                case UTOXAV_CALL_END: {
                    call_count--;
                    FRIEND *f = get_friend(msg->param1);
                    if (f
                        && f->call_state_self & (TOXAV_FRIEND_CALL_STATE_SENDING_V | TOXAV_FRIEND_CALL_STATE_ACCEPTING_V))
                    {
                        utox_video_stop(false);
                    }

                    if (f && f->call_started != 0) {
                        char notice_msg[64];
                        int duration = difftime(time(NULL), f->call_started);
                        int length = snprintf(notice_msg, 64, "%s: %02u:%02u:%02u",
                                S(CALL_ENDED), duration / 3600, (duration / 60) % 60, duration % 60);
                        if (length < 64) {
                            message_add_type_notice(&f->msg, notice_msg, length, true);
                        }
                        f->call_started = 0;
                    }

                    postmessage_audio(UTOXAUDIO_STOP_FRIEND, msg->param1, msg->param2, NULL);
                    postmessage_audio(UTOXAUDIO_STOP_RINGTONE, msg->param1, msg->param2, NULL);
                    break;
                }

                case UTOXAV_GROUPCALL_START: {
                    call_count++;
                    LOG_INFO("uToxAv", "Starting group call in groupchat %u", msg->param1);
                    postmessage_audio(UTOXAUDIO_GROUPCHAT_START, msg->param1, msg->param2, NULL);
                    break;
                }

                case UTOXAV_GROUPCALL_END: {
                    GROUPCHAT *g = get_group(msg->param1);
                    if (!g) {
                        LOG_ERR("uToxAv", "Could not get group %u", msg->param1);
                        break;
                    }

                    if (!call_count) {
                        LOG_ERR("uToxAv", "Trying to end a call when no call is active.");
                        break;
                    }

                    LOG_INFO("uToxAv", "Ending group call in groupchat %u", msg->param1);
                    postmessage_audio(UTOXAUDIO_GROUPCHAT_STOP, msg->param1, msg->param2, NULL);
                    call_count--;
                    break;
                }

                case UTOXAV_START_AUDIO: {
                    if (msg->param1) { /* Start audio preview */
                        call_count++;
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
                        toxav_video_set_bit_rate(av, msg->param1, UTOX_DEFAULT_BITRATE_V, &bitrate_err);
                    }
                    break;
                }

                case UTOXAV_STOP_VIDEO: {
                    if (msg->param2) {
                        utox_video_stop(1);
                    } else {
                        utox_video_stop(0);
                        TOXAV_ERR_BIT_RATE_SET bitrate_err = 0;
                        toxav_video_set_bit_rate(av, msg->param1, -1, &bitrate_err);
                    }
                    postmessage_utox(AV_CLOSE_WINDOW, msg->param1, 0, NULL);
                    break;
                }

                case UTOXAV_SET_AUDIO_IN: {
                    LOG_TRACE("uToxAV", "Set audio in" );
                    if (audio_in) {
                        postmessage_audio(UTOXAUDIO_CHANGE_MIC, 0, 0, NULL);
                    }

                    utox_audio_in_device_set(msg->data);

                    if (msg->data != utox_audio_in_device_get()) {
                        LOG_TRACE("uToxAV", "Error changing audio in" );
                        audio_in   = 0;
                        call_count = 0;
                        break;
                    }

                    // TODO get a count in audio.c and allow count restore
                    // if (audio_in) {
                    //     utox_audio_in_device_open();
                    //     utox_audio_in_listen();
                    // }
                    break;
                }

                case UTOXAV_SET_AUDIO_OUT: {
                    LOG_TRACE("uToxAV", "Set audio out" );
                    postmessage_audio(UTOXAUDIO_CHANGE_SPEAKER, 0, 0, NULL);
                    utox_audio_out_device_set(msg->data);
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
        }

        toxav_thread_msg = false;

        if (av) {
            toxav_iterate(av);
            yieldcpu(toxav_iteration_interval(av));
        } else {
            yieldcpu(10);
        }
    }


    postmessage_audio(UTOXAUDIO_KILL, 0, 0, NULL);
    postmessage_video(UTOXVIDEO_KILL, 0, 0, NULL);

    // Wait for all a/v threads to return 0
    while (utox_audio_thread_init || utox_video_thread_init) {
        yieldcpu(1);
    }

    toxav_thread_msg  = false;
    utox_av_ctrl_init = false;

    toxav_kill(av);
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
            toxav_video_set_bit_rate(av, friend_number, 0, &bitrate_err);
            postmessage_utoxav(UTOXAV_STOP_VIDEO, friend_number, 0, NULL);
            f->call_state_self &= (0xFF ^ TOXAV_FRIEND_CALL_STATE_SENDING_V);
            break;
        }

        case TOXAV_CALL_CONTROL_SHOW_VIDEO: {
            toxav_video_set_bit_rate(av, friend_number, UTOX_DEFAULT_BITRATE_V, &bitrate_err);
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
                                     int32_t ustride, int32_t vstride, void *UNUSED(user_data))
{
    /* copy the vpx_image */
    /* 4 bits for the H*W, then a pixel for each color * size */
    LOG_TRACE("uToxAV", "new video frame from friend %u" , friend_number);
    FRIEND *f = get_friend(friend_number);
    if (f == NULL) {
        LOG_ERR("uToxAV", "Incoming frame for a friend we don't know about! (%u)", friend_number);
        return;
    }
    f->video_width  = width;
    f->video_height = height;
    size_t size     = width * height * 4;

    UTOX_FRAME_PKG *frame = calloc(1, sizeof(UTOX_FRAME_PKG));

    if (!frame) {
        LOG_ERR("uToxAV", "Can't malloc for incoming frame.");
        return;
    }

    frame->w    = width;
    frame->h    = height;
    frame->size = size;
    frame->img  = malloc(size);
    if (!frame->img) {
        LOG_TRACE("uToxAV", "Could not allocate memory for image.");
        free(frame);
        return;
    }

    yuv420tobgr(width, height, y, u, v, ystride, ustride, vstride, frame->img);
    if (f->video_inline) {
        if (!inline_set_frame(width, height, size, frame->img)) {
            LOG_ERR("uToxAV", "Error setting frame for inline video.");
        }

        postmessage_utox(AV_INLINE_FRAME, friend_number, 0, NULL);
        free(frame->img);
        free(frame);
    } else {
        postmessage_utox(AV_VIDEO_FRAME, friend_number, 0, (void *)frame);
    }
}

static void utox_audio_friend_accepted(ToxAV *av, uint32_t friend_number, uint32_t state) {
    /* First accepted call back */
    LOG_NOTE("uToxAV", "Friend accepted call" );
    FRIEND *f = get_friend(friend_number);
    if (!f) {
        LOG_FATAL_ERR(EXIT_FAILURE, "uToxAV", "Unable to get friend when A/V call accepted %u", friend_number);
    }
    f->call_state_friend = state;
    if (SELF_SEND_VIDEO(friend_number) && !FRIEND_ACCEPTING_VIDEO(friend_number)) {
        utox_av_local_call_control(av, friend_number, TOXAV_CALL_CONTROL_HIDE_VIDEO);
    }
    postmessage_utoxav(UTOXAV_OUTGOING_CALL_ACCEPTED, friend_number, 0, NULL);
    postmessage_utox(AV_CALL_ACCEPTED, friend_number, 0, NULL);
}

/** respond to a Audio Video state change call back from toxav */
static void utox_callback_av_change_state(ToxAV *av, uint32_t friend_number, uint32_t state, void *UNUSED(userdata)) {
    FRIEND *f = get_friend(friend_number);
    if (!f) {
        LOG_ERR("uToxAV", "Unable to get friend when A/V state changed %u", friend_number);
        return;
    }
    if (state == 1) {
        // handle error
        LOG_ERR("uToxAV", "Change state with an error, this should never happen. Please send bug report!");
        utox_av_remote_disconnect(av, friend_number);
        return;
    } else if (state == 2) {
        LOG_NOTE("uToxAV", "Call ended with friend_number %u." , friend_number);
        utox_av_remote_disconnect(av, friend_number);
        return;
    } else if (!f->call_state_friend) {
        utox_audio_friend_accepted(av, friend_number, state);
    }

    if (f->call_state_friend ^ (state & TOXAV_FRIEND_CALL_STATE_SENDING_A)) {
        if (state & TOXAV_FRIEND_CALL_STATE_SENDING_A) {
            LOG_INFO("uToxAV", "Friend %u is now sending audio." , friend_number);
        } else {
            LOG_INFO("uToxAV", "Friend %u is no longer sending audio." , friend_number);
        }
    }
    if (f->call_state_friend ^ (state & TOXAV_FRIEND_CALL_STATE_SENDING_V)) {
        if (state & TOXAV_FRIEND_CALL_STATE_SENDING_V) {
            LOG_INFO("uToxAV", "Friend %u is now sending video." , friend_number);
        } else {
            LOG_INFO("uToxAV", "Friend %u is no longer sending video." , friend_number);
            flist_reselect_current();
        }
    }
    if (f->call_state_friend ^ (state & TOXAV_FRIEND_CALL_STATE_ACCEPTING_A)) {
        if (state & TOXAV_FRIEND_CALL_STATE_ACCEPTING_A) {
            LOG_INFO("uToxAV", "Friend %u is now accepting audio." , friend_number);
        } else {
            LOG_INFO("uToxAV", "Friend %u is no longer accepting audio." , friend_number);
        }
    }
    if (f->call_state_friend ^ (state & TOXAV_FRIEND_CALL_STATE_ACCEPTING_V)) {
        if (state & TOXAV_FRIEND_CALL_STATE_ACCEPTING_V) {
            LOG_INFO("uToxAV", "Friend %u is now accepting video." , friend_number);
        } else {
            LOG_INFO("uToxAV", "Friend %u is no longer accepting video." , friend_number);
        }
    }

    f->call_state_friend = state;
}

static void utox_incoming_video_rate_change(ToxAV *AV, uint32_t f_num, uint32_t v_bitrate, void *UNUSED(ud)) {
    /* Just accept what toxav wants the bitrate to be... */
    if (v_bitrate > (uint32_t)UTOX_MIN_BITRATE_VIDEO) {
        TOXAV_ERR_BIT_RATE_SET error = 0;
        toxav_video_set_bit_rate(AV, f_num, v_bitrate, &error);
        if (error) {
            LOG_ERR("ToxAV", "Setting new Video bitrate has failed with error #%u" , error);
        } else {
            LOG_NOTE("uToxAV", "Video bitrate changed to %u" , v_bitrate);
        }
    } else {
        LOG_NOTE("uToxAV", "Video bitrate unchanged %u is less than %u" , v_bitrate, UTOX_MIN_BITRATE_VIDEO);
    }
}

static void utox_incoming_audio_rate_change(ToxAV *AV, uint32_t friend_number, uint32_t audio_bitrate, void *UNUSED(userdata)){
    if (audio_bitrate > (uint32_t)UTOX_MIN_BITRATE_VIDEO) {
        TOXAV_ERR_BIT_RATE_SET error = 0;
        toxav_video_set_bit_rate(AV, friend_number, audio_bitrate, &error);
        if (error) {
            LOG_ERR("ToxAV", "Setting new audio bitrate has failed with error #%u" , error);
        } else {
            LOG_NOTE("uToxAV", "Audio bitrate changed to %u" , audio_bitrate);
        }
    } else {
        LOG_NOTE("uToxAV", "Audio bitrate unchanged %u is less than %u" , audio_bitrate, UTOX_MIN_BITRATE_AUDIO);
    }
}

void set_av_callbacks(ToxAV *av) {
    /* Friend update callbacks */
    toxav_callback_call(av, &utox_av_incoming_call, NULL);
    toxav_callback_call_state(av, &utox_callback_av_change_state, NULL);

    /* Incoming data callbacks */
    toxav_callback_audio_receive_frame(av, &utox_av_incoming_frame_a, NULL);
    toxav_callback_video_receive_frame(av, &utox_av_incoming_frame_v, NULL);

    /* Data type change callbacks. */
    toxav_callback_video_bit_rate(av, &utox_incoming_video_rate_change, NULL);
    toxav_callback_audio_bit_rate(av, &utox_incoming_audio_rate_change, NULL);
}
