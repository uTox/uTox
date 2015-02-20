
void drawtext_common(int target, int x, int y, char_t *str, STRING_IDX length){
    _drawtext_common(0, x, INT_MAX, y, str, length);
}

int drawtext_getwidth_common(int target, int x, int y, char_t *str, STRING_IDX length){
    return _drawtext_common(0, x, INT_MAX, y, str, length) - x;
}

void drawtextrange_common(target, int target, int x, int xmax, int y, char_t *str, STRING_IDX length){
    x = _drawtext_common(0, x, xmax, y, str, length);
    if(x < 0) {
        _drawtext_common(0, -x, INT_MAX, y, (char_t*)"...", 3);
    }
}

void drawtextwidth_common(target,int target, int x, int width, int y, char_t *str, STRING_IDX length){
    drawtextrange_common(target, x, x + width, y, str, length);
}

void drawtextwidth_right_common(int target, int x, int width, int y, char_t *str, STRING_IDX length){
    int w = textwidth_common(0, str, length);
    if (w < width) {
        drawtext_common(0, x + width - w, y, str, length);
    } else {
        drawtextrange_common(target, x, x + width, y, str, length);
    }
}

int textwidth_common(int target, char_t *str, STRING_IDX length){
    GLYPH *g;
    uint8_t len;
    uint32_t ch;
    int x = 0;
    while(length) {
        len = utf8_len_read(str, &ch);
        str += len;
        length -= len;

        g = font_getglyph(sfont, ch);
        if(g) {
            x += g->xadvance;
        }
    }
    return x;
}

int textfit_common(int target, char_t *str, STRING_IDX length, int width){
    GLYPH *g;
    uint8_t len;
    uint32_t ch;
    int x = 0;

    STRING_IDX i = 0;
    while(i != length) {
        len = utf8_len_read(str, &ch);
        str += len;

        g = font_getglyph(sfont, ch);
        if(g) {
            x += g->xadvance;
            if(x > width) {
                return i;
            }
        }

        i += len;
    }

    return length;
}

int textfit_near_common(target, char_t *str, STRING_IDX length, int width){
    GLYPH *g;
    uint8_t len;
    uint32_t ch;
    int x = 0;

    STRING_IDX i = 0;
    while(i != length) {
        len = utf8_len_read(str, &ch);
        str += len;

        g = font_getglyph(sfont, ch);
        if(g) {
            x += g->xadvance;
            if(x > width) {
                return i;
            }
        }

        i += len;
    }

    return length;
}

void setfont_common(int target, int id){
    sfont = &font[id];
}

