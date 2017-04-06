#include "tox.h"

#include "avatar.h"
#include "dns.h"
#include "file_transfers.h"
#include "flist.h"
#include "friend.h"
#include "groups.h"
#include "debug.h"
#include "macros.h"
#include "self.h"
#include "settings.h"
#include "text.h"
#include "tox_bootstrap.h"
#include "tox_callbacks.h"
#include "utox.h"

#include "av/audio.h"
#include "av/utox_av.h"
#include "av/video.h"


#include "ui/edit.h"     // FIXME the toxcore thread shouldn't be interacting directly with the UI
#include "ui/switch.h"   // FIXME the toxcore thread shouldn't be interacting directly with the UI
#include "ui/dropdown.h"

#include "layout/background.h"
#include "layout/settings.h"

#include "native/thread.h"
#include "native/time.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <tox/tox.h>
#include <tox/toxencryptsave.h>

#include "main.h"

static bool save_needed = true;

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
        LOG_FATAL_ERR(EXIT_FAILURE, "Toxcore", "Fatal Error; unable to encrypt data!\n");
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
static void toxcore_bootstrap(Tox *tox, bool ipv6_enabled) {
    static unsigned int j = 0;

    if (j == 0) {
        j = rand();
    }

    int i = 0;
    while (i < 4) {
        struct bootstrap_node *d = &bootstrap_nodes[j++ % COUNTOF(bootstrap_nodes)];
        // do not add IPv6 bootstrap nodes if IPv6 is not enabled
        if (!ipv6_enabled && d->ipv6) {
            continue;
        }
        LOG_TRACE("Toxcore", "Bootstrapping with node %s udp: %d, tcp: %d", d->address, d->port_udp, d->port_tcp);
        tox_bootstrap(tox, d->address, d->port_udp, d->key, 0);
        tox_add_tcp_relay(tox, d->address, d->port_tcp, d->key, 0);
        i++;
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
    init_groups();

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
        LOG_TRACE("Toxcore", "Unencrypted save data written" );
    } else {
        UTOX_ENC_ERR enc_err = utox_encrypt_data(clear_data, clear_length, encrypted_data);
        if (enc_err) {
            /* encryption failed, write clear text data */
            save_needed = utox_data_save_tox(clear_data, clear_length);
            LOG_TRACE("Toxcore", "\n\n\t\tWARNING UTOX WAS UNABLE TO ENCRYPT DATA!\n\t\tDATA WRITTEN IN CLEAR TEXT!\n" );
        } else {
            save_needed = utox_data_save_tox(encrypted_data, encrypted_length);
            LOG_TRACE("Toxcore", "Encrypted save data written" );
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

    LOG_NOTE("Toxcore", "Restarting Toxcore");
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
            LOG_TRACE("Toxcore", "Sent typing state to friend (%d): %d" , typing_state.friendnumber, typing_state.sent_value);
        }
    }
}

static int load_toxcore_save(struct Tox_Options *options) {
    settings.save_encryption = 0;
    size_t   raw_length;
    uint8_t *raw_data = utox_data_load_tox(&raw_length);

    /* Check if we're loading a saved profile */
    if (raw_data && raw_length) {
        if (tox_is_data_encrypted(raw_data)) {
            size_t   cleartext_length = raw_length - TOX_PASS_ENCRYPTION_EXTRA_LENGTH;
            uint8_t *clear_data       = calloc(1, cleartext_length);
            settings.save_encryption   = 1;
            LOG_INFO("Toxcore", "Using encrypted data, trying password: ");

            UTOX_ENC_ERR decrypt_err = utox_decrypt_data(raw_data, raw_length, clear_data);
            if (decrypt_err) {
                if (decrypt_err == UTOX_ENC_ERR_LENGTH) {
                    LOG_WARN("Toxcore", "Password too short!\r");
                } else if (decrypt_err == UTOX_ENC_ERR_LENGTH) {
                    LOG_ERR("Toxcore", "Couldn't decrypt, wrong password?\r");
                } else {
                    LOG_ERR("Toxcore", "Unknown error, please file a bug report!" );
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
            LOG_INFO("Toxcore", "Using unencrypted save file; this could be insecure!");
            options->savedata_type   = TOX_SAVEDATA_TYPE_TOX_SAVE;
            options->savedata_data   = raw_data;
            options->savedata_length = raw_length;
            return 0;
        }
    }
    /* No save file at all, create new profile! */
    return -2;
}

static void log_callback(Tox *UNUSED(tox), TOX_LOG_LEVEL level, const char *file, uint32_t line,
                         const char *func, const char *message, void *UNUSED(user_data)) {
    if (message && file && line) {
        LOG_NET_TRACE("Toxcore", "TOXCORE LOGGING ERROR (%u): %s" , level, message);
        LOG_NET_TRACE("Toxcore", "     in: %s:%u" , file, line);
    } else if (func) {
        LOG_NET_TRACE("Toxcore", "TOXCORE LOGGING ERROR: %s" , func);
    } else {
        LOG_ERR("Toxcore logging", "TOXCORE LOGGING is broken!!:\tOpen an bug upstream");
    }
}

// initialize toxcore based on current settings
// returns 0 on success
// returns -1 on temporary error (waiting for password encryption)
// returns -2 on fatal error
static int init_toxcore(Tox **tox) {
    tox_thread_init = UTOX_TOX_THREAD_INIT_NONE;
    int save_status = 0;

    struct Tox_Options topt;
    tox_options_default(&topt);
    // tox_options_set_start_port(&topt, 0);
    // tox_options_set_end_port(&topt, 0);

    tox_options_set_log_callback(&topt, log_callback);

    tox_options_set_ipv6_enabled(&topt, settings.enable_ipv6);
    tox_options_set_udp_enabled(&topt, settings.enable_udp);

    tox_options_set_proxy_type(&topt, TOX_PROXY_TYPE_NONE);
    tox_options_set_proxy_host(&topt, proxy_address);
    tox_options_set_proxy_port(&topt, settings.proxy_port);

    #ifdef ENABLE_MULTIDEVICE
    tox_options_set_mdev_mirror_sent(&topt, 1);
    #endif

    save_status = load_toxcore_save(&topt);

    // TODO tox.c shouldn't be interacting with the UI on this level
    if (save_status == -1) {
        /* Save file exist, couldn't decrypt, don't start a tox instance
        TODO: throw an error to the UI! */
        panel_profile_password.disabled = false;
        panel_settings_master.disabled  = true;
        edit_setfocus(&edit_profile_password);
        postmessage_utox(REDRAW, 0, 0, NULL);
        return -1;
    } else if (save_status == -2) {
        /* New profile! */
        panel_profile_password.disabled = true;
        panel_settings_master.disabled  = false;
    } else {
        panel_profile_password.disabled = true;
        if (settings.show_splash) {
            panel_splash_page.disabled = false;
        } else {
            panel_settings_master.disabled = false;
        }
        edit_resetfocus();
    }
    postmessage_utox(REDRAW, 0, 0, NULL);

    if (settings.use_proxy) {
        topt.proxy_type = TOX_PROXY_TYPE_SOCKS5;
    }

    // Create main connection
    LOG_INFO("Toxcore", "Creating New Toxcore instance.\n"
             "\t\tIPv6 : %u\n"
             "\t\tUDP  : %u\n"
             "\t\tProxy: %u %s %u",
             topt.ipv6_enabled, topt.udp_enabled, topt.proxy_type, topt.proxy_host, topt.proxy_port);


    TOX_ERR_NEW tox_new_err = 0;

    *tox = tox_new(&topt, &tox_new_err);

    if (*tox == NULL) {
        if (settings.force_proxy) {
            LOG_ERR("Toxcore", "\t\tError #%u, Not going to try without proxy because of user settings.", tox_new_err);
            return -2;
        }
        LOG_ERR("Toxcore", "\t\tError #%u, Going to try without proxy.", tox_new_err);

        // reset proxy options as well as GUI and settings
        topt.proxy_type = TOX_PROXY_TYPE_NONE;
        settings.use_proxy = settings.force_proxy = 0;
        switch_proxy.switch_on = 0;

        *tox = tox_new(&topt, &tox_new_err);

        if (*tox == NULL) {
            LOG_ERR("Toxcore", "\t\tError #%u, Going to try without IPv6.", tox_new_err);

            // reset IPv6 options as well as GUI and settings
            topt.ipv6_enabled = 0;
            switch_ipv6.switch_on = settings.enable_ipv6 = 0;

            *tox = tox_new(&topt, &tox_new_err);

            if (*tox == NULL) {
                LOG_ERR("Toxcore", "\t\tFatal Error creating a Tox instance... Error #%u", tox_new_err);
                return -2;
            }
        }
    }

    free((void *)topt.savedata_data);

    /* Give toxcore the functions to call */
    set_callbacks(*tox);

    /* Connect to bootstrapped nodes in "tox_bootstrap.h" */
    toxcore_bootstrap(*tox, settings.enable_ipv6);

    if (save_status == -2) {
        LOG_NOTE("Toxcore", "No save file, using defaults" );
        load_defaults(*tox);
    }
    tox_after_load(*tox);

    return 0;
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
        if (toxcore_init_err == -2) {
            // fatal failure, unable to create tox instance
            LOG_ERR("Toxcore", "Unable to create Tox Instance (%d)" , toxcore_init_err);
            // set init to true because other code is waiting for it.
            // but indicate error state
            tox_thread_init = UTOX_TOX_THREAD_INIT_ERROR;
            while (!reconfig) {
                // Waiting for a message triggering the next reconfigure
                // avoid trying the creation of thousands of tox instances before user changes the settings
                if (tox_thread_msg) {
                    TOX_MSG *msg = &tox_msg;
                    // If msg->msg is 0, reconfig
                    if (!msg->msg) {
                        reconfig = (bool) msg->param1;
                        tox_thread_init = UTOX_TOX_THREAD_INIT_NONE;
                    }
                    // tox is not configured at this point ignore all other messages
                    tox_thread_msg = 0;
                } else {
                    yieldcpu(300);
                }
            }
            continue;
        } else if (toxcore_init_err) {
            /* Couldn't init toxcore, probably waiting for user password */
            yieldcpu(300);
            tox_thread_init = UTOX_TOX_THREAD_INIT_NONE;
            // ignore all messages in this stage
            tox_thread_msg = 0;
            reconfig = 1;
            continue;
        } else {
            init_self(tox);

            // Start the tox av session.
            TOXAV_ERR_NEW toxav_error;
            av = toxav_new(tox, &toxav_error);

            if (!av) {
                LOG_ERR("Toxcore", "Unable to get ToxAV (%u)" , toxav_error);
            }

            // Give toxcore the av functions to call
            set_av_callbacks(av);

            tox_thread_init = UTOX_TOX_THREAD_INIT_SUCCESS;

            /* init the friends list. */
            flist_start();
            postmessage_utox(UPDATE_TRAY, 0, 0, NULL);
            postmessage_utox(PROFILE_DID_LOAD, 0, 0, NULL);

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
                postmessage_utox(DHT_CONNECTED, connected, 0, NULL);
            }

            /* Wait 10 Billion ticks then verify connection. */
            time = get_time();
            if (time - last_connection >= (uint64_t)10 * 1000 * 1000 * 1000) {
                last_connection = time;
                if (!connected) {
                    toxcore_bootstrap(tox, settings.enable_ipv6);
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
                    tox_thread_init = UTOX_TOX_THREAD_INIT_NONE;
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
        LOG_TRACE("Toxcore", "av_thread exit, tox thread ending");
        toxav_kill(av);
        tox_kill(tox);
    }

    tox_thread_init = UTOX_TOX_THREAD_INIT_NONE;
    free_friends();
    free_groups();
    LOG_TRACE("Toxcore", "Tox thread:\tClean exit!");
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
                               void *data)
{
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
            /* param1: new nospam value
             */
            char *old_id = self.id_str;

            self.nospam = param1;

            sprintf(self.nospam_str, "%08X", self.nospam);
            tox_self_set_nospam(tox, self.nospam);

            /* update tox id */
            tox_self_get_address(tox, self.id_binary);
            id_to_string(self.id_str, self.id_binary);
            LOG_TRACE("Toxcore", "Tox ID: %.*s" , (int)self.id_str_length, self.id_str);

            /* Update avatar */
            avatar_move((uint8_t *)old_id, (uint8_t *)self.id_str);
            edit_setstr(&edit_nospam, self.nospam_str, sizeof(uint32_t) * 2);

            save_needed = true;
            break;
        }

        case TOX_SELF_NEW_DEVICE: {
        #ifdef ENABLE_MULTIDEVICE

            TOX_ERR_DEVICE_ADD error = 0;
            tox_self_add_device(tox, data + TOX_ADDRESS_SIZE, param1, data, &error);

            if (error) {
                LOG_ERR("Toxcore", "problem with adding device to self %u" , error);
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
                fid = tox_friend_add(tox, data, (uint8_t *)data + TOX_ADDRESS_SIZE, param1, &f_err);
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
                postmessage_utox(FRIEND_SEND_REQUEST, 1, addf_error, data);
            } else {
                utox_friend_init(tox, fid);
                postmessage_utox(FRIEND_SEND_REQUEST, 0, fid, data);
            }
            save_needed = 1;
            break;
        }

        case TOX_FRIEND_NEW_DEVICE: {
        #ifdef ENABLE_MULTIDEVICE
            LOG_INFO("Toxcore", "Adding new device to peer %u" , param1);
            tox_friend_add_device(tox, data, param1, 0);
            free(data);
            save_needed = 1;
        #endif
            break;
        }

        case TOX_FRIEND_ACCEPT: {
            /* data: FREQUEST
             */
            FREQUEST *req = data;
            TOX_ERR_FRIEND_ADD f_err;
            uint32_t fid = tox_friend_add_norequest(tox, req->bin_id, &f_err);
            if (!f_err) {
                utox_friend_init(tox, fid);
                postmessage_utox(FRIEND_ACCEPT_REQUEST, fid, 0, req);
            } else {
                char hex_id[TOX_ADDRESS_SIZE * 2];
                id_to_string(hex_id, req->bin_id);
                LOG_TRACE("Toxcore", "Unable to accept friend %s, error num = %i" , hex_id, fid);
            }
            save_needed = 1;
            break;
        }
        case TOX_FRIEND_DELETE: {
            /* param1: friend #
             */
            tox_friend_delete(tox, param1, 0);
            postmessage_utox(FRIEND_REMOVE, 0, 0, data);
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
            MSG_HEADER *mmsg = (MSG_HEADER *)data;

            TOX_MESSAGE_TYPE type;
            if (msg == TOX_SEND_ACTION) {
                type = TOX_MESSAGE_TYPE_ACTION;
            } else {
                type = TOX_MESSAGE_TYPE_NORMAL;
            }

            uint8_t *next = (uint8_t *)mmsg->via.txt.msg;
            while (param2 > TOX_MAX_MESSAGE_LENGTH) {
                uint16_t len = TOX_MAX_MESSAGE_LENGTH - utf8_unlen((char *)next + TOX_MAX_MESSAGE_LENGTH);
                tox_friend_send_message(tox, param1, type, next, len, 0);
                param2 -= len;
                next += len;
            }

            TOX_ERR_FRIEND_SEND_MESSAGE error = 0;

            // Send last or only message
            mmsg->receipt      = tox_friend_send_message(tox, param1, type, next, param2, &error);
            mmsg->receipt_time = 0;

            LOG_INFO("Toxcore", "Sending message, receipt %u" , mmsg->receipt);
            if (error) {
                LOG_ERR("Toxcore", "Error sending message... %u" , error);
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

            // LOG_TRACE("Toxcore", "Set typing state for friend (%d): %d" , typing_state.friendnumber, typing_state.sent_value);
            break;
        }

        /* File transfers are so in right now. */
        case TOX_FILE_SEND_NEW:
        case TOX_FILE_SEND_NEW_SLASH: {
            /* param1: friend #
             * param2: offset of first file name in data
             * data: file names
             */

            if (param2 == 0) {
                // This is the new default. Where the caller sends an opened file.
                UTOX_MSG_FT *msg = data;
                ft_send_file(tox, param1, msg->file, msg->name, strlen((char*)msg->name), NULL);
                free(msg->name);
                free(msg);
                break;
            }

            break;
        }

        case TOX_FILE_SEND_NEW_INLINE: {
            /* param1: friend id
               data: pointer to a TOX_SEND_INLINE_MSG struct
             */
            LOG_INFO("Toxcore", "Sending picture inline." );

            struct TOX_SEND_INLINE_MSG *img = data;
            uint8_t name[] = "utox-inline.png";
            ft_send_data(tox, param1, img->image, img->image_size, name, strlen((char *)name));
            free(data);
            break;
        }

        case TOX_FILE_ACCEPT: {
            /* param1: friend #
             * param2: file #
             * data: path to write file */
            if (utox_file_start_write(param1, param2, data, 0)) {
                /*  tox, friend#, file#,        START_FILE      */
                ft_local_control(tox, param1, param2, TOX_FILE_CONTROL_RESUME);
            } else {
                ft_local_control(tox, param1, param2, TOX_FILE_CONTROL_CANCEL);
            }
            free(data);
            break;
        }

        case TOX_FILE_ACCEPT_AUTO: {
            /* param1: friend #
             * param2: file #
             * data: open handle to file */
            if (utox_file_start_write(param1, param2, data, 1)) {
                /*  tox, friend#, file#,        START_FILE      */
                ft_local_control(tox, param1, param2, TOX_FILE_CONTROL_RESUME);
            } else {
                ft_local_control(tox, param1, param2, TOX_FILE_CONTROL_CANCEL);
            }
            break;
        }

        case TOX_FILE_RESUME: {
            if (data) {
                param2 = ((FILE_TRANSFER*)data)->file_number;
            }
            ft_local_control(tox, param1, param2, TOX_FILE_CONTROL_RESUME);
            break;
        }

        case TOX_FILE_PAUSE: {
            if (data) {
                param2 = ((FILE_TRANSFER*)data)->file_number;
            }
            ft_local_control(tox, param1, param2, TOX_FILE_CONTROL_PAUSE);
            break;
        }

        case TOX_FILE_CANCEL: {
            if (data) {
                param2 = ((FILE_TRANSFER*)data)->file_number;
            }
            ft_local_control(tox, param1, param2, TOX_FILE_CONTROL_CANCEL);
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
                LOG_TRACE("Toxcore", "Sending video call to friend %u" , param1);
            } else {
                v_bitrate = 0;
                LOG_TRACE("Toxcore", "Sending call to friend %u" , param1);
            }
            postmessage_utoxav(UTOXAV_OUTGOING_CALL_PENDING, param1, param2, NULL);

            TOXAV_ERR_CALL error = 0;
            toxav_call(av, param1, UTOX_DEFAULT_BITRATE_A, v_bitrate, &error);
            if (error) {
                switch (error) {
                    case TOXAV_ERR_CALL_MALLOC: {
                        LOG_TRACE("Toxcore", "Error making call to friend %u; Unable to malloc for this call." , param1);
                        break;
                    }
                    case TOXAV_ERR_CALL_FRIEND_ALREADY_IN_CALL: {
                        /* This shouldn't happen, but just in case toxav gets a call before uTox gets this message we
                         * can just pretend like we're answering a call... */
                        LOG_TRACE("Toxcore", "Error making call to friend %u; Already in call." , param1);
                        LOG_TRACE("Toxcore", "Forwarding and accepting call!" );

                        TOXAV_ERR_ANSWER ans_error = 0;
                        toxav_answer(av, param1, UTOX_DEFAULT_BITRATE_A, v_bitrate, &ans_error);
                        if (ans_error) {
                            LOG_TRACE("Toxcore", "Error trying to toxav_answer error (%i)" , ans_error);
                        } else {
                            postmessage_utoxav(UTOXAV_OUTGOING_CALL_ACCEPTED, param1, param2, NULL);
                        }
                        postmessage_utox(AV_CALL_ACCEPTED, param1, 0, NULL);

                        break;
                    }
                    default: {
                        /* Un-handled errors
                        TOXAV_ERR_CALL_SYNC,
                        TOXAV_ERR_CALL_FRIEND_NOT_FOUND,
                        TOXAV_ERR_CALL_FRIEND_NOT_CONNECTED,
                        TOXAV_ERR_CALL_FRIEND_ALREADY_IN_CALL,
                        TOXAV_ERR_CALL_INVALID_BIT_RATE,*/
                        LOG_TRACE("Toxcore", "Error making call to %u, error num is %i." , param1, error);
                        break;
                    }
                }
            } else {
                postmessage_utox(AV_CALL_RINGING, param1, param2, NULL);
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
                LOG_TRACE("Toxcore", "Answering video call." );
            } else {
                v_bitrate = 0;
                LOG_TRACE("Toxcore", "Answering audio call." );
            }

            toxav_answer(av, param1, UTOX_DEFAULT_BITRATE_A, v_bitrate, &error);

            if (error) {
                LOG_TRACE("Toxcore", "Error trying to toxav_answer error (%i)" , error);
            } else {
                postmessage_utoxav(UTOXAV_INCOMING_CALL_ANSWER, param1, param2, NULL);
            }
            postmessage_utox(AV_CALL_ACCEPTED, param1, 0, NULL);
            break;
        }
        case TOX_CALL_PAUSE_AUDIO: {
            /* param1: friend # */
            LOG_TRACE("TToxcore", "ODO bug, please report 001!!" );
            break;
        }
        case TOX_CALL_PAUSE_VIDEO: {
            /* param1: friend # */
            LOG_TRACE("Toxcore", "Ending video for active call!" );
            utox_av_local_call_control(av, param1, TOXAV_CALL_CONTROL_HIDE_VIDEO);
            break;
        }
        case TOX_CALL_RESUME_AUDIO: {
            /* param1: friend # */
            LOG_TRACE("Toxcore", "TODO bug, please report 002!!" );
            break;
        }
        case TOX_CALL_RESUME_VIDEO: {
            /* param1: friend # */
            LOG_TRACE("Toxcore", "Starting video for active call!" );
            utox_av_local_call_control(av, param1, TOXAV_CALL_CONTROL_SHOW_VIDEO);
            get_friend(param1)->call_state_self |= TOXAV_FRIEND_CALL_STATE_SENDING_V | TOXAV_FRIEND_CALL_STATE_ACCEPTING_V;
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
                GROUPCHAT *g = group_make(g_num);
                if (!g) {
                    return;
                }
                group_init(g, g_num, param2);
                postmessage_utox(GROUP_ADD, g_num, param2, NULL);
            }
            save_needed = true;
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
            save_needed = true;
            break;
        }
        case TOX_GROUP_SEND_INVITE: {
            /* param1: group #
             * param2: friend #
             */
            TOX_ERR_CONFERENCE_INVITE error = 0;
            tox_conference_invite(tox, param2, param1, &error);
            save_needed = true;
            break;
        }
        case TOX_GROUP_SET_TOPIC: {
            /* param1: group #
             * param2: topic length
             * data: topic
             */
            TOX_ERR_CONFERENCE_TITLE error = 0;

            tox_conference_set_title(tox, param1, data, param2, &error);
            postmessage_utox(GROUP_TOPIC, param1, param2, data);
            save_needed = true;
            break;
        }
        case TOX_GROUP_SEND_MESSAGE:
        case TOX_GROUP_SEND_ACTION: {
            /* param1: group #
             * param2: message length
             * data: message
             */
            TOX_MESSAGE_TYPE type;
            type = (msg == TOX_GROUP_SEND_ACTION ? TOX_MESSAGE_TYPE_ACTION : TOX_MESSAGE_TYPE_NORMAL);

            TOX_ERR_CONFERENCE_SEND_MESSAGE error = 0;
            tox_conference_send_message(tox, param1, type, data, param2, &error);
            free(data);
            break;
        }
        /* Disabled */
        case TOX_GROUP_AUDIO_START: {
            /* param1: group #
             */
            break;
            postmessage_utox(GROUP_AUDIO_START, param1, 0, NULL);
        }
        /* Disabled */
        case TOX_GROUP_AUDIO_END: {
            /* param1: group #
             */
            break;
            postmessage_utox(GROUP_AUDIO_END, param1, 0, NULL);
        }
    } // End of switch.
}

void id_to_string(char *dest, uint8_t *src) {
    to_hex(dest, src, TOX_ADDRESS_SIZE);
}
