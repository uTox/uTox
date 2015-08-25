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
    openfileavatar();
}

/* TODO this is placed here (out of use order) for the following function, todo; create an external function to switch to the correct
 * settings page... */
extern PANEL panel_settings_profile, panel_settings_net, panel_settings_ui, panel_settings_av;
extern SCROLLABLE scrollbar_settings;
static void button_settings_sub_profile_onpress(void){
    scrollbar_settings.content_height = 130 * SCALE;
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

    tox_postmessage(TOX_SETSTATUS, self.status, 0, NULL);
}

static void button_jump_button_switch_onpress(void) {
    panel_quick_buttons.disabled = !panel_quick_buttons.disabled;
    panel_search_filter.disabled = !panel_search_filter.disabled;
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
    list_selectaddfriend();
}

static void button_create_group_onpress(void) {
    tox_postmessage(TOX_NEWGROUP, 1, 0, NULL);
}

static void button_settings_onpress(void) {
    list_selectsettings();
}

static void button_filter_friends_mdown(void) {
        FILTER = !FILTER;
}







static void button_copyid_onpress(void) {
    edit_setfocus(&edit_toxid);
    copy(0);
}



#ifdef EMOJI_IDS
static void button_change_id_type_onpress(void)
{
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

static void button_audiopreview_onpress(void)
{
    if (!audio_preview)
        toxaudio_postmessage(AUDIO_PREVIEW_START, 0, 0, NULL);
    else
        toxaudio_postmessage(AUDIO_PREVIEW_END, 0, 0, NULL);

    audio_preview = !audio_preview;
}

static void button_audiopreview_update(BUTTON *b)
{
    if (audio_preview){
        button_setcolors_danger(b);
    } else {
        button_setcolors_success(b);
    }
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

static void button_videopreview_update(BUTTON *b)
{
    if (video_preview)
        button_setcolors_danger(b);
    else
        button_setcolors_success(b);
}

static void button_send_friend_request_onpress(void) {
    friend_add(edit_add_id.data, edit_add_id.length, edit_add_msg.data, edit_add_msg.length);
    edit_resetfocus();
}


static void button_settings_sub_net_onpress(void){
    scrollbar_settings.content_height = 90 * SCALE;
    list_selectsettings();
    panel_settings_profile.disabled = 1;
    panel_settings_net.disabled  = 0;
    panel_settings_ui.disabled   = 1;
    panel_settings_av.disabled   = 1;
}

static void button_settings_sub_ui_onpress(void){
    scrollbar_settings.content_height = 140 * SCALE;
    list_selectsettings();
    panel_settings_profile.disabled = 1;
    panel_settings_net.disabled  = 1;
    panel_settings_ui.disabled   = 0;
    panel_settings_av.disabled   = 1;
}

static void button_settings_sub_av_onpress(void){
    scrollbar_settings.content_height = 150 * SCALE;
    list_selectsettings();
    panel_settings_profile.disabled = 1;
    panel_settings_net.disabled  = 1;
    panel_settings_ui.disabled   = 1;
    panel_settings_av.disabled   = 0;
}

static void button_group_audio_onpress(void)
{
    GROUPCHAT *g = selected_item->data;
    if (g->audio_calling) {
        tox_postmessage(TOX_GROUP_AUDIO_END, (g - group), 0, NULL);
    } else {
        tox_postmessage(TOX_GROUP_AUDIO_START, (g - group), 0, NULL);
    }
}

static void button_group_audio_update(BUTTON *b)
{
    GROUPCHAT *g = selected_item->data;
    if (g->type == TOX_GROUPCHAT_TYPE_AV) {
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

static void button_call_onpress(void)
{
    FRIEND *f = selected_item->data;

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

static void button_call_update(BUTTON *b)
{
    FRIEND *f = selected_item->data;

    switch(f->calling) {
    case CALL_INVITED: {
        b->disabled = 0;
        button_setcolors_warning(b);
        break;
    }

    case CALL_RINGING: {
        b->disabled = 0;
        button_setcolors_warning(b);
        break;
    }

    case CALL_NONE: {
        if (f->online) {
            b->disabled = 0;
            button_setcolors_success(b);
            break;
        }
        /* fall through */
    }

    case CALL_RINGING_VIDEO:
    case CALL_INVITED_VIDEO: {
        b->disabled = 1;
        button_setcolors_disabled(b);
        break;
    }

    case CALL_OK:
    case CALL_OK_VIDEO: {
        b->disabled = 0;
        button_setcolors_danger(b);
        break;
    }
    }
}

static void button_video_onpress(void)
{
    FRIEND *f = selected_item->data;

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

static void button_video_update(BUTTON *b)
{
    FRIEND *f = selected_item->data;

    switch(f->calling) {
    case CALL_INVITED_VIDEO: {
        b->disabled = 0;
        button_setcolors_warning(b);
        break;
    }

    case CALL_RINGING_VIDEO: {
        b->disabled = 0;
        button_setcolors_warning(b);
        break;
    }

    case CALL_NONE: {
        if (f->online) {
            b->disabled = 0;
            button_setcolors_success(b);
            break;
        }
        /* fall through */
    }

    case CALL_RINGING:
    case CALL_INVITED: {
        b->disabled = 1;
        button_setcolors_disabled(b);
        break;
    }

    case CALL_OK: {
        b->disabled = 0;
        button_setcolors_success(b);
        break;
    }

    case CALL_OK_VIDEO: {
        b->disabled = 0;
        button_setcolors_danger(b);
        break;
    }
    }
}

static void button_bottommenu_update(BUTTON *b)
{
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

static void button_sendfile_onpress(void)
{
    FRIEND *f = selected_item->data;
    if (f->online) {
        openfilesend();
    }
}

static void button_sendfile_update(BUTTON *b)
{
    FRIEND *f = selected_item->data;
    if (f->online) {
        b->disabled = 0;
        button_setcolors_success(b);
    } else {
        b->disabled = 1;
        button_setcolors_disabled(b);
    }
}

static void button_accept_friend_onpress(void){
    FRIENDREQ *req = selected_item->data;
    tox_postmessage(TOX_ACCEPTFRIEND, 0, 0, req);
    panel_friend_request.disabled = 1;
    // list_reselect_current();
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

/* top right chat message window button */
static void button_chat1_onpress(void)
{
    FRIEND *f = selected_item->data;
    if (f->online) {
        desktopgrab(0);
    }
}

static void button_chat1_update(BUTTON *b)
{
    FRIEND *f = selected_item->data;
    if (f->online) {
        b->disabled = 0;
        button_setcolors_success(b);
    } else {
        b->disabled = 1;
        button_setcolors_disabled(b);
    }
}

/* bottom right chat message window button */
static void button_chat2_onpress(void){
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

static void button_chat_send_update(BUTTON *b){
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
},

button_menu = {
    .bm2     = BM_SETTINGS_THREE_BAR,
    .bw      = _BM_THREE_BAR_WIDTH,
    .bh      = _BM_THREE_BAR_WIDTH,
    .update  = button_menu_update,
    .onpress = button_jump_button_switch_onpress,
    .tooltip_text = { .i18nal = STR_USERSETTINGS },
},

button_filter_friends = {
    .nodraw       = 1,
    .onpress      = button_filter_friends_mdown,
    .tooltip_text = { .i18nal = STR_FILTER_CONTACT_TOGGLE },
},

button_add_new_contact = {
    .bm2 = BM_ADD,
    .bw = _BM_ADD_WIDTH,
    .bh = _BM_ADD_WIDTH,
    .update = button_menu_update,
    .onpress = button_add_new_contact_onpress,
    .tooltip_text = { .i18nal = STR_ADDFRIENDS },
},

button_create_group = {
    .bm2 = BM_GROUPS,
    .bw = _BM_ADD_WIDTH,
    .bh = _BM_ADD_WIDTH,
    .update = button_bottommenu_update,
    .onpress = button_create_group_onpress,
    .tooltip_text = { .i18nal = STR_CREATEGROUPCHAT },
},

button_settings = {
    .bm2     = BM_SETTINGS,
    .bw      = _BM_ADD_WIDTH,
    .bh      = _BM_ADD_WIDTH,
    .update  = button_bottommenu_update,
    .onpress = button_settings_onpress,
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
    .button_text = { .i18nal = STR_BUTTON_ADD_FRIEND },
    .update = button_setcolors_success,
    .onpress = button_send_friend_request_onpress,
    .disabled = 0,
},

button_call = {
    .bm = BM_LBUTTON,
    .bm2 = BM_CALL,
    .bw = _BM_LBICON_WIDTH,
    .bh = _BM_LBICON_HEIGHT,
    .onpress = button_call_onpress,
    .update = button_call_update,
},

button_group_audio = {
    .bm = BM_LBUTTON,
    .bm2 = BM_CALL,
    .bw = _BM_LBICON_WIDTH,
    .bh = _BM_LBICON_HEIGHT,
    .onpress = button_group_audio_onpress,
    .update = button_group_audio_update,
},

button_video = {
    .bm = BM_LBUTTON,
    .bm2 = BM_VIDEO,
    .bw = _BM_LBICON_WIDTH,
    .bh = _BM_LBICON_HEIGHT,
    .onpress = button_video_onpress,
    .update = button_video_update,
},

button_sendfile = {
    .bm = BM_LBUTTON,
    .bm2 = BM_FILE,
    .bw = _BM_LBICON_WIDTH,
    .bh = _BM_LBICON_HEIGHT,
    .onpress = button_sendfile_onpress,
    .update = button_sendfile_update,
},

button_accept_friend = {
    .bm = BM_SBUTTON,
    .button_text = { .i18nal = STR_BUTTON_ACCEPT_FRIEND },
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
    .bm = BM_LBUTTON,
    .bm2 = BM_VIDEO,
    .bw = _BM_LBICON_WIDTH,
    .bh = _BM_LBICON_HEIGHT,
    .onpress = button_videopreview_onpress,
    .update = button_videopreview_update,
    .disabled = 0,
},

/* left chat message window button */
button_chat_left = {
    .bm = BM_CB2,
    // @TODO: replace with something useful
    // .bm2 = BM_ADD,
    // .bw = _BM_ADD_WIDTH,
    // .bh = _BM_ADD_WIDTH,
    .onpress = button_chat2_onpress,
    .update = button_setcolors_disabled,
    .disabled = 1,
},

/* right chat message window button */
button_chat_right = {
    .bm = BM_CB1,
    .bm2 = BM_CI1,
    .bw = _BM_CI_WIDTH,
    .bh = _BM_CI_WIDTH,
    .update = button_chat1_update,
    .onpress = button_chat1_onpress,
    .tooltip_text = { .i18nal = STR_SENDSCREENSHOT },
},


button_chat_send = {
    .bm  = BM_CHAT_SEND,
    .bm2 = BM_CHAT_SEND_OVERLAY,
    .bw  = _BM_CHAT_SEND_OVERLAY_WIDTH,
    .bh  = _BM_CHAT_SEND_OVERLAY_HEIGHT,
    .onpress = button_chat_send_onpress,
    .update = button_chat_send_update,
};
