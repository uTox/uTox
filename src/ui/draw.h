#ifndef DRAW_H
#define DRAW_H

#include <stdint.h>

int font_small_lineheight, font_msg_lineheight;

void drawtext(int x, int y, const char *str, uint16_t length);
int drawtext_getwidth(int x, int y, const char *str, uint16_t length);
void drawtextwidth(int x, int width, int y, const char *str, uint16_t length);
void drawtextwidth_right(int x, int width, int y, const char *str, uint16_t length);
void drawtextrange(int x, int x2, int y, const char *str, uint16_t length);
void drawtextrangecut(int x, int x2, int y, const char *str, uint16_t length);

int textwidth(const char *str, uint16_t length);
int textfit(const char *str, uint16_t length, int width);
int textfit_near(const char *str, uint16_t length, int width);

void drawrect(int x, int y, int right, int bottom, uint32_t color);
void draw_rect_frame(int x, int y, int width, int height, uint32_t color);
void draw_rect_fill(int x, int y, int width, int height, uint32_t color);

void drawhline(int x, int y, int x2, uint32_t color);
void drawvline(int x, int y, int y2, uint32_t color);
#define drawpixel(x, y, color) drawvline(x, y, (y) + 1, color)

void setfont(int id);
uint32_t setcolor(uint32_t color);
void pushclip(int x, int y, int width, int height);
void popclip(void);
void enddraw(int x, int y, int width, int height);

void drawalpha(int bm, int x, int y, int width, int height, uint32_t color);
void loadalpha(int bm, void *data, int width, int height);

#endif
