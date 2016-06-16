#include "../main.h"
/* buttons */
#ifdef UNITY
#include "xlib/mmenu.h"
extern _Bool unity_running;
#endif

/* Quick color change functions */
static void button_setcolors_success(BUTTON *b) {
    b->c1 = COLOR_BUTTON_SUCCESS_BACKGROUND;
    b->c2 = COLOR_BUTTON_SUCCESS_HOVER_BACKGROUND;
    b->c3 = COLOR_BUTTON_SUCCESS_HOVER_BACKGROUND;
    b->ct1 = COLOR_BUTTON_SUCCESS_TEXT;
    b->ct2 = COLOR_BUTTON_SUCCESS_HOVER_TEXT;
}

static void button_setcolors_danger(BUTTON *b) {
    b->c1 = COLOR_BUTTON_DANGER_BACKGROUND;
    b->c2 = COLOR_BUTTON_DANGER_HOVER_BACKGROUND;
    b->c3 = COLOR_BUTTON_DANGER_HOVER_BACKGROUND;
    b->ct1 = COLOR_BUTTON_DANGER_TEXT;
    b->ct2 = COLOR_BUTTON_DANGER_HOVER_TEXT;
}

static void button_setcolors_warning(BUTTON *b) {
    b->c1 = COLOR_BUTTON_WARNING_BACKGROUND;
    b->c2 = COLOR_BUTTON_WARNING_HOVER_BACKGROUND;
    b->c3 = COLOR_BUTTON_WARNING_HOVER_BACKGROUND;
    b->ct1 = COLOR_BUTTON_WARNING_TEXT;
    b->ct2 = COLOR_BUTTON_WARNING_HOVER_TEXT;
}

static void button_setcolors_disabled(BUTTON *b) {
    b->c1 = COLOR_BUTTON_DISABLED_BACKGROUND;
    b->c2 = COLOR_BUTTON_DISABLED_BACKGROUND;
    b->c3 = COLOR_BUTTON_DISABLED_BACKGROUND;
    b->ct1 = COLOR_BUTTON_DISABLED_TEXT;
    b->ct2 = COLOR_BUTTON_DISABLED_TEXT;
}

/* On-press functions followed by the update functions when needed... */
static void button_avatar_onpress(void) {
    if (tox_thread_init)
        openfileavatar();
}

/* TODO this is placed here (out of use order) for the following function, todo; create an external function to switch to the correct
 * settings page... */
extern PANEL panel_settings_profile, panel_settings_net, panel_settings_ui, panel_settings_av;
extern SCROLLABLE scrollbar_settings;
static void button_settings_sub_profile_onpress(void){
    scrollbar_settings.content_height = SCALE(260);
    list_selectsettings();
    panel_settings_profile.disabled = 0;
    panel_settings_net.disabled  = 1;
    panel_settings_ui.disabled   = 1;
    panel_settings_av.disabled   = 1;
}

static void button_name_onpress(void){
    button_settings_sub_profile_onpress();
    edit_setfocus(&edit_name);
}

static void button_statusmsg_onpress(void){
    button_settings_sub_profile_onpress();
    edit_setfocus(&edit_status);
}

static void button_status_onpress(void) {
    self.status++;
    if (self.status == 3) {
        self.status = 0;
    }

    #ifdef UNITY
    if(unity_running) {
        mm_set_status(self.status);
    }
    #endif

    postmessage_toxcore(TOX_SELF_SET_STATE, self.status, 0, NULL);
}

static void button_menu_update(BUTTON *b) {
    b->c1 = COLOR_BACKGROUND_MENU;
    b->c2 = COLOR_BACKGROUND_MENU_HOVER;
    b->c3 = COLOR_BACKGROUND_MENU_ACTIVE;
    b->ct1 = COLOR_MENU_TEXT;
    b->ct2 = COLOR_MENU_TEXT;
    if (b->mousedown || b->disabled) {
        b->ct1 = COLOR_MENU_ACTIVE_TEXT;
        b->ct2 = COLOR_MENU_ACTIVE_TEXT;
    }
    b->cd = COLOR_BACKGROUND_MENU_ACTIVE;
}

static void button_add_new_contact_onpress(void) {
    if (tox_thread_init) {
        /* Only change if we're logged in! */
        edit_setstr(&edit_add_id, (char_t *)edit_search.data, edit_search.length);
        edit_setstr(&edit_search, (char_t *)"", 0);
        list_selectaddfriend();
        edit_setfocus(&edit_add_msg);
    }
}

static void button_settings_onpress(void) {
    if (tox_thread_init) {
        list_selectsettings();
    }
}

static void button_filter_friends_onpress(void) {
    list_set_filter(!list_get_filter()); // this only works because right now there are only 2 filters
                                         // (none or online), basically a bool
}

static void button_copyid_onpress(void) {
    edit_setfocus(&edit_toxid);
    copy(0);
}

#ifdef EMOJI_IDS
static void button_change_id_type_onpress(void) {
    edit_resetfocus();
    if (self.id_buffer_length == TOX_FRIEND_ADDRESS_SIZE * 2) {
        self.id_buffer_length = bytes_to_emoji_string(self.id_buffer, sizeof(self.id_buffer), self.id_binary, TOX_FRIEND_ADDRESS_SIZE);
        edit_toxid.length = self.id_buffer_length;
    } else {
        id_to_string(self.id_buffer, self.id_binary);
        self.id_buffer_length = edit_toxid.length = TOX_FRIEND_ADDRESS_SIZE * 2;
    }
}
#endif

static void button_audiopreview_onpress(void) {
    if (!settings.audio_preview) {
        postmessage_utoxav(UTOXAV_START_AUDIO, 1, 0, NULL);
    } else {
        postmessage_utoxav(UTOXAV_STOP_AUDIO, 1, 0, NULL);
    }
    settings.audio_preview = !settings.audio_preview;
}

static void button_audiopreview_update(BUTTON *b) {
    if (settings.audio_preview){
        button_setcolors_danger(b);
    } else {
        button_setcolors_success(b);
    }
}

static void button_videopreview_onpress(void) {
    if (settings.video_preview) {
        postmessage_utoxav(UTOXAV_STOP_VIDEO, 0, 1, NULL);
    } else if (video_width && video_height) {
        postmessage_utoxav(UTOXAV_START_VIDEO, 0, 1, NULL);
    } else {
        debug("Button ERR:\tVideo_width = 0, can't preview\n");
    }
    settings.video_preview = !settings.video_preview;
}

static void button_videopreview_update(BUTTON *b) {
    if (settings.video_preview) {
        button_setcolors_danger(b);
    } else {
        button_setcolors_success(b);
    }
}

static void button_send_friend_request_onpress(void) {
    friend_add(edit_add_id.data, edit_add_id.length, edit_add_msg.data, edit_add_msg.length);
    edit_resetfocus();
}

static void button_settings_sub_net_onpress(void){
    scrollbar_settings.content_height = UTOX_SCALE(90);
    list_selectsettings();
    panel_settings_profile.disabled = 1;
    panel_settings_net.disabled  = 0;
    panel_settings_ui.disabled   = 1;
    panel_settings_av.disabled   = 1;
}

static void button_settings_sub_ui_onpress(void){
    scrollbar_settings.content_height = UTOX_SCALE(140);
    list_selectsettings();
    panel_settings_profile.disabled = 1;
    panel_settings_net.disabled  = 1;
    panel_settings_ui.disabled   = 0;
    panel_settings_av.disabled   = 1;
}

static void button_settings_sub_av_onpress(void){
    scrollbar_settings.content_height = UTOX_SCALE(150);
    list_selectsettings();
    panel_settings_profile.disabled = 1;
    panel_settings_net.disabled  = 1;
    panel_settings_ui.disabled   = 1;
    panel_settings_av.disabled   = 0;
}

static void button_group_audio_onpress(void) {
    GROUPCHAT *g = selected_item->data;
    if (g->audio_calling) {
        postmessage_toxcore(TOX_GROUP_AUDIO_END, (g - group), 0, NULL);
    } else {
        postmessage_toxcore(TOX_GROUP_AUDIO_START, (g - group), 0, NULL);
    }
}

static void button_group_audio_update(BUTTON *b) {
    GROUPCHAT *g = selected_item->data;
    if (g->av_group) {
        b->disabled = 0;
        if (g->audio_calling)
            button_setcolors_danger(b);
        else
            button_setcolors_success(b);
    } else {
        b->disabled = 1;
        button_setcolors_disabled(b);
    }
}

static void button_call_audio_onpress(void) {
    FRIEND *f = selected_item->data;
    if (f->call_state_self) {
        if (UTOX_SENDING_AUDIO(f->number)) {
            debug("Ending call: %u\n", f->number);
            /* var 3/4 = bool send video */
            postmessage_toxcore(TOX_CALL_DISCONNECT,  f->number, 0, NULL);
        } else {
            debug("Canceling call: friend = %d\n", f->number);
            postmessage_toxcore(TOX_CALL_DISCONNECT,  f->number, 0, NULL);
        }
    } else if (UTOX_AVAILABLE_AUDIO(f->number)) {
        debug("Accept Call: %u\n", f->number);
        postmessage_toxcore(TOX_CALL_ANSWER, f->number, 0, NULL);
    } else {
        if (f->online) {
            postmessage_toxcore(TOX_CALL_SEND, f->number, 0, NULL);
            debug("Calling friend: %u\n", f->number);
        }
    }
}

static void button_call_audio_update(BUTTON *b) {
    FRIEND *f = selected_item->data;
    if (UTOX_SENDING_AUDIO(f->number)) {
        button_setcolors_danger(b);
        b->disabled = 0;
    } else if (UTOX_AVAILABLE_AUDIO(f->number)) {
        button_setcolors_warning(b);
        b->disabled = 0;
    } else {
        if (f->online) {
            button_setcolors_success(b);
            b->disabled = 0;
        } else {
            button_setcolors_disabled(b);
            b->disabled = 1;
        }
    }
}

static void button_call_video_onpress(void) {
    FRIEND *f = selected_item->data;
    if (f->call_state_self) {
        if (SELF_ACCEPT_VIDEO(f->number)) {
            debug("Canceling call (video): %u\n", f->number);
            postmessage_toxcore(TOX_CALL_PAUSE_VIDEO,  f->number, 1, NULL);
        } else if (UTOX_SENDING_AUDIO(f->number)) {
            debug("Audio call inprogress, adding video\n");
            postmessage_toxcore(TOX_CALL_RESUME_VIDEO, f->number, 1, NULL);
        } else {
            debug("Ending call (video): %u\n",   f->number);
            postmessage_toxcore(TOX_CALL_DISCONNECT, f->number, 1, NULL);
        }
    } else if (f->call_state_friend) {
        debug("Accept Call (video): %u %u\n", f->number, f->call_state_friend);
        postmessage_toxcore(TOX_CALL_ANSWER, f->number, 1, NULL);
    } else {
        if (f->online) {
            postmessage_toxcore(TOX_CALL_SEND, f->number, 1, NULL);
            debug("Calling friend (video): %u\n", f->number);
        }
    }
}

static void button_call_video_update(BUTTON *b) {
    FRIEND *f = selected_item->data;
    if (SELF_SEND_VIDEO(f->number)) {
        button_setcolors_danger(b);
        b->disabled = 0;
    } else if (FRIEND_SENDING_VIDEO(f->number)) {
        button_setcolors_warning(b);
        b->disabled = 0;
    } else {
        if (f->online) {
            button_setcolors_success(b);
            b->disabled = 0;
        } else {
            button_setcolors_disabled(b);
            b->disabled = 1;
        }
    }
}

static void button_bottommenu_update(BUTTON *b) {
    b->c1 = COLOR_BACKGROUND_MENU;
    b->c2 = COLOR_BACKGROUND_MENU_HOVER;
    b->c3 = COLOR_BACKGROUND_MENU_ACTIVE;
    b->ct1 = COLOR_MENU_TEXT;
    b->ct2 = COLOR_MENU_TEXT;
    if (b->mousedown || b->disabled) {
        b->ct1 = COLOR_MENU_ACTIVE_TEXT;
        b->ct2 = COLOR_MENU_ACTIVE_TEXT;
    }
    b->cd = COLOR_BACKGROUND_MENU_ACTIVE;
}

static void button_accept_friend_onpress(void) {
    FRIENDREQ *req = selected_item->data;
    postmessage_toxcore(TOX_FRIEND_ACCEPT, 0, 0, req);
    panel_friend_request.disabled = 1;
}

static void contextmenu_avatar_onselect(uint8_t i) {
    if (i == 0)
        self_remove_avatar();
}

static void button_avatar_onright(void) {
    if (self_has_avatar()) {
        static UI_STRING_ID menu[] = {STR_REMOVE};
        contextmenu_new(countof(menu), menu, contextmenu_avatar_onselect);
    }
}

static void button_send_file_onpress(void) {
    FRIEND *f = selected_item->data;
    if (f->online) {
        openfilesend();
    }
}

static void button_send_file_update(BUTTON *b) {
    FRIEND *f = selected_item->data;
    if (f->online) {
        b->disabled = 0;
        button_setcolors_success(b);
    } else {
        b->disabled = 1;
        button_setcolors_disabled(b);
    }
}

static void button_send_screenshot_onpress(void) {
    FRIEND *f = selected_item->data;
    if (f->online) {
        desktopgrab(0);
    }
}

static void button_send_screenshot_update(BUTTON *b) {
    FRIEND *f = selected_item->data;
    if (f->online) {
        b->disabled = 0;
        button_setcolors_success(b);
    } else {
        b->disabled = 1;
        button_setcolors_disabled(b);
    }
}

/* Button to send chat message */
static void button_chat_send_onpress(void){
    if (selected_item->item == ITEM_FRIEND) {
        FRIEND *f = selected_item->data;
        if (f->online) {
            // TODO clear the chat bar with a /slash command
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

static void button_chat_send_update(BUTTON *b) {
    if (selected_item->item == ITEM_FRIEND) {
        FRIEND *f = selected_item->data;
        if (f->online) {
            b->disabled = 0;
            button_setcolors_success(b);
        } else {
            b->disabled = 1;
            button_setcolors_disabled(b);
        }
    } else {
        b->disabled = 0;
        button_setcolors_success(b);
    }
}

static void button_lock_uTox_onpress(void) {
    if (tox_thread_init && edit_profile_password.length > 3) {
        list_selectsettings();
        panel_profile_password.disabled = 0;
        panel_settings_master.disabled  = 1;
        tox_settingschanged();
    }
}

static void button_show_password_settings_onpress(void) {
    panel_profile_password_settings.disabled = 0;
}


BUTTON button_avatar = {
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
    .tooltip_text = { .i18nal = STR_STATUS },
},

button_filter_friends = {
    .nodraw       = 1,
    .onpress      = button_filter_friends_onpress,
    .tooltip_text = { .i18nal = STR_FILTER_CONTACT_TOGGLE },
},

button_add_new_contact = {
    .bm2          = BM_ADD,
    .bw           = _BM_ADD_WIDTH,
    .bh           = _BM_ADD_WIDTH,
    .update       = button_menu_update,
    .onpress      = button_add_new_contact_onpress,
    .disabled     = 1,
    .nodraw       = 1,
    .tooltip_text = { .i18nal = STR_ADDFRIENDS },
},

button_settings = {
    .bm2          = BM_SETTINGS,
    .bw           = _BM_ADD_WIDTH,
    .bh           = _BM_ADD_WIDTH,
    .update       = button_bottommenu_update,
    .onpress      = button_settings_onpress,
    .disabled     = 0,
    .nodraw       = 0,
    .tooltip_text = { .i18nal = STR_USERSETTINGS },
},

button_settings_sub_profile = {
    .nodraw       = 1,
    .onpress      = button_settings_sub_profile_onpress,
    .tooltip_text = { .i18nal = STR_UTOX_SETTINGS },
},

button_settings_sub_net = {
    .nodraw       = 1,
    .onpress      = button_settings_sub_net_onpress,
    .tooltip_text = { .i18nal = STR_NETWORK_SETTINGS },
},

button_settings_sub_ui = {
    .nodraw       = 1,
    .onpress      = button_settings_sub_ui_onpress,
    .tooltip_text = { .i18nal = STR_USERSETTINGS },
},

button_settings_sub_av = {
    .nodraw       = 1,
    .onpress      = button_settings_sub_av_onpress,
    .tooltip_text = { .i18nal = STR_AUDIO_VIDEO },
},


button_copyid = {
    .bm = BM_SBUTTON,
    .button_text = { .i18nal = STR_COPY_TOX_ID },
    .update = button_setcolors_success,
    .onpress = button_copyid_onpress,
    .disabled = 0,
},

#ifdef EMOJI_IDS
button_change_id_type = {
    .bm = BM_SBUTTON,
    //.button_text = { .i18nal = STR_COPY_TOX_ID },
    .update = button_setcolors_success,
    .onpress = button_change_id_type_onpress,
    .disabled = 0,
},
#endif

button_send_friend_request = {
    .bm = BM_SBUTTON,
    .button_text = { .i18nal = STR_ADD },
    .update = button_setcolors_success,
    .onpress = button_send_friend_request_onpress,
    .disabled = 0,
},

button_call_audio = {
    .bm = BM_LBUTTON,
    .bm2 = BM_CALL,
    .bw = _BM_LBICON_WIDTH,
    .bh = _BM_LBICON_HEIGHT,
    .onpress = button_call_audio_onpress,
    .update = button_call_audio_update,
    .tooltip_text = { .i18nal = STR_START_AUDIO_CALL },
},

button_call_video = {
    .bm = BM_LBUTTON,
    .bm2 = BM_VIDEO,
    .bw = _BM_LBICON_WIDTH,
    .bh = _BM_LBICON_HEIGHT,
    .onpress = button_call_video_onpress,
    .update = button_call_video_update,
    .tooltip_text = { .i18nal = STR_START_VIDEO_CALL },
},

button_group_audio = {
    .bm = BM_LBUTTON,
    .bm2 = BM_CALL,
    .bw = _BM_LBICON_WIDTH,
    .bh = _BM_LBICON_HEIGHT,
    .onpress = button_group_audio_onpress,
    .update = button_group_audio_update,
    .tooltip_text = { .i18nal = STR_GROUPCHAT_JOIN_AUDIO },
},

button_accept_friend = {
    .bm = BM_SBUTTON,
    .button_text = { .i18nal = STR_ADD },
    .update = button_setcolors_success,
    .onpress = button_accept_friend_onpress,
},

button_callpreview = {
    .bm = BM_LBUTTON,
    .bm2 = BM_CALL,
    .bw = _BM_LBICON_WIDTH,
    .bh = _BM_LBICON_HEIGHT,
    .onpress = button_audiopreview_onpress,
    .update = button_audiopreview_update,
    .disabled = 0,
},

button_videopreview = {
    .bm  = BM_LBUTTON,
    .bm2 = BM_VIDEO,
    .bw  = _BM_LBICON_WIDTH,
    .bh  = _BM_LBICON_HEIGHT,
    .onpress  = button_videopreview_onpress,
    .update   = button_videopreview_update,
    .disabled = 0,
},

button_send_file = {
    .bm  = BM_CHAT_BUTTON_LEFT,
    .bm2 = BM_FILE,
    .bw  = _BM_FILE_WIDTH,
    .bh  = _BM_FILE_HEIGHT,
    .onpress = button_send_file_onpress,
    .update  = button_send_file_update,
    .disabled = 1,
    .tooltip_text = { .i18nal = STR_SEND_FILE },
},

button_send_screenshot = {
    .bm  = BM_CHAT_BUTTON_RIGHT,
    .bm2 = BM_CHAT_BUTTON_OVERLAY_SCREENSHOT,
    .bw  = _BM_CHAT_BUTTON_OVERLAY_WIDTH,
    .bh  = _BM_CHAT_BUTTON_OVERLAY_HEIGHT,
    .update       = button_send_screenshot_update,
    .onpress      = button_send_screenshot_onpress,
    .tooltip_text = { .i18nal = STR_SENDSCREENSHOT },
},

button_chat_send = {
    .bm  = BM_CHAT_SEND,
    .bm2 = BM_CHAT_SEND_OVERLAY,
    .bw  = _BM_CHAT_SEND_OVERLAY_WIDTH,
    .bh  = _BM_CHAT_SEND_OVERLAY_HEIGHT,
    .onpress = button_chat_send_onpress,
    .update = button_chat_send_update,
    .tooltip_text = { .i18nal = STR_SENDMESSAGE },
},

button_lock_uTox = {
    .bm           = BM_SBUTTON,
    .update       = button_setcolors_success,
    .onpress      = button_lock_uTox_onpress,
    .button_text  = { .i18nal = STR_LOCK      },
    .tooltip_text = { .i18nal = STR_LOCK_UTOX },
},

button_show_password_settings = {
    .bm           = BM_SBUTTON,
    .update       = button_setcolors_success,
    .onpress      = button_show_password_settings_onpress,
    .button_text  = { .i18nal = STR_SHOW      },
    .tooltip_text = { .i18nal = STR_SHOW_UI_PASSWORD },
};

button_export_chatlog = {
    .bm = BM_SBUTTON,
    .button_text = { .i18nal = STR_COPY_TOX_ID },
    .update = button_setcolors_success,
    .onpress = button_export_chatlog_onpress,
    .disabled = 0,
},
