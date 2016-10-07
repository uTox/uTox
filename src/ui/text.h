#ifndef TEXT_H
#define TEXT_H

/* Here for legacy reasons, please use utox_draw_text_multiline_within_box() */
int      utox_draw_text_multiline_compat(int x, int right, int y, int top, int bottom, uint16_t lineheight, char_t *data, uint16_t length, uint16_t h, uint16_t hlen, uint16_t mark, uint16_t marklen, _Bool multiline);


/** Used to draw text within a specified box, starting with the x, y, of the first line of the text.
    Followed by right, top, then bottom borders of the box we're allowed to draw within.
    If any line would be drawn OUTSIDE of the box, it is skipped. */
int      utox_draw_text_multiline_within_box(int x, int y,
                                             int right, int top, int bottom,
                                             uint16_t lineheight,
                                             const char_t *data, uint16_t length,
                                             uint16_t h, uint16_t hlen,
                                             uint16_t mark, uint16_t marklen,
                                             _Bool multiline);

uint16_t hittextmultiline(int mx, int right, int my, int height, uint16_t lineheight, char_t *str, uint16_t length, _Bool multiline);
int      text_height(int right, uint16_t lineheight, char_t *str, uint16_t length);

uint16_t text_lineup(int width, int height, uint16_t p, uint16_t lineheight, char_t *str, uint16_t length, SCROLLABLE *scroll);
uint16_t text_linedown(int width, int height, uint16_t p, uint16_t lineheight, char_t *str, uint16_t length, SCROLLABLE *scroll);

#endif
