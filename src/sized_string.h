#ifndef SIZED_STRING_H
#define SIZED_STRING_H

typedef uint8_t char_t;
// Unsigned integer type for string indices and sizes.

typedef struct {
    char_t  *str;
    uint16_t length;
} STRING;

#define STRING_INIT(x) { .str = (char_t*)x, .length = sizeof(x) - 1 }

#endif
