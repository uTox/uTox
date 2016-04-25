#include "main.h"

void group_init(GROUPCHAT *g, uint32_t group_number, _Bool av_group) {
    g->name_length = snprintf((char*)g->name, sizeof(g->name), "Groupchat #%u", group_number);
    if (g->name_length >= sizeof(g->name)) {
        g->name_length = sizeof(g->name) - 1;
    }
    if (av_group) {
        g->topic_length = sizeof("Error creating voice group, not supported yet") - 1;
        strcpy2(g->topic, "Error creating voice group, not supported yet");
    } else {
        g->topic_length = sizeof("Drag friends to invite them") - 1;
        memcpy(g->topic, "Drag friends to invite them", sizeof("Drag friends to invite them") - 1);
    }

    g->msg.scroll               = 1.0;
    g->msg.panel.type           = PANEL_MESSAGES;
    g->msg.panel.content_scroll = &scrollbar_group;
    g->msg.panel.y              = MAIN_TOP;
    g->msg.panel.height         = CHAT_BOX_TOP;
    g->msg.panel.width          = -SCROLL_WIDTH;
    g->msg.is_groupchat         = 1;

    g->av_group                 = av_group;

    list_addgroup(g);
    roster_select_last();
}

void group_free(GROUPCHAT *g) {
    uint16_t i = 0;
    while(i != g->edit_history_length) {
        free(g->edit_history[i]);
        i++;
    }
    free(g->edit_history);

    char_t **np = g->peername;
    uint32_t j = 0;
    while (j < g->peers) {
        char_t *n = *np++;
        if(n) {
            free(n);
        }
        j++;
    }

    uint32_t k = 0;
    while (k < g->msg.number) {
        free(g->msg.data[k]);
        k++;
    }

    free(g->msg.data);

    memset(g, 0, sizeof(GROUPCHAT));//
}
