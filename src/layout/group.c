#include "group.h"

#include "create.h"

#include "../commands.h"
#include "../debug.h"
#include "../flist.h"
#include "../groups.h"
#include "../macros.h"
#include "../settings.h"
#include "../text.h"
#include "../theme.h"
#include "../tox.h"

#include "../ui/button.h"
#include "../ui/draw.h"
#include "../ui/edit.h"
#include "../ui/panel.h"
#include "../ui/scrollable.h"
#include "../ui/svg.h"
#include "../ui/switch.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tox/tox.h>

SCROLLABLE scrollbar_group = {
    .panel = { .type = PANEL_SCROLLABLE, },
    .color = C_SCROLL,
};

static void draw_group(int x, int UNUSED(y), int UNUSED(w), int UNUSED(height)) {
    GROUPCHAT *g = flist_get_groupchat();
    if (!g) {
        LOG_ERR("Group", "Could not get selected groupchat.");
        return;
    }

    drawalpha(BM_GROUP, x + SCALE(10), SCALE(10), BM_CONTACT_WIDTH, BM_CONTACT_WIDTH, COLOR_MAIN_TEXT);

    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_TITLE);
    drawtextrange(x + SCALE(60), settings.window_width - SCALE(64), SCALE(2), g->name, g->name_length);

    setcolor(COLOR_MAIN_TEXT_SUBTEXT);
    setfont(FONT_STATUS);
    drawtextrange(x + SCALE(60), settings.window_width - SCALE(64), SCALE(16), g->topic, g->topic_length);

    uint32_t i = 0;
    unsigned k = x + SCALE(60);

    unsigned int pos_y = 15;
    while (i < g->peer_count) {
        GROUP_PEER *peer = g->peer[i];

        if (peer && peer->name_length) {
            char buf[TOX_MAX_NAME_LENGTH];

            snprintf((char *)buf, sizeof(buf), "%.*s, ",
                     (int)peer->name_length, peer->name);
            int buf_len = strnlen(buf, sizeof(buf) - 1);

            unsigned w = textwidth(buf, buf_len);
            if (peer->name_color) {
                setcolor(peer->name_color);
            } else {
                setcolor(COLOR_GROUP_PEER);
            }

            if (k + w >= (settings.window_width - SCALE(64))) {
                if (pos_y == 15) {
                    pos_y += 6;
                    k = x + SCALE(60);
                } else {
                    drawtext(k, SCALE(pos_y * 2), "...", 3);
                    break;
                }
            }

            drawtext(k, SCALE(pos_y * 2), buf, buf_len);

            k += w;
        }
        i++;
    }
}

static void draw_group_settings(int x, int y, int UNUSED(width), int UNUSED(height)) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);

    drawstr(x + SCALE(10), y + SCALE(MAIN_TOP + 10), GROUP_TOPIC);
    drawstr(x + SCALE(10), y + SCALE(MAIN_TOP + 60), GROUP_NOTIFICATIONS);
}

static void draw_group_create(int x, int y, int UNUSED(width), int UNUSED(height)) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);

    drawstr(x + SCALE(10), y + SCALE(MAIN_TOP + 10), CREATEGROUPCHAT);
    drawstr(x + SCALE(20) + BM_SWITCH_WIDTH, y + SCALE(MAIN_TOP + 40), GROUP_CREATE_WITH_AUDIO);
}

static void button_create_group_on_mup(void) {
    postmessage_toxcore(TOX_GROUP_CREATE, 0, switch_group_type.switch_on, NULL);
}

static void switchfxn_group_type(void) {
    if (switch_group_type.switch_on) {
        maybe_i18nal_string_set_i18nal(&button_create_group.button_text, STR_GROUP_CREATE_VOICE);
    } else {
        maybe_i18nal_string_set_i18nal(&button_create_group.button_text, STR_GROUP_CREATE_TEXT);
    }
}

PANEL
panel_group = {
    .type = PANEL_NONE,
    .disabled = 1,
    .child = (PANEL*[]) {
        &panel_group_chat,
        &panel_group_video,
        &panel_group_settings,
        NULL
    }
},
panel_group_create = {
    .type = PANEL_NONE,
    .disabled = true,
    .drawfunc = draw_group_create,
    .child = (PANEL*[]) {
        (PANEL*)&button_create_group,
        (PANEL*)&switch_group_type,
        NULL
    }
},
panel_group_chat = {
    .type = PANEL_NONE,
    .disabled = 0,
    .drawfunc = draw_group,
    .child = (PANEL*[]) {
        (PANEL*)&scrollbar_group,
        (PANEL*)&edit_chat_msg_group, // this needs to be one of the first, to get events before the others
        (PANEL*)&messages_group,
        (PANEL*)&button_group_audio,
        (PANEL*)&button_chat_send_group,
        NULL
    }
},
panel_group_video = {
    .type = PANEL_NONE,
    .disabled = 1,
    .child = (PANEL*[]) {
        NULL
    }
},
panel_group_settings = {
    .type = PANEL_NONE,
    .disabled = 1,
    .drawfunc = draw_group_settings,
    .child = (PANEL*[]) {
        (PANEL*)&edit_group_topic,
        (PANEL*)&dropdown_notify_groupchats,
        NULL
    }
},
messages_group = {
    .type = PANEL_MESSAGES,
    .content_scroll = &scrollbar_group,
};

static void button_group_audio_on_mup(void) {
    GROUPCHAT *g = flist_get_groupchat();
    if (!g) {
        LOG_ERR("Group", "Could not get selected groupchat.");
        return;
    }

    // We have to take the long way around, because the UI shouldn't depend on AV
    if (g->active_call) {
        postmessage_toxcore(TOX_GROUP_AUDIO_END, g->number, 0, NULL);
    } else {
        postmessage_toxcore(TOX_GROUP_AUDIO_START, g->number, 0, NULL);
    }
}


static void button_group_audio_update(BUTTON *b) {
    GROUPCHAT *g = flist_get_groupchat();
    if (!g) {
        LOG_ERR("Group", "Could not get selected groupchat.");
        return;
    }

    if (g->av_group) {
        b->disabled = false;
        if (g->active_call) {
            button_setcolors_danger(b);
        } else {
            button_setcolors_success(b);
        }
    } else {
        b->disabled = true;
        button_setcolors_disabled(b);
    }
}

BUTTON button_group_audio = {
    .panel = {
        .type   = PANEL_BUTTON,
        .x      = -62,
        .y      =  10,
        .width  = _BM_LBUTTON_WIDTH,
        .height = _BM_LBUTTON_HEIGHT,
    },
    .bm_fill      = BM_LBUTTON,
    .bm_icon      = BM_CALL,
    .icon_w       = _BM_LBICON_WIDTH,
    .icon_h       = _BM_LBICON_HEIGHT,
    .on_mup       = button_group_audio_on_mup,
    .update       = button_group_audio_update,
    .tooltip_text = {.i18nal = STR_GROUPCHAT_JOIN_AUDIO },
};


static uint32_t peers_deduplicate(char **dedup, size_t *dedup_size, GROUP_PEER **peers, uint32_t peer_count) {
    uint32_t count = 0;
    for (size_t peer = 0; peer < peer_count; peer++) {

        GROUP_PEER *p = peers[peer];
        if (!p) {
            continue;
        }

        uint8_t *nick      = p->name;
        size_t nick_len = p->name_length;

        size_t i = 0;
        if (nick) {
            bool found = false;

            while (!found && i < count) {
                if (nick_len == dedup_size[i] && !memcmp(nick, dedup[i], nick_len)) {
                    found = true;
                }

                i++;
            }

            if (!found) {
                dedup[count]      = (char *)nick;
                dedup_size[count] = nick_len;
                count++;
            }
        }
    }

    return count;
}

static struct {
    uint16_t start, end, cursorpos;
    uint32_t length, spacing;
    bool     active;
    bool     edited;
} completion;

static uint8_t nick_completion_search(EDIT *edit, char *found_nick, int direction) {
    char *        text = edit->data;
    uint32_t      i, peers, prev_index, compsize = completion.length;
    char *        nick     = NULL;
    size_t        nick_len = 0;
    bool          found    = 0;
    static char * dedup[65536];      /* TODO magic numbers */
    static size_t dedup_size[65536]; /* TODO magic numbers */
    GROUPCHAT *   g = flist_get_groupchat();
    if (!g) {
        LOG_ERR("Group", "Could not get selected groupchat.");
        return 0;
    }

    peers = peers_deduplicate(dedup, dedup_size, g->peer, g->peer_count);

    i = 0;
    while (!found) {
        if (i >= peers) {
            found = 1;
            i     = 0;
        } else {
            nick     = dedup[i];
            nick_len = dedup_size[i];
            if (nick_len == completion.end - completion.start - completion.spacing
                && !memcmp(nick, text + completion.start, nick_len)) {
                found = 1;
            } else {
                i++;
            }
        }
    }

    prev_index = i;
    found      = 0;
    do {
        if (direction == -1 && i == 0) {
            i = peers;
        }
        i += direction;

        if (i >= peers) {
            i = 0;
        }

        nick     = dedup[i];
        nick_len = dedup_size[i];

        if (nick_len >= compsize && !memcmp_case(nick, text + completion.start, compsize)) {
            found = 1;
        }
    } while (!found && i != prev_index);

    if (found) {
        memcpy(found_nick, nick, nick_len);
        return nick_len;
    } else {
        return 0;
    }
}

static void nick_completion_replace(EDIT *edit, char *nick, uint32_t size) {
    char *   text      = edit->data;
    uint16_t length    = edit->length;
    uint16_t maxlength = edit->data_size - 1;

    int offset;

    completion.spacing = 1;
    size += 1;
    if (!completion.start) {
        size += 1;
        completion.spacing += 1;
        nick[size - 2] = ':';
    }

    nick[size - 1] = ' ';
    if (length > completion.end) {
        size -= 1;
        completion.spacing -= 1;
    }

    if (completion.start + size > maxlength) {
        size = maxlength - completion.start;
    }

    offset = completion.end - completion.start - size;

    edit_do(edit, completion.start, completion.end - completion.start, 1);

    memmove(text + completion.end - offset, text + completion.end,
            length - offset > maxlength ? maxlength - completion.end + offset : length - completion.end);

    memcpy(text + completion.start, nick, size);

    edit_do(edit, completion.start, size, 0);

    if (length - offset > maxlength) {
        edit->length = maxlength;
    } else {
        edit->length -= offset;
    }
    completion.end -= offset;
}

static void e_chat_msg_ontab(EDIT *edit) {
    char *text = edit->data;
    uint16_t length = edit->length;

    if (flist_get_type() == ITEM_FRIEND || flist_get_type() == ITEM_GROUP) {
        char    nick[130];
        uint8_t nick_length;

        if (completion.cursorpos != edit_getcursorpos()) {
            completion.active = 0;
        }

        if (!completion.active) {
            if ((length == 6 && !memcmp(text, "/topic", 6)) || (length == 7 && !memcmp(text, "/topic ", 7))) {
                GROUPCHAT *g = flist_get_groupchat();
                if (!g) {
                    LOG_ERR("Group", "Could not get selected groupchat.");
                    return;
                }

                text[6] = ' ';
                memcpy(text + 7, g->name, g->name_length);
                edit->length = g->name_length + 7;
                edit_setcursorpos(edit, edit->length);

                return;
            }

            completion.start = edit_getcursorpos();
            while (completion.start > 0 && text[completion.start - 1] != ' ') {
                completion.start--;
            }

            completion.end = completion.start;
            while (completion.end < length && text[completion.end] != ' ') {
                completion.end++;
            }

            completion.active = 1;
            completion.length = completion.end - completion.start;
        }

        nick_length = nick_completion_search(edit, nick, 1);
        if (nick_length) {
            completion.edited = 1;
            if (!(nick_length == completion.end - completion.start - completion.spacing
                  && !memcmp(nick, text + completion.start, nick_length))) {
                nick_completion_replace(edit, nick, nick_length);
            }
            edit_setcursorpos(edit, completion.end);
            completion.cursorpos = edit_getcursorpos();
        }
    } else {
        completion.active = 0;
    }
}

void e_group_msg_onenter(EDIT *edit) {
    char *text = edit->data;
    uint16_t length = edit->length;

    if (length <= 0) {
        return;
    }

    uint16_t command_length = 0; //, argument_length = 0;
    char *command = NULL;
    char *argument = NULL;

    command_length = utox_run_command(text, length, &command, &argument, 1);

    // TODO: Magic number
    if (command_length == UINT16_MAX) {
        edit->length = 0;
        return;
    }

    // LOG_NOTE("Group", "cmd %u\n", command_length);

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

    GROUPCHAT *g = flist_get_groupchat();
    if (g) {
        void *d = malloc(length);
        if (!d) {
            LOG_ERR("Layout Group", "edit_msg_onenter:\t Ran out of memory.");
            return;
        }
        memcpy(d, text, length);
        postmessage_toxcore((action ? TOX_GROUP_SEND_ACTION : TOX_GROUP_SEND_MESSAGE), g->number, length, d);
    } else {
        LOG_ERR("Groups", "No Group selected!");
    }

    completion.active = 0;
    edit->length      = 0;
}

static void e_chat_msg_onshifttab(EDIT *edit) {
    char *text = edit->data;

    if (flist_get_type() == ITEM_GROUP) {
        char    nick[130];
        uint8_t nick_length;

        if (completion.cursorpos != edit_getcursorpos()) {
            completion.active = 0;
        }

        if (completion.active) {
            nick_length = nick_completion_search(edit, nick, -1);
            if (nick_length) {
                completion.edited = 1;
                if (!(nick_length == completion.end - completion.start - completion.spacing
                      && !memcmp(nick, text + completion.start, nick_length))) {
                    nick_completion_replace(edit, nick, nick_length);
                }
                edit_setcursorpos(edit, completion.end);
                completion.cursorpos = edit_getcursorpos();
            }
        }
    } else {
        completion.active = 0;
    }
}

static void edit_msg_onlosefocus(EDIT *UNUSED(edit)) {
    completion.active = 0;
}

SCROLLABLE e_chat_msg_group_scroll = {
    .panel = { .type = PANEL_SCROLLABLE, },
    .d     = 1.0,
    .color = C_SCROLL,
};

static char e_chat_msg_group_data[65535];
EDIT edit_chat_msg_group = {
    .multiline   = true,
    .data_size   = sizeof e_chat_msg_group_data,
    .data        = e_chat_msg_group_data,
    .onenter     = e_group_msg_onenter,
    .ontab       = e_chat_msg_ontab,
    .onshifttab  = e_chat_msg_onshifttab,
    .onlosefocus = edit_msg_onlosefocus,
    .scroll      = &e_chat_msg_group_scroll,
};

static void e_group_topic_onenter(EDIT *edit) {
    GROUPCHAT *g = flist_get_groupchat();
    if (!g) {
        LOG_ERR("Layout Groups", "Can't set a topic when a group isn't selected!");
        return;
    }

    void *d = malloc(edit->length);
    if (!d){
        LOG_ERR("Layout Groups", "Unable to change group topic.");
        return;
    }
    memcpy(d, edit->data, edit->length);
    postmessage_toxcore(TOX_GROUP_SET_TOPIC, g->number, edit->length, d);
}

static char e_group_topic_data[1024];
EDIT edit_group_topic = {
    .panel = {
        .type   = PANEL_EDIT,
        .x      = 10,
        .y      = 88,
        .width  = -10,
        .height = 24
    },
    .data           = e_group_topic_data,
    .data_size      = sizeof e_group_topic_data,
    .onenter        = e_group_topic_onenter,
    .onlosefocus    = e_group_topic_onenter,
    .noborder       = false,
    .empty_str      = {.plain = STRING_INIT("") },
};

static void button_chat_send_on_mup(void) {
    if (flist_get_type() == ITEM_GROUP) {
        e_group_msg_onenter(&edit_chat_msg_group);
        // reset focus to the chat window on send to prevent segfault. May break on android.
        edit_setfocus(&edit_chat_msg_group);
    }
}

static void button_chat_send_group_update(BUTTON *b) {
    b->disabled = false;
    button_setcolors_success(b);
}

BUTTON button_chat_send_group = {
    .panel = {
        .type   = PANEL_BUTTON,
        .x      =  -6 - _BM_CHAT_SEND_WIDTH,
        .y      = -46,
        .width  = _BM_CHAT_SEND_WIDTH,
        .height = _BM_CHAT_SEND_HEIGHT,
    },
    .bm_fill        = BM_CHAT_SEND,
    .bm_icon        = BM_CHAT_SEND_OVERLAY,
    .icon_w         = _BM_CHAT_SEND_OVERLAY_WIDTH,
    .icon_h         = _BM_CHAT_SEND_OVERLAY_HEIGHT,
    .on_mup         = button_chat_send_on_mup,
    .update         = button_chat_send_group_update,
    .tooltip_text   = {.i18nal = STR_SENDMESSAGE },
    .nodraw         = false
};

BUTTON button_create_group = {
    .panel = {
        .type   = PANEL_BUTTON,
        .x      = 10,
        .y      = MAIN_TOP + 67,
        .width  = _BM_SBUTTON_WIDTH,
        .height = _BM_SBUTTON_HEIGHT
    },
    .bm_fill      = BM_SBUTTON,
    .update       = button_setcolors_success,
    .on_mup       = button_create_group_on_mup,
    .disabled     = false,
    .button_text  = {.i18nal = STR_GROUP_CREATE_TEXT },
};

UISWITCH switch_group_type = {
    .panel = {
        .type   = PANEL_SWITCH,
        .x      = 10,
        .y      = MAIN_TOP + 35,
        .width  = _BM_SWITCH_WIDTH,
        .height = _BM_SWITCH_HEIGHT
    },
    .style_outer    = BM_SWITCH,
    .style_toggle   = BM_SWITCH_TOGGLE,
    .style_icon_off = BM_NO,
    .style_icon_on  = BM_YES,
    .update         = switch_update,
    .on_mup         = switchfxn_group_type,
    .tooltip_text   = {.i18nal = STR_GROUP_CREATE_VOICE },
};
