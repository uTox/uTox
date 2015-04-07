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

volatile _Bool kill_thread;

void tooltip_reset(void)
{
    TOOLTIP *b = &tooltip;

    b->visible = 0;
    b->can_show = 0;

    if (b->thread) {
        kill_thread = 1;
        b->thread = 0;
    }
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

    if (b->thread) {
        kill_thread = 1;
        b->thread = 0;
    }

    return 1;
}

_Bool tooltip_mdown(void)
{
    TOOLTIP *b = &tooltip;

    b->can_show = 0;
    b->mouse_down = 1;
    b->visible = 0;

    if (b->thread) {
        kill_thread = 1;
        b->thread = 0;
    }

    return 0;
}

_Bool tooltip_mup(void)
{
    TOOLTIP *b = &tooltip;

    b->can_show = 0;
    b->mouse_down = 0;

    if (b->thread) {
        kill_thread = 1;
        b->thread = 0;
    }

    return 0;
}

void tooltip_show(void)
{

    TOOLTIP *b = &tooltip;

    if (!b->can_show)
        return;

    b->y = mouse.y + TOOLTIP_YOFFSET;
    b->height = TOOLTIP_HEIGHT;
    if(b->y + b->height >= utox_window_height) {
        b->y -= (b->height + TOOLTIP_YOFFSET);
    }
    b->x = mouse.x;
    b->width = TOOLTIP_WIDTH;

    b->visible = 1;

    if (b->thread) {
        kill_thread = 1;
        b->thread = 0;
    }

}

volatile _Bool reset_time;

static void tooltip_thread(void *UNUSED(args))
{
    uint64_t last_move_time = ~0;
    while (1) {
        if (kill_thread) {
            break;
        }

        TOOLTIP *b = &tooltip;
        if (reset_time) {
            last_move_time = get_time() + 500 * 1000 * 1000;
            reset_time = 0;
        }

        if (get_time() > last_move_time) {
            postmessage(TOOLTIP_SHOW, 0, 0, NULL);
            last_move_time = ~0;
        }

        yieldcpu(100);
    }

    kill_thread = 0;
}

// This is being called everytime the mouse is moving above a button
void tooltip_new(MAYBE_I18NAL_STRING* text)
{
    TOOLTIP *b = &tooltip;

    b->can_show = 1;
    b->tt_text = text;

    if(b->visible || b->mouse_down) {
        return;
    }

    if (!b->thread && !kill_thread) {
        thread(tooltip_thread, NULL);
        b->thread = 1;
    }

    reset_time = 1;
}


