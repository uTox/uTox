
static void callback_av_invite(int32_t call_index, void *arg)
{
    int fid = toxav_get_peer_id(arg, call_index, 0);
    postmessage(FRIEND_CALL_INVITE, fid, call_index, NULL);

    debug("A/V Invite (%i)\n", call_index);
}

static void callback_av_start(int32_t call_index, void *arg)
{
    toxav_prepare_transmission(arg, call_index, (ToxAvCodecSettings*)&av_DefaultSettings, 0);
    call[call_index].active = 1;

    int fid = toxav_get_peer_id(arg, call_index, 0);
    postmessage(FRIEND_CALL_START, fid, call_index, NULL);

    debug("A/V Start (%i)\n", call_index);
}

static void callback_av_cancel(int32_t call_index, void *arg)
{
    call[call_index].active = 0;

    int fid = toxav_get_peer_id(arg, call_index, 0);
    postmessage(FRIEND_CALL_END, fid, call_index, NULL);

    debug("A/V Cancel (%i)\n", call_index);
}

static void callback_av_reject(int32_t call_index, void *arg)
{
    debug("A/V Reject (%i)\n", call_index);

    int fid = toxav_get_peer_id(arg, call_index, 0);
    postmessage(FRIEND_CALL_END, fid, call_index, NULL);

    debug("A/V End (%i)\n", call_index);
}

static void callback_av_end(int32_t call_index, void *arg)
{
    call[call_index].active = 0;

    int fid = toxav_get_peer_id(arg, call_index, 0);
    postmessage(FRIEND_CALL_END, fid, call_index, NULL);

    debug("A/V End (%i)\n", call_index);
}

static void callback_av_ringing(int32_t call_index, void *arg)
{
    debug("A/V Ringing (%i)\n", call_index);
}

static void callback_av_starting(int32_t call_index, void *arg)
{
    toxav_prepare_transmission(arg, call_index, (ToxAvCodecSettings*)&av_DefaultSettings, 0);
    call[call_index].active = 1;

    int fid = toxav_get_peer_id(arg, call_index, 0);
    postmessage(FRIEND_CALL_START, fid, call_index, NULL);

    debug("A/V Starting (%i)\n", call_index);
}

static void callback_av_ending(int32_t call_index, void *arg)
{
    call[call_index].active = 0;

    int fid = toxav_get_peer_id(arg, call_index, 0);
    postmessage(FRIEND_CALL_END, fid, call_index, NULL);

    debug("A/V Ending (%i)\n", call_index);
}

static void callback_av_error(int32_t call_index, void *arg)
{
    call[call_index].active = 0;

    int fid = toxav_get_peer_id(arg, call_index, 0);
    postmessage(FRIEND_CALL_END, fid, call_index, NULL);

    debug("A/V Error (%i)\n", call_index);
}

static void callback_av_requesttimeout(int32_t call_index, void *arg)
{
    int fid = toxav_get_peer_id(arg, call_index, 0);
    postmessage(FRIEND_CALL_END, fid, call_index, NULL);

    debug("A/V ReqTimeout (%i)\n", call_index);
}

static void callback_av_peertimeout(int32_t call_index, void *arg)
{
    call[call_index].active = 0;

    int fid = toxav_get_peer_id(arg, call_index, 0);
    postmessage(FRIEND_CALL_END, fid, call_index, NULL);

    debug("A/V PeerTimeout (%i)\n", call_index);
}
