#ifndef SIZED_STRING_H
#define SIZED_STRING_H

#include <inttypes.h>

typedef struct {
    char *str;

    uint16_t length;
} STRING;

#define STRING_INIT(x) \
    { .str = (char *)x, .length = sizeof(x) - 1 }

#endif
