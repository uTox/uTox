/*todo: draw nice button */

typedef struct {
    PANEL panel;
    int bm;
    _Bool mouseover, mousedown;
    uint16_t text_length;
    uint8_t *text;
    void (*onpress)(void);
} BUTTON;

void button_draw(BUTTON *b, int x, int y, int width, int height);
_Bool button_mmove(BUTTON *b, int x, int y, int dy, int width, int height);
_Bool button_mdown(BUTTON *b);
_Bool button_mright(BUTTON *b);
_Bool button_mwheel(BUTTON *b, int height, double d);
_Bool button_mup(BUTTON *b);
_Bool button_mleave(BUTTON *b);
