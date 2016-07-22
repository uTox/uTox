#include "../main.h"

static void calculate_pos_and_width(UISWITCH *s, int *x, int *w) {
    int old_w = *w;

    // Push away from the right border to fit,
    // if our panel is right-adjusted.
    if (s->panel.x < 0) {
        *x -= *w - old_w;
    }
}

void switch_draw(UISWITCH *s, int x, int y, int w, int h) {
    // Switch is hidden
    if (s->nodraw) {
        return;
    }

    // If `update` function is defined, call it on each draw
    if (s->update) {
        s->update(s);
    }

    // Switch background color
    uint32_t color = s->mousedown ? s->press_color : (s->mouseover ? s->hover_color : s->bg_color);
    draw_rect_fill(x, y, w, h, s->disabled ? (s->disabled_color ? s->disabled_color : s->disabled_color) : color);

    if (s->style) {
        if (s->switch_on) {
            drawalpha(s->style, x + (w / 2), y, w / 2, h, s->sw_color);
        } else {
            drawalpha(s->style, x,           y, w / 2, h, s->sw_color);
        }
    }
}

_Bool switch_mmove(UISWITCH *s, int UNUSED(x), int UNUSED(y), int width, int height, int mx, int my,
                                                                                int UNUSED(dx), int UNUSED(dy))
{
    // Ensure that font is set before calculating position and width.
    setfont(FONT_SELF_NAME);

    int real_x = 0, real_w = width;
    calculate_pos_and_width(s, &real_x, &real_w);

    _Bool mouseover = inrect(mx, my, real_x, 0, real_w, height);
    if (mouseover) {
        if(!s->disabled) {
            cursor = CURSOR_HAND;
        }

        if (maybe_i18nal_string_is_valid(&s->tooltip_text)) {
            tooltip_new(&s->tooltip_text);
        }

    }
    if (mouseover != s->mouseover) {
        s->mouseover = mouseover;
        return 1;
    }

    return 0;
}

_Bool switch_mdown(UISWITCH *s) {
    if (!s->mousedown && s->mouseover) {
        s->mousedown = 1;
        return 1;
    }

    return 0;
}

_Bool switch_mright(UISWITCH *s) {
    if (s->mouseover && s->onright) {
        s->onright();
        return 1;
    }
    return 0;
}

_Bool switch_mwheel(UISWITCH *UNUSED(s), int UNUSED(height), double UNUSED(d), _Bool UNUSED(smooth)) { return 0; }

_Bool switch_mup(UISWITCH *s) {
    if (s->mousedown) {
        if (s->mouseover) {
            s->switch_on = !s->switch_on;
            s->onpress();
        }
        s->mousedown = 0;
        return 1;
    }
    return 0;
}

_Bool switch_mleave(UISWITCH *s) {
    if (s->mouseover) {
        s->mouseover = 0;
        return 1;
    }
    return 0;
}
