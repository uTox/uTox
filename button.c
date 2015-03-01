#include "main.h"

extern int COLOUR_MAIN_BACKGROUND;
extern int COLOUR_MAIN_FOREGROUND;
extern int COLOUR_LIST_BACKGROUND;

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
    if(b->nodraw) {
        return;
    }

    if(b->updatecolor) {
        b->updatecolor(b);
    }

    // Ensure that font is set before calculating position and width.
    setfont(FONT_SELF_NAME);
    setcolor(COLOUR_MAIN_FOREGROUND);

    int w = width;
    calculate_pos_and_width(b, &x, &w);

    uint32_t color = b->mousedown ? b->c3 : (b->mouseover ? b->c2 : b->c1);
    if(b->bm) {
        drawalpha(b->bm, x, y, width, height, color);
    } else {
        drawrectw(x, y, w, height, b->disabled ? (b->cd ? b->cd : 0xffff00) : color);

        //setfont(FONT_TEXT_LARGE);
        //setcolor(b->mouseover ? 0x222222 : 0x555555);
        //drawtext(x + 5, y, b->text, b->text_length);
    }

    if(b->bm2) {
        int bx = w / 2 - b->bw * SCALE / 2, by = height / 2 - b->bh * SCALE / 2;
        drawalpha(b->bm2, x + bx, y + by, b->bw * SCALE, b->bh * SCALE, b->ic ? b->ic : COLOUR_MAIN_BACKGROUND);
    }

    if(maybe_i18nal_string_is_valid(&b->button_text)) {
        if(b->bm) {
            while(w > width) {
                // The text didn't fit into the original width.
                // Fill the rest of the new width with the image
                // and hope for the best.
                drawalpha(b->bm, x - width + w, y, width, height, color);
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
        cursor = CURSOR_HAND;
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
