#include "main.h"
#include "math.h"

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

static void drawrectrounded(uint8_t *data, int width, int height, int radius) {
    int x, y;
    double hw = (double)radius - 0.5;
    for(y = 0; y != height; y++) {
        for(x = 0; x != width; x++) {
            if((x < radius || x >= width - radius) && (y < radius || y >= height - radius)) {
                double dx, dy;
                dx = (x < radius) ? x - hw : x + hw - width + 1.0;
                dy = (y < radius) ? y - hw : y + hw - height + 1.0;
                double d = sqrt(dx * dx  + dy * dy) - hw;
                *data++ = pixel(d);
            } else {
                *data++ = 0xFF;
            }
        }
    }
}

static void drawrectroundedex(uint8_t *data, int width, int height, int radius, uint8_t flags) {
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
                double d = sqrt(dx * dx  + dy * dy) - hw;
                *data++ = pixel(d);
            } else {
                *data++ = 0xFF;
            }
        }
    }
}

static void drawrectroundedsub(uint8_t *p, int width, int height, int sx, int sy, int sw, int sh, int radius) {
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
                double d = sqrt(dx * dx  + dy * dy) - hw;
                *data = pixel(d);
            } else {
                *data = 0xFF;
            }

            x += sx;
            y += sy;
        }
    }
}

static void drawrectroundedneg(uint8_t *p, int width, int height, int sx, int sy, int sw, int sh, int radius) {
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
                double d = sqrt(dx * dx  + dy * dy) - hw;
                *data = 0xFF - pixel(d);
            } else {
                *data = 0;
            }

            x += sx;
            y += sy;
        }
    }
}

static void drawcircle(uint8_t *data, int width) {
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

static void drawnewcircle(uint8_t *data, int width, int height, double cx, double cy, double subwidth) {
    int x, y;
    double hw = cx - 0.5, vw = cy - 0.5, sw = (double)subwidth / 2.0;

    for(y = 0; y != height; y++) {
        for(x = 0; x != width; x++) {
            double dx = (x - hw), dy = (y - vw);
            double d = sqrt(dx * dx  + dy * dy) - sw;
            *data = pixelmax(d, *data); data++;
        }
    }
}

static void drawnewcircle2(uint8_t *data, int width, int height, double cx, double cy, double subwidth, uint8_t flags) {
    int x, y;
    double hw = cx - 0.5, vw = cy - 0.5, sw = (double)subwidth / 2.0;
    _Bool b = (flags & 1) != 0;

    for(y = 0; y != height; y++) {
        for(x = 0; x != width; x++) {
            double dx = (x - hw), dy = (y - vw);
            if(b && dy > 0) {
                dy *= 1.25;
            }

            if(!b && dx > 0) {
                dx *= 1.25;
            }

            double d = sqrt(dx * dx  + dy * dy) - sw;

            if((b && dx < 0) || (!b && dy < 0)) {
                *data++ = pixel(d);
            } else {
                *data = pixelmax(d, *data); data++;
            }

        }
    }
}

static void drawhead(uint8_t *data, int width, double cx, double cy, double subwidth) {
    int x, y;
    double hw = (double)cx - 0.5, vw = (double)cy - 0.5, sw = (double)subwidth / 2.0;

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

static void drawsubcircle(uint8_t *data, int width, int height, double cx, double cy, double subwidth) {
    int x, y;
    double hw = cx - 0.5, vw = cy - 0.5, sw = subwidth / 2.0;

    for(y = 0; y != height; y++) {
        for(x = 0; x != width; x++) {
            double dx = (x - hw), dy = (y - vw);
            double d = sqrt(dx * dx  + dy * dy) - sw;
            *data = pixelmin(d, *data); data++;
        }
    }
}

static void drawcross(uint8_t *data, int width) {
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

static void drawxcross(uint8_t *data, int width, int height, int radius) {
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

static void drawline(uint8_t *data, int width, int height, double sx, double sy, double span, double radius) {
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

static void drawlinedown(uint8_t *data, int width, int height, double sx, double sy, double span, double radius) {
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

static void drawlinevert(uint8_t *data, int width, int height, double sx, double w) {
    int x, y;
    double cx = sx + w / 2.0 - 0.5;

    for(y = 0; y != height; y++) {
        for(x = 0; x != width; x++) {
            double d = fabs((double)x - cx) - w / 2.0;
            *data = pixelmax(d, *data); data++;
        }
    }
}

static void drawtri(uint8_t *data, int width, int height, double sx, double sy, double size, uint8_t dir) {
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

static void drawlineround(uint8_t *data, int width, int height, double sx, double sy, double span, double radius, double subwidth, uint8_t flags) {
    int x, y;
    double cx = sx - 0.5, cy = sy - 0.5, sw = (double)subwidth / 2.0;

    for(y = 0; y != height; y++) {
        for(x = 0; x != width; x++) {
            double dx = (double)x - cx, dy = (double)y - cy;
            double d = (SQRT2 / 2.0) * fabs(dx + dy) - radius;
            d = fmax(fabs(dx) + fabs(dy) - (SQRT2 / 2.0) * (double)span - radius, d);

            double ddx, ddy, d2;

            if(!flags) {
                ddx = (double)x - cx - span * SQRT2; ddy = (double)y - cy + span * SQRT2;
                d2 = sqrt(ddx * ddx  + ddy * ddy) - sw;

                d = fmin(d, d2);
            }

            ddx = (double)x - cx + span * SQRT2; ddy = (double)y - cy - span * SQRT2;
            d2 = sqrt(ddx * ddx  + ddy * ddy) - sw;

            d = fmin(d, d2);

            *data = pixelmax(d, *data); data++;
        }
    }
}

static void drawlineroundempty(uint8_t *data, int width, int height, double sx, double sy, double span, double radius, double subwidth) {
    int x, y;
    double cx = sx - 0.5, cy = sy - 0.5, sw = (double)subwidth / 2.0;

    for(y = 0; y != height; y++) {
        for(x = 0; x != width; x++) {
            double dx = (double)x - cx, dy = (double)y - cy;
            double d = (SQRT2 / 2.0) * fabs(dx + dy) - radius;
            d = fmax(fabs(dx) + fabs(dy) - (SQRT2 / 2.0) * (double)span - radius, d);

            double ddx = (double)x - cx - span * SQRT2 , ddy = (double)y - cy + span * SQRT2;
            double d2 = sqrt(ddx * ddx  + ddy * ddy) - sw;

            d = fmin(d, d2);

            ddx = (double)x - cx + span * SQRT2; ddy = (double)y - cy - span * SQRT2;
            d2 = sqrt(ddx * ddx  + ddy * ddy) - sw;

            d = fmin(d, d2);

            *data = pixelmin(d, *data); data++;
        }
    }
}


static void drawgroup(uint8_t *data, int width) {
    double s = (double)width / BM_CONTACT_WIDTH;
    drawnewcircle(data, width, s * 9 * SCALE, s * 5 * SCALE, s * 9 * SCALE, s * 7.5 * SCALE);
    drawnewcircle(data, width, s * 9 * SCALE, s * 15 * SCALE, s * 9 * SCALE, s * 7.5 * SCALE);
    drawsubcircle(data, width, width, s * 5 * SCALE, s * 4 * SCALE, s * 5.5 * SCALE);
    drawsubcircle(data, width, width, s * 15 * SCALE, s * 4 * SCALE, s * 5.5 * SCALE);
    drawhead(data, width, s * 5 * SCALE, s * 3 * SCALE, s * 4.5 * SCALE);
    drawhead(data, width, s * 15 * SCALE, s * 3 * SCALE, s * 4.5 * SCALE);

    drawnewcircle(data, width, s * 20 * SCALE, s * 10 * SCALE, s * 20 * SCALE, s * 14.5 * SCALE);
    drawsubcircle(data, width, width, s * 10 * SCALE, s * 12 * SCALE, s * 6.5 * SCALE);
    drawsubcircle(data, width, width, s * 10 * SCALE, s * 8 * SCALE, s * 9.5 * SCALE);
    drawhead(data, width, s * 10 * SCALE, s * 8 * SCALE, s * 7.5 * SCALE);
}

_Bool svg_draw(_Bool needmemory) {
    int size, s;
    void *p;

    if(svg_data) {
        free(svg_data);
    }

    /* Build what we expect the size to be. */
    size = SCROLL_WIDTH * SCROLL_WIDTH +
        BM_STATUSAREA_WIDTH * BM_STATUSAREA_HEIGHT +
        /* Panel buttons */
        BM_ADD_WIDTH * BM_ADD_WIDTH * 4 +
        BM_THREE_BAR_WIDTH * BM_THREE_BAR_WIDTH +
        BM_CONTACT_WIDTH * BM_CONTACT_WIDTH * 2 +
        BM_LBICON_WIDTH * BM_LBICON_HEIGHT * 3 + // file, call, video
        BM_STATUS_WIDTH * BM_STATUS_WIDTH * 4 +
        BM_STATUS_NOTIFY_WIDTH * BM_STATUS_NOTIFY_WIDTH +
        BM_LBUTTON_WIDTH * BM_LBUTTON_HEIGHT +
        BM_SBUTTON_WIDTH * BM_SBUTTON_HEIGHT +
        /* File transfer */
        BM_FT_WIDTH * BM_FT_HEIGHT + BM_FTM_WIDTH * BM_FT_HEIGHT +
        (BM_FTB_WIDTH * (BM_FTB_HEIGHT + SCALE) + BM_FTB_WIDTH * BM_FTB_HEIGHT) +
        BM_FB_WIDTH * BM_FB_HEIGHT * 4 +
        /* Chat Buttons */
        BM_CHAT_BUTTON_WIDTH * (BM_CHAT_BUTTON_HEIGHT + SCALE) * 2 + // Chat button 1, 2
        BM_CHAT_SEND_WIDTH * BM_CHAT_SEND_HEIGHT +
        BM_CHAT_SEND_OVERLAY_WIDTH * BM_CHAT_SEND_OVERLAY_HEIGHT +
        BM_CI_WIDTH * BM_CI_WIDTH;


    svg_data = calloc(1, size);

    if(!svg_data) {
        return 0;
    }

    p = svg_data;

    drawcircle(p, SCROLL_WIDTH);
    loadalpha(BM_SCROLLHALFTOP, p, SCROLL_WIDTH, SCROLL_WIDTH / 2);
    loadalpha(BM_SCROLLHALFBOT, p + SCROLL_WIDTH * SCROLL_WIDTH / 2, SCROLL_WIDTH, SCROLL_WIDTH / 2);
    p += SCROLL_WIDTH * SCROLL_WIDTH;

    drawrectrounded(p, BM_STATUSAREA_WIDTH, BM_STATUSAREA_HEIGHT, SCALE * 2);
    loadalpha(BM_STATUSAREA, p, BM_STATUSAREA_WIDTH, BM_STATUSAREA_HEIGHT);
    p += BM_STATUSAREA_WIDTH * BM_STATUSAREA_HEIGHT;

    /* Draw panel Buttons */
    drawcross(p, BM_ADD_WIDTH);
    loadalpha(BM_ADD, p, BM_ADD_WIDTH, BM_ADD_WIDTH);
    p += BM_ADD_WIDTH * BM_ADD_WIDTH;

    /* New group bitmap */
    drawgroup(p, BM_ADD_WIDTH);
    loadalpha(BM_GROUPS, p, BM_ADD_WIDTH, BM_ADD_WIDTH);
    p += BM_ADD_WIDTH * BM_ADD_WIDTH;

    drawline(p, BM_ADD_WIDTH, BM_ADD_WIDTH, SCALE * 3, SCALE * 3, SCALE * 5, 0.75 * SCALE);
    drawline(p, BM_ADD_WIDTH, BM_ADD_WIDTH, SCALE * 6, SCALE * 6, SCALE * 5, 0.75 * SCALE);
    drawtri(p, BM_ADD_WIDTH, BM_ADD_WIDTH, 6 * SCALE, 0, 4 * SCALE, 0);
    drawtri(p, BM_ADD_WIDTH, BM_ADD_WIDTH, 3 * SCALE, 9 * SCALE, 4 * SCALE, 1);
    loadalpha(BM_TRANSFER, p, BM_ADD_WIDTH, BM_ADD_WIDTH);
    p += BM_ADD_WIDTH * BM_ADD_WIDTH;

    /* Settings gear bitmap */
    drawcross(p, BM_ADD_WIDTH);
    drawxcross(p, BM_ADD_WIDTH, BM_ADD_WIDTH, BM_ADD_WIDTH);
    drawnewcircle(p, BM_ADD_WIDTH, BM_ADD_WIDTH, 0.5 * BM_ADD_WIDTH, 0.5 * BM_ADD_WIDTH, 7 * SCALE);
    drawsubcircle(p, BM_ADD_WIDTH, BM_ADD_WIDTH, 0.5 * BM_ADD_WIDTH, 0.5 * BM_ADD_WIDTH, 3 * SCALE);
    loadalpha(BM_SETTINGS, p, BM_ADD_WIDTH, BM_ADD_WIDTH);
    p += BM_ADD_WIDTH * BM_ADD_WIDTH;

    /* Settings 3 line bitmap          width              height                   x                                                y               width                  height   radius */
    drawrectroundedsub(p, BM_THREE_BAR_WIDTH, BM_THREE_BAR_WIDTH, BM_THREE_BAR_WIDTH, BM_THREE_BAR_WIDTH /6 - BM_THREE_BAR_WIDTH   /8, BM_THREE_BAR_WIDTH, (BM_THREE_BAR_WIDTH /8), 2);
    drawrectroundedsub(p, BM_THREE_BAR_WIDTH, BM_THREE_BAR_WIDTH, BM_THREE_BAR_WIDTH, BM_THREE_BAR_WIDTH /2 - BM_THREE_BAR_WIDTH   /8, BM_THREE_BAR_WIDTH, (BM_THREE_BAR_WIDTH /8), 2);
    drawrectroundedsub(p, BM_THREE_BAR_WIDTH, BM_THREE_BAR_WIDTH, BM_THREE_BAR_WIDTH, BM_THREE_BAR_WIDTH /1 - BM_THREE_BAR_WIDTH *2/8, BM_THREE_BAR_WIDTH, (BM_THREE_BAR_WIDTH /8), 2);
    loadalpha(BM_SETTINGS_THREE_BAR, p, BM_THREE_BAR_WIDTH, BM_THREE_BAR_WIDTH);
    p += BM_THREE_BAR_WIDTH * BM_THREE_BAR_WIDTH;

    /* Contact avatar default bitmap */
    drawnewcircle(p, BM_CONTACT_WIDTH, 18 * SCALE, 10 * SCALE, 18 * SCALE, 14 * SCALE);
    drawsubcircle(p, BM_CONTACT_WIDTH, BM_CONTACT_WIDTH, 10 * SCALE, 10 * SCALE, 6 * SCALE);
    drawhead(p, BM_CONTACT_WIDTH, 10 * SCALE, 6 * SCALE, 8 * SCALE);
    loadalpha(BM_CONTACT, p, BM_CONTACT_WIDTH, BM_CONTACT_WIDTH);
    p += BM_CONTACT_WIDTH * BM_CONTACT_WIDTH;

    /* Group heads default bitmap */
    drawgroup(p, BM_CONTACT_WIDTH);
    loadalpha(BM_GROUP, p, BM_CONTACT_WIDTH, BM_CONTACT_WIDTH);
    p += BM_CONTACT_WIDTH * BM_CONTACT_WIDTH;

    /* Draw button icon overlays. */

    drawlineround(p, BM_LBICON_WIDTH, BM_LBICON_HEIGHT, 5.5 * SCALE, 5 * SCALE, 1 * SCALE, 3.85 * SCALE, 6.6 * SCALE, 0);
    drawlineroundempty(p, BM_LBICON_WIDTH, BM_LBICON_HEIGHT, 5.5 * SCALE, 5 * SCALE, 1 * SCALE, 2.4 * SCALE, 4.5 * SCALE);
    drawsubcircle(p, BM_LBICON_WIDTH, BM_LBICON_HEIGHT, 6.0 * SCALE, 8.1 * SCALE, 2.5 * SCALE);
    drawlineround(p, BM_LBICON_WIDTH, BM_LBICON_HEIGHT, 7.0 * SCALE, 5.40 * SCALE, 0.5 * SCALE, 2.2 * SCALE, 3.75 * SCALE, 1);
    drawlineroundempty(p, BM_LBICON_WIDTH, BM_LBICON_HEIGHT, 7.25 * SCALE, 5.15 * SCALE, 0.75 * SCALE, 0.75 * SCALE, 1.5 * SCALE);
    loadalpha(BM_FILE, p, BM_LBICON_WIDTH, BM_LBICON_HEIGHT);
    p += BM_LBICON_WIDTH * BM_LBICON_HEIGHT;

    drawnewcircle(p, BM_LBICON_WIDTH, BM_LBICON_HEIGHT, SCALE, 0, 19 * SCALE);
    drawsubcircle(p, BM_LBICON_WIDTH, BM_LBICON_HEIGHT, SCALE, 0, 15 * SCALE);
    drawnewcircle2(p, BM_LBICON_WIDTH, BM_LBICON_HEIGHT, 9 * SCALE, 2 * SCALE, 3 * SCALE, 0);
    drawnewcircle2(p, BM_LBICON_WIDTH, BM_LBICON_HEIGHT, 3 * SCALE, 8 * SCALE, 3 * SCALE, 1);
    loadalpha(BM_CALL, p, BM_LBICON_WIDTH, BM_LBICON_HEIGHT);
    p += BM_LBICON_WIDTH * BM_LBICON_HEIGHT;

    /* Video start end bitmap */
    int x, y;
    uint8_t *data = p;
    /* left triangle lens thing */
    for(y = 0; y != BM_LBICON_HEIGHT; y++) {
        for(x = 0; x != SCALE * 4; x++) {
            double d = fabs(y - 4.5 * SCALE) - 0.66 * (SCALE * 4 - x);
            *data++ = pixel(d);
        }
        data += BM_LBICON_WIDTH - SCALE * 4;
    }
    drawrectroundedsub(p, BM_LBICON_WIDTH, BM_LBICON_HEIGHT, 4 * SCALE, SCALE, 7 * SCALE, 7 * SCALE, SCALE);
    loadalpha(BM_VIDEO, p, BM_LBICON_WIDTH, BM_LBICON_HEIGHT);
    p += BM_LBICON_WIDTH * BM_LBICON_HEIGHT;

    /* Draw user status Buttons */
    s = BM_STATUS_WIDTH * BM_STATUS_WIDTH;
    drawcircle(p, BM_STATUS_WIDTH);
    loadalpha(BM_ONLINE, p, BM_STATUS_WIDTH, BM_STATUS_WIDTH);
    p += s;

    drawcircle(p, BM_STATUS_WIDTH);
    drawsubcircle(p, BM_STATUS_WIDTH, BM_STATUS_WIDTH / 2, 0.5 * BM_STATUS_WIDTH, 0.5 * BM_STATUS_WIDTH, 3 * SCALE);
    loadalpha(BM_AWAY, p, BM_STATUS_WIDTH, BM_STATUS_WIDTH);
    p += s;

    drawcircle(p, BM_STATUS_WIDTH);
    drawsubcircle(p, BM_STATUS_WIDTH, BM_STATUS_WIDTH / 2, 0.5 * BM_STATUS_WIDTH, 0.5 * BM_STATUS_WIDTH, 3 * SCALE);
    loadalpha(BM_BUSY, p, BM_STATUS_WIDTH, BM_STATUS_WIDTH);
    p += s;

    drawcircle(p, BM_STATUS_WIDTH);
    drawsubcircle(p, BM_STATUS_WIDTH, BM_STATUS_WIDTH, 0.5 * BM_STATUS_WIDTH, 0.5 * BM_STATUS_WIDTH, 3 * SCALE);
    loadalpha(BM_OFFLINE, p, BM_STATUS_WIDTH, BM_STATUS_WIDTH);
    p += s;

    drawcircle(p, BM_STATUS_NOTIFY_WIDTH);
    drawsubcircle(p, BM_STATUS_NOTIFY_WIDTH, BM_STATUS_NOTIFY_WIDTH, 0.5 * BM_STATUS_NOTIFY_WIDTH, 0.5 * BM_STATUS_NOTIFY_WIDTH, 5 * SCALE);
    loadalpha(BM_STATUS_NOTIFY, p, BM_STATUS_NOTIFY_WIDTH, BM_STATUS_NOTIFY_WIDTH);
    p += BM_STATUS_NOTIFY_WIDTH * BM_STATUS_NOTIFY_WIDTH;

    drawrectrounded(p, BM_LBUTTON_WIDTH, BM_LBUTTON_HEIGHT, SCALE * 2);
    loadalpha(BM_LBUTTON, p, BM_LBUTTON_WIDTH, BM_LBUTTON_HEIGHT);
    p += BM_LBUTTON_WIDTH * BM_LBUTTON_HEIGHT;

    drawrectrounded(p, BM_SBUTTON_WIDTH, BM_SBUTTON_HEIGHT, SCALE);
    loadalpha(BM_SBUTTON, p, BM_SBUTTON_WIDTH, BM_SBUTTON_HEIGHT);
    p += BM_SBUTTON_WIDTH * BM_SBUTTON_HEIGHT;

    /* Draw file transfer buttons */
    drawrectrounded(p, BM_FT_WIDTH, BM_FT_HEIGHT, SCALE * 2);
    loadalpha(BM_FT, p, BM_FT_WIDTH, BM_FT_HEIGHT);
    p += BM_FT_WIDTH * BM_FT_HEIGHT;

    drawrectroundedex(p, BM_FTM_WIDTH, BM_FT_HEIGHT, SCALE * 2, 13);
    loadalpha(BM_FTM, p, BM_FTM_WIDTH, BM_FT_HEIGHT);
    p += BM_FTM_WIDTH * BM_FT_HEIGHT;

    drawrectroundedex(p, BM_FTB_WIDTH, BM_FTB_HEIGHT + SCALE, SCALE * 2, 6);
    loadalpha(BM_FTB1, p, BM_FTB_WIDTH, BM_FTB_HEIGHT + SCALE);
    p += BM_FTB_WIDTH * (BM_FTB_HEIGHT + SCALE);

    drawrectroundedex(p, BM_FTB_WIDTH, BM_FTB_HEIGHT, SCALE * 2, 10);
    loadalpha(BM_FTB2, p, BM_FTB_WIDTH, BM_FTB_HEIGHT);
    p += BM_FTB_WIDTH * BM_FTB_HEIGHT;

    s = BM_FB_WIDTH * BM_FB_HEIGHT;

    drawxcross(p, BM_FB_WIDTH, BM_FB_HEIGHT, BM_FB_HEIGHT);
    loadalpha(BM_NO, p, BM_FB_WIDTH, BM_FB_HEIGHT);
    p += s;

    drawlinevert(p, BM_FB_WIDTH, BM_FB_HEIGHT, 0.75 * SCALE, 1.25 * SCALE);
    drawlinevert(p, BM_FB_WIDTH, BM_FB_HEIGHT, 4.25 * SCALE, 1.25 * SCALE);
    loadalpha(BM_PAUSE, p, BM_FB_WIDTH, BM_FB_HEIGHT);
    p += s;

    drawline(p, BM_FB_WIDTH, BM_FB_HEIGHT, SCALE * 1.75, SCALE * 3.5, SCALE * 2.5, 0.5 * SCALE);
    drawline(p, BM_FB_WIDTH, BM_FB_HEIGHT, SCALE * 4, SCALE * 3.5, SCALE * 2.5, 0.5 * SCALE);
    drawlinedown(p, BM_FB_WIDTH, BM_FB_HEIGHT, SCALE * 1.75, SCALE * 1.75, SCALE * 2.5, 0.5 * SCALE);
    drawlinedown(p, BM_FB_WIDTH, BM_FB_HEIGHT, SCALE * 4, SCALE * 1.75, SCALE * 2.5, 0.5 * SCALE);
    loadalpha(BM_RESUME, p, BM_FB_WIDTH, BM_FB_HEIGHT);
    p += s;

    drawline(p, BM_FB_WIDTH, BM_FB_HEIGHT, SCALE * 3.75, SCALE * 2.75, SCALE * 4, 0.5 * SCALE);
    drawlinedown(p, BM_FB_WIDTH, BM_FB_HEIGHT, SCALE * 1.9, SCALE * 3.25, SCALE * 2.5, 0.5 * SCALE);
    loadalpha(BM_YES, p, BM_FB_WIDTH, BM_FB_HEIGHT);
    p += s;

    drawrectroundedex(p, BM_CHAT_BUTTON_WIDTH, BM_CHAT_BUTTON_HEIGHT, SCALE * 2, 6);
    loadalpha(BM_CB1, p, BM_CHAT_BUTTON_WIDTH, BM_CHAT_BUTTON_HEIGHT);
    p += BM_CHAT_BUTTON_WIDTH * BM_CHAT_BUTTON_HEIGHT;

    drawrectroundedex(p, BM_CHAT_BUTTON_WIDTH, BM_CHAT_BUTTON_HEIGHT + SCALE, SCALE * 2, 10);
    loadalpha(BM_CB2, p, BM_CHAT_BUTTON_WIDTH, BM_CHAT_BUTTON_HEIGHT + SCALE);
    p += BM_CHAT_BUTTON_WIDTH * (BM_CHAT_BUTTON_HEIGHT + SCALE);

    /* Draw chat send button */
    drawrectrounded(p, BM_CHAT_SEND_WIDTH, BM_CHAT_SEND_HEIGHT, 4 * SCALE);
    loadalpha(BM_CHAT_SEND, p, BM_CHAT_SEND_WIDTH, BM_CHAT_SEND_HEIGHT);
    p += BM_CHAT_SEND_WIDTH * BM_CHAT_SEND_HEIGHT;

    /* Draw chat send overlay */
    drawnewcircle(p, BM_CHAT_SEND_OVERLAY_WIDTH, BM_CHAT_SEND_OVERLAY_HEIGHT, 10 * SCALE, 9 * SCALE, 15 * SCALE);
    drawtri(p, BM_CHAT_SEND_OVERLAY_WIDTH, BM_CHAT_SEND_OVERLAY_HEIGHT, 14 * SCALE, 14 * SCALE, 5 * SCALE, 0);
    loadalpha(BM_CHAT_SEND_OVERLAY, p, BM_CHAT_SEND_OVERLAY_WIDTH, BM_CHAT_SEND_OVERLAY_HEIGHT);
    p += BM_CHAT_SEND_OVERLAY_WIDTH * BM_CHAT_SEND_OVERLAY_HEIGHT;

    drawrectrounded(p, BM_CI_WIDTH, BM_CI_WIDTH, SCALE * 2);
    drawrectroundedneg(p, BM_CI_WIDTH, BM_CI_WIDTH, SCALE, SCALE, (_BM_CI_WIDTH - 2) * SCALE, (_BM_CI_WIDTH - 2) * SCALE, 1 * SCALE);
    loadalpha(BM_CI1, p, BM_CI_WIDTH, BM_CI_WIDTH);
    p += BM_CI_WIDTH * BM_CI_WIDTH;

    if(p - svg_data != size) {
        debug("something went wrong\n");
    }

    if(!needmemory) {
        free(svg_data);
        svg_data = NULL;
    }

    return 1;
}
