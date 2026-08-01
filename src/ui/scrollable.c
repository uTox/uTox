#include "scrollable.h"

#include "draw.h"
#include "svg.h"

#include "../macros.h"
#include "../ui.h"

static uint32_t scroll_thumb_height(uint32_t content_height, uint32_t viewport_height) {
    if (content_height <= viewport_height) {
        return viewport_height;
    }

    uint32_t m = (viewport_height * viewport_height) / content_height;
    if (m < SCROLL_THUMB_MIN_HEIGHT) {
        m = SCROLL_THUMB_MIN_HEIGHT;
    }
    if (m > viewport_height) {
        m = viewport_height;
    }
    return m;
}

static void scroll_remember_viewport(SCROLLABLE *s, int height) {
    if (height > 0) {
        s->viewport_height = height;
    }
}

/* Prefer the content viewport (from scroll_gety) over the scrollbar chrome
 * panel height — they can differ slightly and would skew wheel / drag math. */
static int scroll_content_viewport(SCROLLABLE *s, int fallback_height) {
    if (s->viewport_height > 0) {
        return s->viewport_height;
    }
    return fallback_height;
}

void scroll_draw(SCROLLABLE *s, int x, int y, int width, int height) {
    uint32_t c            = s->content_height;
    uint32_t h            = (uint32_t)scroll_content_viewport(s, height), m, dy;
    uint32_t scroll_width = 0;
    if (s->small) {
        scroll_width = SCROLL_WIDTH / 2;
    } else {
        scroll_width = SCROLL_WIDTH;
    }

    if (h >= c) {
        // If h(eight) > c(ontent height), don't draw anything.
        return;
    } else {
        m        = scroll_thumb_height(c, h);
        double d = (h - m);
        dy       = (s->d * d) + 0.5;
    }

    y += dy;
    x += s->x;

    if (!s->left) {
        x += width - scroll_width;
    }

    drawalpha(s->small ? BM_SCROLLHALFTOP_SMALL : BM_SCROLLHALFTOP, x, y, scroll_width, scroll_width / 2, s->color);

    y += scroll_width / 2;
    int y2 = y + m - scroll_width;
    if (scroll_width > m) {
        y2 = y;
    }
    drawrect(x, y, x + scroll_width, y2, s->color);

    drawalpha(s->small ? BM_SCROLLHALFBOT_SMALL : BM_SCROLLHALFBOT, x, y2, scroll_width, scroll_width / 2, s->color);
}

int scroll_gety(SCROLLABLE *s, int height) {
    scroll_remember_viewport(s, height);

    int c = s->content_height;

    if (c > height) {
        return (s->d * (double)(c - height)) + 0.5;
    }

    return 0;
}

bool scroll_mmove(SCROLLABLE *s, int UNUSED(px), int UNUSED(py), int width, int height, int x, int y, int UNUSED(dx),
                  int dy) {
    bool draw = false;

    bool hit = inrect(x, y, s->left ? 0 : (width - SCROLL_WIDTH), 0, SCROLL_WIDTH, height);
    if (s->mouseover != hit) {
        s->mouseover = hit;
        draw         = true;
    }

    s->mouseover2 = inrect(x, y, 0, 0, width, height);

    if (s->mousedown) {
        uint32_t c = s->content_height;
        uint32_t h = (uint32_t)scroll_content_viewport(s, height);

        if (c > h) {
            uint32_t m = scroll_thumb_height(c, h);
            double   d = (h - m);

            if (d > 0.0) {
                s->d = ((s->d * d) + (double)dy) / d;

                if (s->d < 0.0) {
                    s->d = 0.0;
                } else if (s->d >= 1.0) {
                    s->d = 1.0;
                }

                draw = true;
            }
        }
    }

    return draw;
}

bool scroll_mdown(SCROLLABLE *s) {
    if (s->mouseover) {
        s->mousedown = 1;
        return true;
    }

    return false;
}

bool scroll_mright(SCROLLABLE *UNUSED(s)) { return false; }

static int scroll_line_step(void) {
    int line = font_small_lineheight;
    const int design = SCALE(12);
    if (line < design) {
        line = design;
    }
    /* messages_draw advances by text height plus MESSAGES_SPACING between rows. */
    line += MESSAGES_SPACING;
    if (line < 1) {
        line = 1;
    }
    return line;
}

bool scroll_mwheel(SCROLLABLE *s, int height, double delta, bool UNUSED(smooth)) {
    if (!s->mouseover2) {
        return false;
    }

    const int port_height    = scroll_content_viewport(s, height);
    const int content_height = s->content_height;
    const int scroll_range   = content_height - port_height;
    if (scroll_range <= 0) {
        return false;
    }

    const int line = scroll_line_step();

    /* Keep a fractional remainder so high-res wheel events still add up to
     * whole lines instead of truncating short every notch. */
    double move = delta * (double)line + s->wheel_accum;
    int    step = (int)move;
    s->wheel_accum = move - (double)step;
    if (step == 0) {
        return false;
    }

    int y = (int)(s->d * (double)scroll_range + 0.5);
    y -= step;
    if (y < 0) {
        y = 0;
    } else if (y > scroll_range) {
        y = scroll_range;
    }

    s->d = (double)y / (double)scroll_range;
    return true;
}

bool scroll_mup(SCROLLABLE *s) {
    if (s->mousedown) {
        s->mousedown = 0;
        return true;
    }

    return false;
}

bool scroll_mleave(SCROLLABLE *s) {
    if (s->mouseover) {
        s->mouseover = 0;
        return true;
    }

    s->mouseover2 = 0;

    return false;
}
