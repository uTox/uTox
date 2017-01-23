#include "text.h"

#include "draw.h"
#include "scrollable.h"
#include "../theme.h"
#include "../util.h"

#include <limits.h>
#include <string.h>

static void text_draw_word_hl(int x, int y, const char *str, uint16_t length, int d, int h, int hlen,
                              uint16_t lineheight) {
    // Draw cursor
    /* multiline drawing goes word by word so str is not what you think it will be
     * drawing word by word could be the WORST way to go about it. (It's at least super frustrating without the
     * documentation). Ideally I'd like to process in one loop through, then draw in the next, but that's a fix for
     * another time. */
    h -= d;
    if (h + hlen < 0 || h > length) {
        drawtext(x, y, str, length);
        return;
    } else if (hlen == 0) {
        drawtext(x, y, str, length);
        int w = textwidth(str, h + hlen);
        drawvline(x + w, y, y + lineheight, COLOR_MAIN_TEXT);
        return;
    }

    if (h < 0) {
        hlen += h;
        h = 0;
        if (hlen < 0) {
            hlen = 0;
        }
    }

    if (h + hlen > length) {
        hlen = length - h;
    }

    int width = drawtext_getwidth(x, y, str, h);

    uint32_t color = setcolor(COLOR_SELECTION_TEXT);

    int w = textwidth(str + h, hlen);
    draw_rect_fill(x + width, y, w, lineheight, COLOR_SELECTION_BACKGROUND);
    drawtext(x + width, y, str + h, hlen);
    width += w;

    setcolor(color);

    drawtext(x + width, y, str + h + hlen, length - (h + hlen));
}

static void drawtextmark(int x, int y, const char *str, uint16_t length, int d, int h, int hlen, uint16_t lineheight) {
    h -= d;
    if (h + hlen < 0 || h > length || hlen == 0) {
        return;
    }

    if (h < 0) {
        hlen += h;
        h = 0;
        if (hlen < 0) {
            hlen = 0;
        }
    }

    if (h + hlen > length) {
        hlen = length - h;
    }

    int width = textwidth(str, h);

    int w = textwidth(str + h, hlen);
    drawhline(x + width, y + lineheight - 1, x + width + w, COLOR_MAIN_TEXT);
}

int utox_draw_text_multiline_within_box(int x, int y, /* x, y of the top left corner of the box */
                                        int right, int top, int bottom, uint16_t lineheight, const char *data,
                                        uint16_t length, /* text, and length of the text*/
                                        uint16_t h, uint16_t hlen, uint16_t mark, uint16_t marklen, bool multiline) {
    uint32_t c1, c2;

    bool greentext = 0, link = 0, draw = y + lineheight >= top;
    int  xc = x;

    const char *a = data, *b = a, *end = a + length;
    while (1) {
        if (a != end) {
            if (*a == '>' && (a == data || *(a - 1) == '\n')) {
                c1        = setcolor(COLOR_MAIN_TEXT_QUOTE);
                greentext = 1;
            }

            if ((a == data || *(a - 1) == '\n' || *(a - 1) == ' ')
                && ((end - a >= 7 && memcmp(a, "http://", 7) == 0) || (end - a >= 8 && memcmp(a, "https://", 8) == 0))) {
                c2   = setcolor(COLOR_MAIN_TEXT_URL);
                link = 1;
            }

            if (a == data || *(a - 1) == '\n') {
                const char *r = a;
                while (r != end && *r != '\n') {
                    r++;
                }
                if (*(r - 1) == '<') {
                    if (greentext) {
                        setcolor(COLOR_MAIN_TEXT_RED);
                    } else {
                        greentext = 1;
                        c1        = setcolor(COLOR_MAIN_TEXT_RED);
                    }
                }
            }
        }

        if (a == end || *a == ' ' || *a == '\n') {
            int count = a - b, w = textwidth(b, count);
            while (x + w > right) {
                if (multiline && x == xc) {
                    int fit = textfit(b, count, right - x);
                    if (draw) {
                        text_draw_word_hl(x, y, b, fit, b - data, h, hlen, lineheight);
                        drawtextmark(x, y, b, fit, b - data, mark, marklen, lineheight);
                    }
                    count -= fit;
                    b += fit;
                    y += lineheight;
                    draw = (y + lineheight >= top && y < bottom);
                } else if (!multiline) {
                    int fit = textfit(b, count, right - x);
                    if (draw) {
                        text_draw_word_hl(x, y, b, fit, b - data, h, hlen, lineheight);
                        drawtextmark(x, y, b, fit, b - data, mark, marklen, lineheight);
                    }
                    return y + lineheight;
                } else {
                    y += lineheight;
                    draw  = (y + lineheight >= top && y < bottom);
                    int l = utf8_len(b);
                    count -= l;
                    b += l;
                }
                x = xc;
                w = textwidth(b, count);
            }

            if (draw) {
                text_draw_word_hl(x, y, b, count, b - data, h, hlen, lineheight);
                drawtextmark(x, y, b, count, b - data, mark, marklen, lineheight);
            }

            x += w;
            b = a;

            if (link) {
                setcolor(c2);
                link = 0;
            }

            if (a == end) {
                if (greentext) {
                    setcolor(c1);
                    greentext = 0;
                }
                break;
            }

            if (*a == '\n') {
                if (greentext) {
                    setcolor(c1);
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

uint16_t hittextmultiline(int mx, int right, int my, int height, uint16_t lineheight, char *str, uint16_t length,
                          bool multiline) {
    if (my < 0) {
        return 0;
    }

    if (my >= height) {
        return length;
    }

    int   x = 0;
    char *a = str, *b = str, *end = str + length;
    while (1) {
        if (a == end || *a == '\n' || *a == ' ') {
            int count = a - b, w = textwidth(b, a - b);
            while (x + w > right && my >= lineheight) {
                if (multiline && x == 0) {
                    int fit = textfit(b, count, right);
                    count -= fit;
                    b += fit;
                    my -= lineheight;
                    height -= lineheight;
                } else if (!multiline) {
                    break;
                } else {
                    my -= lineheight;
                    height -= lineheight;
                    int l = utf8_len(b);
                    count -= l;
                    b += l;
                }

                if (my >= -lineheight && my < 0) {
                    x = mx;
                    break;
                }

                x = 0;
                w = textwidth(b, count);
            }

            if (a == end) {
                if (my >= lineheight) {
                    return length;
                }
                break;
            }
            if ((my >= 0 && my < lineheight) && (mx < 0 || (mx >= x && mx < x + w))) {
                break;
            }
            x += w;
            b = a;

            if (*a == '\n') {
                if (my >= 0 && my < lineheight) {
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
    if (mx >= right) {
        fit = textfit(b, a - b, right - x);
    } else if (mx - x > 0) {
        int len = a - b;
        fit     = textfit_near(b, len + (a != end), mx - x);
    } else {
        fit = 0;
    }

    return (b - str) + fit;
}

int text_height(int right, uint16_t lineheight, char *str, uint16_t length) {
    int   x = 0, y = 0;
    char *a = str, *b = a, *end = a + length;
    while (1) {
        if (a == end || *a == ' ' || *a == '\n') {
            int count = a - b, w = textwidth(b, count);
            while (x + w > right) {
                if (x == 0) {
                    int fit = textfit(b, count, right);
                    count -= fit;
                    if (fit == 0 && (count != 0 || *b == '\n')) {
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
                w = textwidth(b, count);
            }

            x += w;
            b = a;

            if (a == end) {
                break;
            }

            if (*a == '\n') {
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

static void textxy(int width, uint16_t pp, uint16_t lineheight, char *str, uint16_t length, int *outx, int *outy) {
    int   x = 0, y = 0;
    char *a = str, *b = str, *end = str + length, *p = str + pp;
    while (1) {
        if (a == end || *a == '\n' || *a == ' ') {
            int count = a - b, w = textwidth(b, a - b);
            while (x + w > width) {
                if (x == 0) {
                    int fit = textfit(b, count, width);
                    if (p >= b && p < b + fit) {
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
                w = textwidth(b, count);
            }

            if (p >= b && p < b + count) {
                w = textwidth(b, p - b);
                a = end;
            }

            x += w;
            if (a == end) {
                break;
            }
            b = a;

            if (*a == '\n') {
                if (p == a) {
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

uint16_t text_lineup(int width, int height, uint16_t p, uint16_t lineheight, char *str, uint16_t length,
                     SCROLLABLE *scroll) {
    // lazy
    int x, y;
    textxy(width, p, lineheight, str, length, &x, &y);
    if (y == 0) {
        scroll->d = 0.0;
        return p;
    }

    y -= lineheight;

    if (scroll->content_height > height) {
        double d1 = (double)y / (double)(scroll->content_height - height);
        double d2 = (double)(y - height + lineheight) / (double)(scroll->content_height - height);
        if (d1 < scroll->d) {
            scroll->d = d1;
        } else if (d2 > scroll->d) {
            scroll->d = d2;
        }
    }

    return hittextmultiline(x, width, y, INT_MAX, lineheight, str, length, 1);
}

uint16_t text_linedown(int width, int height, uint16_t p, uint16_t lineheight, char *str, uint16_t length,
                       SCROLLABLE *scroll) {
    // lazy
    int x, y;
    textxy(width, p, lineheight, str, length, &x, &y);

    y += lineheight;

    if (scroll->content_height > height) {
        double d1 = (double)y / (double)(scroll->content_height - height);
        double d2 = (double)(y - height + lineheight) / (double)(scroll->content_height - height);
        if (d2 > scroll->d) {
            scroll->d = d2 > 1.0 ? 1.0 : d2;
        } else if (d1 < scroll->d) {
            scroll->d = d1;
        }
    }

    return hittextmultiline(x, width, y, INT_MAX, lineheight, str, length, 1);
}
