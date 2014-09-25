
typedef struct {
    uint8_t *str;
    uint16_t length;
} STRING;

#define STRING_INIT(x) { .str = (uint8_t*)x, .length = sizeof(x) - 1 }
