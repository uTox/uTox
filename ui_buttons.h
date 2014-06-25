/* buttons */

static void button_copyid_onpress(void)
{
    address_to_clipboard();
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
    .panel = {
        .type = PANEL_BUTTON,
        .x = 0,
        .y = LIST_BOTTOM,
        .width = SCALE * 27,
        .height = -LIST_BOTTOM - 1,
    },
    .c1 = LIST_DARK,
    .c2 = LIST_DARK_LIGHT,
    .c3 = LIST_MAIN,
    .bm2 = BM_ADD,
    .bw = BM_ADD_WIDTH,
    .bh = BM_ADD_WIDTH,

    .onpress = button_add_onpress
},

button_groups = {
    .panel = {
        .type = PANEL_BUTTON,
        .x = SCALE * 28 * 1,
        .y = LIST_BOTTOM,
        .width = SCALE * 27,
        .height = -LIST_BOTTOM - 1,
    },
    .c1 = LIST_DARK,
    .c2 = LIST_DARK_LIGHT,
    .c3 = LIST_MAIN,
    .bm2 = BM_GROUPS,
    .bw = BM_ADD_WIDTH,
    .bh = BM_ADD_WIDTH,

    .onpress = button_groups_onpress
},

button_transfer = {
    .panel = {
        .type = PANEL_BUTTON,
        .x = SCALE * 28 * 2,
        .y = LIST_BOTTOM,
        .width = SCALE * 27,
        .height = -LIST_BOTTOM - 1,
    },
    .c1 = LIST_DARK,
    .c2 = LIST_DARK_LIGHT,
    .c3 = LIST_MAIN,
    .bm2 = BM_TRANSFER,
    .bw = BM_ADD_WIDTH,
    .bh = BM_ADD_WIDTH,

    .onpress = button_transfer_onpress
},

button_settings = {
    .panel = {
        .type = PANEL_BUTTON,
        .x = SCALE * 28 * 3,
        .y = LIST_BOTTOM,
        .width = SCALE * 27,
        .height = -LIST_BOTTOM - 1,
    },
    .c1 = LIST_DARK,
    .c2 = LIST_DARK_LIGHT,
    .c3 = LIST_MAIN,
    .bm2 = BM_SETTINGS,
    .bw = BM_ADD_WIDTH,
    .bh = BM_ADD_WIDTH,

    .onpress = button_settings_onpress
},

button_copyid = {
    .panel = {
        .type = PANEL_BUTTON,
        .x = SCALE * 31,
        .y = SCALE * 53,
        .width = BM_SBUTTON_WIDTH,
        .height = BM_SBUTTON_HEIGHT,
    },
    .bm = BM_SBUTTON,
    .c1 = C_GREEN,
    .c2 = C_GREEN_LIGHT,
    .c3 = C_GREEN_LIGHT,

    .onpress = button_copyid_onpress,
},

button_addfriend = {
    .panel = {
        .type = PANEL_BUTTON,
        .x = -SCALE * 5 - BM_SBUTTON_WIDTH - SCROLL_WIDTH,
        .y = SCALE * 84,
        .width = BM_SBUTTON_WIDTH,
        .height = BM_SBUTTON_HEIGHT,
    },
    .bm = BM_SBUTTON,
    .c1 = C_GREEN,
    .c2 = C_GREEN_LIGHT,
    .c3 = C_GREEN_LIGHT,

    .onpress = button_addfriend_onpress,
},

button_call = {
    .panel = {
        .type = PANEL_BUTTON,
        .x = -62 * SCALE,
        .y = 5 * SCALE,
        .width = BM_LBUTTON_WIDTH,
        .height = BM_LBUTTON_HEIGHT,
    },
    .bm = BM_LBUTTON,
    .c1 = C_GREEN,
    .c2 = C_GREEN_LIGHT,
    .c3 = C_GREEN_LIGHT,
    .bm2 = BM_CALL,
    .bw = BM_LBICON_WIDTH,
    .bh = BM_LBICON_HEIGHT,

    .onpress = button_call_onpress,
},

button_video = {
    .panel = {
        .type = PANEL_BUTTON,
        .x = -31 * SCALE,
        .y = 5 * SCALE,
        .width = BM_LBUTTON_WIDTH,
        .height = BM_LBUTTON_HEIGHT,
    },
    .bm = BM_LBUTTON,
    .c1 = C_GREEN,
    .c2 = C_GREEN_LIGHT,
    .c3 = C_GREEN_LIGHT,
    .bm2 = BM_VIDEO,
    .bw = BM_LBICON_WIDTH,
    .bh = BM_LBICON_HEIGHT,

    .onpress = button_video_onpress,
},


button_sendfile = {
    .panel = {
        .type = PANEL_BUTTON,
        .x = -93 * SCALE,
        .y = 5 * SCALE,
        .width = BM_LBUTTON_WIDTH,
        .height = BM_LBUTTON_HEIGHT,
    },
    .bm = BM_LBUTTON,
    .c1 = C_GREEN,
    .c2 = C_GREEN_LIGHT,
    .c3 = C_GREEN_LIGHT,
    .bm2 = BM_FILE,
    .bw = BM_LBICON_WIDTH,
    .bh = BM_LBICON_HEIGHT,

    .onpress = button_sendfile_onpress,
},

button_acceptfriend = {
    .panel = {
        .type = PANEL_BUTTON,
        .x = SCALE * 5,
        .y = LIST_Y + SCALE * 5,
        .width = BM_SBUTTON_WIDTH,
        .height = BM_SBUTTON_HEIGHT,
    },
    .bm = BM_SBUTTON,
    .c1 = C_GREEN,
    .c2 = C_GREEN_LIGHT,
    .c3 = C_GREEN_LIGHT,

    .onpress = button_acceptfriend_onpress,
};
