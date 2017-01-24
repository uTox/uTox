#include "tooltip.h"

#include "draw.h"

#include "../macros.h"
#include "../main_native.h"
#include "../settings.h"
#include "../theme.h"
#include "../ui.h"
#include "../utox.h"

#include "../main.h" // mouse, thread

static TOOLTIP tooltip;

#define TOOLTIP_WIDTH SCALE(24)
#define TOOLTIP_HEIGHT SCALE(24)
#define TOOLTIP_YOFFSET 12

static void calculate_pos_and_width(TOOLTIP *b, int *x, int *w) {
    *x = b->x;
    *w = b->width;

    // Increase width if needed, so that tooltip text fits.
    if (maybe_i18nal_string_is_valid(b->tt_text)) {
        const STRING *s    = maybe_i18nal_string_get(b->tt_text);
        const int needed_w = textwidth(s->str, s->length) + SCALE(8);
        if (*w < needed_w) {
            *w = needed_w;
        }
    }

    // Push away from the right border to fit.
    if (*x + *w >= (int)settings.window_width) {
        *x -= *w;
    }
}

volatile bool kill_thread;

void tooltip_reset(void) {
    TOOLTIP *b = &tooltip;

    b->visible  = false;
    b->can_show = false;

    if (b->thread) {
        kill_thread = true;
        b->thread   = false;
    }
}

void tooltip_draw(void) {
    TOOLTIP *b = &tooltip;
    if (!b->visible) {
        return;
    }

    // Ensure that font is set before calculating position and width.
    setfont(FONT_TEXT);
    setcolor(COLOR_MAIN_TEXT);

    int x, w;
    calculate_pos_and_width(b, &x, &w);

    draw_rect_fill(x, b->y, w, b->height, COLOR_BKGRND_MAIN);

    STRING *s = maybe_i18nal_string_get(b->tt_text);
    drawtext(x + SCALE(4), b->y + SCALE(4), s->str, s->length);

    draw_rect_frame(x, b->y, w, b->height, COLOR_EDGE_NORMAL);
}

bool tooltip_mmove(void) {
    TOOLTIP *b = &tooltip;

    b->can_show = false;

    if (!b->visible) {
        return false;
    }

    b->visible = false;

    if (b->thread) {
        kill_thread = true;
        b->thread   = false;
    }

    return true;
}

bool tooltip_mdown(void) {
    TOOLTIP *b = &tooltip;

    b->can_show   = false;
    b->mouse_down = true;
    b->visible    = false;

    if (b->thread) {
        kill_thread = true;
        b->thread   = false;
    }

    return false;
}

bool tooltip_mup(void) {
    TOOLTIP *b = &tooltip;

    b->can_show   = false;
    b->mouse_down = false;

    if (b->thread) {
        kill_thread = true;
        b->thread   = false;
    }

    return false;
}

void tooltip_show(void) {

    TOOLTIP *b = &tooltip;

    if (!b->can_show) {
        return;
    }

    b->y      = mouse.y + TOOLTIP_YOFFSET;
    b->height = TOOLTIP_HEIGHT;
    if (b->y + b->height >= (int)settings.window_height) {
        b->y -= (b->height + TOOLTIP_YOFFSET);
    }
    b->x     = mouse.x;
    b->width = TOOLTIP_WIDTH;

    b->visible = true;

    if (b->thread) {
        kill_thread = true;
        b->thread   = false;
    }
}

volatile bool reset_time;

static void tooltip_thread(void *UNUSED(args)) {
    uint64_t last_move_time = ~0;
    while (1) {
        if (kill_thread) {
            break;
        }

        if (reset_time) {
            last_move_time = get_time() + 500 * 1000 * 1000;
            reset_time     = 0;
        }

        if (get_time() > last_move_time) {
            postmessage_utox(TOOLTIP_SHOW, 0, 0, NULL);
            last_move_time = ~0;
        }

        yieldcpu(100);
    }

    kill_thread = false;
}

// This is being called every time the mouse is moving above a button
void tooltip_new(MAYBE_I18NAL_STRING *text) {
    TOOLTIP *b = &tooltip;

    b->can_show = true;
    b->tt_text  = text;

    if (b->visible || b->mouse_down) {
        return;
    }

    if (!b->thread && !kill_thread) {
        thread(tooltip_thread, NULL);
        b->thread = true;
    }

    reset_time = 1;
}
