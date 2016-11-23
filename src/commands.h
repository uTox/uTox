#ifndef COMMANDS_H
#define COMMANDS_H

#include "friend.h"

/** slash_send_file()
 *
 * takes a file from the message box, and send it to the current friend.
 *
 * TODO, make sure the file exists.
 */
int slash_send_file(FRIEND *friend_handle, const char *filepath);

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
