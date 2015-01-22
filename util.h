/*todo: sprint_bytes */

/* read a whole file from a path,
 *  on success: returns pointer to data (must be free()'d later), writes size of data to *size if size is not NULL
 *  on failure: returns NULL
 */
void* file_raw(char *path, uint32_t *size);

//add null terminator to data
void* file_text(char *path);

/* returns non-zero if substring is found */
_Bool strstr_case(const char *a, const char *b);

/* convert tox id to string
 *  notes: dest must be (TOX_FRIEND_ADDRESS_SIZE * 2) bytes large, src must be TOX_FRIEND_ADDRESS_SIZE bytes large
 */
void id_to_string(char_t *dest, char_t *src);

/* same as id_to_string(), but for TOX_CLIENT_ID_SIZE
 */
void cid_to_string(char_t *dest, char_t *src);

/* convert tox hash to string, 
 *  notes: dest must be (TOX_HASH_LENGTH * 2) bytes large, src must be TOX_HASH_LENGTH bytes large
 */
void hash_to_string(char_t *dest, char_t *src);

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
int sprint_bytes(uint8_t *dest, unsigned int size, uint64_t bytes);

/* length of a utf-8 character
    returns the size of the character in bytes
    returns -1 if the size of the character is greater than len or if the character is invalid
 */
uint8_t utf8_len(char_t *data);
/* read the character into ch */
uint8_t utf8_len_read(char_t *data, uint32_t *ch);
/* backwards length */
uint8_t utf8_unlen(char_t *data);

/* remove invalid characters from utf8 string
 * returns the new length after invalid characters have been removed
 */
int utf8_validate(const char_t *data, int len);

/*
 */
uint8_t unicode_to_utf8_len(uint32_t ch);
void unicode_to_utf8(uint32_t ch, char_t *dst);

/* compare first n bytes of s1 and s2, ignoring the case of alpha chars
 *  match: returns 0
 *  no match: returns 1
 *  notes: n must be <= length of s1 and <= length of s2
 */
_Bool memcmp_case(const char_t *s1, const char_t *s2, uint32_t n);

/* replace html entities (<,>,&) with html
 */
char_t* tohtml(char_t *str, STRING_IDX len);

/* color format conversion functions
 *
 */
void yuv420torgb(vpx_image_t *img, uint8_t *out);
void yuv422to420(uint8_t *plane_y, uint8_t *plane_u, uint8_t *plane_v, uint8_t *input, uint16_t width, uint16_t height);
void rgbtoyuv420(uint8_t *plane_y, uint8_t *plane_u, uint8_t *plane_v, uint8_t *rgb, uint16_t width, uint16_t height);
void rgbxtoyuv420(uint8_t *plane_y, uint8_t *plane_u, uint8_t *plane_v, uint8_t *rgb, uint16_t width, uint16_t height);

/*
 */
void scale_rgbx_image(uint8_t *old_rgbx, uint16_t old_width, uint16_t old_height, uint8_t *new_rgbx, uint16_t new_width, uint16_t new_height);

/*
 */
UTOX_SAVE* config_load(void);
void config_save(UTOX_SAVE *save);
