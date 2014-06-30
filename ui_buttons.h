/* buttons */

static void button_copyid_onpress(void)
{
    address_to_clipboard();
}

static void button_audiopreview_onpress(void)
{
    if(!audio_preview) {
        audio_preview = 1;
        toxav_postmessage(AV_AUDIO_PREVIEW_START, 0, 0, NULL);
    } else {
        audio_preview = 0;
        toxav_postmessage(AV_AUDIO_PREVIEW_STOP, 0, 0, NULL);
    }
}

static void button_audiopreview_updatecolor(BUTTON *b)
{
    if(audio_preview) {
        b->c1 = C_RED;
        b->c2 = C_RED_LIGHT;
        b->c3 = C_RED_LIGHT;
    } else {
        b->c1 = C_GREEN;
        b->c2 = C_GREEN_LIGHT;
        b->c3 = C_GREEN_LIGHT;
    }
}

static void button_videopreview_onpress(void)
{
    if(video_preview) {
        video_end(0);
        video_preview = 0;
    } else if(video_width) {
        video_begin(0, (uint8_t*)"Video Preview", sizeof("Video Preview") - 1, video_width, video_height);
        video_preview = 1;
    }
}

static void button_videopreview_updatecolor(BUTTON *b)
{
    if(video_preview) {
        b->c1 = C_RED;
        b->c2 = C_RED_LIGHT;
        b->c3 = C_RED_LIGHT;
    } else {
        b->c1 = C_GREEN;
        b->c2 = C_GREEN_LIGHT;
        b->c3 = C_GREEN_LIGHT;
    }
}

static void button_addfriend_onpress(void)
{
    friend_add(edit_addid_data, edit_addid.length, edit_addmsg.data, edit_addmsg.length);
    edit_resetfocus();
}

static void button_add_onpress(void)
{
    list_selectaddfriend();
}

static void button_groups_onpress(void)
{
    tox_postmessage(TOX_NEWGROUP, 0, 0, NULL);
}

static void button_transfer_onpress(void)
{
    list_selectswap();
}

static void button_settings_onpress(void)
{
    list_selectsettings();
}

static void button_call_onpress(void)
{
    FRIEND *f = sitem->data;
    if(f->calling & 4) {
        return;
    }

    switch(f->calling & 3) {
    case CALL_NONE: {
        tox_postmessage(TOX_CALL, f - friend, 0, NULL);
        debug("Calling friend: %u\n", (uint32_t)(f - friend));
        break;
    }

    case CALL_INVITED: {
        tox_postmessage(TOX_ACCEPTCALL, f->callid, 0, NULL);
        debug("Accept Call: %u\n", f->callid);
        break;
    }

    case CALL_OK: {
        tox_postmessage(TOX_HANGUP, f->callid, 0, NULL);
        debug("Ending call: %u\n", f->callid);
        break;
    }
    }
}

static void button_video_onpress(void)
{
    FRIEND *f = sitem->data;
    if(f->calling && !(f->calling & 4)) {
        return;
    }

    switch(f->calling & 3) {
    case CALL_NONE: {
        tox_postmessage(TOX_CALL_VIDEO, f - friend, 0, NULL);
        debug("Calling friend: %u\n", (uint32_t)(f - friend));
        break;
    }

    case CALL_INVITED: {
        tox_postmessage(TOX_ACCEPTCALL, f->callid, 1, NULL);
        debug("Accept Call: %u\n", f->callid);
        break;
    }

    case CALL_OK: {
        tox_postmessage(TOX_HANGUP, f->callid, 0, NULL);
        debug("Ending call: %u\n", f->callid);
        break;
    }
    }
}


static void button_sendfile_onpress(void)
{
    openfilesend();
}

static void button_acceptfriend_onpress(void)
{
    FRIENDREQ *req = sitem->data;
    tox_postmessage(TOX_ACCEPTFRIEND, 0, 0, req);
}

BUTTON

button_add = {
    .c1 = LIST_DARK,
    .c2 = LIST_DARK_LIGHT,
    .c3 = LIST_MAIN,
    .bm2 = BM_ADD,
    .bw = _BM_ADD_WIDTH,
    .bh = _BM_ADD_WIDTH,

    .onpress = button_add_onpress
},

button_groups = {
    .c1 = LIST_DARK,
    .c2 = LIST_DARK_LIGHT,
    .c3 = LIST_MAIN,
    .bm2 = BM_GROUPS,
    .bw = _BM_ADD_WIDTH,
    .bh = _BM_ADD_WIDTH,

    .onpress = button_groups_onpress
},

button_transfer = {
    .c1 = LIST_DARK,
    .c2 = LIST_DARK_LIGHT,
    .c3 = LIST_MAIN,
    .bm2 = BM_TRANSFER,
    .bw = _BM_ADD_WIDTH,
    .bh = _BM_ADD_WIDTH,

    .onpress = button_transfer_onpress
},

button_settings = {
    .c1 = LIST_DARK,
    .c2 = LIST_DARK_LIGHT,
    .c3 = LIST_MAIN,
    .bm2 = BM_SETTINGS,
    .bw = _BM_ADD_WIDTH,
    .bh = _BM_ADD_WIDTH,

    .onpress = button_settings_onpress
},

button_copyid = {
    .bm = BM_SBUTTON,
    .c1 = C_GREEN,
    .c2 = C_GREEN_LIGHT,
    .c3 = C_GREEN_LIGHT,
    .str = "Copy",

    .onpress = button_copyid_onpress,
},

button_addfriend = {
    .bm = BM_SBUTTON,
    .c1 = C_GREEN,
    .c2 = C_GREEN_LIGHT,
    .c3 = C_GREEN_LIGHT,
    .str = "Add",

    .onpress = button_addfriend_onpress,
},

button_call = {
    .bm = BM_LBUTTON,
    .c1 = C_GREEN,
    .c2 = C_GREEN_LIGHT,
    .c3 = C_GREEN_LIGHT,
    .bm2 = BM_CALL,
    .bw = _BM_LBICON_WIDTH,
    .bh = _BM_LBICON_HEIGHT,

    .onpress = button_call_onpress,
},

button_video = {
    .bm = BM_LBUTTON,
    .c1 = C_GREEN,
    .c2 = C_GREEN_LIGHT,
    .c3 = C_GREEN_LIGHT,
    .bm2 = BM_VIDEO,
    .bw = _BM_LBICON_WIDTH,
    .bh = _BM_LBICON_HEIGHT,

    .onpress = button_video_onpress,
},


button_sendfile = {
    .bm = BM_LBUTTON,
    .c1 = C_GREEN,
    .c2 = C_GREEN_LIGHT,
    .c3 = C_GREEN_LIGHT,
    .bm2 = BM_FILE,
    .bw = _BM_LBICON_WIDTH,
    .bh = _BM_LBICON_HEIGHT,

    .onpress = button_sendfile_onpress,
},

button_acceptfriend = {
    .bm = BM_SBUTTON,
    .c1 = C_GREEN,
    .c2 = C_GREEN_LIGHT,
    .c3 = C_GREEN_LIGHT,
    .str = "Add",

    .onpress = button_acceptfriend_onpress,
},

button_callpreview = {
    .bm = BM_LBUTTON,
    .c1 = C_GREEN,
    .c2 = C_GREEN_LIGHT,
    .c3 = C_GREEN_LIGHT,
    .bm2 = BM_CALL,
    .bw = _BM_LBICON_WIDTH,
    .bh = _BM_LBICON_HEIGHT,

    .onpress = button_audiopreview_onpress,
    .updatecolor = button_audiopreview_updatecolor,
},

button_videopreview = {
    .bm = BM_LBUTTON,
    .c1 = C_GREEN,
    .c2 = C_GREEN_LIGHT,
    .c3 = C_GREEN_LIGHT,
    .bm2 = BM_VIDEO,
    .bw = _BM_LBICON_WIDTH,
    .bh = _BM_LBICON_HEIGHT,

    .onpress = button_videopreview_onpress,
    .updatecolor = button_videopreview_updatecolor,
};
