
void drawtext(int x, int y, const char *str, uint16_t length) {
    _drawtext(x, INT_MAX, y, str, length);
}

int drawtext_getwidth(int x, int y, const char *str, uint16_t length) {
    return _drawtext(x, INT_MAX, y, str, length) - x;
}

void drawtextrange(int x, int xmax, int y, const char *str, uint16_t length) {
    x = _drawtext(x, xmax, y, str, length);
    if (x < 0) {
        _drawtext(-x, INT_MAX, y, (char *)"...", 3);
    }
}

void drawtextwidth(int x, int width, int y, const char *str, uint16_t length) {
    drawtextrange(x, x + width, y, str, length);
}

void drawtextwidth_right(int x, int width, int y, const char *str, uint16_t length) {
    int w = textwidth(str, length);
    if (w <= width) {
        drawtext(x + width - w, y, str, length);
    } else {
        drawtextrange(x, x + width, y, str, length);
    }
}

int textwidth(const char *str, uint16_t length) {
    GLYPH *  g;
    uint8_t  len;
    uint32_t ch;
    int      x = 0;
    while (length) {
        len = utf8_len_read(str, &ch);
        str += len;
        length -= len;

        g = font_getglyph(sfont, ch);
        if (g) {
            x += g->xadvance;
        }
    }
    return x;
}

int textfit(const char *str, uint16_t length, int width) {
    GLYPH *  g;
    uint8_t  len;
    uint32_t ch;
    int      x = 0;

    uint16_t i = 0;
    while (i != length) {
        len = utf8_len_read(str, &ch);
        str += len;

        g = font_getglyph(sfont, ch);
        if (g) {
            x += g->xadvance;
            if (x > width) {
                return i;
            }
        }

        i += len;
    }

    return length;
}

int textfit_near(const char *str, uint16_t length, int width) {
    GLYPH *  g;
    uint8_t  len;
    uint32_t ch;
    int      x = 0;

    uint16_t i = 0;
    while (i != length) {
        len = utf8_len_read(str, &ch);
        str += len;

        g = font_getglyph(sfont, ch);
        if (g) {
            x += g->xadvance;
            if (x > width) {
                return i;
            }
        }

        i += len;
    }

    return length;
}

void setfont(int id) {
    sfont = &font[id];
}
