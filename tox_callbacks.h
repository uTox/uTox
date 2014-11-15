static void* copy_message(const uint8_t *str, uint16_t length, uint8_t msg_type)
{
    length = utf8_validate(str, length);

    MESSAGE *msg = malloc(sizeof(MESSAGE) + length);
    msg->author = 0;
    msg->msg_type = msg_type;
    msg->length = length;
    memcpy(msg->msg, str, length);

    return msg;
}

static void* copy_groupmessage(Tox *tox, const uint8_t *str, uint16_t length, uint8_t msg_type, int gid, int pid)
{
    uint8_t name[TOX_MAX_NAME_LENGTH];
    int namelen = tox_group_peername(tox, gid, pid, name);
    if(namelen == 0 || namelen == -1) {
        memcpy(name, "<unknown>", 9);
        namelen = 9;
    }

    length = utf8_validate(str, length);
    namelen = utf8_validate(name, namelen);


    MESSAGE *msg = malloc(sizeof(MESSAGE) + 1 + length + namelen);
    msg->author = 0;
    msg->msg_type = msg_type;
    msg->length = length;
    memcpy(msg->msg, str, length);

    msg->msg[length] = (char_t)namelen;
    memcpy(&msg->msg[length] + 1, name, namelen);

    return msg;
}

static void callback_friend_request(Tox *UNUSED(tox), const uint8_t *id, const uint8_t *msg, uint16_t length, void *UNUSED(userdata))
{
    length = utf8_validate(msg, length);

    FRIENDREQ *req = malloc(sizeof(FRIENDREQ) + length);

    req->length = length;
    memcpy(req->id, id, sizeof(req->id));
    memcpy(req->msg, msg, length);

    postmessage(FRIEND_REQUEST, 0, 0, req);

    /*int r = tox_add_friend_norequest(tox, id);
    void *data = malloc(TOX_FRIEND_ADDRESS_SIZE);
    memcpy(data, id, TOX_FRIEND_ADDRESS_SIZE);

    postmessage(FRIEND_ACCEPT, (r < 0), (r < 0) ? 0 : r, data);*/
}

static void callback_friend_message(Tox *tox, int fid, const uint8_t *message, uint16_t length, void *UNUSED(userdata))
{
    postmessage(FRIEND_MESSAGE, fid, 0, copy_message(message, length, MSG_TYPE_TEXT));

    debug("Friend Message (%u): %.*s\n", fid, length, message);

    log_write(tox, fid, message, length, 0, LOG_FILE_MSG_TYPE_TEXT);
}

static void callback_friend_action(Tox *tox, int fid, const uint8_t *action, uint16_t length, void *UNUSED(userdata))
{
    postmessage(FRIEND_MESSAGE, fid, 0, copy_message(action, length, MSG_TYPE_ACTION_TEXT));

    debug("Friend Action (%u): %.*s\n", fid, length, action);

    log_write(tox, fid, action, length, 0, LOG_FILE_MSG_TYPE_ACTION);
}

static void callback_name_change(Tox *UNUSED(tox), int fid, const uint8_t *newname, uint16_t length, void *UNUSED(userdata))
{
    length = utf8_validate(newname, length);

    void *data = malloc(length);
    memcpy(data, newname, length);

    postmessage(FRIEND_NAME, fid, length, data);

    debug("Friend Name (%u): %.*s\n", fid, length, newname);
}

static void callback_status_message(Tox *UNUSED(tox), int fid, const uint8_t *newstatus, uint16_t length, void *UNUSED(userdata))
{
    length = utf8_validate(newstatus, length);

    void *data = malloc(length);
    memcpy(data, newstatus, length);

    postmessage(FRIEND_STATUS_MESSAGE, fid, length, data);

    debug("Friend Status Message (%u): %.*s\n", fid, length, newstatus);
}

static void callback_user_status(Tox *UNUSED(tox), int fid, uint8_t status, void *UNUSED(userdata))
{
    postmessage(FRIEND_STATUS, fid, status, NULL);

    debug("Friend Userstatus (%u): %u\n", fid, status);
}

static void callback_typing_change(Tox *UNUSED(tox), int fid, uint8_t is_typing, void *UNUSED(userdata))
{
    postmessage(FRIEND_TYPING, fid, is_typing, NULL);

    debug("Friend Typing (%u): %u\n", fid, is_typing);
}

static void callback_read_receipt(Tox *UNUSED(tox), int fid, uint32_t receipt, void *UNUSED(userdata))
{
    //postmessage(FRIEND_RECEIPT, fid, receipt);

    debug("Friend Receipt (%u): %u\n", fid, receipt);
}

static void callback_connection_status(Tox *tox, int fid, uint8_t status, void *UNUSED(userdata))
{
    FRIEND *f = &friend[fid];
    int i;

    postmessage(FRIEND_ONLINE, fid, status, NULL);

    if(!status) {
        /* break all file transfers */
        for(i = 0; i != countof(f->incoming); i++) {
            if(f->incoming[i].status) {
                f->incoming[i].status = FT_BROKE;
                postmessage(FRIEND_FILE_IN_STATUS, fid, i, (void*)FILE_BROKEN);
            }
        }
        for(i = 0; i != countof(f->outgoing); i++) {
            if(f->outgoing[i].status) {
                f->outgoing[i].status = FT_BROKE;
                postmessage(FRIEND_FILE_OUT_STATUS, fid, i, (void*)FILE_BROKEN);
            }
        }
    } else {
        /* resume any broken file transfers */
        for(i = 0; i != countof(f->incoming); i++) {
            if(f->incoming[i].status == FT_BROKE) {
                tox_file_send_control(tox, fid, 1, i, TOX_FILECONTROL_RESUME_BROKEN, (void*)&f->incoming[i].bytes, sizeof(uint64_t));
            }
        }
    }

    debug("Friend Online/Offline (%u): %u\n", fid, status);
}

void callback_av_group_audio(Tox *tox, int groupnumber, int peernumber, const int16_t *pcm, unsigned int samples,
                                    uint8_t channels, unsigned int sample_rate, void *userdata);

static void callback_group_invite(Tox *tox, int fid, uint8_t type, const uint8_t *data, uint16_t length, void *UNUSED(userdata))
{
    int gid = -1;
    if (type == TOX_GROUPCHAT_TYPE_TEXT) {
        gid = tox_join_groupchat(tox, fid, data, length);
    } else if (type == TOX_GROUPCHAT_TYPE_AV) {
        gid = toxav_join_av_groupchat(tox, fid, data, length, &callback_av_group_audio, NULL);
    }

    if(gid != -1) {
        postmessage(GROUP_ADD, gid, 0, NULL);
    }

    debug("Group Invite (%i,f:%i) type %u\n", gid, fid, type);
}

static void callback_group_message(Tox *tox, int gid, int pid, const uint8_t *message, uint16_t length, void *UNUSED(userdata))
{
    postmessage(GROUP_MESSAGE, gid, 0, copy_groupmessage(tox, message, length, MSG_TYPE_TEXT, gid, pid));

    debug("Group Message (%u, %u): %.*s\n", gid, pid, length, message);
}

static void callback_group_action(Tox *tox, int gid, int pid, const uint8_t *action, uint16_t length, void *UNUSED(userdata))
{
    postmessage(GROUP_MESSAGE, gid, 0, copy_groupmessage(tox, action, length, MSG_TYPE_ACTION_TEXT, gid, pid));

    debug("Group Action (%u, %u): %.*s\n", gid, pid, length, action);
}

static void callback_group_namelist_change(Tox *tox, int gid, int pid, uint8_t change, void *UNUSED(userdata))
{
    switch(change) {
    case TOX_CHAT_CHANGE_PEER_ADD: {
        postmessage(GROUP_PEER_ADD, gid, pid, tox);
        break;
    }

    case TOX_CHAT_CHANGE_PEER_DEL: {
        postmessage(GROUP_PEER_DEL, gid, pid, tox);
        break;
    }

    case TOX_CHAT_CHANGE_PEER_NAME: {
        uint8_t name[TOX_MAX_NAME_LENGTH];
        int len = tox_group_peername(tox, gid, pid, name);

        len = utf8_validate(name, len);

        uint8_t *data = malloc(len + 1);
        data[0] = len;
        memcpy(data + 1, name, len);

        postmessage(GROUP_PEER_NAME, gid, pid, data);
        break;
    }
    }
    debug("Group Namelist Change (%u, %u): %u\n", gid, pid, change);
}

static void callback_group_title(Tox *tox, int gid, int pid, const uint8_t *title, uint8_t length, void *UNUSED(userdata))
{
    length = utf8_validate(title, length);
    if (!length)
        return;

    uint8_t *copy_title = malloc(length);
    if (!copy_title)
        return;

    memcpy(copy_title, title, length);
    postmessage(GROUP_TITLE, gid, length, copy_title);

    debug("Group Title (%u, %u): %.*s\n", gid, pid, length, title);
}
