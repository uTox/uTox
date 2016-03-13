
typedef uint8_t char_t;
// Unsigned integer type for string indices and sizes.
typedef uint16_t uint16_t;
#define uint16_t_MAX (UINT16_MAX)

typedef struct {
    char_t *str;
    uint16_t length;
} STRING;

#define STRING_INIT(x) { .str = (char_t*)x, .length = sizeof(x) - 1 }
