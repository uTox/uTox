#include "main.h"
#include "math.h"

#include "icons/file.c"
#include "icons/call.c"

#define SQRT2 1.41421356237309504880168872420969807856967187537694807317667973799

static uint8_t pixel(double d) {
    if(d >= 1.0) {
        return 0;
    } else if(d <= 0.0) {
        return 0xFF;
    } else {
        return (1.0 - d) * 255.0;
    }
}

static uint8_t pixelmin(double d, uint8_t p) {
    if(d >= 1.0) {
        return p;
    } else if(d <= 0.0) {
        return 0;
    } else {
        uint8_t value = d * 255.0;
        if(value >= p) {
            return p;
        } else {
            return value;
        }
    }
}

static uint8_t pixelmax(double d, uint8_t p) {
    if(d >= 1.0) {
        return p;
    } else if(d <= 0.0) {
        return 0xFF;
    } else {
        uint8_t value = (1.0 - d) * 255.0;
        if(value >= p) {
            return value;
        } else {
            return p;
        }
    }
}

static void drawrectrounded(uint8_t *data, int width, int height, int radius)
{
    int x, y;
    double hw = (double)radius - 0.5;
    for(y = 0; y != height; y++) {
        for(x = 0; x != width; x++) {
            if((x < radius || x >= width - radius) && (y < radius || y >= height - radius)) {
                double dx, dy;
                dx = (x < radius) ? x - hw : x + hw - width + 1.0;
                dy = (y < radius) ? y - hw : y + hw - height + 1.0;
                double d = sqrt(dx * dx  + dy * dy) - hw + 0.5;
                *data++ = pixel(d);
            } else {
                *data++ = 0xFF;
            }
        }
    }
}

static void drawrectroundedex(uint8_t *data, int width, int height, int radius, uint8_t flags)
{
    _Bool c1 = ((flags & 1) != 0);
    _Bool c2 = ((flags & 2) != 0);
    _Bool c3 = ((flags & 4) != 0);
    _Bool c4 = ((flags & 8) != 0);

    int x, y;
    double hw = (double)radius - 0.5;
    for(y = 0; y != height; y++) {
        for(x = 0; x != width; x++) {
            if(((c1 && x < radius) || (c2 && x >= width - radius)) && ((c3 && y < radius) || (c4 && y >= height - radius))) {
                double dx, dy;
                dx = (x < radius) ? x - hw : x + hw - width + 1.0;
                dy = (y < radius) ? y - hw : y + hw - height + 1.0;
                double d = sqrt(dx * dx  + dy * dy) - hw + 0.5;
                *data++ = pixel(d);
            } else {
                *data++ = 0xFF;
            }
        }
    }
}

static void drawrectroundedsub(uint8_t *p, int width, int height, int sx, int sy, int sw, int sh, int radius)
{
    int x, y;
    double hw = (double)radius - 0.5;
    for(y = sy; y != sy + sh; y++) {
        for(x = sx; x != sx + sw; x++) {
            uint8_t *data = &p[y * width + x];
            x -= sx;
            y -= sy;

            if((x < radius || x >= sw - radius) && (y < radius || y >= sh - radius)) {
                double dx, dy;
                dx = (x < radius) ? x - hw : x + hw - sw + 1.0;
                dy = (y < radius) ? y - hw : y + hw - sh + 1.0;
                double d = sqrt(dx * dx  + dy * dy) - hw + 0.5;
                *data = pixel(d);
            } else {
                *data = 0xFF;
            }

            x += sx;
            y += sy;
        }
    }
}

static void drawcircle(uint8_t *data, int width)
{
    int x, y;
    double hw = (double)width / 2.0 - 0.5;
    for(y = 0; y != width; y++) {
        for(x = 0; x != width; x++) {
            double dx = (x - hw), dy = (y - hw);
            double d = sqrt(dx * dx  + dy * dy) - hw + 0.5;
            *data++ = pixel(d);
        }
    }
}

static void drawnewcircle(uint8_t *data, int width, int height, double cx, double cy, double subwidth)
{
    int x, y;
    double hw = cx - 0.5, vw = cy - 0.5, sw = (double)subwidth / 2.0 - 1.0;

    for(y = 0; y != height; y++) {
        for(x = 0; x != width; x++) {
            double dx = (x - hw), dy = (y - vw);
            double d = sqrt(dx * dx  + dy * dy) - sw;
            *data = pixelmax(d, *data); data++;
        }
    }
}

static void drawhead(uint8_t *data, int width, int cx, int cy, int subwidth)
{
    int x, y;
    double hw = (double)cx - 0.5, vw = (double)cy - 0.5, sw = (double)subwidth / 2.0 - 1.0;

    for(y = 0; y != width; y++) {
        for(x = 0; x != width; x++) {
            double dx = (x - hw), dy = (y - vw);
            if(dy > 0) {
                dy *= 0.75;
            }
            double d = sqrt(dx * dx  + dy * dy) - sw;
            *data = pixelmax(d, *data); data++;
        }
    }
}

static void drawsubcircle(uint8_t *data, int width, double cx, double cy, double subwidth)
{
    int x, y;
    double hw = cx - 0.5, vw = cy - 0.5, sw = subwidth / 2.0 - 1.0;

    for(y = 0; y != width; y++) {
        for(x = 0; x != width; x++) {
            double dx = (x - hw), dy = (y - vw);
            double d = sqrt(dx * dx  + dy * dy) - sw;
            *data = pixelmin(d, *data); data++;
        }
    }
}

static void drawcross(uint8_t *data, int width)
{
    int x, y;
    double hw = 0.5 * (double)(width - 1);
    double w = 0.0625 * (double)width;
    for(y = 0; y != width; y++) {
        for(x = 0; x != width; x++) {
            double dx = fabs(x - hw), dy = fabs(y - hw);
            double d = fmin(dx, dy) - w;
            *data++ = pixel(d);
        }
    }
}

static void drawxcross(uint8_t *data, int width, int height, int radius)
{
    int x, y;
    double cx = 0.5 * (double)(width - 1);
    double cy = 0.5 * (double)(height - 1);
    double w = 0.0625 * (double)radius;
    for(y = 0; y != height; y++) {
        for(x = 0; x != width; x++) {
            double dx = (double)x - cx, dy = (double)y - cy;
            double d1 = (SQRT2 / 2.0) * fabs(dx + dy), d2 = (SQRT2 / 2.0) * fabs(dx - dy);
            double d = fmin(d1, d2) - w;
            d = fmax(fabs(dx) + fabs(dy) - (SQRT2 / 2.0) * (double)height - w, d);
            *data = pixelmax(d, *data); data++;
        }
    }
}

static void drawline(uint8_t *data, int width, int height, double sx, double sy, double span, double radius)
{
    int x, y;
    double cx = sx - 0.5, cy = sy - 0.5;

    for(y = 0; y != height; y++) {
        for(x = 0; x != width; x++) {
            double dx = (double)x - cx, dy = (double)y - cy;
            double d = (SQRT2 / 2.0) * fabs(dx + dy) - radius;
            d = fmax(fabs(dx) + fabs(dy) - (SQRT2 / 2.0) * (double)span - radius, d);
            *data = pixelmax(d, *data); data++;
        }
    }
}

static void drawlinedown(uint8_t *data, int width, int height, double sx, double sy, double span, double radius)
{
    int x, y;
    double cx = sx - 0.5, cy = sy - 0.5;

    for(y = 0; y != height; y++) {
        for(x = 0; x != width; x++) {
            double dx = (double)x - cx, dy = (double)y - cy;
            double d = (SQRT2 / 2.0) * fabs(dx - dy) - radius;
            d = fmax(fabs(dx) + fabs(dy) - (SQRT2 / 2.0) * (double)span - radius, d);
            *data = pixelmax(d, *data); data++;
        }
    }
}

static void drawlinevert(uint8_t *data, int width, int height, double sx, double w)
{
    int x, y;
    double cx = sx + w / 2.0 - 0.5;

    for(y = 0; y != height; y++) {
        for(x = 0; x != width; x++) {
            double d = fabs((double)x - cx) - w / 2.0;
            *data = pixelmax(d, *data); data++;
        }
    }
}

static void drawtri(uint8_t *data, int width, int height, double sx, double sy, double size, uint8_t dir)
{
    int x, y;
    double cx = sx - 0.5, cy = sy - 0.5;

    for(y = 0; y != height; y++) {
        for(x = 0; x != width; x++) {
            double dx = (double)x - cx, dy = (double)y - cy;
            if(!dir) {
                if(dx < 0.0 && dy > 0.0) {
                    double d = -dx + dy - size;
                    *data = pixelmax(d, *data);
                }
            } else {
                if(dx > 0.0 && dy < 0.0) {
                    double d = dx - dy - size;
                    *data = pixelmax(d, *data);
                }
            }
            data++;
        }
    }
}

static void drawgroup(uint8_t *data, int width)
{
    double s = (double)width / BM_CONTACT_WIDTH;
    drawnewcircle(data, width, s * 9 * SCALE, s * 5 * SCALE, s * 9 * SCALE, s * 8 * SCALE);
    drawnewcircle(data, width, s * 9 * SCALE, s * 15 * SCALE, s * 9 * SCALE, s * 8 * SCALE);
    drawsubcircle(data, width, s * 5 * SCALE, s * 4 * SCALE, s * 6 * SCALE);
    drawsubcircle(data, width, s * 15 * SCALE, s * 4 * SCALE, s * 6 * SCALE);
    drawhead(data, width, s * 5 * SCALE, s * 3 * SCALE, s * 5 * SCALE);
    drawhead(data, width, s * 15 * SCALE, s * 3 * SCALE, s * 5 * SCALE);

    drawnewcircle(data, width, s * 20 * SCALE, s * 10 * SCALE, s * 20 * SCALE, s * 15 * SCALE);
    drawsubcircle(data, width, s * 10 * SCALE, s * 12 * SCALE, s * 7 * SCALE);
    drawsubcircle(data, width, s * 10 * SCALE, s * 8 * SCALE, s * 10 * SCALE);
    drawhead(data, width, s * 10 * SCALE, s * 8 * SCALE, s * 8 * SCALE);
}

static void convert(uint8_t *dest, uint8_t *src, int size)
{
    uint8_t *end = dest + size;
    while(dest != end) {
        *dest++ = *src;
        src += 3;
    }
}

void svg_draw(void)
{
    bm_scroll_bits = malloc(SCROLL_WIDTH * SCROLL_WIDTH);
    drawcircle(bm_scroll_bits, SCROLL_WIDTH);

    bm_statusarea = malloc(BM_STATUSAREA_WIDTH * BM_STATUSAREA_HEIGHT);
    drawrectrounded(bm_statusarea, BM_STATUSAREA_WIDTH, BM_STATUSAREA_HEIGHT, SCALE * 2);

    bm_add = malloc(BM_ADD_WIDTH * BM_ADD_WIDTH);
    drawcross(bm_add, BM_ADD_WIDTH);

    int s = BM_ADD_WIDTH * BM_ADD_WIDTH;
    bm_groups = calloc(1, s);
    drawgroup(bm_groups, BM_ADD_WIDTH);

    bm_transfer = calloc(1, s);
    drawline(bm_transfer, BM_ADD_WIDTH, BM_ADD_WIDTH, SCALE * 3, SCALE * 3, SCALE * 5, 0.75 * SCALE);
    drawline(bm_transfer, BM_ADD_WIDTH, BM_ADD_WIDTH, SCALE * 6, SCALE * 6, SCALE * 5, 0.75 * SCALE);
    drawtri(bm_transfer, BM_ADD_WIDTH, BM_ADD_WIDTH, 6 * SCALE, 0, 4 * SCALE, 0);
    drawtri(bm_transfer, BM_ADD_WIDTH, BM_ADD_WIDTH, 3 * SCALE, 9 * SCALE, 4 * SCALE, 1);

    bm_settings = malloc(s);
    drawcross(bm_settings, BM_ADD_WIDTH);
    drawxcross(bm_settings, BM_ADD_WIDTH, BM_ADD_WIDTH, BM_ADD_WIDTH);
    drawnewcircle(bm_settings, BM_ADD_WIDTH, BM_ADD_WIDTH, 0.5 * BM_ADD_WIDTH, 0.5 * BM_ADD_WIDTH, 8 * SCALE);
    drawsubcircle(bm_settings, BM_ADD_WIDTH, 0.5 * BM_ADD_WIDTH, 0.5 * BM_ADD_WIDTH, 4 * SCALE);

    s = BM_CONTACT_WIDTH * BM_CONTACT_WIDTH;
    bm_contact = calloc(1, s);
    drawnewcircle(bm_contact, BM_CONTACT_WIDTH, 18 * SCALE, 10 * SCALE, 18 * SCALE, 15 * SCALE);
    drawsubcircle(bm_contact, BM_CONTACT_WIDTH, 10 * SCALE, 10 * SCALE, 7 * SCALE);
    drawhead(bm_contact, BM_CONTACT_WIDTH, 10 * SCALE, 6 * SCALE, 8 * SCALE);

    bm_group = calloc(1, s);
    drawgroup(bm_group, BM_CONTACT_WIDTH);

    s = BM_LBICON_WIDTH * BM_LBICON_HEIGHT;
    bm_file = malloc(s);
    convert(bm_file, bm_file_bits, s);

    bm_call = malloc(s);
    convert(bm_call, bm_call_bits, s);

    bm_video = calloc(1, s);
    int x, y;
    uint8_t *data = bm_video;
    for(y = 0; y != BM_LBICON_HEIGHT; y++) {
        for(x = 0; x != SCALE * 4; x++) {
            double d = fabs(y - 4.5 * SCALE) - 0.66 * (SCALE * 4 - x);
            *data++ = pixel(d);
        }
        data += BM_LBICON_WIDTH - SCALE * 4;
    }
    drawrectroundedsub(bm_video, BM_LBICON_WIDTH, BM_LBICON_HEIGHT, 4 * SCALE, SCALE, 7 * SCALE, 7 * SCALE, SCALE);

    #define S (BM_STATUS_WIDTH * BM_STATUS_WIDTH)
    bm_status_bits = malloc(S * 4);
    drawcircle(bm_status_bits, BM_STATUS_WIDTH);
    drawcircle(bm_status_bits + S, BM_STATUS_WIDTH);
    drawcircle(bm_status_bits + S * 2, BM_STATUS_WIDTH);
    drawcircle(bm_status_bits + S * 3, BM_STATUS_WIDTH);
    drawsubcircle(bm_status_bits + S * 3, BM_STATUS_WIDTH, 0.5 * BM_STATUS_WIDTH, 0.5 * BM_STATUS_WIDTH, 4 * SCALE);
    #undef S

    bm_lbutton = malloc(BM_LBUTTON_WIDTH * BM_LBUTTON_HEIGHT);
    drawrectrounded(bm_lbutton, BM_LBUTTON_WIDTH, BM_LBUTTON_HEIGHT, SCALE * 2);

    bm_sbutton = malloc(BM_SBUTTON_WIDTH * BM_SBUTTON_HEIGHT);
    drawrectrounded(bm_sbutton, BM_SBUTTON_WIDTH, BM_SBUTTON_HEIGHT, SCALE);

    bm_ft = malloc(BM_FT_WIDTH * BM_FT_HEIGHT);
    drawrectrounded(bm_ft, BM_FT_WIDTH, BM_FT_HEIGHT, SCALE * 2);

    bm_ftm = malloc(BM_FTM_WIDTH * BM_FT_HEIGHT);
    drawrectroundedex(bm_ftm, BM_FTM_WIDTH, BM_FT_HEIGHT, SCALE * 2, 13);

    bm_ftb = malloc(BM_FTB_WIDTH * (BM_FTB_HEIGHT + SCALE) + BM_FTB_WIDTH * BM_FTB_HEIGHT);
    drawrectroundedex(bm_ftb, BM_FTB_WIDTH, BM_FTB_HEIGHT + SCALE, SCALE * 2, 6);
    drawrectroundedex(bm_ftb + BM_FTB_WIDTH * (BM_FTB_HEIGHT + SCALE), BM_FTB_WIDTH, BM_FTB_HEIGHT, SCALE * 2, 10);

    s = BM_FB_WIDTH * BM_FB_HEIGHT;
    bm_no = calloc(1, s);
    drawxcross(bm_no, BM_FB_WIDTH, BM_FB_HEIGHT, BM_FB_HEIGHT);
    bm_pause = calloc(1, s);
    drawlinevert(bm_pause, BM_FB_WIDTH, BM_FB_HEIGHT, 0.75 * SCALE, 1.25 * SCALE);
    drawlinevert(bm_pause, BM_FB_WIDTH, BM_FB_HEIGHT, 4.25 * SCALE, 1.25 * SCALE);

    bm_yes = calloc(1, s);
    drawline(bm_yes, BM_FB_WIDTH, BM_FB_HEIGHT, SCALE * 3.75, SCALE * 2.75, SCALE * 4, 0.5 * SCALE);
    drawlinedown(bm_yes, BM_FB_WIDTH, BM_FB_HEIGHT, SCALE * 1.9, SCALE * 3.25, SCALE * 2.5, 0.5 * SCALE);

}
