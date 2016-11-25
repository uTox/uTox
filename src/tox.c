// tox.c

#include <stdlib.h>
#include <tox/toxencryptsave.h>

#include "tox.h"
#include "tox_callbacks.h"

#include "avatar.h"
#include "commands.h"
#include "file_transfers.h"
#include "flist.h"
#include "friend.h"
#include "groups.h"
#include "main.h"
#include "tox_bootstrap.h"
#include "util.h"

#include "av/utox_av.h"
#include "ui/dropdown.h"
#include "ui/dropdowns.h"
#include "ui/edits.h"
#include "ui/switch.h"
#include "ui/switches.h"

static bool save_needed = 1;

enum {
    LOG_FILE_MSG_TYPE_TEXT   = 0,
    LOG_FILE_MSG_TYPE_ACTION = 1,
};

typedef struct {
    uint64_t time;
    uint16_t namelen, length;
    uint8_t  flags;
    uint8_t  msg_type;
    uint8_t  zeroes[2];
} LOG_FILE_MSG_HEADER_COMPAT;

static void tox_thread_message(Tox *tox, ToxAV *av, uint64_t time, uint8_t msg, uint32_t param1, uint32_t param2,
                               void *data);

void postmessage_toxcore(uint8_t msg, uint32_t param1, uint32_t param2, void *data) {
    while (tox_thread_msg) {
        yieldcpu(1);
    }

    if (!tox_thread_init) {
        /* Tox is not yet active, drop message (Probably a mistake) */
        return;
    }

    tox_msg.msg    = msg;
    tox_msg.param1 = param1;
    tox_msg.param2 = param2;
    tox_msg.data   = data;

    tox_thread_msg = 1;
}

static int utox_encrypt_data(void *clear_text, size_t clear_length, uint8_t *cypher_data) {
    size_t passphrase_length = edit_profile_password.length;

    if (passphrase_length < 4) {
        return UTOX_ENC_ERR_LENGTH;
    }

    uint8_t passphrase[passphrase_length];
    memcpy(passphrase, edit_profile_password.data, passphrase_length);
    TOX_ERR_ENCRYPTION err = 0;

    tox_pass_encrypt((uint8_t *)clear_text, clear_length, (uint8_t *)passphrase, passphrase_length, cypher_data, &err);

    if (err) {
        debug_error("Fatal Error; unable to encrypt data!\n");
        exit(10);
    }

    return err;
}

static int utox_decrypt_data(void *cypher_data, size_t cypher_length, uint8_t *clear_text) {
    size_t passphrase_length = edit_profile_password.length;

    if (passphrase_length < 4) {
        return UTOX_ENC_ERR_LENGTH;
    }

    uint8_t passphrase[passphrase_length];
    memcpy(passphrase, edit_profile_password.data, passphrase_length);
    TOX_ERR_DECRYPTION err = 0;
    tox_pass_decrypt((uint8_t *)cypher_data, cypher_length, (uint8_t *)passphrase, passphrase_length, clear_text, &err);

    switch (err) {
        case TOX_ERR_DECRYPTION_OK: return 0;
        case TOX_ERR_DECRYPTION_NULL:
        case TOX_ERR_DECRYPTION_INVALID_LENGTH:
        case TOX_ERR_DECRYPTION_BAD_FORMAT: return UTOX_ENC_ERR_BAD_DATA;
        case TOX_ERR_DECRYPTION_KEY_DERIVATION_FAILED: return UTOX_ENC_ERR_UNKNOWN;
        case TOX_ERR_DECRYPTION_FAILED: return UTOX_ENC_ERR_BAD_PASS;
    }
    return -1;
}

/* bootstrap to dht with bootstrap_nodes */
static void toxcore_bootstrap(Tox *tox) {
    static unsigned int j = 0;

    if (j == 0)
        j = rand();

    int i = 0;
    while (i < 4) {
        struct bootstrap_node *d = &bootstrap_nodes[j % countof(bootstrap_nodes)];
        tox_bootstrap(tox, d->address, d->port, d->key, 0);
        tox_add_tcp_relay(tox, d->address, d->port, d->key, 0);
        i++;
        j++;
    }
}

static void set_callbacks(Tox *tox) {
    utox_set_callbacks_friends(tox);
    utox_set_callbacks_groups(tox);
#ifdef ENABLE_MULTIDEVICE
    utox_set_callbacks_mdevice(tox);
#endif
    utox_set_callbacks_file_transfer(tox);
}

void tox_after_load(Tox *tox) {
    utox_friend_list_init(tox);

#ifdef ENABLE_MULTIDEVICE
    // self.group_list_count = tox_self_get_(tox);
    self.device_list_count = tox_self_get_device_count(tox);

    // devices_update_list();
    utox_devices_init();
    devices_update_ui();

    uint32_t i;
    for (i = 0; i < self.device_list_count; ++i) {
        utox_device_init(tox, i);
    }
#endif

    self.name_length = tox_self_get_name_size(tox);
    tox_self_get_name(tox, (uint8_t *)self.name);
    self.statusmsg_length = tox_self_get_status_message_size(tox);
    tox_self_get_status_message(tox, (uint8_t *)self.statusmsg);
    self.status = tox_self_get_status(tox);
}

static void load_defaults(Tox *tox) {
    uint8_t *name = (uint8_t *)DEFAULT_NAME, *status = (uint8_t *)DEFAULT_STATUS;
    uint16_t name_len = sizeof(DEFAULT_NAME) - 1, status_len = sizeof(DEFAULT_STATUS) - 1;

    tox_self_set_name(tox, name, name_len, 0);
    tox_self_set_status_message(tox, status, status_len, 0);
}

static void write_save(Tox *tox) {
    /* Get toxsave info from tox*/
    size_t clear_length     = tox_get_savedata_size(tox);
    size_t encrypted_length = clear_length + TOX_PASS_ENCRYPTION_EXTRA_LENGTH;

    uint8_t clear_data[clear_length];
    uint8_t encrypted_data[encrypted_length];

    tox_get_savedata(tox, clear_data);

    if (edit_profile_password.length == 0) {
        // user doesn't use encryption
        save_needed = utox_data_save_tox(clear_data, clear_length);
        debug("Toxcore:\tUnencrypted save data written\n");
    } else {
        UTOX_ENC_ERR enc_err = utox_encrypt_data(clear_data, clear_length, encrypted_data);
        if (enc_err) {
            /* encryption failed, write clear text data */
            save_needed = utox_data_save_tox(clear_data, clear_length);
            debug("\n\n\t\tWARNING UTOX WAS UNABLE TO ENCRYPT DATA!\n\t\tDATA WRITTEN IN CLEAR TEXT!\n\n");
        } else {
            save_needed = utox_data_save_tox(encrypted_data, encrypted_length);
            debug("Toxcore:\tEncrypted save data written\n");
        }
    }
}

void tox_settingschanged(void) {
    // free everything
    tox_connected = 0;

#ifdef ENABLE_MULTIDEVICE
    utox_devices_decon();
#endif

    flist_freeall();

    dropdown_list_clear(&dropdown_audio_in);
    dropdown_list_clear(&dropdown_audio_out);
    dropdown_list_clear(&dropdown_video);

    postmessage_utoxav(UTOXAV_KILL, 0, 0, NULL);

    // send the reconfig message!
    postmessage_toxcore(0, 1, 0, NULL);

    debug("Core:\tRestarting Toxcore");
    while (!tox_thread_init) {
        yieldcpu(1);
    }
}

/* 6 seconds */
#define UTOX_TYPING_NOTIFICATION_TIMEOUT (6ul * 1000 * 1000 * 1000)

static struct {
    Tox *    tox;
    uint16_t friendnumber;
    uint64_t time;
    bool     sent_value;
} typing_state = {
    .tox = NULL, .friendnumber = 0, .time = 0, .sent_value = 0,
};

static void utox_thread_work_for_typing_notifications(Tox *tox, uint64_t time) {
    if (typing_state.tox != tox) {
        // Guard against Tox engine restarts.
        return;
    }

    bool is_typing = (time < typing_state.time + UTOX_TYPING_NOTIFICATION_TIMEOUT);
    if (typing_state.sent_value ^ is_typing) {
        // Need to send an update.
        if (tox_self_set_typing(tox, typing_state.friendnumber, is_typing, 0)) {
            // Successfully sent. Mark new state.
            typing_state.sent_value = is_typing;
            debug("Sent typing state to friend (%d): %d\n", typing_state.friendnumber, typing_state.sent_value);
        }
    }
}

static int load_toxcore_save(struct Tox_Options *options) {
    settings.use_encryption = 0;
    size_t   raw_length;
    uint8_t *raw_data = utox_data_load_tox(&raw_length);

    /* Check if we're loading a saved profile */
    if (raw_data && raw_length) {
        if (tox_is_data_encrypted(raw_data)) {
            size_t   cleartext_length = raw_length - TOX_PASS_ENCRYPTION_EXTRA_LENGTH;
            uint8_t *clear_data       = calloc(1, cleartext_length);
            settings.use_encryption   = 1;
            debug("Using encrypted data, trying password: ");

            UTOX_ENC_ERR decrypt_err = utox_decrypt_data(raw_data, raw_length, clear_data);
            if (decrypt_err) {
                if (decrypt_err == UTOX_ENC_ERR_LENGTH) {
                    debug("Password too short!\r");
                } else if (decrypt_err == UTOX_ENC_ERR_LENGTH) {
                    debug("Couldn't decrypt, wrong password?\r");
                } else {
                    debug("Unknown error, please file a bug report!\n");
                }
                return -1;
            }

            if (clear_data && cleartext_length) {
                options->savedata_type   = TOX_SAVEDATA_TYPE_TOX_SAVE;
                options->savedata_data   = clear_data;
                options->savedata_length = cleartext_length;

                return 0;
            }
        } else {
            debug_info("Using unencrypted save file; this is insecure!\n\n");
            options->savedata_type   = TOX_SAVEDATA_TYPE_TOX_SAVE;
            options->savedata_data   = raw_data;
            options->savedata_length = raw_length;
            return 0;
        }
    }
    /* No save file at all, create new profile! */
    return -2;
}

static void log_callback(Tox *UNUSED(tox), TOX_LOG_LEVEL UNUSED(level), const char *UNUSED(file), uint32_t UNUSED(line), const char *func,
                         const char *message, void *UNUSED(user_data)) {
    if (message) {
        debug("TOXCORE LOGGING ERROR: %s\n", message);
    } else if (func) {
        debug("TOXCORE LOGGING ERROR: %s\n", func);
    }
}


static int init_toxcore(Tox **tox) {
    tox_thread_init = 0;
    int save_status = 0;

    struct Tox_Options topt;
    tox_options_default(&topt);
    // tox_options_set_start_port(&topt, 0);
    // tox_options_set_end_port(&topt, 0);

    tox_options_set_ipv6_enabled(&topt, settings.enable_ipv6);
    tox_options_set_udp_enabled(&topt, settings.enable_udp);

    tox_options_set_proxy_type(&topt, TOX_PROXY_TYPE_NONE);
    tox_options_set_proxy_host(&topt, proxy_address);
    tox_options_set_proxy_port(&topt, settings.proxy_port);

#ifdef ENABLE_MULTIDEVICE
    tox_options_set_mdev_mirror_sent(&topt, 1);
#endif

    save_status = load_toxcore_save(&topt);

    if (save_status == -1) {
        /* Save file exist, couldn't decrypt, don't start a tox instance
        TODO: throw an error to the UI! */
        panel_profile_password.disabled = 0;
        panel_settings_master.disabled  = 1;
        edit_setfocus(&edit_profile_password);
        return -1;
    } else if (save_status == -2) {
        /* New profile! */
        panel_profile_password.disabled = 1;
        panel_settings_master.disabled  = 0;
    } else {
        panel_profile_password.disabled = 1;
        if (settings.show_splash) {
            panel_splash_page.disabled = 0;
        } else {
            panel_settings_master.disabled = 0;
        }
        edit_resetfocus();
    }
    postmessage(REDRAW, 0, 0, NULL);

    if (settings.use_proxy) {
        topt.proxy_type = TOX_PROXY_TYPE_SOCKS5;
    }

    // Create main connection
    debug("CORE:\tCreating New Toxcore instance.\n"
          "\t\tIPv6 : %u\n"
          "\t\tUDP  : %u\n"
          "\t\tProxy: %u %s %u\n",
          topt.ipv6_enabled, topt.udp_enabled, topt.proxy_type, topt.proxy_host, topt.proxy_port);


    TOX_ERR_NEW tox_new_err = 0;

    *tox = tox_new(&topt, &tox_new_err);

    if (*tox == NULL) {
        debug("\t\tError #%u, Going to try without proxy.\n", tox_new_err);

        topt.proxy_type         = TOX_PROXY_TYPE_NONE;
        dropdown_proxy.selected = dropdown_proxy.over = 0;

        *tox = tox_new(&topt, &tox_new_err);

        if (!topt.proxy_type || *tox == NULL) {
            debug("\t\tError #%u, Going to try without IPv6.\n", tox_new_err);

            topt.ipv6_enabled     = 0;
            switch_ipv6.switch_on = settings.enable_ipv6 = 1;
            *tox                                         = tox_new(&topt, &tox_new_err);

            if (!topt.ipv6_enabled || *tox == NULL) {
                debug("\t\tFatal Error creating a Tox instance... Error #%u\n", tox_new_err);
                return -2;
            }
        }
    }

    free((void *)topt.savedata_data);

    /* Give toxcore the functions to call */
    set_callbacks(*tox);

    // tox_callback_log(*tox, &log_callback, NULL);

    /* Connect to bootstrapped nodes in "tox_bootstrap.h" */
    toxcore_bootstrap(*tox);

    if (save_status == -2) {
        debug("No save file, using defaults\n");
        load_defaults(*tox);
    }
    tox_after_load(*tox);

    return 0;
}

static void init_self(Tox *tox) {
    /* Set local info for self */
    edit_setstr(&edit_name, self.name, self.name_length);
    edit_setstr(&edit_status, self.statusmsg, self.statusmsg_length);

    /* Get tox id, and gets the hex version for utox */
    tox_self_get_address(tox, self.id_binary);
    id_to_string(self.id_str, self.id_binary);
    self.id_str_length = TOX_FRIEND_ADDRESS_SIZE * 2;
    debug("Tox ID: %.*s\n", (int)self.id_str_length, self.id_str);

    /* Get nospam */
    self.nospam = tox_self_get_nospam(tox);
    sprintf(self.nospam_str, "%X", self.nospam);

    avatar_init_self();
}

/** void toxcore_thread(void)
 *
 * Main tox function, starts a new toxcore for utox to use, and then spawns its
 * threads.
 *
 * Accepts and returns nothing.
 */
void toxcore_thread(void *UNUSED(args)) {
    Tox *  tox              = NULL;
    ToxAV *av               = NULL;
    bool   reconfig         = 1;
    int    toxcore_init_err = 0;

    while (reconfig) {
        reconfig = 0;

        toxcore_init_err = init_toxcore(&tox);
        if (toxcore_init_err) {
            /* Couldn't init toxcore, probably waiting for user password */
            yieldcpu(300);
            tox_thread_init = 0;
            reconfig        = 1;
            continue;
        } else {
            init_self(tox);

            // Start the tox av session.
            TOXAV_ERR_NEW toxav_error;
            av = toxav_new(tox, &toxav_error);

            if (!av) {
                debug_error("Tox:\tUnable to get toxAV (%u)\n", toxav_error);
            }

            // Give toxcore the av functions to call
            set_av_callbacks(av);

            tox_thread_init = 1;

            /* init the friends list. */
            flist_start();
            postmessage(UPDATE_TRAY, 0, 0, NULL);
            postmessage(PROFILE_DID_LOAD, 0, 0, NULL);

            // Start the treads
            thread(utox_av_ctrl_thread, av);

            /* Moved into the utoxav ctrl thread */
            // thread(utox_audio_thread, av);
            // thread(utox_video_thread, av);
        }

        bool     connected = 0;
        uint64_t last_save = get_time(), last_connection = get_time(), time;

        while (1) {
            // Put toxcore to work
            tox_iterate(tox, NULL);

            // Check currents connection
            if (!!tox_self_get_connection_status(tox) != connected) {
                connected = !connected;
                postmessage(DHT_CONNECTED, connected, 0, NULL);
            }

            /* Wait 10 Billion ticks then verify connection. */
            time = get_time();
            if (time - last_connection >= (uint64_t)10 * 1000 * 1000 * 1000) {
                last_connection = time;
                if (!connected) {
                    toxcore_bootstrap(tox);
                }

                // save every 1000.
                if (save_needed || (time - last_save >= (uint64_t)1000 * 1000 * 1000 * 1000)) {
                    // Save tox data
                    write_save(tox);
                    last_save = time;
                }
            }

            // If there's a message, load it, and send to the tox message thread
            if (tox_thread_msg) {
                TOX_MSG *msg = &tox_msg;
                // If msg->msg is 0, reconfig if needed and break from tox_do
                if (!msg->msg) {
                    reconfig        = msg->param1;
                    tox_thread_msg  = 0;
                    tox_thread_init = 0;
                    break;
                }
                tox_thread_message(tox, av, time, msg->msg, msg->param1, msg->param2, msg->data);
                tox_thread_msg = 0;
            }

            if (settings.send_typing_status) {
                // Thread active transfers and check if friend is typing
                utox_thread_work_for_typing_notifications(tox, time);
            }

            /* Ask toxcore how many ms to wait, then wait at the most 20ms */
            uint32_t interval = tox_iteration_interval(tox);
            yieldcpu((interval > 20) ? 20 : interval);
        }

        /* If for anyreason, we exit, write the save, and clear the password */
        write_save(tox);
        edit_setstr(&edit_profile_password, (char *)"", 0);

        // Wait for all a/v threads to return 0
        while (utox_audio_thread_init || utox_video_thread_init || utox_av_ctrl_init) {
            yieldcpu(1);
        }

        // Stop av threads, and toxcore.
        debug("av_thread exit, tox thread ending\n");
        toxav_kill(av);
        tox_kill(tox);
    }

    tox_thread_init = 0;
    debug("Tox thread:\tClean exit!\n");
}

/** General recommendations for working with threads in uTox
 *
 * There are two main threads, the tox worker thread, that interacts with Toxcore, and receives the callbacks. The other
 * is the 'uTox' thread that interacts with the user, (rather sends information to the GUI.) The tox thread and the uTox
 * thread may interact with each other, as you see fit. However the Toxcore thread has child threads that are a bit
 * temperamental. The ToxAV thread is a child of the Toxcore thread, and therefor will ideally only be called by the tox
 * thread. The ToxAV thread also has two children of it's own, an audio and a video thread. Both a & v threads should
 * only be called by the ToxAV thread to avoid deadlocks.
 */
static void tox_thread_message(Tox *tox, ToxAV *av, uint64_t time, uint8_t msg, uint32_t param1, uint32_t param2,
                               void *data) {
    switch (msg) {
        case TOX_SAVE: {
            save_needed = 1;
            break;
        }
        /* Change Self in core */
        case TOX_SELF_SET_NAME: {
            /* param1: name length
             * data: name
             */
            tox_self_set_name(tox, data, param1, 0);
            save_needed = 1;
            break;
        }
        case TOX_SELF_SET_STATUS: {
            /* param1: status length
             * data: status message
             */
            tox_self_set_status_message(tox, data, param1, 0);
            save_needed = 1;
            break;
        }
        case TOX_SELF_SET_STATE: {
            /* param1: status
             */
            tox_self_set_status(tox, param1);
            save_needed = 1;
            break;
        }

        case TOX_SELF_CHANGE_NOSPAM: {
            /* param1: new nospam
             */
            tox_self_set_nospam(tox, param1);

            //update tox id
            tox_self_get_address(tox, self.id_binary);
            id_to_string(self.id_str, self.id_binary);
            debug("Tox ID: %.*s\n", (int)self.id_str_length, self.id_str);

            save_needed = 1;
            break;
        }

        case TOX_SELF_NEW_DEVICE: {
#ifdef ENABLE_MULTIDEVICE

            TOX_ERR_DEVICE_ADD error = 0;
            tox_self_add_device(tox, data + TOX_FRIEND_ADDRESS_SIZE, param1, data, &error);

            if (error) {
                debug_error("Toxcore:\tproblem with adding device to self %u\n", error);
            } else {
                self.device_list_count++;
            }

#endif
            break;
        }


        /* Avatar status */
        case TOX_AVATAR_SET: {
            /* param1: avatar format
             * param2: length of avatar data
             * data: raw avatar data (PNG)
             */

            avatar_set_self(data, param2);
            save_needed = 1;
            break;
        }
        case TOX_AVATAR_UNSET: {
            avatar_unset_self();
            save_needed = 1;
            break;
        }

        /* Interact with contacts */
        case TOX_FRIEND_NEW: {
            /* param1: length of message
             * data: friend id + message
             */
            uint32_t           fid;
            TOX_ERR_FRIEND_ADD f_err;

            if (!param1) {
                STRING *default_add_msg = SPTR(DEFAULT_FRIEND_REQUEST_MESSAGE);
                fid = tox_friend_add(tox, data, (const uint8_t *)default_add_msg->str, default_add_msg->length, &f_err);
            } else {
                fid = tox_friend_add(tox, data, data + TOX_FRIEND_ADDRESS_SIZE, param1, &f_err);
            }

            if (f_err != TOX_ERR_FRIEND_ADD_OK) {
                uint8_t addf_error;
                switch (f_err) {
                    case TOX_ERR_FRIEND_ADD_TOO_LONG: addf_error       = ADDF_TOOLONG; break;
                    case TOX_ERR_FRIEND_ADD_NO_MESSAGE: addf_error     = ADDF_NOMESSAGE; break;
                    case TOX_ERR_FRIEND_ADD_OWN_KEY: addf_error        = ADDF_OWNKEY; break;
                    case TOX_ERR_FRIEND_ADD_ALREADY_SENT: addf_error   = ADDF_ALREADYSENT; break;
                    case TOX_ERR_FRIEND_ADD_BAD_CHECKSUM: addf_error   = ADDF_BADCHECKSUM; break;
                    case TOX_ERR_FRIEND_ADD_SET_NEW_NOSPAM: addf_error = ADDF_SETNEWNOSPAM; break;
                    case TOX_ERR_FRIEND_ADD_MALLOC: addf_error         = ADDF_NOMEM; break;
                    default: addf_error                                = ADDF_UNKNOWN; break;
                }
                postmessage(FRIEND_SEND_REQUEST, 1, addf_error, data);
            } else {
                utox_friend_init(tox, fid);
                postmessage(FRIEND_SEND_REQUEST, 0, fid, data);
            }
            save_needed = 1;
            break;
        }

        case TOX_FRIEND_NEW_DEVICE: {
#ifdef ENABLE_MULTIDEVICE
            debug_info("Toxcore:\tAdding new device to peer %u\n", param1);
            tox_friend_add_device(tox, data, param1, 0);
            free(data);
            save_needed = 1;
            break;
#endif
        }

        case TOX_FRIEND_ACCEPT: {
            /* data: FRIENDREQ
             */
            FRIENDREQ *        req = data;
            TOX_ERR_FRIEND_ADD f_err;
            uint32_t           fid = tox_friend_add_norequest(tox, req->id, &f_err);
            if (!f_err) {
                utox_friend_init(tox, fid);
                postmessage(FRIEND_ACCEPT_REQUEST, (f_err != TOX_ERR_FRIEND_ADD_OK),
                            (f_err != TOX_ERR_FRIEND_ADD_OK) ? 0 : fid, req);
            } else {
                char hex_id[TOX_FRIEND_ADDRESS_SIZE * 2];
                id_to_string(hex_id, self.id_binary);
                debug("Toxcore:\tUnable to accept friend %s, error num = %i\n", hex_id, fid);
            }
            save_needed = 1;
            break;
        }
        case TOX_FRIEND_DELETE: {
            /* param1: friend #
             */
            tox_friend_delete(tox, param1, 0);
            postmessage(FRIEND_REMOVE, 0, 0, data);
            save_needed = 1;
            break;
        }
        case TOX_FRIEND_ONLINE: {
            /* Moved to the call back... */
            break;
        }

        /* Default actions */
        case TOX_SEND_MESSAGE:
        case TOX_SEND_ACTION: {
            /* param1: friend #
             * param2: message length
             * data: message
             */
            MSG_TEXT *message = (void *)data;
            void *    p       = message->msg;

            TOX_MESSAGE_TYPE type;
            if (msg == TOX_SEND_ACTION) {
                type = TOX_MESSAGE_TYPE_ACTION;
            } else {
                type = TOX_MESSAGE_TYPE_NORMAL;
            }

            while (param2 > TOX_MAX_MESSAGE_LENGTH) {
                uint16_t len = TOX_MAX_MESSAGE_LENGTH - utf8_unlen(p + TOX_MAX_MESSAGE_LENGTH);
                tox_friend_send_message(tox, param1, type, p, len, 0);
                param2 -= len;
                p += len;
            }

            TOX_ERR_FRIEND_SEND_MESSAGE error = 0;

            // Send last or only message
            message->receipt      = tox_friend_send_message(tox, param1, type, p, param2, &error);
            message->receipt_time = 0;

            debug_info("Toxcore:\tSending message, receipt %u\n", message->receipt);
            if (error) {
                debug_error("Toxcore:\tError sending message... %u\n", error);
            }

            break;
        }
        case TOX_SEND_TYPING: {
            /* param1: friend #
             */

            // Check if user has switched to another friend window chat.
            // Take care not to react on obsolete data from old Tox instance.
            bool need_resetting =
                (typing_state.tox == tox) && (typing_state.friendnumber != param1) && (typing_state.sent_value);

            if (need_resetting) {
                // Tell previous friend that he's betrayed.
                tox_self_set_typing(tox, typing_state.friendnumber, 0, 0);
                // Mark that new friend doesn't know that we're typing yet.
                typing_state.sent_value = 0;
            }

            // Mark us as typing to this friend at the moment.
            // utox_thread_work_for_typing_notifications() will
            // send a notification if it deems necessary.
            typing_state.tox          = tox;
            typing_state.friendnumber = param1;
            typing_state.time         = time;

            // debug("Set typing state for friend (%d): %d\n", typing_state.friendnumber, typing_state.sent_value);
            break;
        }

        /* File transfers are so in right now. */
        case TOX_FILE_SEND_NEW:
        case TOX_FILE_SEND_NEW_SLASH: {
            /* param1: friend #
             * param2: offset of first file name in data
             * data: file names
             */

            /* If friend doesn't exist, don't send file. */
            if (param1 >= self.friend_list_size) {
                break;
            }

            if (param2 == 0xFFFF) {
                // paths with line breaks
                uint8_t *name = data, *p = data, *s = name;
                while (*p) {
                    bool end = 1;
                    while (*p) {
                        if (*p == '\n') {
                            *p  = 0;
                            end = 0;
                            break;
                        }

                        if (*p == '/' || *p == '\\') {
                            s = p + 1;
                        }
                        p++;
                    }

                    if (strcmp2(name, "file://") == 0) {
                        name += 7;
                    } /* tox, friend, path, filename, filename_length */
                    outgoing_file_send(tox, param1, name, s, p - s, TOX_FILE_KIND_DATA);
                    p++;
                    s = name = p;

                    if (end) {
                        break;
                    }
                }
            } else {
                // windows path list
                uint8_t *name      = data;
                bool     multifile = (name[param2 - 1] == 0);
                if (!multifile) {
                    /* tox, Friend, path, filename,      filename_length */
                    outgoing_file_send(tox,                                   /* tox              */
                                       param1,                                /* friend number    */
                                       name,                                  /* file path        */
                                       name + param2,                         /* file name        */
                                       strlen((const char *)(name + param2)), /* file name length */
                                       TOX_FILE_KIND_DATA);                   /* data type (file) */
                } else {
                    uint8_t *p = name + param2;
                    name += param2 - 1;
                    if (*(name - 1) != '\\') {
                        *name++ = '\\';
                    }
                    while (*p) {
                        int len = strlen((char *)p) + 1;
                        memmove(name, p, len);
                        p += len;
                        outgoing_file_send(tox, param1, data, name, len - 1, TOX_FILE_KIND_DATA);
                    }
                }
            }

            if (msg != TOX_FILE_SEND_NEW_SLASH) {
                free(data);
            }

            break;
        }
        case TOX_FILE_SEND_NEW_INLINE: {
            /* param1: friend id
               data: pointer to a TOX_SEND_INLINE_MSG struct
             */
            debug("Toxcore:\tSending picture inline.");

            outgoing_file_send(tox, param1, NULL, ((struct TOX_SEND_INLINE_MSG *)data)->image,
                               ((struct TOX_SEND_INLINE_MSG *)data)->image_size, TOX_FILE_KIND_DATA);
            free(data);
            break;
        }
        case TOX_FILE_ACCEPT: {
            /* param1: friend #
             * param2: file #
             * data: path to write file */
            if (utox_file_start_write(param1, param2, data) == 0) {
                /*  tox, friend#, file#,        START_FILE      */
                file_transfer_local_control(tox, param1, param2, TOX_FILE_CONTROL_RESUME);
            } else {
                file_transfer_local_control(tox, param1, param2, TOX_FILE_CONTROL_CANCEL);
            }
            break;
            free(data);
        }
        case TOX_FILE_ACCEPT_AUTO: {
            /* param1: friend #
             * param2: file #
             * data: path to write file */
            if (utox_file_start_write(param1, param2, data) == 0) {
                /*  tox, friend#, file#,        START_FILE      */
                file_transfer_local_control(tox, param1, param2, TOX_FILE_CONTROL_RESUME);
            } else {
                file_transfer_local_control(tox, param1, param2, TOX_FILE_CONTROL_CANCEL);
            }
            break;
            free(data);
        }
        case TOX_FILE_RESUME: {
            /* param1: friend #
             * param2: file #           tox, friend#, file#,       RESUME_FILE */
            file_transfer_local_control(tox, param1, param2, TOX_FILE_CONTROL_RESUME);
            break;
        }
        case TOX_FILE_PAUSE: {
            /* param1: friend #
             * param2: file #           tox, friend#, file#,        PAUSE_FILE */
            file_transfer_local_control(tox, param1, param2, TOX_FILE_CONTROL_PAUSE);
            break;
        }
        case TOX_FILE_CANCEL: {
            /* param1: friend #
             * param2: file #           tox, friend#, file#,        CANCEL_FILE */
            file_transfer_local_control(tox, param1, param2, TOX_FILE_CONTROL_CANCEL);
            break;
        }

        /* Audio & Video */
        case TOX_CALL_SEND: {
            /* param1: friend #
             */
            /* Set the video bitrate, if we're starting a video call. */
            int v_bitrate = 0;
            if (param2) {
                v_bitrate = UTOX_DEFAULT_BITRATE_V;
                debug("Toxcore:\tSending video call to friend %u\n", param1);
            } else {
                v_bitrate = 0;
                debug("Toxcore:\tSending call to friend %u\n", param1);
            }
            postmessage_utoxav(UTOXAV_OUTGOING_CALL_PENDING, param1, param2, NULL);

            TOXAV_ERR_CALL error = 0;
            toxav_call(av, param1, UTOX_DEFAULT_BITRATE_A, v_bitrate, &error);
            if (error) {
                switch (error) {
                    case TOXAV_ERR_CALL_MALLOC: {
                        debug("Toxcore:\tError making call to friend %u; Unable to malloc for this call.\n", param1);
                        break;
                    }
                    case TOXAV_ERR_CALL_FRIEND_ALREADY_IN_CALL: {
                        /* This shouldn't happen, but just in case toxav gets a call before uTox gets this message we
                         * can just pretend like we're answering a call... */
                        debug("Toxcore:\tError making call to friend %u; Already in call.\n", param1);
                        debug("Toxcore:\tForwarding and accepting call!\n");

                        TOXAV_ERR_ANSWER ans_error = 0;
                        toxav_answer(av, param1, UTOX_DEFAULT_BITRATE_A, v_bitrate, &ans_error);
                        if (error) {
                            debug("Toxcore:\tError trying to toxav_answer error (%i)\n", error);
                        } else {
                            postmessage_utoxav(UTOXAV_OUTGOING_CALL_ACCEPTED, param1, param2, NULL);
                        }
                        postmessage(AV_CALL_ACCEPTED, param1, 0, NULL);

                        break;
                    }
                    default: {
                        /* Un-handled errors
                        TOXAV_ERR_CALL_SYNC,
                        TOXAV_ERR_CALL_FRIEND_NOT_FOUND,
                        TOXAV_ERR_CALL_FRIEND_NOT_CONNECTED,
                        TOXAV_ERR_CALL_FRIEND_ALREADY_IN_CALL,
                        TOXAV_ERR_CALL_INVALID_BIT_RATE,*/
                        debug("Toxcore:\tError making call to %u, error num is %i.\n", param1, error);
                        break;
                    }
                }
            } else {
                postmessage(AV_CALL_RINGING, param1, param2, NULL);
            }
            break;
        }
        case TOX_CALL_INCOMING: { /* This is a call back, todo remove */ break;
        }
        case TOX_CALL_ANSWER: {
            /* param1: Friend_number #
             * param2: Accept Video? #
             */
            TOXAV_ERR_ANSWER error     = 0;
            int              v_bitrate = 0;

            if (param2) {
                v_bitrate = UTOX_DEFAULT_BITRATE_V;
                debug("Toxcore:\tAnswering video call.\n");
            } else {
                v_bitrate = 0;
                debug("Toxcore:\tAnswering audio call.\n");
            }

            toxav_answer(av, param1, UTOX_DEFAULT_BITRATE_A, v_bitrate, &error);

            if (error) {
                debug("Toxcore:\tError trying to toxav_answer error (%i)\n", error);
            } else {
                postmessage_utoxav(UTOXAV_INCOMING_CALL_ANSWER, param1, param2, NULL);
            }
            postmessage(AV_CALL_ACCEPTED, param1, 0, NULL);
            break;
        }
        case TOX_CALL_PAUSE_AUDIO: {
            /* param1: friend # */
            debug("TToxcore:\tODO bug, please report 001!!\n");
            break;
        }
        case TOX_CALL_PAUSE_VIDEO: {
            /* param1: friend # */
            debug("Toxcore:\tEnding video for active call!\n");
            utox_av_local_call_control(av, param1, TOXAV_CALL_CONTROL_HIDE_VIDEO);
            break;
        }
        case TOX_CALL_RESUME_AUDIO: {
            /* param1: friend # */
            debug("Toxcore:\tTODO bug, please report 002!!\n");
            break;
        }
        case TOX_CALL_RESUME_VIDEO: {
            /* param1: friend # */
            debug("Toxcore:\tStarting video for active call!\n");
            utox_av_local_call_control(av, param1, TOXAV_CALL_CONTROL_SHOW_VIDEO);
            friend[param1].call_state_self |= TOXAV_FRIEND_CALL_STATE_SENDING_V | TOXAV_FRIEND_CALL_STATE_ACCEPTING_V;
            break;
        }
        case TOX_CALL_DISCONNECT: {
            /* param1: friend_number
             */
            utox_av_local_disconnect(av, param1);
            break;
        }

        /* Groups are broken while we await the new GCs getting merged. */
        /*
        TOX_GROUP_JOIN,
        TOX_GROUP_PART, // 30
        TOX_GROUP_INVITE,
        TOX_GROUP_SET_TOPIC,
        TOX_GROUP_SEND_MESSAGE,
        TOX_GROUP_SEND_ACTION,
        TOX_GROUP_AUDIO_START, // 35
        TOX_GROUP_AUDIO_END,*/

        case TOX_GROUP_CREATE: {
            int g_num = -1;

            TOX_ERR_CONFERENCE_NEW error = 0;
            if (param1) {
                // TODO FIX THIS AFTER NEW GROUP API
                // g = toxav_add_av_groupchat(tox, &callback_av_group_audio, NULL);
                g_num = tox_conference_new(tox, &error);
            } else {
                g_num = tox_conference_new(tox, &error);
            }

            if (g_num != -1) {
                GROUPCHAT *g = &group[g_num];
                group_init(g, g_num, param2);
                postmessage(GROUP_ADD, g_num, param2, NULL);
            }
            save_needed = 1;
            break;
        }
        case TOX_GROUP_JOIN: {
        }
        case TOX_GROUP_PART: {
            /* param1: group #
             */
            postmessage_utoxav(UTOXAV_GROUPCALL_END, param1, param1, NULL);

            TOX_ERR_CONFERENCE_DELETE error = 0;
            tox_conference_delete(tox, param1, &error);
            save_needed = 1;
            break;
        }
        case TOX_GROUP_SEND_INVITE: {
            /* param1: group #
             * param2: friend #
             */
            TOX_ERR_CONFERENCE_INVITE error = 0;
            tox_conference_invite(tox, param2, param1, &error);
            save_needed = 1;
            break;
        }
        case TOX_GROUP_SET_TOPIC: {
            /* param1: group #
             * param2: topic length
             * data: topic
             */
            TOX_ERR_CONFERENCE_TITLE error = 0;

            tox_conference_set_title(tox, param1, data, param2, &error);
            postmessage(GROUP_TOPIC, param1, param2, data);
            save_needed = 1;
            break;
        }
        case TOX_GROUP_SEND_MESSAGE: {
            case TOX_GROUP_SEND_ACTION: {
                /* param1: group #
                 * param2: message length
                 * data: message
                 */
                TOX_MESSAGE_TYPE type =
                    (msg == TOX_GROUP_SEND_ACTION ? TOX_MESSAGE_TYPE_ACTION : TOX_MESSAGE_TYPE_NORMAL);

                TOX_ERR_CONFERENCE_SEND_MESSAGE error = 0;
                tox_conference_send_message(tox, param1, type, data, param2, &error);
                free(data);
                break;
            }
                /* param1: group #
                 * param2: message length
                 * data: message
                 */
                tox_conference_action_send(tox, param1, data, param2);
                free(data);
                break;
        }
        /* Disabled */
        case TOX_GROUP_AUDIO_START: {
            /* param1: group #
             */
            break;
            postmessage(GROUP_AUDIO_START, param1, 0, NULL);
        }
        /* Disabled */
        case TOX_GROUP_AUDIO_END: {
            /* param1: group #
             */
            break;
            postmessage(GROUP_AUDIO_END, param1, 0, NULL);
        }
    } // End of switch.
}

/** Translates status code to text then sends back to the user */
static void file_notify(FRIEND *f, MSG_FILE *msg) {
    STRING *str;
    switch (msg->file_status) {
        case FILE_TRANSFER_STATUS_NONE: {
            str = SPTR(TRANSFER_NEW);
            break;
        }
        case FILE_TRANSFER_STATUS_ACTIVE: {
            str = SPTR(TRANSFER_STARTED);
            break;
        }
        case FILE_TRANSFER_STATUS_PAUSED_BOTH: {
            str = SPTR(TRANSFER___);
            break;
        }
        case FILE_TRANSFER_STATUS_PAUSED_US:
        case FILE_TRANSFER_STATUS_PAUSED_THEM: {
            str = SPTR(TRANSFER_PAUSED);
            break;
        }
        case FILE_TRANSFER_STATUS_KILLED: {
            str = SPTR(TRANSFER_CANCELLED);
            break;
        }
        case FILE_TRANSFER_STATUS_COMPLETED: {
            str = SPTR(TRANSFER_COMPLETE);
            break;
        }
        case FILE_TRANSFER_STATUS_BROKEN:
        default: { // render unknown status as "transfer broken"
            str = SPTR(TRANSFER_BROKEN);
            break;
        }
    }

    friend_notify_msg(f, str->str, str->length);
}

static void call_notify(FRIEND *f, uint8_t status) {
    STRING *str;
    switch (status) {
        case UTOX_AV_INVITE: {
            str = SPTR(CALL_INVITED);
            break;
        }
        case UTOX_AV_RINGING: {
            str = SPTR(CALL_RINGING);
            break;
        }
        case UTOX_AV_STARTED: {
            str = SPTR(CALL_STARTED);
            break;
        }
        default: { // render unknown status as "call canceled"
            str = SPTR(CALL_CANCELLED);
            break;
        }
    }

    friend_notify_msg(f, str->str, str->length);
}

void tox_message(uint8_t tox_message_id, uint16_t param1, uint16_t param2, void *data) {
    switch (tox_message_id) {
        /* General core and networking messages */
        case TOX_DONE: {
            /* Does nothing. */
            break;
        }
        case DHT_CONNECTED: {
            /* param1: connection status (1 = connected, 0 = disconnected) */
            tox_connected = param1;
            if (tox_connected) {
                debug_notice("uTox:\tConnected to DHT!\n");
            } else {
                debug_notice("uTox:\tDisconnected from DHT!\n");
            }
            redraw();
            break;
        }
        case DNS_RESULT: {
            /* param1: result (0 = failure, 1 = success)
             * data: resolved tox id (if successful)
             */
            if (param1) {
                friend_addid(data, edit_add_msg.data, edit_add_msg.length);
            } else {
                addfriend_status = ADDF_BADNAME;
            }
            free(data);
            redraw();
            break;
        }

        /* OS interaction/integration messages */
        case AUDIO_IN_DEVICE: {
            /* param1: string
             * param2: default device?
             * data: device identifier.
             */
            if (UI_STRING_ID_INVALID == param1) {
                dropdown_list_add_hardcoded(&dropdown_audio_in, data, data);
            } else {
                dropdown_list_add_localized(&dropdown_audio_in, param1, data);
            }

            if (loaded_audio_in_device == (uint16_t)~0 && param2) {
                loaded_audio_in_device = (dropdown_audio_in.dropcount - 1);
            }

            if (loaded_audio_in_device != 0 && (dropdown_audio_in.dropcount - 1) == loaded_audio_in_device) {
                postmessage_utoxav(UTOXAV_SET_AUDIO_IN, 0, 0, data);
                dropdown_audio_in.selected = loaded_audio_in_device;
                loaded_audio_in_device     = 0;
            }
            break;
        }
        case AUDIO_OUT_DEVICE: {
            dropdown_list_add_hardcoded(&dropdown_audio_out, data, data);

            if (loaded_audio_out_device != 0 && (dropdown_audio_out.dropcount - 1) == loaded_audio_out_device) {
                postmessage_utoxav(UTOXAV_SET_AUDIO_OUT, 0, 0, data);
                dropdown_audio_out.selected = loaded_audio_out_device;
                loaded_audio_out_device     = 0;
            }

            break;
        }

        /* Client/User Interface messages. */
        case REDRAW: {
            ui_set_scale(ui_scale);
            redraw();
            break;
        }
        case TOOLTIP_SHOW: {
            tooltip_show();
            redraw();
            break;
        }
        case SELF_AVATAR_SET: {
            /* param1: size of data
             * data: png data
             */
            self_set_and_save_avatar(data, param1);
            free(data);
            redraw();
            break;
        }
        case UPDATE_TRAY: {
            update_tray();
            break;
        }
        case PROFILE_DID_LOAD: {
            if (g_select_add_friend_later) {
                g_select_add_friend_later = 0;
                flist_selectaddfriend();
            }
            redraw();
            break;
        }

        /* File transfer messages */
        case FILE_SEND_NEW: {
            FRIEND *       f    = &friend[param1];
            FILE_TRANSFER *file = data;

            message_add_type_file(&f->msg, file);
            // file_notify(f, file->ui_data);
            redraw();
            break;
        }
        case FILE_INCOMING_NEW: {
            FILE_TRANSFER *file = data;
            FRIEND *       f    = &friend[file->friend_number];

            if (f->ft_autoaccept) {
                debug("sending accept to core\n");
                native_autoselect_dir_ft(file->friend_number, file);
            }

            message_add_type_file(&f->msg, file);
            file_notify(f, file->ui_data);
            redraw();
            break;
        }
        case FILE_INCOMING_ACCEPT: {
            postmessage_toxcore(TOX_FILE_ACCEPT, param1, param2 << 16, data);
            break;
        }
        case FILE_UPDATE_STATUS: {
            if (!data) {
                break;
            }

            FILE_TRANSFER *file = data;
            FRIEND *       f    = &friend[file->friend_number];
            MSG_FILE *     msg  = file->ui_data;

            if (!msg) {
                break;
            }

            if (msg->file_status != file->status) {
                file_notify(f, msg);
                msg->file_status = file->status;
            }
            msg->progress = file->size_transferred;
            msg->speed    = file->speed;

            if (file->in_memory) {
                msg->path = (char *)file->memory;
            } else {
                msg->path = (char *)file->path;
            }
            redraw();
            // free(file);
            break;
        }
        case FILE_INLINE_IMAGE: {
            FRIEND * f = &friend[param1];
            uint16_t width, height;
            uint8_t *image;
            memcpy(&width, data, sizeof(uint16_t));
            memcpy(&height, data + sizeof(uint16_t), sizeof(uint16_t));
            memcpy(&image, data + sizeof(uint16_t) * 2, sizeof(uint8_t *));
            free(data);
            friend_recvimage(f, (NATIVE_IMAGE *)image, width, height);
            redraw();
            break;
        }

        /* Friend interaction messages. */
        /* Handshake
         * param1: friend id
         * param2: new online status(bool) */
        case FRIEND_ONLINE: {
            FRIEND *f = &friend[param1];

            if (friend_set_online(f, param2)) {
                redraw();
            }
            messages_send_from_queue(&f->msg, param1);
            break;
        }
        case FRIEND_NAME: {
            FRIEND *f = &friend[param1];
            friend_setname(f, data, param2);

            redraw();
            free(data);
            break;
        }
        case FRIEND_STATUS_MESSAGE: {
            FRIEND *f = &friend[param1];
            free(f->status_message);
            f->status_length  = param2;
            f->status_message = data;
            redraw();
            break;
        }
        case FRIEND_STATE: {
            FRIEND *f = &friend[param1];
            f->status = param2;
            redraw();
            break;
        }
        case FRIEND_AVATAR_SET: {
            /* param1: friend id
             * param2: png size
             * data: png data    */
            FRIEND *f = &friend[param1];
            uint8_t *avatar = data;
            size_t   size   = param2;

            avatar_set(&friend[param1].avatar, avatar, size);
            avatar_save(f->id_str, avatar, size);

            free(avatar);
            redraw();
            break;
        }
        case FRIEND_AVATAR_UNSET: {
            FRIEND *f = &friend[param1];
            avatar_unset(&f->avatar);
            // remove avatar from disk
            avatar_delete(f->id_str);

            redraw();
            break;
        }
        /* Interactions */
        case FRIEND_TYPING: {
            FRIEND *f = &friend[param1];
            friend_set_typing(f, param2);
            redraw();
            break;
        }
        case FRIEND_MESSAGE: {
            redraw();
            break;
        }
        /* Adding and deleting */
        case FRIEND_INCOMING_REQUEST: {
            /* data: pointer to FRIENDREQ structure
             */
            flist_addfriendreq(data);
            redraw();
            break;
        }
        case FRIEND_ACCEPT_REQUEST: {
            /* confirmation that friend has been added to friend list (accept) */
            if (!param1) {
                FRIEND *   f   = &friend[param2];
                FRIENDREQ *req = data;
                flist_addfriend2(f, req);
                flist_reselect_current();
                redraw();
            }

            free(data);
            break;
        }
        case FRIEND_SEND_REQUEST: {
            /* confirmation that friend has been added to friend list (add) */
            if (param1) {
                /* friend was not added */
                addfriend_status = param2;
            } else {
                /* friend was added */
                edit_add_id.length  = 0;
                edit_add_msg.length = 0;

                FRIEND *f = &friend[param2];
                memcpy(f->cid, data, sizeof(f->cid));
                flist_addfriend(f);

                addfriend_status = ADDF_SENT;
            }
            free(data);
            redraw();
            break;
        }
        case FRIEND_REMOVE: {
            FRIEND *f = data;
            // commented out incase you have multiple clients in the same data dir
            // and remove one as friend from the other
            //   (it would remove his avatar locally too otherwise)
            // char cid[TOX_PUBLIC_KEY_SIZE * 2];
            // cid_to_string(cid, f->cid);
            // delete_saved_avatar(friend_number);
            friend_free(f);
            break;
        }

        case AV_CALL_INCOMING: {
            call_notify(&friend[param1], UTOX_AV_INVITE);
            redraw();
            break;
        }
        case AV_CALL_RINGING: {
            call_notify(&friend[param1], UTOX_AV_RINGING);
            redraw();
            break;
        }
        case AV_CALL_ACCEPTED: {
            call_notify(&friend[param1], UTOX_AV_STARTED);
            redraw();
            break;
        }
        case AV_CALL_DISCONNECTED: {
            call_notify(&friend[param1], UTOX_AV_NONE);
            redraw();
            break;
        }
        case AV_VIDEO_FRAME: {
            /* param1: video handle to send frame to (friend id + 1 or 0 for preview)
               param2: self preview frame for pending call.
               data: packaged frame data */

            UTOX_FRAME_PKG *frame = data;
            if (ACCEPT_VIDEO_FRAME(param1 - 1) || param2) {
                STRING *s = SPTR(WINDOW_TITLE_VIDEO_PREVIEW);
                video_begin(param1, s->str, s->length, frame->w, frame->h);
                video_frame(param1, frame->img, frame->w, frame->h, 0);
                // TODO re-enable the resize option, disabled for reasons
            }
            free(frame->img);
            free(data);
            // Intentional fall through
        }
        case AV_INLINE_FRAME: {
            redraw();
            break;
        }
        case AV_CLOSE_WINDOW: {
            debug_info("uTox:\tClosing video feed\n");
            video_end(param1);
            redraw();
            break;
        }
        /* Group chat functions */
        case GROUP_ADD: {
            redraw();
            break;
        }
        case GROUP_MESSAGE: {
            GROUPCHAT *g = &group[param1];

            GROUPCHAT *selected = flist_get_selected()->data;
            if (selected != g) {
                g->unread_msg = 1;
            }
            redraw(); // ui_drawmain();

            break;
        }
        case GROUP_PEER_DEL: {
            GROUPCHAT *g = &group[param1];

            if (g->av_group) {
                g->last_recv_audio[param2]        = g->last_recv_audio[g->peer_count];
                g->last_recv_audio[g->peer_count] = 0;
                // REMOVED UNTIL AFTER NEW GCs group_av_peer_remove(g, param2);
                g->source[param2] = g->source[g->peer_count];
            }

            g->topic_length = snprintf((char *)g->topic, sizeof(g->topic), "%u users in chat", g->peer_count);
            if (g->topic_length >= sizeof(g->topic)) {
                g->topic_length = sizeof(g->topic) - 1;
            }

            redraw();

            break;
        }
        case GROUP_PEER_ADD:
        case GROUP_PEER_NAME: {
            GROUPCHAT *g = &group[param1];

            g->topic_length = snprintf((char *)g->topic, sizeof(g->topic), "%u users in chat", g->peer_count);
            if (g->topic_length >= sizeof(g->topic)) {
                g->topic_length = sizeof(g->topic) - 1;
            }

            GROUPCHAT *selected = flist_get_selected()->data;
            if (selected != g) {
                g->unread_msg = 1;
            }
            redraw();
            break;
        }

        case GROUP_TOPIC: {
            GROUPCHAT *g = &group[param1];

            if (param2 > sizeof(g->name)) {
                memcpy(g->name, data, sizeof(g->name));
                g->name_length = sizeof(g->name);
            } else {
                memcpy(g->name, data, param2);
                g->name_length = param2;
            }

            free(data);
            redraw();
            break;
        }
        case GROUP_AUDIO_START: {
            GROUPCHAT *g = &group[param1];

            if (g->av_group) {
                g->audio_calling = 1;
                postmessage_utoxav(UTOXAV_GROUPCALL_START, 0, param1, NULL);
                redraw();
            }
            break;
        }
        case GROUP_AUDIO_END: {
            GROUPCHAT *g = &group[param1];

            if (g->av_group) {
                g->audio_calling = 0;
                postmessage_utoxav(UTOXAV_GROUPCALL_END, 0, param1, NULL);
                redraw();
            }
            break;
        }

        case GROUP_UPDATE: {
            redraw();
            break;
        }
    }
}
