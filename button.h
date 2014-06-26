/*todo: draw nice button */

struct button {
    PANEL panel;
    int bm, bm2, bw, bh;
    uint32_t c1, c2, c3;
    char *str;
    _Bool mouseover, mousedown, disabled;
    void (*onpress)(void);
};

void button_draw(BUTTON *b, int x, int y, int width, int height);
_Bool button_mmove(BUTTON *b, int x, int y, int dy, int width, int height);
_Bool button_mdown(BUTTON *b);
_Bool button_mright(BUTTON *b);
_Bool button_mwheel(BUTTON *b, int height, double d);
_Bool button_mup(BUTTON *b);
_Bool button_mleave(BUTTON *b);
