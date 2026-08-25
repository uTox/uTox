#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdbool.h>
#include <stdint.h>

#define MAX_NUM_CMDS 256

struct Command {
    char *cmd;
    int   cmd_length;
    bool (*func)(void *object, char *arg, int arg_length);
};

/** utox_run_command()
 *
 * Parse `string` for a slash-command. If it is supported, run it; otherwise
 * return the command name and argument.
 *
 * `cmd` and `argument` are required out-parameters and must not be NULL.
 *
 * Returns the remaining string length.
 */
uint16_t utox_run_command(char *string, uint16_t string_length, char **cmd, char **argument, int trusted);

extern bool g_select_add_friend_later;
void do_tox_url(uint8_t *url_string, int len);

#endif
