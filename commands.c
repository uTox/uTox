#include "main.h"

STRING_IDX utox_run_command(char_t *string, STRING_IDX string_length, char_t **cmd, char_t **argument, int trusted){
    if(trusted == 0){
        return 0; /* We don't currently support commands from non-trusted sources, before you run commands from friends
                   * or elsewhere, you MUST implement error checking better than what exists */
    }

    STRING_IDX cmd_length, argument_length;

    if (string[0] == '/') { /* Cool it's a command we support! */
        debug("command found!\n");
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
        debug("No command found\n"); /* Sad, we done support this command. */
        *argument = string;
        cmd = NULL;
        return 0;
    }

    /* Start accepting actions */
    if ((cmd_length == 5) && (memcmp(*cmd, "alias", 5) == 0) && *argument ) {
        if(sitem->item == ITEM_FRIEND) {
            FRIEND *f = sitem->data;
            friend_set_alias(f, *argument, argument_length);
        }
    } else {
        debug("Command unsupported!\n");
    }
    return cmd_length;
}
