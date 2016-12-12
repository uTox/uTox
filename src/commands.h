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
 * takes data string and parses it for a command, if that command is supported, acts on that command, else it simply
 * results the processed command and argv.
 *
 * Returns the remaining string length.
 */
uint16_t utox_run_command(char *string, uint16_t string_length, char **cmd, char **argument, int trusted);

extern bool g_select_add_friend_later;
void do_tox_url(uint8_t *url_string, int len);

#endif
