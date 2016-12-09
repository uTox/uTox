#ifndef COMMAND_FUNCS_H
#define COMMAND_FUNCS_H

#include <stdbool.h>

/** slash_send_file()
 *
 * takes a file from the message box, and send it to the current friend.
 *
 * TODO, make sure the file exists.
 */
bool slash_send_file(void *friend_handle, char *filepath, int arg_length);

/**
 * Adds a device to a friend
 */
bool slash_device(void *f, char *argument, int arg_length);

/**
 * Sets the current friend's alias to the value of arg
 */
bool slash_alias(void *f, char *arg, int arg_length);

/**
 * Invites a friend to the current groupchat
 */
bool slash_invite(void *f, char *arg, int arg_length);

/**
 * Sets the topic of the current groupchat to the value of arg
 */
bool slash_topic(void *object, char *arg, int arg_length);

#endif
