#include "main.h"

static void drawtexth(int x, int y, char_t *str, uint16_t length, int d, int h, int hlen, uint16_t lineheight)
{
    h -= d;
    if(h + hlen < 0 || h > length) {
        drawtext(x, y, str, length);
        return;
    } else if(hlen == 0) {
        drawtext(x, y, str, length);
        int w =  textwidth(str, h + hlen);
        drawvline(x + w, y, y + lineheight, BLACK);
        return;
    }

    if(h < 0) {
        hlen += h;
        h = 0;
        if(hlen < 0) {
            hlen = 0;
        }
    }

    if(h + hlen > length) {
        hlen = length - h;
    }

    int width;

    width = drawtext_getwidth(x, y, str, h);

    uint32_t color = setcolor(TEXT_HIGHLIGHT);

    int w = textwidth(str + h, hlen);
    drawrectw(x + width, y, w, lineheight, TEXT_HIGHLIGHT_BG);
    drawtext(x + width, y, str + h, hlen);
    width += w;

    setcolor(color);

    drawtext(x + width, y, str + h + hlen, length - (h + hlen));
}

int drawtextmultiline(int x, int right, int y, uint16_t lineheight, char_t *data, uint16_t length, uint16_t h, uint16_t hlen, _Bool multiline)
{
    int xc = x;
    char_t *a = data, *b = a, *end = a + length;
    while(1) {
        if(a == end || *a == ' ' || *a == '\n') {
            int count = a - b, w = textwidth(b, count);
            if(x + w > right) {
                if(x != xc || *a == '\n') {
                    y += lineheight;
                    if(!multiline) {
                        return y;
                    }
                    int l = utf8_len(b);
                    count -= l;
                    b += l;
                }
                x = xc;
                w = textwidth(b, count);
            }

            drawtexth(x, y, b, count, b - data, h, hlen, lineheight);

            x += w;
            b = a;

            if(a == end) {
                break;
            }

            if(*a == '\n') {
                y += lineheight;
                if(!multiline) {
                    return y;
                }
                b += utf8_len(b);
                x = xc;
            }

        }
        a += utf8_len(a);
    }

    return y + lineheight;
}

uint16_t hittextmultiline(int mx, int right, int my, int height, uint16_t lineheight, char_t *str, uint16_t length, _Bool multiline)
{
    if(mx < 0 && my >= height) {
        return length;
    }

    int x = 0;
    char_t *a = str, *b = str, *end = str + length;
    while(1) {
        if(a == end ||  *a == '\n' || *a == ' ') {
            int count = a - b, w = textwidth(b, a - b);
            if(x + w > right) {
                if(x != 0 || *a == '\n') {
                    if(!multiline) {
                        return 0;
                    }
                    my -= lineheight;
                    height -= lineheight;
                    int l = utf8_len(b);
                    count -= l;
                    b += l;
                }

                if(my >= -lineheight && my <= 0) {
                    x = mx;
                    break;
                }

                x = 0;
                w = textwidth(b, count);
            }

            if(a == end || (mx < 0 && my >= 0 && my <= lineheight) || (mx >= x && mx < x + w && my >= 0 && (my < lineheight || height == lineheight))) {
                break;
            }
            x += w;
            b = a;

            if(*a == '\n') {
                b += utf8_len(b);
                if(my >= 0 && my < lineheight) {
                    x = mx;
                    break;
                }
                if(!multiline) {
                    return 0;
                }
                my -= lineheight;
                height -= lineheight;
                x = 0;
            }
        }
        a += utf8_len(a);
    }

    int fit;
    if(mx - x > 0) {
        int len = a - b;
        fit = textfit(b, len, mx - x);
    } else {
        fit = 0;
    }

    return (b - str) + fit;
}
