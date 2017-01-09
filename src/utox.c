#include "utox.h"

#include "tox.h"
#include "filesys.h"
#include "friend.h"
#include "flist.h"
#include "commands.h"

#include "ui/dropdowns.h"
#include "ui/tooltip.h"

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

void utox_message_dispatch(UTOX_MSG utox_msg_id, uint16_t param1, uint16_t param2, void *data) {
    switch (utox_msg_id) {
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

            MSG_FILE *m = message_add_type_file(&f->msg, param2, file->incoming, file->inline_img, file->status,
                                                file->name, file->name_length,
                                                file->target_size, file->current_size);
            file_notify(f, m);

            // Give File Trasfers the ablity to update the message
            // TODO, make message references a thing, and add callback instead
            file->ui_data = m;

            redraw();
            break;
        }
        case FILE_INCOMING_NEW: {
            FILE_TRANSFER *file = data;

            FRIEND *f    = get_friend(param1);

            if (f->ft_autoaccept) {
                debug("Toxcore:\tAuto Accept enabled for this friend: sending accept to system\n");
                native_autoselect_dir_ft(param1, file);
            }

            MSG_FILE *m = message_add_type_file(&f->msg, param2, file->incoming, file->inline_img, file->status,
                                   file->name, file->name_length,
                                   file->target_size, file->current_size);
            file_notify(f, m);


            // Give File Trasfers the ablity to update the message
            // TODO, make message references a thing, and add callback instead
            file->ui_data = m;

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
            FRIEND        *f    = &friend[file->friend_number];
            MSG_FILE      *msg  = file->ui_data;

            if (!msg) {
                break;
            }

            if (msg->file_status != file->status) {
                file_notify(f, msg);
                msg->file_status = file->status;
            }
            msg->progress = file->current_size;
            msg->speed    = file->speed;

            if (file->in_memory) {
                msg->path = file->via.memory;
            } else {
                msg->path = file->path;
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
            // TODO implement notification
            //native_notify_new(NULL, NULL); // Intentional fallthrough
        }
        case FRIEND_MESSAGE_UPDATE: {
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
