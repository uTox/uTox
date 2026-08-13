#include "commands.h"

#include "command_funcs.h"
#include "debug.h"
#include "flist.h"
#include "tox.h"

#include "layout/friend.h" // TODO, we should try to remove this dependency
#include "ui/edit.h"

#include <stdlib.h>
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
    if (argument) {
        *argument = NULL;
    }

    if (string[0] == '/') { /* Cool it's a command we support! */
        // LOG_TRACE("Commands", "command found!" );
        uint16_t i;
        cmd_length = string_length;
        for (i = 0; i < string_length; ++i) {
            if (string[i] == ' ') {
                cmd_length = i;
                break;
            }
        }

        if (i < string_length) {
            ++i;
            for (; i < string_length; ++i) {
                if (string[i] != ' ') {
                    argument_length = string_length - i;
                    *argument       = string + i;
                    break;
                }
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
            void* object = flist_get_sel_friend() ? (void*)flist_get_sel_friend() : (void*)flist_get_sel_group();
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

    if (len <= 4 || memcmp(url_string, "tox:", 4) != 0) {
        return;
    }

    url_string += 4;
    len -= 4;

    if (!edit_add_new_friend_id.data || !edit_add_new_friend_msg.data
        || edit_add_new_friend_id.data_size == 0 || edit_add_new_friend_msg.data_size == 0) {
        return;
    }

    char *id_buf  = calloc(1, edit_add_new_friend_id.data_size);
    char *msg_buf = calloc(1, edit_add_new_friend_msg.data_size);
    if (!id_buf || !msg_buf) {
        free(id_buf);
        free(msg_buf);
        return;
    }

    uint8_t  *b = (uint8_t *)id_buf;
    uint16_t  id_len = 0, msg_len = 0;
    uint16_t *l = &id_len;
    size_t    cap = edit_add_new_friend_id.data_size;
    bool      writing_msg = false;
    bool      ok = true;
    uint8_t  *a = url_string, *end = url_string + len;

    while (ok && a != end) {
        switch (*a) {
            case 'a' ... 'z':
            case 'A' ... 'Z':
            case '0' ... '9':
            case '@':
            case '.':
            case ' ': {
                if ((size_t)*l + 1 >= cap) {
                    ok = false;
                    break;
                }
                *b++ = *a;
                *l   = *l + 1;
                break;
            }

            case '+': {
                if ((size_t)*l + 1 >= cap) {
                    ok = false;
                    break;
                }
                *b++ = ' ';
                *l   = *l + 1;
                break;
            }

            case '?':
            case '&': {
                a++;
                if (end - a >= 8 && memcmp(a, "message=", 8) == 0) {
                    b           = (uint8_t *)msg_buf;
                    l           = &msg_len;
                    cap         = edit_add_new_friend_msg.data_size;
                    writing_msg = true;
                    a += 7;
                } else {
                    while (*a != '&' && a != end) {
                        a++;
                    }
                    a--;
                }
                break;
            }

            case '/': {
                break;
            }

            default: {
                ok = false;
                break;
            }
        }
        if (ok) {
            a++;
        }
    }

    if (ok) {
        memcpy(edit_add_new_friend_id.data, id_buf, id_len);
        edit_add_new_friend_id.length = id_len;
        if (writing_msg) {
            memcpy(edit_add_new_friend_msg.data, msg_buf, msg_len);
            edit_add_new_friend_msg.length = msg_len;
        } else {
            edit_add_new_friend_msg.length = 0;
        }

        if (tox_thread_init != UTOX_TOX_THREAD_INIT_SUCCESS) {
            g_select_add_friend_later = 1;
        } else {
            flist_selectaddfriend();
        }
    }

    free(id_buf);
    free(msg_buf);
}
