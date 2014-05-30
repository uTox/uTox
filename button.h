/*todo: draw nice button */

typedef struct {
    _Bool mouseover, mousedown;
    uint16_t text_length;
    int x, y, width, height;
    uint8_t *text;
    void (*onpress)(void);
    void (*onredraw)(void);
} BUTTON;

void button_draw(BUTTON *b);
void button_mousemove(BUTTON *b, int x, int y);
void button_mousedown(BUTTON *b, int x, int y);
void button_mouseup(BUTTON *b);
void button_mouseleave(BUTTON *b);
