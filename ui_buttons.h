/* buttons */

static void button_setcolors_success(BUTTON *b)
{
    b->c1 = COLOR_BUTTON_SUCCESS_BACKGROUND;
    b->c2 = COLOR_BUTTON_SUCCESS_HOVER_BACKGROUND;
    b->c3 = COLOR_BUTTON_SUCCESS_HOVER_BACKGROUND;
}

static void button_setcolors_danger(BUTTON *b)
{
    b->c1 = COLOR_BUTTON_DANGER_BACKGROUND;
    b->c2 = COLOR_BUTTON_DANGER_HOVER_BACKGROUND;
    b->c3 = COLOR_BUTTON_DANGER_HOVER_BACKGROUND;
}

static void button_setcolors_warning(BUTTON *b)
{
    b->c1 = COLOR_BUTTON_WARNING_BACKGROUND;
    b->c2 = COLOR_BUTTON_WARNING_HOVER_BACKGROUND;
    b->c3 = COLOR_BUTTON_WARNING_HOVER_BACKGROUND;
}

static void button_setcolors_disabled(BUTTON *b)
{
    b->c1 = COLOR_BUTTON_DISABLED_BACKGROUND;
    b->c2 = COLOR_BUTTON_DISABLED_BACKGROUND;
    b->c3 = COLOR_BUTTON_DISABLED_BACKGROUND;
}

static void button_copyid_onpress(void)
{
    address_to_clipboard();
}

static void button_audiopreview_onpress(void)
{
    if (!audio_preview)
        toxaudio_postmessage(AUDIO_PREVIEW_START, 0, 0, NULL);
    else
        toxaudio_postmessage(AUDIO_PREVIEW_END, 0, 0, NULL);

    audio_preview = !audio_preview;
}

static void button_audiopreview_updatecolor(BUTTON *b)
{
    if (audio_preview)
        button_setcolors_danger(b);
    else
        button_setcolors_success(b);
}

static void button_videopreview_onpress(void)
{
    if (video_preview) {
        video_preview = 0;
        video_end(0);
        toxvideo_postmessage(VIDEO_PREVIEW_END, 0, 0, NULL);
    } else if (video_width) {
        STRING *s = SPTR(WINDOW_TITLE_VIDEO_PREVIEW);
        video_begin(0, s->str, s->length, video_width, video_height);
        toxvideo_postmessage(VIDEO_PREVIEW_START, 0, 0, NULL);
        video_preview = 1;
    }
}


static void button_videopreview_updatecolor(BUTTON *b)
{
    if (video_preview)
        button_setcolors_danger(b);
    else
        button_setcolors_success(b);
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
    tox_postmessage(TOX_NEWGROUP, 1, 0, NULL);
}

static void button_transfer_onpress(void)
{
    list_selectswap();
}

static void button_settings_onpress(void)
{
    list_selectsettings();
}

static void button_group_audio_onpress(void)
{
    GROUPCHAT *g = sitem->data;
    if (g->audio_calling) {
        tox_postmessage(TOX_GROUP_AUDIO_END, (g - group), 0, NULL);
    } else {
        tox_postmessage(TOX_GROUP_AUDIO_START, (g - group), 0, NULL);
    }
}

static void button_group_audio_updatecolor(BUTTON *b)
{
    GROUPCHAT *g = sitem->data;
    if (g->type == TOX_GROUPCHAT_TYPE_AV)
        if (g->audio_calling)
            button_setcolors_danger(b);
        else
            button_setcolors_success(b);
    else
        button_setcolors_disabled(b);
}

static void button_call_onpress(void)
{
    FRIEND *f = sitem->data;

    switch(f->calling) {
    case CALL_INVITED: {
        tox_postmessage(TOX_ACCEPTCALL, f->callid, 0, NULL);
        debug("Accept Call: %u\n", f->callid);
        break;
    }

    case CALL_NONE: {
        if (f->online) {
            tox_postmessage(TOX_CALL, f - friend, 0, NULL);
            debug("Calling friend: %u\n", (uint32_t)(f - friend));
        }
        break;
    }

    case CALL_RINGING: {
        tox_postmessage(TOX_CANCELCALL, f->callid, f - friend, NULL);
        debug("Cancelling call: id = %u, friend = %d\n", f->callid, (int)(f - friend));
        break;
    }

    case CALL_OK:
    case CALL_OK_VIDEO: {
        tox_postmessage(TOX_HANGUP, f->callid, 0, NULL);
        debug("Ending call: %u\n", f->callid);
        break;
    }
    }
}

static void button_call_updatecolor(BUTTON *b)
{
    FRIEND *f = sitem->data;

    switch(f->calling) {
    case CALL_INVITED: {
        button_setcolors_warning(b);
        break;
    }

    case CALL_RINGING: {
        button_setcolors_warning(b);
        break;
    }

    case CALL_NONE: {
        if (f->online) {
            button_setcolors_success(b);
            break;
        }
        /* fall through */
    }

    case CALL_RINGING_VIDEO:
    case CALL_INVITED_VIDEO: {
        button_setcolors_disabled(b);
        break;
    }

    case CALL_OK:
    case CALL_OK_VIDEO: {
        button_setcolors_danger(b);
        break;
    }
    }
}

static void button_video_onpress(void)
{
    FRIEND *f = sitem->data;

    switch(f->calling) {
    case CALL_INVITED_VIDEO: {
        tox_postmessage(TOX_ACCEPTCALL, f->callid, 1, NULL);
        debug("Accept Call: %u\n", f->callid);
        break;
    }

    case CALL_NONE: {
        if (f->online) {
            tox_postmessage(TOX_CALL_VIDEO, f - friend, 0, NULL);
            debug("Calling friend: %u\n", (uint32_t)(f - friend));
        }
        break;
    }

    case CALL_RINGING_VIDEO: {
        tox_postmessage(TOX_CANCELCALL, f->callid, f - friend, NULL);
        debug("Cancelling call: id = %u, friend = %d\n", f->callid, (int)(f - friend));
        break;
    }


    case CALL_OK: {
        tox_postmessage(TOX_CALL_VIDEO_ON, f - friend, f->callid, NULL);
        debug("start sending video\n");
        break;
    }

    case CALL_OK_VIDEO: {
        tox_postmessage(TOX_CALL_VIDEO_OFF, f - friend, f->callid, NULL);
        debug("stop sending video\n");
        break;
    }
    }
}

static void button_video_updatecolor(BUTTON *b)
{
    FRIEND *f = sitem->data;

    switch(f->calling) {
    case CALL_INVITED_VIDEO: {
        button_setcolors_warning(b);
        break;
    }

    case CALL_RINGING_VIDEO: {
        button_setcolors_warning(b);
        break;
    }

    case CALL_NONE: {
        if (f->online) {
            button_setcolors_success(b);
            break;
        }
        /* fall through */
    }

    case CALL_RINGING:
    case CALL_INVITED: {
        button_setcolors_disabled(b);
        break;
    }

    case CALL_OK: {
        button_setcolors_success(b);
        break;
    }

    case CALL_OK_VIDEO: {
        button_setcolors_danger(b);
        break;
    }
    }
}

static void button_bottommenu_updatecolor(BUTTON *b)
{
    b->c1 = COLOR_MENU_BACKGROUND;
    b->c2 = COLOR_MENU_HOVER_BACKGROUND;
    b->c3 = COLOR_MENU_ACTIVE_BACKGROUND;
    b->ic = COLOR_MENU_TEXT;
    if (b->mousedown || b->disabled)
        b->ic = COLOR_MENU_ACTIVE_TEXT;
    b->cd = COLOR_MENU_ACTIVE_BACKGROUND;
}

static void button_sendfile_onpress(void)
{
    FRIEND *f = sitem->data;
    if (f->online) {
        openfilesend();
    }
}

static void button_sendfile_updatecolor(BUTTON *b)
{
    FRIEND *f = sitem->data;
    if (f->online) 
        button_setcolors_success(b);
     else 
        button_setcolors_disabled(b);
}

static void button_acceptfriend_onpress(void)
{
    FRIENDREQ *req = sitem->data;
    tox_postmessage(TOX_ACCEPTFRIEND, 0, 0, req);
}

static void button_avatar_onpress(void)
{
    openfileavatar();
}

static void contextmenu_avatar_onselect(uint8_t i)
{
    if (i == 0)
        self_remove_avatar();
}

static void button_avatar_onright(void)
{
    if (self_has_avatar()) {
        static UI_STRING_ID menu[] = {STR_REMOVE};
        contextmenu_new(countof(menu), menu, contextmenu_avatar_onselect);
    }
}


static void button_name_onpress(void)
{
    list_selectsettings();
    edit_setfocus(&edit_name);
}

static void button_statusmsg_onpress(void)
{
    list_selectsettings();
    edit_setfocus(&edit_status);
}

static void button_status_onpress(void)
{
    self.status++;
    if (self.status == 3) {
        self.status = 0;
    }

    tox_postmessage(TOX_SETSTATUS, self.status, 0, NULL);
}

/* top right chat message window button */
static void button_chat1_onpress(void)
{
    FRIEND *f = sitem->data;
    if (f->online) {
        desktopgrab(0);
    }
}

static void button_chat1_updatecolor(BUTTON *b)
{
    FRIEND *f = sitem->data;
    if (f->online) {
        button_setcolors_success(b);
    } else {
        button_setcolors_disabled(b);
    }
}

/* bottom right chat message window button */
static void button_chat2_onpress(void){
}

/* Button to send chat message */
static void button_chat_send_onpress(void){
    if (sitem->item == ITEM_FRIEND) {
        FRIEND *f = sitem->data;
        if (f->online) {
            edit_msg_onenter(&edit_msg);
            // reset focus to the chat window on send to prevent segfault. May break on android.
            edit_setfocus(&edit_msg);
        }
    } else {
        edit_msg_onenter(&edit_msg_group);
        // reset focus to the chat window on send to prevent segfault. May break on android.
        edit_setfocus(&edit_msg_group);
    }
}

static void button_chat_send_updatecolor(BUTTON *b){
    if (sitem->item == ITEM_FRIEND) {
        FRIEND *f = sitem->data;
        if (f->online) {
            button_setcolors_success(b);
        } else {
            button_setcolors_disabled(b);
        }
    } else {
        button_setcolors_success(b);
    }
}


BUTTON

button_add = {
    .bm2 = BM_ADD,
    .bw = _BM_ADD_WIDTH,
    .bh = _BM_ADD_WIDTH,
    .updatecolor = button_bottommenu_updatecolor,
    .onpress = button_add_onpress,
    .tooltip_text = { .i18nal = STR_ADDFRIENDS },
},

button_groups = {
    .bm2 = BM_GROUPS,
    .bw = _BM_ADD_WIDTH,
    .bh = _BM_ADD_WIDTH,
    .updatecolor = button_bottommenu_updatecolor,
    .onpress = button_groups_onpress,
},

button_transfer = {
    .bm2 = BM_TRANSFER,
    .bw = _BM_ADD_WIDTH,
    .bh = _BM_ADD_WIDTH,
    .updatecolor = button_bottommenu_updatecolor,
    .onpress = button_transfer_onpress,
},

button_settings = {
    .bm2 = BM_SETTINGS,
    .bw = _BM_ADD_WIDTH,
    .bh = _BM_ADD_WIDTH,
    .updatecolor = button_bottommenu_updatecolor,
    .onpress = button_settings_onpress,
    .tooltip_text = { .i18nal = STR_OTHERSETTINGS },
},

button_copyid = {
    .bm = BM_SBUTTON,
    .button_text = { .i18nal = STR_COPY_TOX_ID },
    .updatecolor = button_setcolors_success,
    .onpress = button_copyid_onpress,
},

button_addfriend = {
    .bm = BM_SBUTTON,
    .button_text = { .i18nal = STR_BUTTON_ADD_FRIEND },
    .updatecolor = button_setcolors_success,
    .onpress = button_addfriend_onpress,
},

button_call = {
    .bm = BM_LBUTTON,
    .bm2 = BM_CALL,
    .bw = _BM_LBICON_WIDTH,
    .bh = _BM_LBICON_HEIGHT,
    .onpress = button_call_onpress,
    .updatecolor = button_call_updatecolor,
},

button_group_audio = {
    .bm = BM_LBUTTON,
    .bm2 = BM_CALL,
    .bw = _BM_LBICON_WIDTH,
    .bh = _BM_LBICON_HEIGHT,
    .onpress = button_group_audio_onpress,
    .updatecolor = button_group_audio_updatecolor,
},

button_video = {
    .bm = BM_LBUTTON,
    .bm2 = BM_VIDEO,
    .bw = _BM_LBICON_WIDTH,
    .bh = _BM_LBICON_HEIGHT,
    .onpress = button_video_onpress,
    .updatecolor = button_video_updatecolor,
},


button_sendfile = {
    .bm = BM_LBUTTON,
    .bm2 = BM_FILE,
    .bw = _BM_LBICON_WIDTH,
    .bh = _BM_LBICON_HEIGHT,
    .onpress = button_sendfile_onpress,
    .updatecolor = button_sendfile_updatecolor,
},

button_acceptfriend = {
    .bm = BM_SBUTTON,
    .button_text = { .i18nal = STR_BUTTON_ACCEPT_FRIEND },
    .updatecolor = button_setcolors_success,
    .onpress = button_acceptfriend_onpress,
},

button_callpreview = {
    .bm = BM_LBUTTON,
    .bm2 = BM_CALL,
    .bw = _BM_LBICON_WIDTH,
    .bh = _BM_LBICON_HEIGHT,
    .onpress = button_audiopreview_onpress,
    .updatecolor = button_audiopreview_updatecolor,
},

button_videopreview = {
    .bm = BM_LBUTTON,
    .bm2 = BM_VIDEO,
    .bw = _BM_LBICON_WIDTH,
    .bh = _BM_LBICON_HEIGHT,
    .onpress = button_videopreview_onpress,
    .updatecolor = button_videopreview_updatecolor,
},

/* top right chat message window button */
button_chat1 = {
    .bm = BM_CB1,
    .bm2 = BM_CI1,
    .bw = _BM_CI_WIDTH,
    .bh = _BM_CI_WIDTH,
    .onpress = button_chat1_onpress,
    .updatecolor = button_chat1_updatecolor,
},

/* bottom right chat message window button */
button_chat2 = {
    .bm = BM_CB2,
    // @TODO: replace with something useful
    // .bm2 = BM_ADD,
    // .bw = _BM_ADD_WIDTH,
    // .bh = _BM_ADD_WIDTH,
    .onpress = button_chat2_onpress,
    .updatecolor = button_setcolors_disabled,
},

/* bottom right chat message window button */
button_chat_send = {
    .bm  = BM_CHAT_SEND,
    .bm2 = BM_CHAT_SEND_OVERLAY,
    .bw  = _BM_CHAT_SEND_OVERLAY_WIDTH,
    .bh  = _BM_CHAT_SEND_OVERLAY_WIDTH,
    .onpress = button_chat_send_onpress,
    .updatecolor = button_chat_send_updatecolor,
},

button_avatar = {
    .nodraw = 1,
    .onpress = button_avatar_onpress,
    .onright = button_avatar_onright,
},

button_name = {
    .nodraw = 1,
    .onpress = button_name_onpress,
},

button_statusmsg = {
    .nodraw = 1,
    .onpress = button_statusmsg_onpress,
},

button_status = {
    .nodraw = 1,
    .onpress = button_status_onpress,
};
