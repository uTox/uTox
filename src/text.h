#ifndef TEXT_H
#define TEXT_H

#include <stdbool.h>
#include <stdint.h>

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

void to_hex(char *out, uint8_t *in, int size);

/* returns non-zero if substring is found */
bool strstr_case(const char *a, const char *b);

#endif
