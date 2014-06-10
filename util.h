/*todo: sprint_bytes */

/* read a whole file from a path,
 *  on success: returns pointer to data (must be free()'d later), writes size of data to *size if size is not NULL
 *  on failure: returns NULL
 */
void* file_raw(char *path, uint32_t *size);

/* convert tox id to string
 *  notes: dest must be (TOX_FRIEND_ADDRESS_SIZE * 2) bytes large, src must be TOX_FRIEND_ADDRESS_SIZE bytes large
 */
void id_to_string(char_t *dest, char_t *src);

/* same as id_to_string(), but for TOX_CLIENT_ID_SIZE
 */
void cid_to_string(char_t *dest, char_t *src);

/* convert string to tox id
 *  on success: returns 1
 *  on failure: returns 0
 *  notes: dest must be TOX_FRIEND_ADDRESS_SIZE bytes large, some data may be written to dest even on failure
 */
_Bool string_to_id(char_t *dest, char_t *src);

/* convert number of bytes to human readable string
    returns number of characters written
    notes: dest should be atleast # characters large
*/
int sprint_bytes(uint8_t *dest, uint64_t bytes);
