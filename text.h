
int drawtextmultiline(int x, int right, int y, int top, int bottom, uint16_t lineheight, char_t *data, STRING_IDX length, STRING_IDX h, STRING_IDX hlen, _Bool multiline);
STRING_IDX hittextmultiline(int mx, int right, int my, int height, uint16_t lineheight, char_t *str, STRING_IDX length, _Bool multiline);
int text_height(int right, uint16_t lineheight, char_t *str, STRING_IDX length);

STRING_IDX text_lineup(int width, int height, STRING_IDX p, uint16_t lineheight, char_t *str, STRING_IDX length, SCROLLABLE *scroll);
STRING_IDX text_linedown(int width, int height, STRING_IDX p, uint16_t lineheight, char_t *str, STRING_IDX length, SCROLLABLE *scroll);
