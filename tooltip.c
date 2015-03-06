#include "main.h"

static TOOLTIP tooltip;

#define TOOLTIP_WIDTH (SCALE * 12)
#define TOOLTIP_HEIGHT (SCALE * 12)
#define TOOLTIP_YOFFSET 12

static void calculate_pos_and_width(TOOLTIP *b, int *x, int *w) {
    *x = b->x;
    *w = b->width;

    // Increase width if needed, so that tooltip text fits.
    if(maybe_i18nal_string_is_valid(b->tt_text)) {
        STRING* s = maybe_i18nal_string_get(b->tt_text);
        int needed_w = textwidth(s->str, s->length) + 4 * SCALE;
        if(*w < needed_w) {
            *w = needed_w;
        }
    }

    // Push away from the right border to fit.
    if(*x + *w >= utox_window_width) {
        *x -= *w;
    }
}

void tooltip_reset(void)
{
    TOOLTIP *b = &tooltip;

    b->visible = 0;
    b->can_show = 0;
}

void tooltip_draw(void)
{
    TOOLTIP *b = &tooltip;
    if(!b->visible) {
        return;
    }

    // Ensure that font is set before calculating position and width.
    setfont(FONT_TEXT);
    setcolor(COLOR_MAIN_TEXT);

    int x, w;
    calculate_pos_and_width(b, &x, &w);

    drawrectw(x, b->y, w, b->height, COLOR_MAIN_BACKGROUND);

    STRING* s = maybe_i18nal_string_get(b->tt_text);
    drawtext(x + SCALE * 2, b->y + SCALE * 2, s->str, s->length);

    framerect(x, b->y, x + w, b->y + b->height, COLOR_EDGE_NORMAL);
}

_Bool tooltip_mmove(void)
{
    TOOLTIP *b = &tooltip;

    b->can_show = 0;

    if(!b->visible) {
        return 0;
    }

    b->visible = 0;
    return 1;
}

_Bool tooltip_mdown(void)
{
    TOOLTIP *b = &tooltip;

    b->can_show = 0;
    b->mouse_down = 1;
    b->visible = 0;

    return 0;
}

_Bool tooltip_mup(void)
{
    TOOLTIP *b = &tooltip;

    b->can_show = 0;
    b->mouse_down = 0;
    return 0;
}

void tooltip_show(void)
{
    TOOLTIP *b = &tooltip;

    b->y = mouse.y + TOOLTIP_YOFFSET;
    b->height = TOOLTIP_HEIGHT;
    if(b->y + b->height >= utox_window_height) {
        b->y -= (b->height + TOOLTIP_YOFFSET);
    }
    b->x = mouse.x;
    b->width = TOOLTIP_WIDTH;

    b->visible = 1;

    // Only needed for Xlib to unblock XNextEvent
    force_redraw();
}

// This is being called everytime the mouse is moving above a button
void tooltip_new(MAYBE_I18NAL_STRING* text)
{
    TOOLTIP *b = &tooltip;

    b->can_show = 1;
    b->tt_text = text;

    if(b->timer_running || b->visible || b->mouse_down) {
        return;
    }

    b->timer_running = 1;
    thread(mouse_pos_check,NULL);
}

void mouse_pos_check(void *UNUSED(args))
{
    TOOLTIP *b = &tooltip;
    int old_x = 0, old_y = 0, tick = 0;

    while(1) {
        if(mouse.x - old_x == 0 && mouse.y - old_y == 0) {
            if(tick >= 5) {
                // ~500ms of not moving, show tooltip
                if(b->can_show && !b->mouse_down) {
                    tooltip_show();
                }
                break;
            } else {
                tick++;
            }
            //debug("Mouse stopped. X: %d, Y: %d, OX: %d, OY: %d\n",mouse.x,mouse.y,old_x,old_y);
        } else {
            old_x = mouse.x;
            old_y = mouse.y;
            tick = 0;
        }
        yieldcpu(100);
    }
    tick = 0;
    b->timer_running = 0;
}
