
#define drawtext(x, y, str, len) TextOut(hdc, x, y, (char*)(str), len)
#define drawstr(x, y, str) TextOut(hdc, x, y, str, sizeof(str) - 1)

//void draw_text(int x, int y, uint8_t *str, int len);


