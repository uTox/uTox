#ifndef TOOLTIP_H
#define TOOLTIP_H

#include <stdbool.h>

#include "../ui.h"

typedef struct tooltip {
    int  x, y, width, height;
    bool visible;

    bool can_show;
    bool mouse_down;
    bool thread;

    MAYBE_I18NAL_STRING *tt_text;
} TOOLTIP;

// removes the tooltip, requires a redraw
void tooltip_reset(void);

void tooltip_draw(void);
bool tooltip_mmove(void);
bool tooltip_mdown(void);
bool tooltip_mup(void);

void tooltip_show(void);
void tooltip_new(MAYBE_I18NAL_STRING *text);

#endif
