#include "main.h"

void postmessage_utoxav(uint8_t msg, uint32_t param1, uint32_t param2, void *data) {
    while(toxav_thread_msg) {
        yieldcpu(1);
    }

    toxav_msg.msg = msg;
    toxav_msg.param1 = param1;
    toxav_msg.param2 = param2;
    toxav_msg.data = data;

    toxav_thread_msg = 1;
}

#define VERIFY_AUDIO_IN()   if (call_count) { \
                                if (!audio_in) { \
                                    utox_audio_in_device_open(); \
                                    utox_audio_in_listen(); \
                                    audio_in = 1; \
                                } \
                            } else { \
                                utox_audio_in_ignore(); \
                                utox_audio_in_device_close(); \
                                audio_in = 0; \
                            } yieldcpu(10)

void utox_av_ctrl_thread(void *args) {
    ToxAV *av = args;

    utox_av_ctrl_init = 1;
    debug("Toxav thread init\n");

    volatile uint32_t call_count = 0;
    volatile _Bool audio_in  = 0;
    volatile _Bool video_on  = 0;

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

            switch(msg->msg) {
                case UTOXAV_INCOMING_CALL: {
                    call_count++;
                    postmessage_audio(AUDIO_PLAY_RINGTONE, msg->param1, msg->param2, NULL);
                    break;
                }
                case UTOXAV_INCOMING_CALL_ANSWER: {
                    if (msg->param1) {
                        VERIFY_AUDIO_IN();
                        FRIEND *f = &friend[msg->param1];
                        postmessage_audio(AUDIO_STOP_RINGTONE, msg->param1, msg->param2, NULL);
                        postmessage_audio(AUDIO_START_FRIEND, msg->param1, msg->param2, NULL);
                        f->call_state_self = ( TOXAV_FRIEND_CALL_STATE_SENDING_A | TOXAV_FRIEND_CALL_STATE_ACCEPTING_A );
                        if (msg->param2) {
                            // postmessage_video(VIDEO_RECORD_START, msg->param1, 0, NULL);
                            f->call_state_self |= (TOXAV_FRIEND_CALL_STATE_SENDING_V | TOXAV_FRIEND_CALL_STATE_ACCEPTING_V);
                        }
                    }
                    break;
                }
                case UTOXAV_INCOMING_CALL_REJECT: {
                    call_count--;
                    postmessage_audio(AUDIO_STOP_RINGTONE, msg->param1, msg->param2, NULL);
                    break;
                }
                case UTOXAV_START_CALL: {
                    call_count++;
                    VERIFY_AUDIO_IN();
                    if (msg->param1) {
                        FRIEND *f = &friend[msg->param1];
                        postmessage_audio(AUDIO_START_FRIEND, msg->param1, msg->param2, NULL);
                        postmessage_audio(AUDIO_STOP_RINGTONE, msg->param1, 0, NULL);
                        f->call_state_self = ( TOXAV_FRIEND_CALL_STATE_SENDING_A | TOXAV_FRIEND_CALL_STATE_ACCEPTING_A );
                        if (msg->param2) {
                            // postmessage_video(VIDEO_RECORD_START, msg->param1, 0, NULL);
                            f->call_state_self |= (TOXAV_FRIEND_CALL_STATE_SENDING_V | TOXAV_FRIEND_CALL_STATE_ACCEPTING_V);
                        }
                    } else if (msg->param2) {
                        // if(!groups_audio[m->param1]) {
                            // break;
                        // }
                        // groups_audio[m->param1] = 0;
                        // if(!call_count && record_on) {
                            debug("stop\n");
                        // }
                    }
                    break;
                }
                case UTOXAV_END_CALL: {
                    call_count--;
                    if (msg->param1) {
                        FRIEND *f = &friend[msg->param1];
                        if ((f->call_state_self | TOXAV_FRIEND_CALL_STATE_SENDING_V | TOXAV_FRIEND_CALL_STATE_ACCEPTING_V)){
                            // postmessage_video(VIDEO_RECORD_STOP, msg->param1, 0, NULL);
                        }
                        postmessage_audio(AUDIO_STOP_FRIEND, msg->param1, msg->param2, NULL);
                    } else if (msg->param2) {
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
                    if (msg->param1) { /* Start audio preview */
                        call_count++;
                        VERIFY_AUDIO_IN();
                        debug("uToxAV:\tStarting Audio Preview\n");
                        postmessage_audio(AUDIO_START_PREVIEW, 0, 0, NULL);
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
                        postmessage_audio(AUDIO_STOP_PREVIEW, 0, 0, NULL);
                    }
                    break;
                }

                case UTOXAV_START_VIDEO: {
                    if (msg->param1) {
                        //is preview
                    }
                    break;
                }
                case UTOXAV_STOP_VIDEO: {
                    if (msg->param1) {
                        //is preview
                    }
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
                    break;
                }

                case UTOXAV_SET_VIDEO_OUT: {
                    break;
                }
            }
            VERIFY_AUDIO_IN();
        }

        toxav_thread_msg = 0;
        toxav_iterate(av);
        yieldcpu(toxav_iteration_interval(av));
    }

    toxav_thread_msg = 0;
    utox_av_ctrl_init = 0;
    debug("UTOXAV:\tClean thread exit!\n");
    return;
}

static void utox_av_incoming_call(ToxAV *av, uint32_t friend_number, bool audio, bool video, void *UNUSED(userdata)) {
    debug("A/V Invite (%u)\n", friend_number);
    FRIEND *f = &friend[friend_number];

    f->call_state_self = 0;
    f->call_state_friend = ( audio << 2 | video << 3 | audio << 4 | video << 5 );
    debug("uTox AV:\tcall friend (%u) state for incoming call: %i\n", friend_number, f->call_state_friend);
    postmessage_utoxav(UTOXAV_INCOMING_CALL, friend_number, 0, NULL);
    postmessage(AV_CALL_INCOMING, friend_number, video, NULL);
}

static void utox_av_remote_disconnect(ToxAV *av, int32_t friend_number) {
    debug("uToxAV:\tRemote disconnect from friend %u\n", friend_number);
    postmessage_utoxav(UTOXAV_END_CALL, friend_number, 0, NULL);
    friend[friend_number].call_state_self = 0;
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
            debug("uToxAV:\tToxAV has disconnected!\n");
            break;
        }
        case TOXAV_ERR_CALL_CONTROL_SYNC: {
            debug("uToxAV:\tToxAV sync error!\n");
            break;
        }
        case TOXAV_ERR_CALL_CONTROL_FRIEND_NOT_FOUND: {
            debug("uToxAV:\tToxAV friend #%i not found.\n", friend_number);
            break;
        }
        case TOXAV_ERR_CALL_CONTROL_FRIEND_NOT_IN_CALL: {
            debug("uToxAV:\tToxAV no existing call for friend #%i.\n", friend_number);
            break;
        }
        case TOXAV_ERR_CALL_CONTROL_INVALID_TRANSITION: {
            debug("uToxAV:\tCall already paused, or already running.\n");
            break;
        }

    }
    friend[friend_number].call_state_self   = 0;
    friend[friend_number].call_state_friend = 0;
    postmessage(AV_CLOSE_WINDOW, friend_number + 1, 0, NULL);
    postmessage(AV_CALL_DISCONNECTED, friend_number, 0, NULL);
    postmessage_utoxav(UTOXAV_END_CALL, friend_number, 0, NULL);
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
                postmessage_video(VIDEO_RECORD_STOP, friend_number, 0, NULL);
                friend[friend_number].call_state_self &= (0xFF ^ TOXAV_FRIEND_CALL_STATE_SENDING_V);
                break;
            }
            case TOXAV_CALL_CONTROL_SHOW_VIDEO: {
                toxav_bit_rate_set(av, friend_number, -1, UTOX_DEFAULT_BITRATE_V, &bitrate_err);
                postmessage_video(VIDEO_RECORD_START, friend_number, 0, NULL);
                friend[friend_number].call_state_self |= TOXAV_FRIEND_CALL_STATE_SENDING_V;
                break;
            }
            default: {
                debug("uToxAV:\tUnhandled local call control\n");
            }
            // TODO
            // TOXAV_CALL_CONTROL_RESUME,
            // TOXAV_CALL_CONTROL_PAUSE,
            // TOXAV_CALL_CONTROL_CANCEL,
            // TOXAV_CALL_CONTROL_MUTE_AUDIO,
            // TOXAV_CALL_CONTROL_UNMUTE_AUDIO,
        }
        if (bitrate_err) {
            debug("uToxAV:\tError setting/changing video bitrate\n");
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
        debug("uToxAV:\tChange state with an error, this should never happen. Please send bug report!\n");
        utox_av_remote_disconnect(av, friend_number);
        return;
    } else if ( state == 2 ) {
        debug("uToxAV:\tCall ended with friend_number %u.\n", friend_number);
        utox_av_remote_disconnect(av, friend_number);
        return;
    } else if (!friend[friend_number].call_state_friend) {
        /* First accepted call back */
        debug("uToxAV:\tFriend accepted call\n");
        friend[friend_number].call_state_friend = state;
        if (SELF_SEND_VIDEO(friend_number) && !FRIEND_ACCEPTING_VIDEO(friend_number)) {
            utox_av_local_call_control(av, friend_number, TOXAV_CALL_CONTROL_HIDE_VIDEO);
        }
        postmessage_audio(AUDIO_STOP_RINGTONE, friend_number, 0, NULL);
        postmessage_audio(AUDIO_START_FRIEND, friend_number, 0, NULL);
        postmessage(AV_CALL_ACCEPTED, friend_number, 0, NULL);
    }

    if (friend[friend_number].call_state_friend ^ (state & TOXAV_FRIEND_CALL_STATE_SENDING_A)) {
        if (state & TOXAV_FRIEND_CALL_STATE_SENDING_A) {
            debug("uToxAV:\tFriend %u is now sending audio.\n", friend_number);
        } else {
            debug("uToxAV:\tFriend %u is no longer sending audio.\n", friend_number);
        }
    }
    if (friend[friend_number].call_state_friend ^ (state & TOXAV_FRIEND_CALL_STATE_SENDING_V)) {
        if (state & TOXAV_FRIEND_CALL_STATE_SENDING_V) {
            debug("uToxAV:\tFriend %u is now sending video.\n", friend_number);
        } else {
            debug("uToxAV:\tFriend %u is no longer sending video.\n", friend_number);
        }
    }
    if (friend[friend_number].call_state_friend ^ (state & TOXAV_FRIEND_CALL_STATE_ACCEPTING_A)) {
        if (state & TOXAV_FRIEND_CALL_STATE_ACCEPTING_A) {
            debug("uToxAV:\tFriend %u is now accepting audio.\n", friend_number);
        } else {
            debug("uToxAV:\tFriend %u is no longer accepting audio.\n", friend_number);
        }
    }
    if (friend[friend_number].call_state_friend ^ (state & TOXAV_FRIEND_CALL_STATE_ACCEPTING_V)) {
        if (state & TOXAV_FRIEND_CALL_STATE_ACCEPTING_V) {
            debug("uToxAV:\tFriend %u is now accepting video.\n", friend_number);
        } else {
            debug("uToxAV:\tFriend %u is no longer accepting video.\n", friend_number);
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
            debug("ToxAV:\tSetting new Video bitrate has failed with error #%u\n", error);
        } else {
            debug("uToxAV:\t\tVideo bitrate changed to %u\n", v_bitrate);
        }
    } else {
        debug("uToxAV:\t\tVideo bitrate unchanged %u is less than %u\n", v_bitrate, UTOX_MIN_BITRATE_VIDEO);
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
// TODO
