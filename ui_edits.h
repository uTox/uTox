/* edits */
static char_t edit_name_data[128], edit_status_data[128], edit_addid_data[TOX_FRIEND_ADDRESS_SIZE * 4], edit_addmsg_data[1024], edit_msg_data[65535], edit_search_data[127],
    edit_proxy_ip_data[256], edit_proxy_port_data[8];

static struct {
    STRING_IDX start, end, cursorpos;
    uint32_t length, spacing;
    _Bool active;
    _Bool edited;
} completion;

static void edit_name_onenter(void)
{
    char_t *data = edit_name_data;
    STRING_IDX length = edit_name.length;

    if(!length) {
        return;
    }

    memcpy(self.name, data, length);
    self.name_length = length;
    update_tray();

    tox_postmessage(TOX_SETNAME, length, 0, self.name);//!
}

static void edit_status_onenter(void)
{
    char_t *data = edit_status_data;
    STRING_IDX length = edit_status.length;

    if(!length) {
        return;
    }

    void *p = realloc(self.statusmsg, length);
    if(!p) {
        return;
    }

    self.statusmsg = p;
    memcpy(self.statusmsg, data, length);
    self.statusmsg_length = length;
    update_tray();

    tox_postmessage(TOX_SETSTATUSMSG, length, 0, self.statusmsg);//!
}

static void edit_msg_onenter(void)
{
    STRING_IDX length = edit_msg.length;
    char_t *text = edit_msg_data;

    if(length <= 0) {
        return;
    }

    STRING_IDX command_length = 0;
    char_t *command = NULL;

    STRING_IDX argument_length = 0;
    char_t *argument = NULL;

    if (text[0] == '/') {
        unsigned int i;
        for (i = 0; i < length; ++i) {
            if (text[i] == ' ') {
                command_length = i;
                break;
            }
        }

        ++i;
        for (; i < length; ++i) {
            if (text[i] != ' ') {
                argument_length = length - i;
                argument = text + i;
                break;
            }
        }

        if (command_length) {
            --command_length;
            command = text + 1;
        }
    }

    _Bool action = 0;
    if ((command_length == 2) && (!memcmp(command, "me", 2))) {
        if (!argument) {
            return;
        }

        action = 1;
        length = argument_length;
        text = argument;
    }

    if(sitem->item == ITEM_FRIEND) {
        FRIEND *f = sitem->data;

        if(!f->online) {
            return;
        }

        MESSAGE *msg = malloc(length + sizeof(MESSAGE));
        msg->author = 1;
        msg->msg_type = action ? MSG_TYPE_ACTION_TEXT : MSG_TYPE_TEXT;
        msg->length = length;
        memcpy(msg->msg, text, length);

        friend_addmessage(f, msg);

        void *d = malloc(length);
        memcpy(d, text, length);

        tox_postmessage((action ? TOX_SENDACTION : TOX_SENDMESSAGE), (f - friend), length, d);
    } else {
        GROUPCHAT *g = sitem->data;

        if ((command_length == 5) && (!memcmp(command, "topic", 5))) {
            void *d = malloc(argument_length);
            memcpy(d, argument, argument_length);

            tox_postmessage(TOX_GROUPCHANGETOPIC, (g - group), argument_length, d);
        } else {
            void *d = malloc(length);
            memcpy(d, text, length);

            tox_postmessage((action ? TOX_SENDACTIONGROUP : TOX_SENDMESSAGEGROUP), (g - group), length, d);
        }
    }

    completion.active = 0;
    edit_msg.length = 0;
}

static uint32_t peers_deduplicate(char_t **dedup, char_t **peernames, uint32_t peers)
{
    int peer, i, count;

    count = 0;
    for (peer = 0; peer < peers; peer++) {
        char_t *nick;

        nick = peernames[peer];

        if (nick) {
            _Bool found = 0;
            i = 0;

            while (!found && i < count) {
                if (nick[0] == dedup[i][0]
                        && !memcmp(nick + 1, dedup[i] + 1, nick[0])) {
                    found = 1;
                }

                i++;
            }

            if (!found) {
                dedup[count] = nick;
                count++;
            }
        }
    }

    return count;
}

static uint8_t nick_completion_search(char_t *found_nick, int direction)
{
    char_t *text = edit_msg_data;
    uint32_t i, peers, prev_index, compsize = completion.length;
    char_t *nick;
    _Bool found = 0;
    static char_t *dedup[65536];
    GROUPCHAT *g = sitem->data;

    peers = peers_deduplicate(dedup, g->peername, g->peers);

    i = 0;
    while (!found) {
        if (i >= peers) {
            found = 1;
            i = 0;
        } else {
            nick = dedup[i];
            if (nick[0] == completion.end - completion.start - completion.spacing
                    && !memcmp(nick + 1, text + completion.start, nick[0])) {
                found = 1;
            } else {
                i++;
            }
        }
    }

    prev_index = i;
    found = 0;
    do {
        if (direction == -1 && i == 0) {
            i = peers;
        }
        i += direction;

        if (i >= peers) {
            i = 0;
        }

        nick = dedup[i];

        if (nick[0] >= compsize
                && !memcmp_case(nick + 1, text + completion.start, compsize)) {
            found = 1;
        }
    } while (!found && i != prev_index);

    if (found) {
        memcpy(found_nick, nick + 1, nick[0]);
        return nick[0];
    } else {
        return 0;
    }
}

static void nick_completion_replace(char_t *nick, uint32_t size)
{
    char_t *text = edit_msg_data;
    STRING_IDX length = edit_msg.length;
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

    if (completion.start + size > edit_msg.maxlength) {
        size = edit_msg.maxlength - completion.start;
    }

    offset = completion.end - completion.start - size;

    edit_do(&edit_msg, completion.start, completion.end - completion.start, 1);

    memmove(text + completion.end - offset,
            text + completion.end, length - completion.end);

    memcpy(text + completion.start, nick, size);

    edit_do(&edit_msg, completion.start, size, 0);

    edit_msg.length -= offset;
    completion.end -= offset;
}

static void edit_msg_ontab(void)
{
    char_t *text = edit_msg_data;
    STRING_IDX length = edit_msg.length;

    if (sitem->item == ITEM_GROUP) {
        char_t nick[130];
        uint8_t nick_length;

        if (completion.cursorpos != edit_getcursorpos()) {
            completion.active = 0;
        }

        if (!completion.active) {
            if ((length == 6 && !memcmp(text, "/topic", 6))
                    || (length == 7 && !memcmp(text, "/topic ", 7))) {
                GROUPCHAT *g = sitem->data;

                text[6] = ' ';
                memcpy(text + 7, g->name, g->name_length);
                edit_msg.length = g->name_length + 7;
                edit_setcursorpos(&edit_msg, edit_msg.length);

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

        nick_length = nick_completion_search(nick, 1);
        if (nick_length) {
            completion.edited = 1;
            if (!(nick_length == completion.end - completion.start - completion.spacing
                    && !memcmp(nick, text + completion.start, nick_length))) {
                nick_completion_replace(nick, nick_length);
            }
            edit_setcursorpos(&edit_msg, completion.end);
            completion.cursorpos = edit_getcursorpos();
        }
    } else {
        completion.active = 0;
    }
}

static void edit_msg_onshifttab(void)
{
    char_t *text = edit_msg_data;

    if (sitem->item == ITEM_GROUP) {
        char_t nick[130];
        uint8_t nick_length;

        if (completion.cursorpos != edit_getcursorpos()) {
            completion.active = 0;
        }

        if (completion.active) {
            nick_length = nick_completion_search(nick, -1);
            if (nick_length) {
                completion.edited = 1;
                if (!(nick_length == completion.end - completion.start - completion.spacing
                            && !memcmp(nick, text + completion.start, nick_length))) {
                    nick_completion_replace(nick, nick_length);
                }
                edit_setcursorpos(&edit_msg, completion.end);
                completion.cursorpos = edit_getcursorpos();
            }
        }
    } else {
        completion.active = 0;
    }
}

static void edit_msg_onlosefocus(void)
{
    completion.active = 0;
}

static void edit_msg_onchange(void)
{
    if(sitem->item == ITEM_FRIEND) {
        FRIEND *f = sitem->data;

        if(!f->online) {
            return;
        }

        tox_postmessage(TOX_SET_TYPING, (f - friend), 0, NULL);
    }

    if (completion.edited) {
        completion.edited = 0;
    } else {
        completion.active = 0;
    }
}

static void edit_search_onchange(void)
{
    char_t *data = edit_search_data;
    STRING_IDX length = edit_search.length;

    if(!length) {
        memset(search_offset, 0, sizeof(search_offset));
        memset(search_unset, 0, sizeof(search_unset));
        SEARCH = 0;
    } else {
        SEARCH = 1;
        memcpy(search_data, data, length);
        search_data[length] = 0;
    }

    redraw();
    return;
}


static void edit_proxy_ip_port_onlosefocus(void)
{
    edit_proxy_port.data[edit_proxy_port.length] = 0;
    uint16_t proxy_port = strtol((char*)edit_proxy_port.data, NULL, 0);

    if (memcmp(options.proxy_address, edit_proxy_ip.data, edit_proxy_ip.length) == 0 &&
        options.proxy_address[edit_proxy_ip.length] == 0 &&
        options.proxy_port == proxy_port)
            return;

    memcpy(options.proxy_address, edit_proxy_ip.data, edit_proxy_ip.length);
    options.proxy_address[edit_proxy_ip.length] = 0;

    options.proxy_port = proxy_port;

    if (options.proxy_type)
        tox_settingschanged();
}

SCROLLABLE edit_addmsg_scroll = {
    .panel = {
        .type = PANEL_SCROLLABLE,
    },
    .d = 1.0,
    .color = C_SCROLL,
},

edit_msg_scroll = {
    .panel = {
        .type = PANEL_SCROLLABLE,
    },
    .d = 1.0,
    .color = C_SCROLL,
};

EDIT edit_name = {
    .maxlength = 128,
    .data = edit_name_data,
    .onenter = edit_name_onenter,
    .onlosefocus = edit_name_onenter,
},

edit_toxid = {
    .length = sizeof(self.id),
    .data = self.id,
    .readonly = 1,
    .noborder = 1,
    .select_completely = 1,
},

edit_status = {
    .maxlength = 128,
    .data = edit_status_data,
    .onenter = edit_status_onenter,
    .onlosefocus = edit_status_onenter,
},

edit_addid = {
    .maxlength = sizeof(edit_addid_data),
    .data = edit_addid_data,
},

edit_addmsg = {
    .multiline = 1,
    .scroll = &edit_addmsg_scroll,
    .maxlength = sizeof(edit_addmsg_data),
    .data = edit_addmsg_data,
},

edit_msg = {
    .multiline = 1,
    .scroll = &edit_msg_scroll,
    .maxlength = sizeof(edit_msg_data),
    .data = edit_msg_data,
    .onenter = edit_msg_onenter,
    .ontab = edit_msg_ontab,
    .onshifttab = edit_msg_onshifttab,
    .onchange = edit_msg_onchange,
    .onlosefocus = edit_msg_onlosefocus,
},

edit_search = {
    .maxlength = sizeof(edit_search_data),
    .data = edit_search_data,
    .empty_str = { .i18nal = STR_CONTACTS_FILTER_EDIT_HINT },
    .onchange = edit_search_onchange,
},

edit_proxy_ip = {
    .maxlength = sizeof(edit_proxy_ip_data) - 1,
    .data = edit_proxy_ip_data,
    .onlosefocus = edit_proxy_ip_port_onlosefocus,
    .empty_str = { .i18nal = STR_PROXY_EDIT_HINT_IP },
},

edit_proxy_port = {
    .maxlength = sizeof(edit_proxy_port_data) - 1,
    .data = edit_proxy_port_data,
    .onlosefocus = edit_proxy_ip_port_onlosefocus,
    .empty_str = { .i18nal = STR_PROXY_EDIT_HINT_PORT },
};
