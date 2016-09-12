#include "../main.h"

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

#define VERIFY_AUDIO_IN()                                                                                              \
    do {                                                                                                               \
        if (call_count) {                                                                                              \
            if (!audio_in) {                                                                                           \
                utox_audio_in_device_open();                                                                           \
                utox_audio_in_listen();                                                                                \
                audio_in = 1;                                                                                          \
            }                                                                                                          \
        } else {                                                                                                       \
            utox_audio_in_ignore();                                                                                    \
            utox_audio_in_device_close();                                                                              \
            audio_in = 0;                                                                                              \
        }                                                                                                              \
        yieldcpu(5);                                                                                                   \
    } while (0)

void utox_av_ctrl_thread(void *args) {
    ToxAV *av = args;

    utox_av_ctrl_init = 1;
    debug("Toxav thread init\n");

    volatile uint32_t call_count = 0;
    volatile _Bool    audio_in   = 0;
    // volatile _Bool video_on  = 0;

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
                    FRIEND *f = &friend[msg->param1];
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
                    FRIEND *f          = &friend[msg->param1];
                    f->call_state_self = (TOXAV_FRIEND_CALL_STATE_SENDING_A | TOXAV_FRIEND_CALL_STATE_ACCEPTING_A);
                    if (msg->param2) {
                        utox_video_start(0);
                        f->call_state_self |= (TOXAV_FRIEND_CALL_STATE_SENDING_V | TOXAV_FRIEND_CALL_STATE_ACCEPTING_V);
                    }
                    break;
                }

                case UTOXAV_OUTGOING_CALL_ACCEPTED: {
                    postmessage_audio(UTOXAUDIO_START_FRIEND, msg->param1, msg->param2, NULL);
                    debug_notice("uToxAV:\tCall accepted by friend\n");
                    // intentional fall thorough
                }
                case UTOXAV_OUTGOING_CALL_REJECTED: {
                    postmessage_audio(UTOXAUDIO_STOP_RINGTONE, msg->param1, msg->param2, NULL);
                    break;
                }


                case UTOXAV_CALL_END: {
                    call_count--;
                    FRIEND *f = &friend[msg->param1];
                    if ((f->call_state_self | TOXAV_FRIEND_CALL_STATE_SENDING_V
                         | TOXAV_FRIEND_CALL_STATE_ACCEPTING_V)) {
                        utox_video_stop(0);
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
                        debug("Starting Audio GroupCall\n");
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
                        debug("uToxAV:\tStarting Audio Preview\n");
                        postmessage_audio(UTOXAUDIO_START_PREVIEW, 0, 0, NULL);
                    }
                    break;
                }
                case UTOXAV_STOP_AUDIO: {
                    if (!call_count) {
                        debug("uToxAV:\tWARNING, trying to stop audio while already closed!\nThis is bad!\n");
                        break;
                    }

                    if (msg->param1) {
                        call_count--;
                        debug("uToxAV:\tStopping Audio Preview\n");
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
                    postmessage(AV_CLOSE_WINDOW, msg->param1, 0, NULL);
                    break;
                }

                case UTOXAV_SET_AUDIO_IN: {
                    debug("uToxAV:\tSet audio in\n");
                    if (audio_in) {
                        utox_audio_in_ignore();
                        utox_audio_in_device_close();
                    }

                    utox_audio_in_device_set(msg->data);

                    if (msg->data != utox_audio_in_device_get()) {
                        debug("uToxAV:\tError changing audio in\n");
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
                    debug("uToxAV:\tSet audio out\n");
                    utox_audio_out_device_close();
                    utox_audio_out_device_set(msg->data);
                    utox_audio_out_device_open();
                    break;
                }

                case UTOXAV_SET_VIDEO_IN: {
                    utox_video_change_device(msg->param1);
                    debug("uToxAV:\tChanged video input device\n");
                    break;
                }

                case UTOXAV_SET_VIDEO_OUT: {
                    break;
                }
            }
            VERIFY_AUDIO_IN();
        }

        toxav_thread_msg = 0;
        toxav_iterate(av, NULL);
        yieldcpu(toxav_iteration_interval(av));
    }


    postmessage_audio(UTOXAUDIO_KILL, 0, 0, NULL);
    postmessage_video(UTOXVIDEO_KILL, 0, 0, NULL);

    // Wait for all a/v threads to return 0
    while (utox_audio_thread_init || utox_video_thread_init) {
        yieldcpu(1);
    }

    toxav_thread_msg  = 0;
    utox_av_ctrl_init = 0;

    debug_notice("UTOXAV:\tClean thread exit!\n");
    return;
}

static void utox_av_incoming_call(ToxAV *av, uint32_t friend_number, bool audio, bool video, void *UNUSED(userdata)) {
    debug("A/V Invite (%u)\n", friend_number);
    FRIEND *f = &friend[friend_number];

    f->call_state_self   = 0;
    f->call_state_friend = (audio << 2 | video << 3 | audio << 4 | video << 5);
    debug("uTox AV:\tcall friend (%u) state for incoming call: %i\n", friend_number, f->call_state_friend);
    postmessage_utoxav(UTOXAV_INCOMING_CALL_PENDING, friend_number, 0, NULL);
    postmessage(AV_CALL_INCOMING, friend_number, video, NULL);
}

static void utox_av_remote_disconnect(ToxAV *av, int32_t friend_number) {
    debug("uToxAV:\tRemote disconnect from friend %u\n", friend_number);
    postmessage_utoxav(UTOXAV_CALL_END, friend_number, 0, NULL);
    friend[friend_number].call_state_self   = 0;
    friend[friend_number].call_state_friend = 0;
    postmessage(AV_CLOSE_WINDOW, friend_number + 1, 0, NULL);
    postmessage(AV_CALL_DISCONNECTED, friend_number, 0, NULL);
}

void utox_av_local_disconnect(ToxAV *av, int32_t friend_number) {
    TOXAV_ERR_CALL_CONTROL error = 0;
    if (av) { /* TODO HACK: tox_callbacks doesn't have access to toxav, so it just sets it as NULL, this is bad! */
        toxav_call_control(av, friend_number, TOXAV_CALL_CONTROL_CANCEL, &error);
    }
    switch (error) {
        case TOXAV_ERR_CALL_CONTROL_OK: {
            debug_notice("uToxAV:\tToxAV has disconnected!\n");
            break;
        }
        case TOXAV_ERR_CALL_CONTROL_SYNC: {
            debug_error("uToxAV:\tToxAV sync error!\n");
            break;
        }
        case TOXAV_ERR_CALL_CONTROL_FRIEND_NOT_FOUND: {
            debug_error("uToxAV:\tToxAV friend #%i not found.\n", friend_number);
            break;
        }
        case TOXAV_ERR_CALL_CONTROL_FRIEND_NOT_IN_CALL: {
            debug_error("uToxAV:\tToxAV no existing call for friend #%i.\n", friend_number);
            break;
        }
        case TOXAV_ERR_CALL_CONTROL_INVALID_TRANSITION: {
            debug_notice("uToxAV:\tCall already paused, or already running.\n");
            break;
        }
    }
    friend[friend_number].call_state_self   = 0;
    friend[friend_number].call_state_friend = 0;
    postmessage(AV_CLOSE_WINDOW, friend_number + 1, 0, NULL); /* TODO move all of this into a static function in that
                                                                 file !*/
    postmessage(AV_CALL_DISCONNECTED, friend_number, 0, NULL);
    postmessage_utoxav(UTOXAV_CALL_END, friend_number, 0, NULL);
}

void utox_av_local_call_control(ToxAV *av, uint32_t friend_number, TOXAV_CALL_CONTROL control) {
    TOXAV_ERR_CALL_CONTROL err = 0;
    toxav_call_control(av, friend_number, control, &err);
    if (err) {
        debug("uToxAV:\tLocal call control error!\n");
    } else {
        TOXAV_ERR_BIT_RATE_SET bitrate_err = 0;
        switch (control) {
            case TOXAV_CALL_CONTROL_HIDE_VIDEO: {
                toxav_bit_rate_set(av, friend_number, -1, 0, &bitrate_err);
                postmessage_utoxav(UTOXAV_STOP_VIDEO, friend_number, 0, NULL);
                friend[friend_number].call_state_self &= (0xFF ^ TOXAV_FRIEND_CALL_STATE_SENDING_V);
                break;
            }
            case TOXAV_CALL_CONTROL_SHOW_VIDEO: {
                toxav_bit_rate_set(av, friend_number, -1, UTOX_DEFAULT_BITRATE_V, &bitrate_err);
                postmessage_utoxav(UTOXAV_START_VIDEO, friend_number, 0, NULL);
                friend[friend_number].call_state_self |= TOXAV_FRIEND_CALL_STATE_SENDING_V;
                break;
            }
            default: { debug_error("uToxAV:\tUnhandled local call control\n"); }
                // TODO
                // TOXAV_CALL_CONTROL_RESUME,
                // TOXAV_CALL_CONTROL_PAUSE,
                // TOXAV_CALL_CONTROL_CANCEL,
                // TOXAV_CALL_CONTROL_MUTE_AUDIO,
                // TOXAV_CALL_CONTROL_UNMUTE_AUDIO,
        }
        if (bitrate_err) {
            debug_error("uToxAV:\tError setting/changing video bitrate\n");
        }
    }

    return;
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
                                     const uint8_t *y, const uint8_t *u, const uint8_t *v, int32_t ystride,
                                     int32_t ustride, int32_t vstride, void *user_data) {
    /* copy the vpx_image */
    /* 4 bits for the H*W, then a pixel for each color * size */
    debug("uToxAV:\tnew video frame from friend %u\n", friend_number);
    FRIEND *f       = &friend[friend_number];
    f->video_width  = width;
    f->video_height = height;
    size_t size     = width * height * 4;

    UTOX_FRAME_PKG *frame = calloc(1, sizeof(*frame));

    if (!frame) {
        debug_error("uToxAV:\tcan't malloc for incoming frame\n");
        return;
    }

    frame->w    = width;
    frame->h    = height;
    frame->size = size;
    frame->img  = malloc(size);
    yuv420tobgr(width, height, y, u, v, ystride, ustride, vstride, frame->img);
    if (f->video_inline) {
        // debug("uToxAV:\tInline this frame only frame.\n");
        if (current_frame) {
            if (current_frame->img) {
                free(current_frame->img);
            }
            free(current_frame);
        }
        current_frame = frame;
        postmessage(AV_INLINE_FRAME, friend_number, 0, NULL);
    } else {
        postmessage(AV_VIDEO_FRAME, friend_number + 1, 0, (void *)frame);
    }
}

static void utox_audio_friend_accepted(ToxAV *av, uint32_t friend_number, uint32_t state) {
    /* First accepted call back */
    debug_notice("uToxAV:\tFriend accepted call\n");
    friend[friend_number].call_state_friend = state;
    if (SELF_SEND_VIDEO(friend_number) && !FRIEND_ACCEPTING_VIDEO(friend_number)) {
        utox_av_local_call_control(av, friend_number, TOXAV_CALL_CONTROL_HIDE_VIDEO);
    }
    postmessage_utoxav(UTOXAV_OUTGOING_CALL_ACCEPTED, friend_number, 0, NULL);
    postmessage(AV_CALL_ACCEPTED, friend_number, 0, NULL);
}

/** respond to a Audio Video state change call back from toxav */
static void utox_callback_av_change_state(ToxAV *av, uint32_t friend_number, uint32_t state, void *userdata) {
    FRIEND *f = &friend[friend_number];
    if (state == 1) {
        // handle error
        debug_error("uToxAV:\tChange state with an error, this should never happen. Please send bug report!\n");
        utox_av_remote_disconnect(av, friend_number);
        return;
    } else if (state == 2) {
        debug_notice("uToxAV:\tCall ended with friend_number %u.\n", friend_number);
        utox_av_remote_disconnect(av, friend_number);
        message_add_type_notice(&f->msg, "Friend Has Ended the call!", 26, 0); /* TODO localization with S() SLEN() */
        return;
    } else if (!friend[friend_number].call_state_friend) {
        utox_audio_friend_accepted(av, friend_number, state);
    }

    if (friend[friend_number].call_state_friend ^ (state & TOXAV_FRIEND_CALL_STATE_SENDING_A)) {
        if (state & TOXAV_FRIEND_CALL_STATE_SENDING_A) {
            debug_info("uToxAV:\tFriend %u is now sending audio.\n", friend_number);
        } else {
            debug_info("uToxAV:\tFriend %u is no longer sending audio.\n", friend_number);
        }
    }
    if (friend[friend_number].call_state_friend ^ (state & TOXAV_FRIEND_CALL_STATE_SENDING_V)) {
        if (state & TOXAV_FRIEND_CALL_STATE_SENDING_V) {
            debug_info("uToxAV:\tFriend %u is now sending video.\n", friend_number);
        } else {
            debug_info("uToxAV:\tFriend %u is no longer sending video.\n", friend_number);
            list_reselect_current();
        }
    }
    if (friend[friend_number].call_state_friend ^ (state & TOXAV_FRIEND_CALL_STATE_ACCEPTING_A)) {
        if (state & TOXAV_FRIEND_CALL_STATE_ACCEPTING_A) {
            debug_info("uToxAV:\tFriend %u is now accepting audio.\n", friend_number);
        } else {
            debug_info("uToxAV:\tFriend %u is no longer accepting audio.\n", friend_number);
        }
    }
    if (friend[friend_number].call_state_friend ^ (state & TOXAV_FRIEND_CALL_STATE_ACCEPTING_V)) {
        if (state & TOXAV_FRIEND_CALL_STATE_ACCEPTING_V) {
            debug_info("uToxAV:\tFriend %u is now accepting video.\n", friend_number);
        } else {
            debug_info("uToxAV:\tFriend %u is no longer accepting video.\n", friend_number);
        }
    }

    friend[friend_number].call_state_friend = state;
}

static void utox_incoming_rate_change(ToxAV *AV, uint32_t f_num, uint32_t a_bitrate, uint32_t v_bitrate, void *ud) {
    /* Just accept what toxav wants the bitrate to be... */
    if (v_bitrate > (uint32_t)UTOX_MIN_BITRATE_VIDEO) {
        TOXAV_ERR_BIT_RATE_SET error = 0;
        toxav_bit_rate_set(AV, f_num, -1, v_bitrate, &error);
        if (error) {
            debug_error("ToxAV:\tSetting new Video bitrate has failed with error #%u\n", error);
        } else {
            debug_notice("uToxAV:\t\tVideo bitrate changed to %u\n", v_bitrate);
        }
    } else {
        debug_notice("uToxAV:\t\tVideo bitrate unchanged %u is less than %u\n", v_bitrate, UTOX_MIN_BITRATE_VIDEO);
    }
    return;
}

void set_av_callbacks(ToxAV *av) {
    /* Friend update callbacks */
    toxav_callback_call(av, &utox_av_incoming_call);
    toxav_callback_call_state(av, &utox_callback_av_change_state);

    /* Incoming data callbacks */
    toxav_callback_audio_receive_frame(av, &utox_av_incoming_frame_a);
    toxav_callback_video_receive_frame(av, &utox_av_incoming_frame_v);

    /* Data type change callbacks. */
    toxav_callback_bit_rate_status(av, &utox_incoming_rate_change);
}
// TODO
