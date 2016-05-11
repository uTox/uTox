#include "main.h"

int slash_send_file(FRIEND *friend_handle, const uint8_t *filepath){
	if (filepath == NULL)
		return 0;

    debug("Slash:\tFile path is: %s\n", filepath);
    postmessage_toxcore(TOX_FILE_SEND_NEW_SLASH, friend_handle - friend, 0xFFFF, (void*)filepath);
    return 1;
}

uint16_t utox_run_command(char_t *string, uint16_t string_length, char_t **cmd, char_t **argument, int trusted){
    if(trusted == 0){
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
                *argument = string + i;
                break;
            }
        }

        if(cmd_length){
            --cmd_length;
            *cmd = string + 1;
        }
    } else {
        // debug("No command found\n"); /* Sad, we don't support this command. */
        *argument = string;
        cmd = NULL;
        return 0;
    }

    /* Start accepting actions */
    if ((cmd_length == 5) && (memcmp(*cmd, "alias", 5) == 0)) {
        if(selected_item->item == ITEM_FRIEND) {
            FRIEND *f = selected_item->data;
            if (*argument) {
                friend_set_alias(f, *argument, argument_length);
            } else {
                friend_set_alias(f, NULL, 0);
            }

            utox_write_metadata(f);

            cmd_length = -1; /* We'll take care of this, don't return to edit */
        }
    } else if ((cmd_length == 8) && (memcmp(*cmd, "sendfile", 8) == 0)){
        if(selected_item->item == ITEM_FRIEND) {
            FRIEND *f = selected_item->data;
            if (slash_send_file(f, *argument)) {
                cmd_length = -1; /* We'll take care of this, don't return to edit */
            } else {
                return 0;
            }
        }
    } else {
        // debug("Command unsupported!\n");
    }
    return cmd_length;
}

_Bool g_select_add_friend_later = 0;

void do_tox_url(uint8_t *url_string, int len) {
    debug("Command: %.*s\n", len, url_string);

    //! lacks max length checks, writes to inputs even on failure, no notice of failure
    //doesnt reset unset inputs

    // slashes are removed later
    if (len > 4 && memcmp(url_string, "tox:", 4) == 0) {
        url_string += 4;
        len -= 4;
    } else {
        return;
    }

    // wtf??
    uint8_t *b = edit_add_id.data, *a = url_string, *end = url_string + len;
    uint16_t *l = &edit_add_id.length;
    *l = 0;
    while(a != end)
    {
        switch(*a)
        {
            case 'a' ... 'z':
            case 'A' ... 'Z':
            case '0' ... '9':
            case '@':
            case '.':
            case ' ':
            {
                *b++ = *a;
                *l = *l + 1;
                break;
            }

            case '+':
            {
                *b++ = ' ';
                *l = *l + 1;
                break;
            }

            case '?':
            case '&':
            {
                a++;
                if(end - a >= 8 && memcmp(a, "message=", 8) == 0)
                {
                    b = edit_add_msg.data;
                    l = &edit_add_msg.length;
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

            case '/':
            {
                break;
            }

            default:
            {
                return;
            }
        }
        a++;
    }

    if (!tox_thread_init) {
        // if we receive a URL event before the profile is loaded, save it for later.
        // this usually happens when we are launched as the result of a URL click.
        g_select_add_friend_later = 1;
    } else {
        list_selectaddfriend();
    }
}
