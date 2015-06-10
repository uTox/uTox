/** utox_run_command()
 *
 * takes data string and parses it for a command, if that command is supported, acts on that command, else it simply
 * results the processed command and argv.
 *
 * Returns the remaining string length.
 */
STRING_IDX utox_run_command(char_t *string, STRING_IDX string_length, char_t **cmd, char_t **argument, int trusted);
