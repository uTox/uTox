typedef struct tooltip {
    int x, y, width, height;
    _Bool visible;

    _Bool can_show;
    _Bool mouse_down;
    _Bool thread;

    MAYBE_I18NAL_STRING* tt_text;
} TOOLTIP;

// removes the tooltip, requires a redraw
void tooltip_reset(void);

void tooltip_draw(void);
_Bool tooltip_mmove(void);
_Bool tooltip_mdown(void);
_Bool tooltip_mup(void);

void tooltip_show(void);
void tooltip_new(MAYBE_I18NAL_STRING* text);

