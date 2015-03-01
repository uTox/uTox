struct button {
    PANEL panel;

    // Button picture id, top-left aligned/centered respectively.
    int bm, bm2;

    // Width/height of bm2 picture. Used for centering.
    int bw, bh;

    // Background RGB color for bm picture, when Idle/Hovered/Pressed respectively.
    uint32_t c1, c2, c3, ic, cd;

    MAYBE_I18NAL_STRING button_text;
    MAYBE_I18NAL_STRING tooltip_text;

    _Bool mouseover, mousedown, disabled, nodraw;

    void (*onright)(void); // called when right mouse button goes down
    void (*onpress)(void);
    void (*updatecolor)(BUTTON *b);
};

void button_draw(BUTTON *b, int x, int y, int width, int height);
_Bool button_mmove(BUTTON *b, int x, int y, int width, int height, int mx, int my, int dx, int dy);
_Bool button_mdown(BUTTON *b);
_Bool button_mright(BUTTON *b);
_Bool button_mwheel(BUTTON *b, int height, double d);
_Bool button_mup(BUTTON *b);
_Bool button_mleave(BUTTON *b);
