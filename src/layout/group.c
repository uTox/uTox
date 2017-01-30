#include "group.h"

#include "../ui/draw.h"
#include "../ui/panel.h"
#include "../ui/scrollable.h"

#include "../flist.h"
#include "../groups.h"
#include "../macros.h"
#include "../settings.h"
#include "../theme.h"

#include "../ui/svg.h"

#include <stddef.h>
#include <stdio.h>
#include <tox/tox.h>

SCROLLABLE scrollbar_group = {
    .panel = { .type = PANEL_SCROLLABLE, },
    .color = C_SCROLL,
};

static void draw_group(int UNUSED(x), int UNUSED(y), int UNUSED(w), int UNUSED(height)) {
    GROUPCHAT *g = flist_get_selected()->data;

    drawalpha(BM_GROUP, MAIN_LEFT + SCALE(10), SCALE(10), BM_CONTACT_WIDTH, BM_CONTACT_WIDTH, COLOR_MAIN_TEXT);

    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_TITLE);
    drawtextrange(MAIN_LEFT + SCALE(60), settings.window_width - SCALE(64), SCALE(2), g->name, g->name_length);

    setcolor(COLOR_MAIN_TEXT_SUBTEXT);
    setfont(FONT_STATUS);
    drawtextrange(MAIN_LEFT + SCALE(60), settings.window_width - SCALE(64), SCALE(16), g->topic, g->topic_length);

    uint32_t i = 0;
    unsigned k = MAIN_LEFT + SCALE(60);

    unsigned int pos_y = 15;
    while (i < g->peer_count) {
        GROUP_PEER *peer = g->peer[i];

        if (peer && peer->name_length) {
            char buf[TOX_MAX_NAME_LENGTH];
            int  text_length = snprintf((char *)buf, TOX_MAX_NAME_LENGTH, "%.*s, ", (int)peer->name_length, peer->name);

            unsigned w = textwidth(buf, text_length);
            if (peer->name_color) {
                setcolor(peer->name_color);
            } else {
                setcolor(COLOR_GROUP_PEER);
            }

            if (k + w >= (settings.window_width - SCALE(64))) {
                if (pos_y == 15) {
                    pos_y += 6;
                    k = MAIN_LEFT + SCALE(60);
                } else {
                    drawtext(k, SCALE(pos_y * 2), "...", 3);
                    break;
                }
            }

            drawtext(k, SCALE(pos_y * 2), buf, text_length);

            k += w;
        }
        i++;
    }
}

static void draw_group_settings(int UNUSED(x), int y, int UNUSED(width), int UNUSED(height)) {
    setcolor(COLOR_MAIN_TEXT);
    setfont(FONT_SELF_NAME);

    drawstr(MAIN_LEFT + SCALE(10), y + MAIN_TOP + SCALE(10), GROUP_TOPIC);
    drawstr(MAIN_LEFT + SCALE(10), y + MAIN_TOP + SCALE(70), GROUP_NOTIFICATIONS);
}

#include "../ui/edits.h"
#include "../ui/buttons.h"
#include "../ui/dropdowns.h"

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
    panel_group_chat = {
        .type = PANEL_NONE,
        .disabled = 0,
        .drawfunc = draw_group,
        .child = (PANEL*[]) {
            (PANEL*)&scrollbar_group,
            (PANEL*)&edit_msg_group, // this needs to be one of the first, to get events before the others
            (PANEL*)&messages_group,
            (PANEL*)&button_group_audio,
            (PANEL*)&button_chat_send,
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

