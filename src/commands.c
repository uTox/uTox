// commands.c
#include "commands.h"

#include "flist.h"


int slash_send_file(FRIEND *friend_handle, const uint8_t *filepath) {
    if (filepath == NULL)
        return 0;

    debug("Slash:\tFile path is: %s\n", filepath);
    postmessage_toxcore(TOX_FILE_SEND_NEW_SLASH, friend_handle - friend, 0xFFFF, (void *)filepath);
    return 1;
}

uint16_t utox_run_command(char *string, uint16_t string_length, char **cmd, char **argument, int trusted) {
    if (trusted == 0) {
        return 0; /* We don't currently support commands from non-trusted sources, before you run commands from friends
                   * or elsewhere, you MUST implement error checking better than what exists */
    }

    uint16_t cmd_length, argument_length;

    if (string[0] == '/') { /* Cool it's a command we support! */
        // debug("command found!\n");
        uint16_t i;
        for (i = 0; i < string_length; ++i) {
            if (string[i] == ' ') {
                cmd_length = i;
                break;
            }
        }

        ++i;
        for (; i < string_length; ++i) {
            if (string[i] != ' ') {
                argument_length = string_length - i;
                *argument       = string + i;
                break;
            }
        }

        if (cmd_length) {
            --cmd_length;
            *cmd = string + 1;
        }
    } else {
        // debug("No command found\n"); /* Sad, we don't support this command. */
        *argument = string;
        cmd       = NULL;
        return 0;
    }

    /* Start accepting actions */
    if ((cmd_length == 1) && (memcmp(*cmd, "d", 1) == 0)) {
        if (flist_get_selected()->item == ITEM_FRIEND) {
            FRIEND *f = flist_get_selected()->data;

            uint8_t id[TOX_FRIEND_ADDRESS_SIZE * 2];
            string_to_id(id, *argument);
            void *data = malloc(TOX_FRIEND_ADDRESS_SIZE * sizeof(char));
            if (data == NULL) {
                debug("utox_run_command:\t Could not allocate memory.\n");
                return 0;
            }
            memcpy(data, id, TOX_FRIEND_ADDRESS_SIZE);

            postmessage_toxcore(TOX_FRIEND_NEW_DEVICE, f->number, 0, data);
        }
    } else if ((cmd_length == 5) && (memcmp(*cmd, "alias", 5) == 0)) {
        if (flist_get_selected()->item == ITEM_FRIEND) {
            FRIEND *f = flist_get_selected()->data;
            if (*argument) {
                friend_set_alias(f, *argument, argument_length);
            } else {
                friend_set_alias(f, NULL, 0);
            }

            utox_write_metadata(f);

            cmd_length = -1; /* We'll take care of this, don't return to edit */
        }
    } else if ((cmd_length == 8) && (memcmp(*cmd, "sendfile", 8) == 0)) {
        if (flist_get_selected()->item == ITEM_FRIEND) {
            FRIEND *f = flist_get_selected()->data;
            if (slash_send_file(f, *argument)) {
                cmd_length = -1; /* We'll take care of this, don't return to edit */
            } else {
                return 0;
            }
        }
    } else if (cmd_length == 6 && memcmp(*cmd, "invite", 6) == 0) {
        if (flist_get_selected()->item == ITEM_GROUP) {
            GROUPCHAT *g = flist_get_selected()->data;
            FRIEND *   f = find_friend_by_name(*argument);
            if (f != NULL && f->online) {
                cmd_length = -1;
                postmessage_toxcore(TOX_GROUP_SEND_INVITE, g->number, f->number, NULL);
            } else {
                return 0;
            }
        }
    } else {
        // debug("Command unsupported!\n");
    }
    return cmd_length;
}

bool g_select_add_friend_later = 0;

void do_tox_url(uint8_t *url_string, int len) {
    debug("Command: %.*s\n", len, url_string);

    //! lacks max length checks, writes to inputs even on failure, no notice of failure
    // doesnt reset unset inputs

    // slashes are removed later
    if (len > 4 && memcmp(url_string, "tox:", 4) == 0) {
        url_string += 4;
        len -= 4;
    } else {
        return;
    }

    // wtf??
    uint8_t * b = edit_add_id.data, *a = url_string, *end = url_string + len;
    uint16_t *l = &edit_add_id.length;
    *l          = 0;
    while (a != end) {
        switch (*a) {
            case 'a' ... 'z':
            case 'A' ... 'Z':
            case '0' ... '9':
            case '@':
            case '.':
            case ' ': {
                *b++ = *a;
                *l   = *l + 1;
                break;
            }

            case '+': {
                *b++ = ' ';
                *l   = *l + 1;
                break;
            }

            case '?':
            case '&': {
                a++;
                if (end - a >= 8 && memcmp(a, "message=", 8) == 0) {
                    b  = edit_add_msg.data;
                    l  = &edit_add_msg.length;
                    *l = 0;
                    a += 7;
                } else {
                    // skip everythng up to the next &
                    while (*a != '&' && a != end) {
                        a++;
                    }
                    // set the track back to the & so we can proceed normally
                    a--;
                }
                break;
            }

            case '/': {
                break;
            }

            default: { return; }
        }
        a++;
    }

    if (!tox_thread_init) {
        // if we receive a URL event before the profile is loaded, save it for later.
        // this usually happens when we are launched as the result of a URL click.
        g_select_add_friend_later = 1;
    } else {
        flist_selectaddfriend();
    }
}
