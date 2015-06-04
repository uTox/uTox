#include "main.h"

static void calculate_pos_and_width(BUTTON *b, int *x, int *w) {
    int old_w = *w;

    // Increase width if needed, so that button text fits.
    if(maybe_i18nal_string_is_valid(&b->button_text)) {
        STRING* s = maybe_i18nal_string_get(&b->button_text);
        int needed_w = textwidth(s->str, s->length) + 6 * SCALE;
        if(*w < needed_w) {
            *w = needed_w;
        }
    }

    // Push away from the right border to fit,
    // if our panel is right-adjusted.
    if(b->panel.x < 0) {
        *x -= *w - old_w;
    }
}

void button_draw(BUTTON *b, int x, int y, int width, int height)
{
    // Button is hidden
    if (b->nodraw)
        return;

    // If `update` function is defined, call it on each draw
    if (b->update) {
        b->update(b);
    }

    // Ensure that font is set before calculating position and width.
    setfont(FONT_SELF_NAME);

    // Button contents colour
    uint32_t color_text = b->mousedown ? b->ct2 : (b->mouseover ? b->ct2 : b->ct1);
    setcolor(color_text);

    int w = width;
    calculate_pos_and_width(b, &x, &w);

    // Button background colour
    uint32_t color_background = b->mousedown ? b->c3 : (b->mouseover ? b->c2 : b->c1);

    if(b->bm) {
        drawalpha(b->bm, x, y, width, height, color_background);
    } else {
        drawrectw(x, y, w, height, b->disabled ? (b->cd ? b->cd : b->cd) : color_background);

        //setfont(FONT_TEXT_LARGE);
        //setcolor(b->mouseover ? 0x222222 : 0x555555);
        //drawtext(x + 5, y, b->text, b->text_length);
    }

    if(b->bm2) {
        int bx = w / 2 - b->bw * SCALE / 2, by = height / 2 - b->bh * SCALE / 2;
        drawalpha(b->bm2, x + bx, y + by, b->bw * SCALE, b->bh * SCALE, color_text);
    }

    if(maybe_i18nal_string_is_valid(&b->button_text)) {
        if(b->bm) {
            while(w > width) {
                // The text didn't fit into the original width.
                // Fill the rest of the new width with the image
                // and hope for the best.
                drawalpha(b->bm, x - width + w, y, width, height, color_background);
                w -= width / 2 + 1;
            }
        }
        STRING* s = maybe_i18nal_string_get(&b->button_text);
        drawtext(x + 3 * SCALE, y + SCALE, s->str, s->length);
    }
}

_Bool button_mmove(BUTTON *b, int UNUSED(x), int UNUSED(y), int width, int height, int mx, int my, int UNUSED(dx), int UNUSED(dy))
{
    // Ensure that font is set before calculating position and width.
    setfont(FONT_SELF_NAME);

    int real_x = 0, real_w = width;
    calculate_pos_and_width(b, &real_x, &real_w);

    _Bool mouseover = inrect(mx, my, real_x, 0, real_w, height);
    if(mouseover) {
        if(!b->disabled) {
            cursor = CURSOR_HAND;
        }

        if(maybe_i18nal_string_is_valid(&b->tooltip_text)) {
            tooltip_new(&b->tooltip_text);
        }

    }
    if(mouseover != b->mouseover) {
        b->mouseover = mouseover;
        return 1;
    }

    return 0;
}

_Bool button_mdown(BUTTON *b)
{
    if(!b->mousedown && b->mouseover) {
        b->mousedown = 1;
        return 1;
    }

    return 0;
}

_Bool button_mright(BUTTON *b)
{
    if(b->mouseover && b->onright) {
        b->onright();
        return 1;
    }

    return 0;
}

_Bool button_mwheel(BUTTON *UNUSED(b), int UNUSED(height), double UNUSED(d))
{
    return 0;
}

_Bool button_mup(BUTTON *b)
{
    if(b->mousedown) {
        if(b->mouseover) {
            b->onpress();
        }

        b->mousedown = 0;
        return 1;
    }

    return 0;
}

_Bool button_mleave(BUTTON *b)
{
    if(b->mouseover) {
        b->mouseover = 0;
        return 1;
    }

    return 0;
}
