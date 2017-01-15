// switch.c

#include "switch.h"

#include "tooltip.h"

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
    drawalpha(s->style_outer, x, y, w, h,
              s->disabled ? (s->disabled_color ? s->disabled_color : s->disabled_color) : color);

    // SVG offsets, used for centering
    int tx = ((w / 2 - s->toggle_w) / 2), ty = ((h - s->toggle_h) / 2), ix0 = ((w / 2 - s->icon_off_w) / 2),
        iy0 = ((h - s->icon_off_h) / 2), ix1 = ((w / 2 - s->icon_on_w) / 2), iy1 = ((h - s->icon_on_h) / 2);

    if (s->style_toggle) {
        if (s->switch_on) {
            drawalpha(s->style_toggle, x + (w / 2) + tx, y + ty, s->toggle_w, s->toggle_h, s->sw_color);
        } else {
            drawalpha(s->style_toggle, x + tx, y + ty, s->toggle_w, s->toggle_h, s->sw_color);
        }
    }

    if (s->style_icon_off && !s->switch_on) {
        drawalpha(s->style_icon_off, x + (w / 2) + ix0, y + iy0, s->icon_off_w, s->icon_off_h, s->sw_color);
    } else if (s->style_icon_on && s->switch_on) {
        drawalpha(s->style_icon_on, x + ix1, y + iy1, s->icon_on_w, s->icon_on_h, s->sw_color);
    }
}

bool switch_mmove(UISWITCH *s, int UNUSED(x), int UNUSED(y), int width, int height, int mx, int my, int UNUSED(dx),
                  int UNUSED(dy)) {
    // Ensure that font is set before calculating position and width.
    setfont(FONT_SELF_NAME);

    int real_x = 0, real_w = width;
    calculate_pos_and_width(s, &real_x, &real_w);

    bool mouseover = inrect(mx, my, real_x, 0, real_w, height);
    if (mouseover) {
        if (!s->disabled) {
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

bool switch_mdown(UISWITCH *s) {
    if (!s->mousedown && s->mouseover) {
        s->mousedown = 1;
        return 1;
    }

    return 0;
}

bool switch_mright(UISWITCH *s) {
    if (s->mouseover && s->onright) {
        s->onright();
        return 1;
    }
    return 0;
}

bool switch_mwheel(UISWITCH *UNUSED(s), int UNUSED(height), double UNUSED(d), bool UNUSED(smooth)) {
    return 0;
}

bool switch_mup(UISWITCH *s) {
    // ignore click when switch is disabled
    if (s->mousedown && !s->disabled) {
        if (s->mouseover) {
            s->switch_on = !s->switch_on;
            s->on_mup();
        }
        s->mousedown = 0;
        return 1;
    }
    s->mousedown = 0;
    return 0;
}

bool switch_mleave(UISWITCH *s) {
    if (s->mouseover) {
        s->mouseover = 0;
        return 1;
    }
    return 0;
}
