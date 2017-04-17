#include "commands.h"

#include "command_funcs.h"
#include "debug.h"
#include "flist.h"
#include "main.h" // tox_thread_init

#include "layout/friend.h" // TODO, we should try to remove this dependency
#include "ui/edit.h"

#include <string.h>

struct Command commands[MAX_NUM_CMDS] = {
    { "alias",    5, slash_alias     },
    { "invite",   6, slash_invite    },
    { "d",        1, slash_device    },
    { "sendfile", 8, slash_send_file },
    { "topic",    5, slash_topic     },
    { NULL,       0, NULL            },
};

uint16_t utox_run_command(char *string, uint16_t string_length, char **cmd, char **argument, int trusted) {
    if (trusted == 0) {
        return 0; /* We don't currently support commands from non-trusted sources, before you run commands from friends
                   * or elsewhere, you MUST implement error checking better than what exists */
    }

    uint16_t cmd_length = 0, argument_length = 0;

    if (string[0] == '/') { /* Cool it's a command we support! */
        // LOG_TRACE("Commands", "command found!" );
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
        // LOG_TRACE("Commands", "No command found" ); /* Sad, we don't support this command. */
        *argument = string;
        cmd       = NULL;
        return 0;
    }

    int i = 0;
    while(commands[i].cmd){
        if (commands[i].cmd_length == cmd_length && memcmp(commands[i].cmd, *cmd, cmd_length) == 0) {
            void* object = flist_get_friend() ? (void*)flist_get_friend() : (void*)flist_get_groupchat();
            bool ret = commands[i].func(object, *argument, argument_length);
            if (ret) {
                cmd_length = -1;
            }
            break;
        }
        i++;
    }

    return cmd_length;
}

bool g_select_add_friend_later = 0;

void do_tox_url(uint8_t *url_string, int len) {
    LOG_TRACE("Commands", "Command: %.*s" , len, url_string);

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
    uint8_t  *b = (uint8_t *)edit_add_new_friend_id.data, *a = url_string, *end = url_string + len;
    uint16_t *l = &edit_add_new_friend_id.length;
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
                    b  = (uint8_t *)edit_add_new_friend_msg.data;
                    l  = &edit_add_new_friend_msg.length;
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

            default: {
                return;
            }
        }
        a++;
    }

    if (tox_thread_init != UTOX_TOX_THREAD_INIT_SUCCESS) {
        // if we receive a URL event before the profile is loaded, save it for later.
        // this usually happens when we are launched as the result of a URL click.
        g_select_add_friend_later = 1;
    } else {
        flist_selectaddfriend();
    }
}
