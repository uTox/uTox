#include "main.h"

static void calculate_pos_and_width(BUTTON *b, int *x, int *w) {
    int old_w = *w;

    // Increase width if needed, so that button text fits.
    if(maybe_i18nal_string_is_valid(&b->button_text)) {
        STRING* s = maybe_i18nal_string_get(&b->button_text);
        int needed_w = textwidth_common(0, s->str, s->length) + 6 * SCALE;
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
    setfont_common(0, FONT_SELF_NAME);
    setcolor_common(0, WHITE);

    int w = width;
    calculate_pos_and_width(b, &x, &w);

    uint32_t color = b->mousedown ? b->c3 : (b->mouseover ? b->c2 : b->c1);
    if(b->bm) {
        drawalpha_common(0, b->bm, x, y, width, height, color);
    } else {
        drawrectw_common(0, x, y, w, height, b->disabled ? LIST_MAIN : color);

        //setfont_common(0, FONT_TEXT_LARGE);
        //setcolor_common(0, b->mouseover ? 0x222222 : 0x555555);
        //drawtext_common(0, x + 5, y, b->text, b->text_length);
    }

    if(b->bm2) {
        int bx = w / 2 - b->bw * SCALE / 2, by = height / 2 - b->bh * SCALE / 2;
        drawalpha_common(0, b->bm2, x + bx, y + by, b->bw * SCALE, b->bh * SCALE, WHITE);
    }

    if(maybe_i18nal_string_is_valid(&b->button_text)) {
        if(b->bm) {
            while(w > width) {
                // The text didn't fit into the original width.
                // Fill the rest of the new width with the image
                // and hope for the best.
                drawalpha_common(0, b->bm, x - width + w, y, width, height, color);
                w -= width / 2 + 1;
            }
        }
        STRING* s = maybe_i18nal_string_get(&b->button_text);
        drawtext_common(0, x + 3 * SCALE, y + SCALE, s->str, s->length);
    }
}

/** button_draw_common
 *
*  takes: button, draw-handle target, xy cords, h,w.
*
* and draws it all pretty like
*
* replaces button_draw
*/
void button_draw_common(BUTTON *b, int target, int x, int y, int width, int height){
    if(b->nodraw) {
        return;
    }

    if(b->updatecolor) {
        b->updatecolor(b);
    }

    // Ensure that font is set before calculating position and width.
    setfont_common(target, FONT_SELF_NAME);
    setcolor_common(target, WHITE);

    int w = width;
    calculate_pos_and_width(b, &x, &w);

    //change color by mouse location
    uint32_t color = b->mousedown ? b->c3 : (b->mouseover ? b->c2 : b->c1);
    if(b->bm) {
        drawalpha_common(target, b->bm, x, y, width, height, color);
    } else {
        drawrectw_common(target, x, y, w, height, b->disabled ? LIST_MAIN : color);

        //setfont_common(0, FONT_TEXT_LARGE);
        //setcolor_common(0, b->mouseover ? 0x222222 : 0x555555);
        //drawtext_common(0, x + 5, y, b->text, b->text_length);
    }

    if(b->bm2) {
        int bx = w / 2 - b->bw * SCALE / 2, by = height / 2 - b->bh * SCALE / 2;
        drawalpha_common(target, b->bm2, x + bx, y + by, b->bw * SCALE, b->bh * SCALE, WHITE);
    }

    // If needed, write text over the button
    if(maybe_i18nal_string_is_valid(&b->button_text)) {
        if(b->bm) {
            while(w > width) {
                // The text didn't fit into the original width.
                // Fill the rest of the new width with the image
                // and hope for the best.
                drawalpha_common(target, b->bm, x - width + w, y, width, height, color);
                w -= width / 2 + 1;
            }
        }
        STRING* s = maybe_i18nal_string_get(&b->button_text);
        drawtext_common(target, x + 3 * SCALE, y + SCALE, s->str, s->length);
    }
}

_Bool button_mmove(BUTTON *b, int UNUSED(x), int UNUSED(y), int width, int height, int mx, int my, int UNUSED(dx), int UNUSED(dy))
{
    // Ensure that font is set before calculating position and width.
    setfont_common(0, FONT_SELF_NAME);

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
