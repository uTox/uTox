typedef struct tooltip {
    int x, y, width, height;
    _Bool visible;

    _Bool timer_running;
    _Bool can_show;
    _Bool mouse_down;

    MAYBE_I18NAL_STRING* tt_text;
} TOOLTIP;

void tooltip_reset(void);
void tooltip_draw(void);
_Bool tooltip_mmove(void);
_Bool tooltip_mdown(void);
_Bool tooltip_mup(void);

void tooltip_show(void);
void tooltip_new(MAYBE_I18NAL_STRING* text);

void mouse_pos_check(void *UNUSED(args));
