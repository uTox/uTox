
typedef uint8_t char_t;

typedef struct {
    char_t *str;
    uint16_t length;
} STRING;

#define STRING_INIT(x) { .str = (char_t*)x, .length = sizeof(x) - 1 }
