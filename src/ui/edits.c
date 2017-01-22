#include "edits.h"

#include "buttons.h"
#include "scrollable.h"

#include "../commands.h"
#include "../flist.h"
#include "../friend.h"
#include "../groups.h"
#include "../logging_native.h"
#include "../tox.h"
#include "../util.h"

// FIXME: Required for UNUSED()
#include "../main.h"

static char edit_name_data[128], edit_status_data[128], edit_addid_data[TOX_FRIEND_ADDRESS_SIZE * 4],
    edit_add_self_device_data[TOX_FRIEND_ADDRESS_SIZE * 4], edit_addmsg_data[1024], edit_msg_data[65535],
    edit_search_data[127], edit_proxy_ip_data[256], edit_proxy_port_data[8], edit_profile_password_data[65535],
    edit_friend_alias_data[128], edit_id_str[TOX_PUBLIC_KEY_SIZE * 2], edit_group_topic_data[128];

static void edit_name_onenter(EDIT *edit) {
    char *   data   = edit->data;
    uint16_t length = edit->length;

    memcpy(self.name, data, length);
    self.name_length = length;
    update_tray();

    postmessage_toxcore(TOX_SELF_SET_NAME, length, 0, self.name); //!
}

static void edit_status_onenter(EDIT *edit) {
    char *   data   = edit->data;
    uint16_t length = edit->length;

    if (length) {
        length = (length <= TOX_MAX_STATUS_MESSAGE_LENGTH) ? length : TOX_MAX_STATUS_MESSAGE_LENGTH;
        memcpy(self.statusmsg, data, length);
        self.statusmsg_length = length;
    } else {
        self.statusmsg_length = length;
    }

    update_tray();

    postmessage_toxcore(TOX_SELF_SET_STATUS, length, 0, self.statusmsg); //!
}

static void edit_add_new_contact(EDIT *UNUSED(edit)) {
    friend_add(edit_add_id.data, edit_add_id.length, edit_add_msg.data, edit_add_msg.length);
}

static void edit_friend_alias_onenter(EDIT *UNUSED(edit)) {
    FRIEND *f = flist_get_selected()->data;

    friend_set_alias(f, (uint8_t *)edit_friend_alias.data, edit_friend_alias.length);

    utox_write_metadata(f);
}

static struct {
    uint16_t start, end, cursorpos;
    uint32_t length, spacing;
    bool     active;
    bool     edited;
} completion;

void edit_msg_onenter(EDIT *edit) {
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

    // debug("cmd %u\n", command_length);

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

    if (flist_get_selected()->item == ITEM_FRIEND) {
        FRIEND *f = flist_get_selected()->data;

        /* Display locally */
        if (action) {
            message_add_type_action(&f->msg, 1, text, length, 1, 1);
        } else {
            message_add_type_text(&f->msg, 1, text, length, 1, 1);
        }
    } else if (flist_get_selected()->item == ITEM_GROUP) {
        GROUPCHAT *g = flist_get_selected()->data;
        void *d = malloc(length);
        if (!d) {
            debug_error("edit_msg_onenter:\t Ran out of memory.\n");
            return;
        }
        memcpy(d, text, length);
        postmessage_toxcore((action ? TOX_GROUP_SEND_ACTION : TOX_GROUP_SEND_MESSAGE), (g - group), length, d);
    }

    completion.active = 0;
    edit->length      = 0;
}

static uint32_t peers_deduplicate(char **dedup, size_t *dedup_size, void **peers, uint32_t peer_count) {
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

static uint8_t nick_completion_search(EDIT *edit, char *found_nick, int direction) {
    char *        text = edit->data;
    uint32_t      i, peers, prev_index, compsize = completion.length;
    char *        nick     = NULL;
    size_t        nick_len = 0;
    bool          found    = 0;
    static char * dedup[65536];      /* TODO magic numbers */
    static size_t dedup_size[65536]; /* TODO magic numbers */
    GROUPCHAT *   g = flist_get_selected()->data;

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
    uint16_t maxlength = edit->maxlength;

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

static void edit_msg_ontab(EDIT *edit) {
    char *   text   = edit->data;
    uint16_t length = edit->length;

    if ((flist_get_selected()->item == ITEM_FRIEND) || (flist_get_selected()->item == ITEM_GROUP)) {
        char    nick[130];
        uint8_t nick_length;

        if (completion.cursorpos != edit_getcursorpos()) {
            completion.active = 0;
        }

        if (!completion.active) {
            if (flist_get_selected()->item == ITEM_FRIEND) {
                if ((length == 6 && !memcmp(text, "/alias", 6)) || (length == 7 && !memcmp(text, "/alias ", 7))) {
                    FRIEND * f = flist_get_selected()->data;
                    char *   last_name;
                    uint16_t last_name_length;

                    if (f->alias) {
                        last_name        = f->alias;
                        last_name_length = f->alias_length;
                    } else {
                        last_name        = f->name;
                        last_name_length = f->name_length;
                    }

                    text[6] = ' ';
                    memcpy(text + 7, last_name, last_name_length);
                    edit->length = last_name_length + 7;
                    edit_setcursorpos(edit, edit->length);
                }

                return;
            }

            if ((length == 6 && !memcmp(text, "/topic", 6)) || (length == 7 && !memcmp(text, "/topic ", 7))) {
                GROUPCHAT *g = flist_get_selected()->data;

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

static void edit_msg_onshifttab(EDIT *edit) {
    char *text = edit->data;

    if (flist_get_selected()->item == ITEM_GROUP) {
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

static void edit_msg_onchange(EDIT *UNUSED(edit)) {
    if (flist_get_selected()->item == ITEM_FRIEND) {
        FRIEND *f = flist_get_selected()->data;

        if (!f->online) {
            return;
        }

        postmessage_toxcore(TOX_SEND_TYPING, (f - friend), 0, NULL);
    }

    if (completion.edited) {
        completion.edited = 0;
    } else {
        completion.active = 0;
    }
}

static void edit_search_onchange(EDIT *edit) {
    char *   data   = edit->data;
    uint16_t length = edit->length;

    if (length) {
        button_add_new_contact.panel.disabled = 0;
        button_add_new_contact.nodraw         = 0;
        button_settings.panel.disabled        = 1;
        button_settings.nodraw                = 1;
        memcpy(search_data, data, length);
        search_data[length] = 0;
        flist_search((char *)search_data);
    } else {
        button_add_new_contact.panel.disabled = 1;
        button_add_new_contact.nodraw         = 1;
        button_settings.panel.disabled        = 0;
        button_settings.nodraw                = 0;
        flist_search(NULL);
    }

    redraw();
    return;
}

static void edit_search_onenter(EDIT *edit) {
    char *   data   = edit->data;
    uint16_t length = edit->length;

    if (length == 76) {
        friend_add(data, length, (char *)"", 0);
        edit_setstr(&edit_search, (char *)"", 0);
    } else {
        if (tox_thread_init == UTOX_TOX_THREAD_INIT_SUCCESS) {
            /* Only change if we're logged in! */
            edit_setstr(&edit_add_id, data, length);
            edit_setstr(&edit_search, (char *)"", 0);
            flist_selectaddfriend();
            edit_setfocus(&edit_add_msg);
        }
    }

    return;
}

static void edit_proxy_ip_port_onlosefocus(EDIT *UNUSED(edit)) {
    edit_proxy_port.data[edit_proxy_port.length] = 0;

    settings.proxy_port = strtol((char *)edit_proxy_port.data, NULL, 0);

    if (memcmp(proxy_address, edit_proxy_ip.data, edit_proxy_ip.length) == 0 && proxy_address[edit_proxy_ip.length] == 0) {
        return;
    }

    memset(proxy_address, 0, 256); /* Magic number from toxcore */
    memcpy(proxy_address, edit_proxy_ip.data, edit_proxy_ip.length);
    proxy_address[edit_proxy_ip.length] = 0;


    if (settings.use_proxy) {
        tox_settingschanged();
    }
}

static void edit_profile_password_update(EDIT *UNUSED(edit)) {
    if (tox_thread_init) {
        postmessage_toxcore(TOX_SAVE, 0, 0, NULL);
    }
}

static void edit_group_topic_onenter(EDIT *edit) {
    GROUPCHAT *g = right_mouse_item->data;
    void *     d = malloc(edit->length);
    memcpy(d, edit->data, edit->length);
    postmessage_toxcore(TOX_GROUP_SET_TOPIC, (g - group), edit->length, d);
}

SCROLLABLE edit_addmsg_scroll =
               {
                 .panel =
                     {
                         .type = PANEL_SCROLLABLE,
                     },
                 .d     = 1.0,
                 .color = C_SCROLL,
               },

           edit_msg_scroll = {
               .panel =
                   {
                       .type = PANEL_SCROLLABLE,
                   },
               .d     = 1.0,
               .color = C_SCROLL,
           };

EDIT edit_name =
         {
           .maxlength = 128, .data = edit_name_data, .onenter = edit_name_onenter, .onlosefocus = edit_name_onenter,
         },

     edit_toxid =
         {
           .length = TOX_FRIEND_ADDRESS_SIZE * 2, .data = self.id_str, .readonly = 1, .noborder = 0, .select_completely = 1,
         },

     edit_friend_pubkey =
         {
           .length            = TOX_PUBLIC_KEY_SIZE * 2,
           .maxlength         = TOX_PUBLIC_KEY_SIZE * 2,
           .data              = edit_id_str,
           .readonly          = 1,
           .noborder          = 0,
           .select_completely = 1,
         },

     edit_status =
         {
           .maxlength = 128, .data = edit_status_data, .onenter = edit_status_onenter, .onlosefocus = edit_status_onenter,
         },

     edit_add_id =
         {
           .maxlength = sizeof(edit_addid_data), .data = edit_addid_data, .onenter = edit_add_new_contact,
         },

     edit_add_msg =
         {
           .multiline = 1,
           .scroll    = &edit_addmsg_scroll,
           .maxlength = sizeof(edit_addmsg_data),
           .data      = edit_addmsg_data,
           .empty_str = {.i18nal = STR_DEFAULT_FRIEND_REQUEST_MESSAGE },
         },

     edit_msg =
         {
           .multiline   = 1,
           .scroll      = &edit_msg_scroll,
           .maxlength   = sizeof(edit_msg_data),
           .data        = edit_msg_data,
           .onenter     = edit_msg_onenter,
           .ontab       = edit_msg_ontab,
           .onshifttab  = edit_msg_onshifttab,
           .onchange    = edit_msg_onchange,
           .onlosefocus = edit_msg_onlosefocus,
         },

     edit_msg_group =
         {
           .multiline   = 1,
           .scroll      = &edit_msg_scroll,
           .maxlength   = sizeof(edit_msg_data),
           .data        = edit_msg_data,
           .onenter     = edit_msg_onenter,
           .ontab       = edit_msg_ontab,
           .onshifttab  = edit_msg_onshifttab,
           .onchange    = edit_msg_onchange,
           .onlosefocus = edit_msg_onlosefocus,
         },

     edit_search =
         {
           .maxlength = sizeof(edit_search_data),
           .data      = edit_search_data,
           .onchange  = edit_search_onchange,
           .onenter   = edit_search_onenter,
           .style     = AUXILIARY_STYLE,
           .vcentered = 1,
           .empty_str = {.i18nal = STR_CONTACT_SEARCH_ADD_HINT },
         },

     edit_proxy_ip =
         {
           .maxlength   = sizeof(edit_proxy_ip_data) - 1,
           .data        = edit_proxy_ip_data,
           .onlosefocus = edit_proxy_ip_port_onlosefocus,
           .empty_str = {.i18nal = STR_PROXY_EDIT_HINT_IP },
           /* TODO .ontab = change to proxy port field */
         },

     edit_proxy_port =
         {
           .maxlength   = sizeof(edit_proxy_port_data) - 1,
           .data        = edit_proxy_port_data,
           .onlosefocus = edit_proxy_ip_port_onlosefocus,
           .empty_str = {.i18nal = STR_PROXY_EDIT_HINT_PORT },
         },

     edit_profile_password =
         {
           .maxlength = sizeof(edit_profile_password) - 1,
           .data      = edit_profile_password_data,
           // .onchange    = edit_profile_password_update,
           .onlosefocus = edit_profile_password_update,
           .password    = 1,
         },

     edit_friend_alias =
         {
           .maxlength       = 128,
           .data            = edit_friend_alias_data,
           .onenter         = edit_friend_alias_onenter,
           .onlosefocus     = edit_friend_alias_onenter,
           .empty_str.plain = STRING_INIT(""), // set dynamically to the friend's name
         },

     edit_group_topic = {.maxlength       = 128,
                         .data            = edit_group_topic_data,
                         .onenter         = edit_group_topic_onenter,
                         .onlosefocus     = edit_group_topic_onenter,
                         .noborder        = 0,
                         .empty_str.plain = STRING_INIT("") },

    edit_nospam = {.length            = sizeof(uint32_t) * 2,
                   .data              = self.nospam_str,
                   .readonly          = true,
                   .noborder          = false,
                   .select_completely = true, };

static char edit_add_new_device_to_self_data[TOX_FRIEND_ADDRESS_SIZE * 4];

static void edit_add_new_device_to_self_onenter(EDIT *UNUSED(edit)) {
#ifdef ENABLE_MULTIDEVICE
    devices_self_add(edit_add_new_device_to_self.data, edit_add_new_device_to_self.length);
#endif
}

EDIT edit_add_new_device_to_self = {
    .maxlength = sizeof(edit_add_new_device_to_self_data),
    .data      = edit_add_new_device_to_self_data,
    .onenter   = edit_add_new_device_to_self_onenter,
};
