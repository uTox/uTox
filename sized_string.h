
typedef uint8_t char_t;
// Unsigned integer type for string indices and sizes.
typedef uint16_t STRING_IDX;
#define STRING_IDX_MAX (UINT16_MAX)

typedef struct {
    char_t *str;
    STRING_IDX length;
} STRING;

#define STRING_INIT(x) { .str = (char_t*)x, .length = sizeof(x) - 1 }
