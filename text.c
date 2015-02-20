#include "main.h"

static void drawtexth(int x, int y, char_t *str, STRING_IDX length, int d, int h, int hlen, uint16_t lineheight)
{
    h -= d;
    if(h + hlen < 0 || h > length) {
        drawtext_common(0, x, y, str, length);
        return;
    } else if(hlen == 0) {
        drawtext_common(0, x, y, str, length);
        int w =  textwidth_common(0, str, h + hlen);
        drawvline_common(0, x + w, y, y + lineheight, BLACK);
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

    width = drawtext_getwidth_common(0, x, y, str, h);

    uint32_t color = setcolor_common(0, TEXT_HIGHLIGHT);

    int w = textwidth_common(0, str + h, hlen);
    drawrectw_common(0, x + width, y, w, lineheight, TEXT_HIGHLIGHT_BG);
    drawtext_common(0, x + width, y, str + h, hlen);
    width += w;

    setcolor_common(0, color);

    drawtext_common(0, x + width, y, str + h + hlen, length - (h + hlen));
}

int drawtextmultiline(int x, int right, int y, int top, int bottom, uint16_t lineheight, char_t *data, STRING_IDX length, STRING_IDX h, STRING_IDX hlen, _Bool multiline)
{
    uint32_t c1, c2;
    _Bool greentext = 0, link = 0, draw = y + lineheight >= top;
    int xc = x;
    char_t *a = data, *b = a, *end = a + length;
    while(1) {
        if(a != end) {
            if(*a == '>' && (a == data || *(a - 1) == '\n'))  {
                c1 = setcolor_common(0, RGB(0, 128, 0));
                greentext = 1;
            }

            if((a == data || *(a - 1) == '\n' || *(a - 1) == ' ') && ((end - a >= 7 && memcmp(a, "http://", 7) == 0) || (end - a >= 8 && memcmp(a, "https://", 8) == 0))) {
                c2 = setcolor_common(0, RGB(0, 0, 255));
                link = 1;
            }
        }

        if(a == end || *a == ' ' || *a == '\n') {
            int count = a - b, w = textwidth_common(0, b, count);
            while(x + w > right) {
                if(multiline && x == xc) {
                    int fit = textfit_common(0, b, count, right - x);
                    if(draw) {
                        drawtexth(x, y, b, fit, b - data, h, hlen, lineheight);
                    }
                    count -= fit;
                    b += fit;
                    y += lineheight;
                    draw = (y + lineheight >= top && y < bottom);
                } else if(!multiline) {
                    int fit = textfit_common(0, b, count, right - x);
                    if(draw) {
                        drawtexth(x, y, b, fit, b - data, h, hlen, lineheight);
                    }
                    return y + lineheight;
                } else {
                    y += lineheight;
                    draw = (y + lineheight >= top && y < bottom);
                    int l = utf8_len(b);
                    count -= l;
                    b += l;
                }
                x = xc;
                w = textwidth_common(0, b, count);
            }

            if(draw) {
                drawtexth(x, y, b, count, b - data, h, hlen, lineheight);
            }

            x += w;
            b = a;

            if(link) {
                setcolor_common(0, c2);
                link = 0;
            }

            if(a == end) {
                if(greentext) {
                    setcolor_common(0, c1);
                    greentext = 0;
                }
                break;
            }

            if(*a == '\n') {
                if(greentext) {
                    setcolor_common(0, c1);
                    greentext = 0;
                }
                y += lineheight;
                draw = (y + lineheight >= top && y < bottom);
                b += utf8_len(b);
                x = xc;
            }

        }
        a += utf8_len(a);
    }

    return y + lineheight;
}

STRING_IDX hittextmultiline(int mx, int right, int my, int height, uint16_t lineheight, char_t *str, STRING_IDX length, _Bool multiline)
{
    if(my < 0) {
        return 0;
    }

    if(my >= height) {
        return length;
    }

    int x = 0;
    char_t *a = str, *b = str, *end = str + length;
    while(1) {
        if(a == end ||  *a == '\n' || *a == ' ') {
            int count = a - b, w = textwidth_common(0, b, a - b);
            while(x + w > right && my >= lineheight) {
                if(multiline && x == 0) {
                    int fit = textfit_common(0, b, count, right);
                    count -= fit;
                    b += fit;
                    my -= lineheight;
                    height -= lineheight;
                } else if(!multiline) {
                    break;
                } else {
                    my -= lineheight;
                    height -= lineheight;
                    int l = utf8_len(b);
                    count -= l;
                    b += l;
                }

                if(my >= -lineheight && my < 0) {
                    x = mx;
                    break;
                }

                x = 0;
                w = textwidth_common(0, b, count);
            }

            if(a == end) {
                if(my >= lineheight) {
                    return length;
                }
                break;
            }
            if((my >= 0 && my < lineheight) && (mx < 0 || (mx >= x && mx < x + w))) {
                break;
            }
            x += w;
            b = a;

            if(*a == '\n') {
                if(my >= 0 && my < lineheight) {
                    x = mx;
                    return a - str;
                }
                b += utf8_len(b);
                my -= lineheight;
                height -= lineheight;
                x = 0;
            }
        }
        a += utf8_len(a);
    }

    int fit;
    if(mx >= right) {
        fit = textfit_common(0, b, a - b, right - x);
    } else if(mx - x > 0) {
        int len = a - b;
        fit = textfit_near_common(0, b, len + (a != end), mx - x);
    } else {
        fit = 0;
    }

    return (b - str) + fit;
}

int text_height(int right, uint16_t lineheight, char_t *str, STRING_IDX length)
{
    int x = 0, y = 0;
    char_t *a = str, *b = a, *end = a + length;
    while(1) {
        if(a == end || *a == ' ' || *a == '\n') {
            int count = a - b, w = textwidth_common(0, b, count);
            while(x + w > right) {
                if(x == 0) {
                    int fit = textfit_common(0, b, count, right);
                    count -= fit;
                    if(fit == 0 && (count != 0 || *b == '\n')) {
                        return 0;
                    }
                    b += fit;
                    y += lineheight;
                } else {
                    y += lineheight;
                    int l = utf8_len(b);
                    count -= l;
                    b += l;
                }
                x = 0;
                w = textwidth_common(0, b, count);
            }

            x += w;
            b = a;

            if(a == end) {
                break;
            }

            if(*a == '\n') {
                y += lineheight;
                b += utf8_len(b);
                x = 0;
            }

        }
        a += utf8_len(a);
    }

    y += lineheight;

    return y;
}

static void textxy(int width, STRING_IDX pp, uint16_t lineheight, char_t *str, STRING_IDX length, int *outx, int *outy)
{
    int x = 0, y = 0;
    char_t *a = str, *b = str, *end = str + length, *p = str + pp;
    while(1) {
        if(a == end ||  *a == '\n' || *a == ' ') {
            int count = a - b, w = textwidth_common(0, b, a - b);
            while(x + w > width) {
                if(x == 0) {
                    int fit = textfit_common(0, b, count, width);
                    if(p >= b && p < b + fit) {
                        break;
                    }
                    count -= fit;
                    b += fit;
                    y += lineheight;
                } else {
                    y += lineheight;
                    int l = utf8_len(b);
                    count -= l;
                    b += l;
                }
                x = 0;
                w = textwidth_common(0, b, count);
            }

            if(p >= b && p < b + count) {
                w = textwidth_common(0, b, p - b);
                a = end;
            }

            x += w;
            if(a == end) {
                break;
            }
            b = a;

            if(*a == '\n') {
                if(p == a) {
                    break;
                }
                b += utf8_len(b);
                y += lineheight;
                x = 0;
            }
        }
        a += utf8_len(a);
    }

    *outx = x;
    *outy = y;
}

STRING_IDX text_lineup(int width, int height, STRING_IDX p, uint16_t lineheight, char_t *str, STRING_IDX length, SCROLLABLE *scroll)
{
    //lazy
    int x, y;
    textxy(width, p, lineheight, str, length, &x, &y);
    if(y == 0) {
        scroll->d = 0.0;
        return p;
    }

    y -= lineheight;

    if(scroll->content_height > height) {
        double d1 = (double)y / (double)(scroll->content_height - height);
        double d2 = (double)(y - height + lineheight) / (double)(scroll->content_height - height);
        if(d1 < scroll->d) {
            scroll->d = d1;
        } else if(d2 > scroll->d) {
            scroll->d = d2;
        }
    }

    return hittextmultiline(x, width, y, INT_MAX, lineheight, str, length, 1);
}

STRING_IDX text_linedown(int width, int height, STRING_IDX p, uint16_t lineheight, char_t *str, STRING_IDX length, SCROLLABLE *scroll)
{
    //lazy
    int x, y;
    textxy(width, p, lineheight, str, length, &x, &y);

    y += lineheight;

    if(scroll->content_height > height) {
        double d1 = (double)y / (double)(scroll->content_height - height);
        double d2 = (double)(y - height + lineheight) / (double)(scroll->content_height - height);
        if(d2 > scroll->d) {
            scroll->d = d2 > 1.0 ? 1.0 : d2;
        } else if(d1 < scroll->d) {
            scroll->d = d1;
        }
    }

    return hittextmultiline(x, width, y, INT_MAX, lineheight, str, length, 1);
}
