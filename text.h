
int drawtextmultiline(int x, int right, int y, uint16_t lineheight, char_t *data, uint16_t length, uint16_t h, uint16_t hlen, _Bool multiline);
uint16_t hittextmultiline(int mx, int right, int my, int height, uint16_t lineheight, char_t *str, uint16_t length, _Bool multiline);

uint16_t text_lineup(int width, uint16_t p, uint16_t lineheight, char_t *str, uint16_t length);
uint16_t text_linedown(int width, uint16_t p, uint16_t lineheight, char_t *str, uint16_t length);
