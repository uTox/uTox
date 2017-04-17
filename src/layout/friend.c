#include "friend.h"

#include "settings.h"
#include "sidebar.h"

#include "../avatar.h"
#include "../debug.h"
#include "../flist.h"
#include "../friend.h"
#include "../macros.h"
#include "../settings.h"
#include "../theme.h"

#include "../native/dialog.h"

#include "../ui/draw.h"
#include "../ui/edit.h"
#include "../ui/scrollable.h"
#include "../ui/svg.h"
#include "../ui/text.h"

#include "../main.h" // add friend status // TODO this is stupid wrong
#include "../dns.h"

#include <string.h>

/* Header for friend chat window */
static void draw_friend(int x, int y, int w, int height) {
    FRIEND *f = flist_get_friend();
    if (!f) {
        LOG_ERR("Friend", "Could not get selected friend.\n");
        return;
    }

    // draw avatar or default image
    if (friend_has_avatar(f)) {
        draw_avatar_image(f->avatar->img, MAIN_LEFT + SCALE(10), SCALE(10), f->avatar->width, f->avatar->height,
                          BM_CONTACT_WIDTH, BM_CONTACT_WIDTH);
    } else {
        drawalpha(BM_CONTACT, MAIN_LEFT + SCALE(10), SCALE(10), BM_CONTACT_WIDTH, BM_CONTACT_WIDTH, COLOR_MAIN_TEXT);
    }

    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_TITLE);

    if (f->alias) {
        drawtextrange(MAIN_LEFT + SCALE(60), settings.window_width - SCALE(128), SCALE(18), f->alias, f->alias_length);
    } else {
        drawtextrange(MAIN_LEFT + SCALE(60), settings.window_width - SCALE(128), SCALE(18), f->name, f->name_length);
    }

    setcolor(COLOR_MAIN_TEXT_SUBTEXT);
    setfont(FONT_STATUS);
    drawtextrange(MAIN_LEFT + SCALE(60), settings.window_width - SCALE(128), SCALE(32), f->status_message,
                  f->status_length);

    if (f->typing) {
        int typing_y = ((y + height) + CHAT_BOX_TOP - SCALE(14));
        setfont(FONT_MISC);
        // @TODO: separate these colors if needed
        setcolor(COLOR_MAIN_TEXT_HINT);
        drawtextwidth_right(x, MESSAGES_X - NAME_OFFSET, typing_y, f->name, f->name_length);
        drawtextwidth(x + MESSAGES_X, x + w, typing_y, S(IS_TYPING), SLEN(IS_TYPING));
    }
}

/* Draw an invite to be a friend window */
static void draw_friend_request(int x, int y, int w, int h) {
    FREQUEST *req = flist_get_frequest();
    if (!req) {
        LOG_ERR("Layout Friend", "Unable to draw a friend request without a friend request.");
        return;
    }

    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(MAIN_LEFT + SCALE(10), SCALE(20), FRIENDREQUEST);

    if (req->msg && req->length) {
        setfont(FONT_TEXT);
        utox_draw_text_multiline_within_box(x + SCALE(10), y + SCALE(70), w + x, y, y + h, font_small_lineheight,
                                            req->msg, req->length, ~0, ~0, 0, 0, true);
    }
}

static void draw_friend_settings(int UNUSED(x), int y, int UNUSED(width), int UNUSED(height)) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);

    drawstr(MAIN_LEFT + SCALE(10), y + MAIN_TOP + SCALE(10), FRIEND_PUBLIC_KEY);
    drawstr(MAIN_LEFT + SCALE(10), y + MAIN_TOP + SCALE(60), FRIEND_ALIAS);
    drawstr(MAIN_LEFT + SCALE(10), y + MAIN_TOP + SCALE(110), FRIEND_AUTOACCEPT);
}

static void draw_friend_deletion(int UNUSED(x), int UNUSED(y), int UNUSED(w), int UNUSED(height)) {
    FRIEND *f = flist_get_friend();
    if (!f) {
        LOG_ERR("Friend", "Could not get selected friend.");
        return;
    }

    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);

    int length = f->name_length + 2;
    char str[length];
    snprintf(str, length, "%.*s?", (int)f->name_length, f->name);

    const int push = UTOX_STR_WIDTH(DELETE_MESSAGE);
    drawstr(MAIN_LEFT + SCALE(10), SCALE(70), DELETE_MESSAGE);
    drawtextrange(push + MAIN_LEFT + SCALE(10), settings.window_width, SCALE(70), str, length - 1);
}

/* Draw add a friend window */
static void draw_add_friend(int UNUSED(x), int UNUSED(y), int UNUSED(w), int height) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);
    drawstr(MAIN_LEFT + SCALE(10), SCALE(20), ADDFRIENDS);

    setcolor(COLOR_MAIN_TEXT_SUBTEXT);
    setfont(FONT_TEXT);
    drawstr(MAIN_LEFT + SCALE(10), MAIN_TOP + SCALE(10), TOXID);

    drawstr(MAIN_LEFT + SCALE(10), MAIN_TOP + SCALE(58), MESSAGE);

    if (settings.force_proxy) {
        int push = UTOX_STR_WIDTH(TOXID);
        setfont(FONT_MISC);
        setcolor(C_RED);
        drawstr(MAIN_LEFT + SCALE(20) + push, MAIN_TOP + SCALE(12), DNS_DISABLED);
    }

    if (addfriend_status) {
        setfont(FONT_MISC);
        setcolor(C_RED);

        STRING *str;

        switch (addfriend_status) {
            case ADDF_SENT:
                str = SPTR(REQ_SENT);
                break;
            case ADDF_DISCOVER:
                str = SPTR(REQ_RESOLVE);
                break;
            case ADDF_BADNAME:
                str = SPTR(REQ_INVALID_ID);
                break;
            case ADDF_NONAME:
                str = SPTR(REQ_EMPTY_ID);
                break;
            case ADDF_TOOLONG: // if message length is too long.
                str = SPTR(REQ_LONG_MSG);
                break;
            case ADDF_NOMESSAGE: // if no message (message length must be >= 1 byte).
                str = SPTR(REQ_NO_MSG);
                break;
            case ADDF_OWNKEY: // if user's own key.
                str = SPTR(REQ_SELF_ID);
                break;
            case ADDF_ALREADYSENT: // if friend request already sent or already a friend.
                str = SPTR(REQ_ALREADY_FRIENDS);
                break;
            case ADDF_BADCHECKSUM: // if bad checksum in address.
                str = SPTR(REQ_BAD_CHECKSUM);
                break;
            case ADDF_SETNEWNOSPAM: // if the friend was already there but the nospam was different.
                str = SPTR(REQ_BAD_NOSPAM);
                break;
            case ADDF_NOMEM: // if increasing the friend list size fails.
                str = SPTR(REQ_NO_MEMORY);
                break;
            case ADDF_UNKNOWN: // for unknown error.
            case ADDF_NONE:    // this case must never be rendered, but if it does, assume it's an error
            default:
                str = SPTR(REQ_UNKNOWN);
                break;
        }

        utox_draw_text_multiline_within_box(MAIN_LEFT + SCALE(10), MAIN_TOP + SCALE(166),
                                            settings.window_width - BM_SBUTTON_WIDTH - SCALE(10), 0, height,
                                            font_small_lineheight, str->str, str->length, 0xFFFF, 0, 0, 0, 1);
    }
}

SCROLLABLE scrollbar_friend = {
    .panel = { .type = PANEL_SCROLLABLE, },
    .color = C_SCROLL,
};

PANEL messages_friend = {
    .type = PANEL_MESSAGES,
    .content_scroll = &scrollbar_friend,
};

PANEL
panel_friend = {
        .type = PANEL_NONE,
        .disabled = 1,
        .child = (PANEL*[]) {
            &panel_friend_chat,
            &panel_friend_video,
            &panel_friend_settings,
            &panel_friend_confirm_deletion,
            NULL
        }
    },
    panel_friend_chat = {
        .type = PANEL_NONE,
        .disabled = 0,
        .drawfunc = draw_friend,
        .child = (PANEL*[]) {
            (PANEL*)&scrollbar_friend,
            (PANEL*)&edit_chat_msg_friend, // this needs to be one of the first, to get events before the others
            (PANEL*)&messages_friend,
            (PANEL*)&button_call_decline,
            (PANEL*)&button_call_audio,
            (PANEL*)&button_call_video,
            (PANEL*)&button_send_file,
            (PANEL*)&button_send_screenshot,
            (PANEL*)&button_chat_send_friend,
            NULL
        }
    },
    panel_friend_video = {
        .type = PANEL_INLINE_VIDEO,
        .disabled = 1,
        .child = (PANEL*[]) {
            NULL
        }
    },
    panel_friend_settings = {
        .type = PANEL_NONE,
        .disabled = 1,
        .drawfunc = draw_friend_settings,
        .child = (PANEL*[]) {
            (PANEL*)&edit_friend_pubkey,
            (PANEL*)&edit_friend_alias,
            (PANEL*)&switch_friend_autoaccept_ft,
            (PANEL*)&button_export_chatlog,
            NULL
        }
    },
    panel_friend_confirm_deletion = {
        .type = PANEL_NONE,
        .disabled = true,
        .drawfunc = draw_friend_deletion,
        .child = (PANEL*[]) {
            (PANEL *)&button_confirm_deletion,
            (PANEL *)&button_deny_deletion,
            NULL
        }

    },
panel_friend_request = {
    .type = PANEL_NONE,
    .disabled = 1,
    .drawfunc = draw_friend_request,
    .child = (PANEL*[]) {
        (PANEL*)&button_accept_friend,
        NULL
    }
},
panel_add_friend = {
    .type = PANEL_NONE,
    .disabled = 1,
    .drawfunc = draw_add_friend,
    .child = (PANEL*[]) {
        (PANEL*)&button_send_friend_request,
        (PANEL*)&edit_add_new_friend_id,
        (PANEL*)&edit_add_new_friend_msg,
        NULL
    }
};

static void button_add_new_contact_on_mup(void) {
    if (tox_thread_init == UTOX_TOX_THREAD_INIT_SUCCESS) {
        /* Only change if we're logged in! */
        edit_setstr(&edit_add_new_friend_id, (char *)edit_search.data, edit_search.length);
        edit_setstr(&edit_search, (char *)"", 0);
        flist_selectaddfriend();
        edit_setfocus(&edit_add_new_friend_msg);
    }
}

static void button_send_friend_request_on_mup(void) {
    friend_add(edit_add_new_friend_id.data, edit_add_new_friend_id.length, edit_add_new_friend_msg.data, edit_add_new_friend_msg.length);
    edit_resetfocus();
}

#include "../tox.h"

static void button_call_decline_on_mup(void) {
    FRIEND *f = flist_get_friend();
    if (!f) {
        LOG_ERR("Friend", "Could not get selected friend.");
        return;
    }

    if (f->call_state_friend) {
        LOG_TRACE("Layout Friend", "Declining call: %u", f->number);
        postmessage_toxcore(TOX_CALL_DISCONNECT, f->number, 0, NULL);
    }
}

#include "../av/utox_av.h"
#include "../av/audio.h"
#include "../ui/button.h"
static void button_call_decline_update(BUTTON *b) {
    FRIEND *f = flist_get_friend();
    if (!f) {
        LOG_ERR("Friend", "Could not get selected friend.");
        return;
    }

    if (UTOX_AVAILABLE_AUDIO(f->number) && !UTOX_SENDING_AUDIO(f->number)) {
        button_setcolors_danger(b);
        b->nodraw   = false;
        b->disabled = false;
    } else {
        button_setcolors_disabled(b);
        b->nodraw   = true;
        b->disabled = true;
    }
}

static void button_call_audio_on_mup(void) {
    FRIEND *f = flist_get_friend();
    if (!f) {
        LOG_ERR("Friend", "Could not get selected friend.");
        return;
    }

    if (f->call_state_self) {
        if (UTOX_SENDING_AUDIO(f->number)) {
            LOG_TRACE("Layout Friend", "Ending call: %u", f->number);
            /* var 3/4 = bool send video */
            postmessage_toxcore(TOX_CALL_DISCONNECT, f->number, 0, NULL);
        } else {
            LOG_TRACE("Layout Friend", "Canceling call: friend = %d", f->number);
            postmessage_toxcore(TOX_CALL_DISCONNECT, f->number, 0, NULL);
        }
    } else if (UTOX_AVAILABLE_AUDIO(f->number)) {
        LOG_TRACE("Layout Friend", "Accept Call: %u", f->number);
        postmessage_toxcore(TOX_CALL_ANSWER, f->number, 0, NULL);
    } else if (f->online) {
        postmessage_toxcore(TOX_CALL_SEND, f->number, 0, NULL);
        LOG_TRACE("Layout Friend", "Calling friend: %u", f->number);
    }
}

static void button_call_audio_update(BUTTON *b) {
    FRIEND *f = flist_get_friend();
    if (!f) {
        LOG_ERR("Friend", "Could not get selected friend.");
        return;
    }

    if (UTOX_SENDING_AUDIO(f->number)) {
        button_setcolors_danger(b);
        b->disabled = false;
    } else if (UTOX_AVAILABLE_AUDIO(f->number)) {
        button_setcolors_warning(b);
        b->disabled = false;
    } else {
        if (f->online) {
            button_setcolors_success(b);
            b->disabled = false;
        } else {
            button_setcolors_disabled(b);
            b->disabled = true;
        }
    }
}

#include "../av/video.h"
static void button_call_video_on_mup(void) {
    FRIEND *f = flist_get_friend();
    if (!f) {
        LOG_ERR("Friend", "Could not get selected friend.");
        return;
    }

    if (f->call_state_self) {
        if (SELF_ACCEPT_VIDEO(f->number)) {
            LOG_TRACE("Layout Friend", "Canceling call (video): %u", f->number);
            postmessage_toxcore(TOX_CALL_PAUSE_VIDEO, f->number, 1, NULL);
        } else if (UTOX_SENDING_AUDIO(f->number)) {
            LOG_TRACE("Layout Friend", "Audio call inprogress, adding video");
            postmessage_toxcore(TOX_CALL_RESUME_VIDEO, f->number, 1, NULL);
        } else {
            LOG_TRACE("Layout Friend", "Ending call (video): %u", f->number);
            postmessage_toxcore(TOX_CALL_DISCONNECT, f->number, 1, NULL);
        }
    } else if (f->call_state_friend) {
        LOG_TRACE("Layout Friend", "Accept Call (video): %u %u", f->number, f->call_state_friend);
        postmessage_toxcore(TOX_CALL_ANSWER, f->number, 1, NULL);
    } else if (f->online) {
        postmessage_toxcore(TOX_CALL_SEND, f->number, 1, NULL);
        LOG_TRACE("Layout Friend", "Calling friend (video): %u", f->number);
    }
}

static void button_call_video_update(BUTTON *b) {
    FRIEND *f = flist_get_friend();
    if (!f) {
        LOG_ERR("Friend", "Could not get selected friend.");
        return;
    }

    if (SELF_SEND_VIDEO(f->number)) {
        button_setcolors_danger(b);
        b->disabled = false;
    } else if (FRIEND_SENDING_VIDEO(f->number)) {
        button_setcolors_warning(b);
        b->disabled = false;
    } else {
        if (f->online) {
            button_setcolors_success(b);
            b->disabled = false;
        } else {
            button_setcolors_disabled(b);
            b->disabled = true;
        }
    }
}

static void button_accept_friend_on_mup(void) {
    FREQUEST *req = flist_get_frequest();
    postmessage_toxcore(TOX_FRIEND_ACCEPT, 0, 0, req);
    panel_friend_request.disabled = true;
}

static void button_menu_update(BUTTON *b) {
    b->c1  = COLOR_BKGRND_MENU;
    b->c2  = COLOR_BKGRND_MENU_HOVER;
    b->c3  = COLOR_BKGRND_MENU_ACTIVE;
    b->ct1 = COLOR_MENU_TEXT;
    b->ct2 = COLOR_MENU_TEXT;
    if (b->mousedown || b->disabled) {
        b->ct1 = COLOR_MENU_TEXT_ACTIVE;
        b->ct2 = COLOR_MENU_TEXT_ACTIVE;
    }
    b->cd = COLOR_BKGRND_MENU_ACTIVE;
}

BUTTON button_add_new_contact = {
    .bm2          = BM_ADD,
    .bw           = _BM_ADD_WIDTH,
    .bh           = _BM_ADD_WIDTH,
    .update       = button_menu_update,
    .on_mup       = button_add_new_contact_on_mup,
    .disabled     = true,
    .nodraw       = true,
    .tooltip_text = {.i18nal = STR_ADDFRIENDS },
};

BUTTON button_send_friend_request = {
    .bm          = BM_SBUTTON,
    .button_text = {.i18nal = STR_ADD },
    .update      = button_setcolors_success,
    .on_mup      = button_send_friend_request_on_mup,
    .disabled    = false,
};


BUTTON button_call_decline = {
    .bm           = BM_LBUTTON,
    .bm2          = BM_DECLINE,
    .bw           = _BM_LBICON_WIDTH,
    .bh           = _BM_LBICON_HEIGHT,
    .on_mup       = button_call_decline_on_mup,
    .update       = button_call_decline_update,
    .tooltip_text = {.i18nal = STR_CALL_DECLINE },
    .nodraw       = true,
    .disabled     = true,
};

BUTTON button_call_audio = {
    .bm           = BM_LBUTTON,
    .bm2          = BM_CALL,
    .bw           = _BM_LBICON_WIDTH,
    .bh           = _BM_LBICON_HEIGHT,
    .on_mup       = button_call_audio_on_mup,
    .update       = button_call_audio_update,
    .tooltip_text = {.i18nal = STR_CALL_START_AUDIO },
};

BUTTON button_call_video = {
    .bm           = BM_LBUTTON,
    .bm2          = BM_VIDEO,
    .bw           = _BM_LBICON_WIDTH,
    .bh           = _BM_LBICON_HEIGHT,
    .on_mup       = button_call_video_on_mup,
    .update       = button_call_video_update,
    .tooltip_text = {.i18nal = STR_CALL_START_VIDEO },
};

static void button_send_file_on_mup(void) {
    FRIEND *f = flist_get_friend();
    if (!f) {
        LOG_ERR("Friend", "Could not get selected friend.");
        return;
    }

    if (f->online) {
        openfilesend();
    }
}

static void button_send_file_update(BUTTON *b) {
    FRIEND *f = flist_get_friend();
    if (!f) {
        LOG_ERR("Friend", "Could not get selected friend.");
        return;
    }

    if (f->online) {
        b->disabled = false;
        button_setcolors_success(b);
    } else {
        b->disabled = true;
        button_setcolors_disabled(b);
    }
}

BUTTON button_send_file = {
    .bm           = BM_CHAT_BUTTON_LEFT,
    .bm2          = BM_FILE,
    .bw           = _BM_FILE_WIDTH,
    .bh           = _BM_FILE_HEIGHT,
    .on_mup       = button_send_file_on_mup,
    .update       = button_send_file_update,
    .disabled     = true,
    .tooltip_text = {.i18nal = STR_SEND_FILE },
};

#include "../screen_grab.h"
static void button_send_screenshot_on_mup(void) {
    FRIEND *f = flist_get_friend();
    if (f != NULL && f->online) {
        utox_screen_grab_desktop(0);
    }
}

static void button_send_screenshot_update(BUTTON *b) {
    FRIEND *f = flist_get_friend();
    if (!f) {
        LOG_ERR("Friend", "Could not get selected friend.");
        return;
    }

    if (f->online) {
        b->disabled = false;
        button_setcolors_success(b);
    } else {
        b->disabled = true;
        button_setcolors_disabled(b);
    }
}

BUTTON button_send_screenshot = {
    .bm           = BM_CHAT_BUTTON_RIGHT,
    .bm2          = BM_CHAT_BUTTON_OVERLAY_SCREENSHOT,
    .bw           = _BM_CHAT_BUTTON_OVERLAY_WIDTH,
    .bh           = _BM_CHAT_BUTTON_OVERLAY_HEIGHT,
    .update       = button_send_screenshot_update,
    .on_mup       = button_send_screenshot_on_mup,
    .tooltip_text = {.i18nal = STR_SENDSCREENSHOT },
};

BUTTON button_accept_friend = {
    .bm          = BM_SBUTTON,
    .button_text = {.i18nal = STR_ADD },
    .update      = button_setcolors_success,
    .on_mup      = button_accept_friend_on_mup,
};

static void switchfxn_autoaccept_ft(void) {
    FRIEND *f = flist_get_friend();
    if (f) {
        f->ft_autoaccept = !f->ft_autoaccept;
    }
}

#include "../ui/switch.h"

static void switch_set_colors(UISWITCH *s) {
    if (s->switch_on) {
        s->bg_color    = COLOR_BTN_SUCCESS_BKGRND;
        s->sw_color    = COLOR_BTN_SUCCESS_TEXT;
        s->press_color = COLOR_BTN_SUCCESS_BKGRND_HOVER;
        s->hover_color = COLOR_BTN_SUCCESS_BKGRND_HOVER;
    } else {
        s->bg_color    = COLOR_BTN_DISABLED_BKGRND;
        s->sw_color    = COLOR_BTN_DISABLED_FORGRND;
        s->hover_color = COLOR_BTN_DISABLED_BKGRND_HOVER;
        s->press_color = COLOR_BTN_DISABLED_BKGRND_HOVER;
    }
}

static void switch_set_size(UISWITCH *s) {
    s->toggle_w   = BM_SWITCH_TOGGLE_WIDTH;
    s->toggle_h   = BM_SWITCH_TOGGLE_HEIGHT;
    s->icon_off_w = BM_FB_WIDTH;
    s->icon_off_h = BM_FB_HEIGHT;
    s->icon_on_w  = BM_FB_WIDTH;
    s->icon_on_h  = BM_FB_HEIGHT;
}

static void switch_update(UISWITCH *s) {
    switch_set_colors(s);
    switch_set_size(s);
}

UISWITCH switch_friend_autoaccept_ft = {
    .style_outer    = BM_SWITCH,
    .style_toggle   = BM_SWITCH_TOGGLE,
    .style_icon_off = BM_NO,
    .style_icon_on  = BM_YES,
    .update         = switch_update,
    .on_mup         = switchfxn_autoaccept_ft,
    .tooltip_text   = {.i18nal = STR_FRIEND_AUTOACCEPT },
};


static void edit_add_new_contact(EDIT *UNUSED(edit)) {
    friend_add(edit_add_new_friend_id.data, edit_add_new_friend_id.length, edit_add_new_friend_msg.data, edit_add_new_friend_msg.length);
}

static char e_friend_pubkey_str[TOX_PUBLIC_KEY_SIZE * 2];
EDIT edit_friend_pubkey = {
    .length            = sizeof e_friend_pubkey_str,
    .maxlength         = sizeof e_friend_pubkey_str,
    .data              = e_friend_pubkey_str,
    .readonly          = true,
    .noborder          = false,
    .select_completely = true,
};


static void edit_friend_alias_onenter(EDIT *UNUSED(edit)) {
    FRIEND *f = flist_get_friend();
    if (!f) {
        LOG_ERR("Friend", "Could not get selected friend.");
        return;
    }

    friend_set_alias(f, (uint8_t *)edit_friend_alias.data, edit_friend_alias.length);

    utox_write_metadata(f);
}

static char e_friend_alias_str[128];
EDIT edit_friend_alias = {
    .maxlength   = sizeof e_friend_alias_str - 1,
    .data        = e_friend_alias_str,
    .onenter     = edit_friend_alias_onenter,
    .onlosefocus = edit_friend_alias_onenter,
    .empty_str   = {.plain = STRING_INIT("") }, // set dynamically to the friend's name
};



static char e_add_new_friend_id_data[TOX_ADDRESS_SIZE * 4];
EDIT edit_add_new_friend_id = {
    .maxlength = sizeof e_add_new_friend_id_data - 1,
    .data      = e_add_new_friend_id_data,
    .onenter   = edit_add_new_contact,
};

SCROLLABLE e_add_new_friend_msg_scroll = {
    .panel = { .type = PANEL_SCROLLABLE, },
    .d     = 1.0,
    .color = C_SCROLL,
};

static char e_add_new_friend_msg_data[1024];
EDIT edit_add_new_friend_msg = {
    .multiline = 1,
    .scroll    = &e_add_new_friend_msg_scroll,
    .data      = e_add_new_friend_msg_data,
    .maxlength = sizeof e_add_new_friend_msg_data - 1,
    .empty_str = {.i18nal = STR_DEFAULT_FRIEND_REQUEST_MESSAGE },
};

#include "../commands.h"
static void e_chat_msg_onenter(EDIT *edit) {
    char *   text   = edit->data;
    uint16_t length = edit->length;

    if (length <= 0) {
        return;
    }

    uint16_t command_length = 0; //, argument_length = 0;
    char *   command = NULL, *argument = NULL;

    command_length = utox_run_command(text, length, &command, &argument, 1);

    // TODO: Magic number
    if (command_length == UINT16_MAX) {
        edit->length = 0;
        return;
    }

    // LOG_TRACE("Layout Friend", "cmd %u", command_length);

    bool action = false;
    if (command_length) {
        length = length - command_length - 2; /* first / and then the SPACE */
        text   = argument;
        if ((command_length == 2) && (!memcmp(command, "me", 2))) {
            if (argument) {
                action = true;
            } else {
                return;
            }
        }
    }

    if (!text) {
        return;
    }

    FRIEND *f = flist_get_friend();
    if (f) {
        /* Display locally */
        if (action) {
            message_add_type_action(&f->msg, 1, text, length, 1, 1);
        } else {
            message_add_type_text(&f->msg, 1, text, length, 1, 1);
        }
    }
    edit->length      = 0;
}

static void e_chat_msg_onchange(EDIT *UNUSED(edit)) {
    FRIEND *f = flist_get_friend();
    if (f) {
        if (!f->online) {
            return;
        }

        postmessage_toxcore(TOX_SEND_TYPING, f->number, 0, NULL);
    }
}

SCROLLABLE e_chat_msg_friend_scroll = {
    .panel = { .type = PANEL_SCROLLABLE, },
    .d     = 1.0,
    .color = C_SCROLL,
};

static char e_chat_msg_friend_data[65535];
EDIT edit_chat_msg_friend = {
    .data        = e_chat_msg_friend_data,
    .maxlength   = sizeof e_chat_msg_friend_data - 1,
    .multiline   = true,
    .onenter     = e_chat_msg_onenter,
    .onchange    = e_chat_msg_onchange,
    .scroll      = &e_chat_msg_friend_scroll,
};

/* Button to send chat message */
static void button_chat_send_friend_on_mup(void) {
    FRIEND *f = flist_get_friend();
    if (f && f->online) {
        // TODO clear the chat bar with a /slash command
        e_chat_msg_onenter(&edit_chat_msg_friend);
        // reset focus to the chat window on send to prevent segfault. May break on android.
        edit_setfocus(&edit_chat_msg_friend);
    }
}

static void button_chat_send_friend_update(BUTTON *b) {
    FRIEND *f = flist_get_friend();
    if (f) {
        if (f->online) {
            b->disabled = false;
            button_setcolors_success(b);
        } else {
            b->disabled = true;
            button_setcolors_disabled(b);
        }
    }
}

BUTTON button_chat_send_friend = {
    .bm           = BM_CHAT_SEND,
    .bm2          = BM_CHAT_SEND_OVERLAY,
    .bw           = _BM_CHAT_SEND_OVERLAY_WIDTH,
    .bh           = _BM_CHAT_SEND_OVERLAY_HEIGHT,
    .on_mup       = button_chat_send_friend_on_mup,
    .update       = button_chat_send_friend_update,
    .tooltip_text = {.i18nal = STR_SENDMESSAGE },
};

static void button_confirm_deletion_on_mup(void) {
    flist_delete_rmouse_item();
}

static void button_deny_deletion_on_mup(void) {
    panel_friend_confirm_deletion.disabled = true;
    panel_friend_chat.disabled             = false;
}

BUTTON button_confirm_deletion = {
    .bm           = BM_SBUTTON,
    .update       = button_setcolors_danger,
    .tooltip_text = {.i18nal = STR_DELETE},
    .button_text  = {.i18nal = STR_DELETE},
    .on_mup       = button_confirm_deletion_on_mup,
};

BUTTON button_deny_deletion = {
    .bm           = BM_SBUTTON,
    .update       = button_setcolors_success,
    .tooltip_text = {.i18nal = STR_KEEP},
    .button_text  = {.i18nal = STR_KEEP},
    .on_mup       = button_deny_deletion_on_mup,
};
