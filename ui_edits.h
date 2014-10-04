/* edits */
static char_t edit_name_data[128], edit_status_data[128], edit_addid_data[TOX_FRIEND_ADDRESS_SIZE * 2], edit_addmsg_data[1024], edit_msg_data[65535], edit_search_data[127],
    edit_proxy_ip_data[256], edit_proxy_port_data[8];

static void edit_name_onenter(void)
{
    char_t *data = edit_name_data;
    STRING_IDX length = edit_name.length;

    if(!length) {
        return;
    }

    memcpy(self.name, data, length);
    self.name_length = length;

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

    tox_postmessage(TOX_SETSTATUSMSG, length, 0, self.statusmsg);//!
}

static void edit_msg_onenter(void)
{
    STRING_IDX length = edit_msg.length;
    if(length == 0) {
        return;
    }

    if(sitem->item == ITEM_FRIEND) {
        FRIEND *f = sitem->data;

        if(!f->online) {
            return;
        }

        MESSAGE *msg = malloc(length + sizeof(MESSAGE));
        msg->author = 1;
        msg->msg_type = MSG_TYPE_TEXT;
        msg->length = length;
        memcpy(msg->msg, edit_msg_data, length);

        friend_addmessage(f, msg);

        void *d = malloc(length);
        memcpy(d, edit_msg_data, length);

        tox_postmessage(TOX_SENDMESSAGE, (f - friend), length, d);
    } else {
        GROUPCHAT *g = sitem->data;

        void *d = malloc(length);
        memcpy(d, edit_msg_data, length);

        tox_postmessage(TOX_SENDMESSAGEGROUP, (g - group), length, d);
    }

    edit_msg.length = 0;
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

    if (options.proxy_enabled)
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
