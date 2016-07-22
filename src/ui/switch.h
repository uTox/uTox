struct uiswitch {
    PANEL panel;

    // Switch switch toggle style.
    SVG_IMG style;

    // Width/height of bm2 picture. Used for centering.
    int bw, bh;

    // Background RGB color for bm picture, when Idle/Hovered/Pressed respectively.
    uint32_t
        bg_color,       // Switch normal background color
        sw_color,       // Switch 'toggle' color
        hover_color,    // Switch mouse over color
        press_color,    // Switch mouse down color
        disabled_color; // Switch disabled bg color

    MAYBE_I18NAL_STRING tooltip_text;

    bool switch_on;
    bool mouseover, mousedown, disabled, nodraw;

    void (*onright)(void); // called when right mouse uiswitch goes down
    void (*onpress)(void);
    void (*update)(UISWITCH *s);
};

void switch_draw(UISWITCH *s, int x, int y, int width, int height);
bool switch_mmove(UISWITCH *s, int x, int y, int width, int height, int mx, int my, int dx, int dy);
bool switch_mdown(UISWITCH *s);
bool switch_mright(UISWITCH *s);
bool switch_mwheel(UISWITCH *s, int height, double d, bool smooth);
bool switch_mup(UISWITCH *s);
bool switch_mleave(UISWITCH *s);
