#ifndef UTIL_H
#define UTIL_H

#include "friend.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
/*todo: sprint_bytes */

/* read a whole file from a path,
 *  on success: returns pointer to data (must be free()'d later), writes size of data to *size if size is not NULL
 *  on failure: returns NULL
 */
void *file_raw(char *path, uint32_t *size);

/*
 * Write *data, of size, to path on disk.
 */
void file_write_raw(uint8_t *path, uint8_t *data, size_t size);

// add null terminator to data
void *file_text(char *path);

/* returns non-zero if substring is found */
bool strstr_case(const char *a, const char *b);

/* convert tox id to string
 *  notes: dest must be (TOX_FRIEND_ADDRESS_SIZE * 2) bytes large, src must be TOX_FRIEND_ADDRESS_SIZE bytes large
 */
void id_to_string(char *dest, uint8_t *src);

/* same as id_to_string(), but for TOX_PUBLIC_KEY_SIZE
 */
void cid_to_string(char *dest, uint8_t *src);

/* same as id_to_string(), but for TOX_FILE_ID_LENGTH
 */
void fid_to_string(char *dest, uint8_t *src);

/* convert tox hash to string,
 *  notes: dest must be (TOX_HASH_LENGTH * 2) bytes large, src must be TOX_HASH_LENGTH bytes large
 */
void hash_to_string(char *dest, uint8_t *src);

/** convert string to tox id
 *  on success: returns 1
 *  on failure: returns 0
 *  notes: dest must be TOX_FRIEND_ADDRESS_SIZE bytes large, some data may be written to dest even on failure
 */
bool string_to_id(uint8_t *dest, char *src);

/** convert number of bytes to human readable string
 *  returns number of characters written
 *  notes: dest MUST be at least size characters large
 */
int sprint_humanread_bytes(char *dest, unsigned int size, uint64_t bytes);

/** length of a utf-8 character
 *  returns the size of the character in bytes
 *  returns -1 if the size of the character is greater than len or if the character is invalid
 */
uint8_t utf8_len(const char *data);
/* read the character into ch */
uint8_t utf8_len_read(const char *data, uint32_t *ch);
/* backwards length */
uint8_t utf8_unlen(char *data);

/* remove invalid characters from utf8 string
 * returns the new length after invalid characters have been removed
 */
int utf8_validate(const uint8_t *data, int len);

/*
 */
uint8_t unicode_to_utf8_len(uint32_t ch);
void unicode_to_utf8(uint32_t ch, char *dst);

/* compare first n bytes of s1 and s2, ignoring the case of alpha chars
 *  match: returns 0
 *  no match: returns 1
 *  notes: n must be <= length of s1 and <= length of s2
 */
bool memcmp_case(const char *s1, const char *s2, uint32_t n);

/* replace html entities (<,>,&) with html
 */
char *tohtml(const char *str, uint16_t len);

/* color format conversion functions
 *
 */
void yuv420tobgr(uint16_t width, uint16_t height, const uint8_t *y, const uint8_t *u, const uint8_t *v,
                 unsigned int ystride, unsigned int ustride, unsigned int vstride, uint8_t *out);
void yuv422to420(uint8_t *plane_y, uint8_t *plane_u, uint8_t *plane_v, uint8_t *input, uint16_t width, uint16_t height);
void bgrtoyuv420(uint8_t *plane_y, uint8_t *plane_u, uint8_t *plane_v, uint8_t *rgb, uint16_t width, uint16_t height);
void bgrxtoyuv420(uint8_t *plane_y, uint8_t *plane_u, uint8_t *plane_v, uint8_t *rgb, uint16_t width, uint16_t height);

/*
 */
void scale_rgbx_image(uint8_t *old_rgbx, uint16_t old_width, uint16_t old_height, uint8_t *new_rgbx, uint16_t new_width,
                      uint16_t new_height);


/** Loads data from file at filepath.
 *  Size of data loaded is written to out_size.
 *  Returns data loaded.
 *  Notes: It is the caller's responsibility to free the data when it is no longer needed.
 */
uint8_t *load_data(uint8_t *filepath, size_t *out_size);

// TODO this needs to me moved out of here so we can drop the ".h" includes above
UTOX_SAVE *config_load(void);
void config_save(UTOX_SAVE *save);


// TODO FIXME this needs to me moved out of here so we can drop the ".h" includes above
/*
 Saves user meta data to disk
 */
void utox_write_metadata(FRIEND *f);

#endif
