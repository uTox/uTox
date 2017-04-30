#ifndef SIZED_STRING_H
#define SIZED_STRING_H

#include <stdint.h>

typedef struct sized_string {
    char *str;

    uint16_t length;
} STRING;

#define STRING_INIT(x) \
    { .str = (char *)x, .length = sizeof(x) - 1 }

#endif
