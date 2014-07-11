/* edits */
static uint8_t edit_name_data[128], edit_status_data[128], edit_addid_data[TOX_FRIEND_ADDRESS_SIZE * 2], edit_addmsg_data[1024], edit_msg_data[65535];

static void edit_name_onenter(void)
{
    uint8_t *data = edit_name_data;
    uint16_t length = edit_name.length;

    if(!length) {
        return;
    }

    memcpy(self.name, data, length);
    self.name_length = length;

    tox_postmessage(TOX_SETNAME, length, 0, self.name);//!
}

static void edit_status_onenter(void)
{
    uint8_t *data = edit_status_data;
    uint16_t length = edit_status.length;

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
    uint16_t length = edit_msg.length;
    if(length == 0) {
        return;
    }

    if(sitem->item == ITEM_FRIEND) {
        FRIEND *f = sitem->data;

        if(!f->online) {
            return;
        }

        MESSAGE *msg = malloc(length + sizeof(MESSAGE));
        msg->flags = 1;
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
};
