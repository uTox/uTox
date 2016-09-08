#include "../main.h"

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
    _Bool c1 = ((flags & 1) != 0); /* left   0001 */
    _Bool c2 = ((flags & 2) != 0); /* right  0010 */
    _Bool c3 = ((flags & 4) != 0); /* top    0100 */
    _Bool c4 = ((flags & 8) != 0); /* bottom 1000 */

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
    double cx = 0.5    * (double)(width - 1);
    double cy = 0.5    * (double)(height - 1);
    double w  = 0.0625 * (double)radius;
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

static void svgdraw_line_neg(uint8_t *data, int width, int height, double sx, double sy, double span, double radius) {
    int x, y;
    double cx = sx - 0.5, cy = sy - 0.5;

    for(y = 0; y != height; y++) {
        for(x = 0; x != width; x++) {
            double dx = (double)x - cx, dy = (double)y - cy;
            double d = (SQRT2 / 2.0) * fabs(dx + dy) - radius;
            d = fmax(fabs(dx) + fabs(dy) - (SQRT2 / 2.0) * (double)span - radius, d);
            *data = pixelmin(d, *data); data++;
        }
    }
}

static void svgdraw_line_down_neg(uint8_t *data, int width, int height, double sx, double sy, double span, double radius) {
    int x, y;
    double cx = sx - 0.5, cy = sy - 0.5;

    for(y = 0; y != height; y++) {
        for(x = 0; x != width; x++) {
            double dx = (double)x - cx, dy = (double)y - cy;
            double d = (SQRT2 / 2.0) * fabs(dx - dy) - radius;
            d = fmax(fabs(dx) + fabs(dy) - (SQRT2 / 2.0) * (double)span - radius, d);
            *data = pixelmin(d, *data); data++;
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
    drawnewcircle(data, width, s * SCALE(18), s * SCALE(10), s * SCALE(18), s * SCALE(15));
    drawnewcircle(data, width, s * SCALE(18), s * SCALE(30), s * SCALE(18), s * SCALE(15));
    drawsubcircle(data, width, width, s * SCALE(10), s * SCALE(8), s * SCALE(9));
    drawsubcircle(data, width, width, s * SCALE(30), s * SCALE(8), s * SCALE(9));
    drawhead(data, width, s * SCALE(10), s * SCALE(6), s * SCALE(9));
    drawhead(data, width, s * SCALE(30), s * SCALE(6), s * SCALE(9));

    drawnewcircle(data, width, s * SCALE(40), s * SCALE(20), s * SCALE(40), s * SCALE(29));
    drawsubcircle(data, width, width, s * SCALE(20), s * SCALE(24), s * SCALE(13));
    drawsubcircle(data, width, width, s * SCALE(20), s * SCALE(16), s * SCALE(19));
    drawhead(data, width, s * SCALE(20), s * SCALE(16), s * SCALE(15));
}

_Bool svg_draw(_Bool needmemory) {
    int size, s;
    void *p;

    if(svg_data) {
        free(svg_data);
    }

    /* Build what we expect the size to be.
     * This section uses unnamed shortcuts, so it really serves no purpose and makes it harder to debug, it needs to be
     * fixed, without shortcuts, and proper comments... TODO FIXME */
    // comments behind the lines match with the comments of the code below that fills the memory
    size =
        SCROLL_WIDTH * SCROLL_WIDTH + /* Scroll bars top bottom halves */
        SCROLL_WIDTH * SCROLL_WIDTH /2 + /* Scroll bars top bottom halves (small)*/

        BM_STATUSAREA_WIDTH * BM_STATUSAREA_HEIGHT + /* status area */
        /* Panel buttons */
        BM_ADD_WIDTH        * BM_ADD_WIDTH +                /* Draw panel Button: Add */
        BM_ADD_WIDTH        * BM_ADD_WIDTH +                /* New group bitmap */
        BM_ADD_WIDTH        * BM_ADD_WIDTH +                /* Draw panel Button: Transfer */
        BM_ADD_WIDTH        * BM_ADD_WIDTH +                /* Settings gear bitmap */

        BM_CONTACT_WIDTH    * BM_CONTACT_WIDTH +            /* Contact avatar default bitmap */
        BM_CONTACT_WIDTH /2 * BM_CONTACT_WIDTH /2 +         /* Contact avatar default bitmap for mini roster */
        BM_CONTACT_WIDTH    * BM_CONTACT_WIDTH +            /* Group heads default bitmap */
        BM_CONTACT_WIDTH /2 * BM_CONTACT_WIDTH /2 +         /* Group heads default bitmap for mini roster */

        BM_FILE_WIDTH       * BM_FILE_HEIGHT +              /* Draw button icon overlays: file paper clip */

        BM_LBICON_WIDTH     * BM_LBICON_HEIGHT +            /* Call button icon */
        BM_LBICON_WIDTH     * BM_LBICON_HEIGHT +            /* Call button icon */
        BM_LBICON_WIDTH     * BM_LBICON_HEIGHT +            /* Video start end bitmap */

        BM_STATUS_WIDTH     * BM_STATUS_WIDTH +             /* user status: online */
        BM_STATUS_WIDTH     * BM_STATUS_WIDTH +             /* user status: away */
        BM_STATUS_WIDTH     * BM_STATUS_WIDTH +             /* user status: busy */
        BM_STATUS_WIDTH     * BM_STATUS_WIDTH +             /* user status: offline */

        BM_STATUS_NOTIFY_WIDTH * BM_STATUS_NOTIFY_WIDTH +   /* user status: notification */

        BM_LBUTTON_WIDTH * BM_LBUTTON_HEIGHT +              /* Generic Large Button */
        BM_SBUTTON_WIDTH * BM_SBUTTON_HEIGHT +              /* Generic Small Button */

        BM_SWITCH_WIDTH       * BM_SWITCH_HEIGHT +          /* Switch */
        BM_SWITCH_TOGGLE_WIDTH * BM_SWITCH_TOGGLE_HEIGHT +  /* Switch toggle */

        /* File transfer */
        BM_FT_CAP_WIDTH * BM_FTB_HEIGHT +
        BM_FT_WIDTH * BM_FT_HEIGHT +
        BM_FTM_WIDTH * BM_FT_HEIGHT +
        (BM_FTB_WIDTH * (BM_FTB_HEIGHT + SCALE(1)) + BM_FTB_WIDTH * BM_FTB_HEIGHT) +
        BM_FB_WIDTH * BM_FB_HEIGHT * 4 +
        /* Chat Buttons */
        BM_CHAT_BUTTON_WIDTH * BM_CHAT_BUTTON_HEIGHT * 2 + // Chat button 1, 2
        BM_CHAT_SEND_WIDTH   * BM_CHAT_SEND_HEIGHT +
        BM_CHAT_SEND_OVERLAY_WIDTH * BM_CHAT_SEND_OVERLAY_HEIGHT +
        BM_CHAT_BUTTON_OVERLAY_WIDTH * BM_CHAT_BUTTON_OVERLAY_HEIGHT;

    svg_data = calloc(1, size);

    if(!svg_data) {
        return 0;
    }

    p = svg_data;

    /* Scroll bars top bottom halves */
    drawcircle(p, SCROLL_WIDTH);
    loadalpha(BM_SCROLLHALFTOP, p,                                  SCROLL_WIDTH, SCROLL_WIDTH /2);
    loadalpha(BM_SCROLLHALFBOT, p + SCROLL_WIDTH * SCROLL_WIDTH /2, SCROLL_WIDTH, SCROLL_WIDTH /2);
    p += SCROLL_WIDTH * SCROLL_WIDTH;

    /* Scroll bars top bottom halves (small)*/
    drawcircle(p, SCROLL_WIDTH /2);
    loadalpha(BM_SCROLLHALFTOP_SMALL, p,                                     SCROLL_WIDTH /2, SCROLL_WIDTH /4);
    loadalpha(BM_SCROLLHALFBOT_SMALL, p + SCROLL_WIDTH /2 * SCROLL_WIDTH /4, SCROLL_WIDTH /2, SCROLL_WIDTH /4);
    p += SCROLL_WIDTH * SCROLL_WIDTH /2;

    /* status area */
    drawrectrounded(p, BM_STATUSAREA_WIDTH, BM_STATUSAREA_HEIGHT, SCALE(4));
    loadalpha(BM_STATUSAREA, p, BM_STATUSAREA_WIDTH, BM_STATUSAREA_HEIGHT);
    p += BM_STATUSAREA_WIDTH * BM_STATUSAREA_HEIGHT;

    /* Draw panel Button: Add */
    drawcross(p, BM_ADD_WIDTH);
    loadalpha(BM_ADD, p, BM_ADD_WIDTH, BM_ADD_WIDTH);
    p += BM_ADD_WIDTH * BM_ADD_WIDTH;

    /* New group bitmap */
    drawgroup(p, BM_ADD_WIDTH);
    loadalpha(BM_GROUPS, p, BM_ADD_WIDTH, BM_ADD_WIDTH);
    p += BM_ADD_WIDTH * BM_ADD_WIDTH;

    /* Draw panel Button: Transfer */
    drawline(p, BM_ADD_WIDTH, BM_ADD_WIDTH, SCALE(6),  SCALE(6),  SCALE(10), SCALE(1.5));
    drawline(p, BM_ADD_WIDTH, BM_ADD_WIDTH, SCALE(12), SCALE(12), SCALE(10), SCALE(1.5));
    drawtri(p, BM_ADD_WIDTH, BM_ADD_WIDTH,  SCALE(12),        0,  SCALE(8),         0);
    drawtri(p, BM_ADD_WIDTH, BM_ADD_WIDTH,  SCALE(6),  SCALE(18), SCALE(8),         1);
    loadalpha(BM_TRANSFER, p, BM_ADD_WIDTH, BM_ADD_WIDTH);
    p += BM_ADD_WIDTH * BM_ADD_WIDTH;

    /* Settings gear bitmap */
    drawcross(p, BM_ADD_WIDTH);
    drawxcross(p, BM_ADD_WIDTH, BM_ADD_WIDTH, BM_ADD_WIDTH);
    drawnewcircle(p, BM_ADD_WIDTH, BM_ADD_WIDTH, 0.5 * BM_ADD_WIDTH, 0.5 * BM_ADD_WIDTH, SCALE(14));
    drawsubcircle(p, BM_ADD_WIDTH, BM_ADD_WIDTH, 0.5 * BM_ADD_WIDTH, 0.5 * BM_ADD_WIDTH, SCALE(6));
    loadalpha(BM_SETTINGS, p, BM_ADD_WIDTH, BM_ADD_WIDTH);
    p += BM_ADD_WIDTH * BM_ADD_WIDTH;

    /* Contact avatar default bitmap */
    drawnewcircle(p, BM_CONTACT_WIDTH,        SCALE(36), SCALE(20), SCALE(36), SCALE(28));
    drawsubcircle(p, BM_CONTACT_WIDTH, BM_CONTACT_WIDTH, SCALE(20), SCALE(20), SCALE(12));
    drawhead(p, BM_CONTACT_WIDTH, SCALE(20), SCALE(12), SCALE(16));
    loadalpha(BM_CONTACT, p, BM_CONTACT_WIDTH, BM_CONTACT_WIDTH);
    p += BM_CONTACT_WIDTH * BM_CONTACT_WIDTH;

    /* Contact avatar default bitmap for mini roster */
    drawnewcircle(p, BM_CONTACT_WIDTH / 2, SCALE(18), SCALE(10), SCALE(18), SCALE(14));
    drawsubcircle(p, BM_CONTACT_WIDTH / 2, BM_CONTACT_WIDTH / 2, SCALE(10), SCALE(10), SCALE(6));
    drawhead(p, BM_CONTACT_WIDTH / 2, SCALE(10), SCALE(6), SCALE(8));
    loadalpha(BM_CONTACT_MINI, p, BM_CONTACT_WIDTH / 2, BM_CONTACT_WIDTH / 2);
    p += BM_CONTACT_WIDTH / 2 * BM_CONTACT_WIDTH / 2;

    /* Group heads default bitmap */
    drawgroup(p, BM_CONTACT_WIDTH);
    loadalpha(BM_GROUP, p, BM_CONTACT_WIDTH, BM_CONTACT_WIDTH);
    p += BM_CONTACT_WIDTH * BM_CONTACT_WIDTH;

    /* Group heads default bitmap for mini roster */
    drawgroup(p, BM_CONTACT_WIDTH / 2);
    loadalpha(BM_GROUP_MINI, p, BM_CONTACT_WIDTH / 2, BM_CONTACT_WIDTH / 2);
    p += BM_CONTACT_WIDTH / 2 * BM_CONTACT_WIDTH / 2;

    /* Draw button icon overlays. */
    drawlineround(p,      BM_FILE_WIDTH, BM_FILE_HEIGHT, UI_FSCALE(10), UI_FSCALE(10), UI_FSCALE(2),   UI_FSCALE(8.3), UI_FSCALE(14), 0);
    drawlineroundempty(p, BM_FILE_WIDTH, BM_FILE_HEIGHT, UI_FSCALE(10), UI_FSCALE(10), UI_FSCALE(2),   UI_FSCALE(6.5), UI_FSCALE(11));
    drawsubcircle(p,      BM_FILE_WIDTH, BM_FILE_HEIGHT, UI_FSCALE(11), UI_FSCALE(18), UI_FSCALE(6));
    drawlineround(p,      BM_FILE_WIDTH, BM_FILE_HEIGHT, UI_FSCALE(12), UI_FSCALE(12), UI_FSCALE(1),   UI_FSCALE(4.5), UI_FSCALE(7.5), 1);
    drawlineroundempty(p, BM_FILE_WIDTH, BM_FILE_HEIGHT, UI_FSCALE(13), UI_FSCALE(11), UI_FSCALE(1.5), UI_FSCALE(3),   UI_FSCALE(5.5));
    loadalpha(BM_FILE, p, BM_FILE_WIDTH, BM_FILE_HEIGHT);
    p += BM_FILE_WIDTH * BM_FILE_HEIGHT;

    /* Decline call button icon */
    drawnewcircle(p, BM_LBICON_WIDTH, BM_LBICON_HEIGHT, SCALE(11),   SCALE(25), SCALE(38));
    drawsubcircle(p, BM_LBICON_WIDTH, BM_LBICON_HEIGHT, SCALE(11),   SCALE(25), SCALE(30));
    drawnewcircle(p, BM_LBICON_WIDTH, BM_LBICON_HEIGHT, SCALE(3),    SCALE(11), SCALE(6));
    drawnewcircle(p, BM_LBICON_WIDTH, BM_LBICON_HEIGHT, SCALE(19.5), SCALE(11), SCALE(6));
    loadalpha(BM_DECLINE, p, BM_LBICON_WIDTH, BM_LBICON_HEIGHT);
    p += BM_LBICON_WIDTH * BM_LBICON_HEIGHT;

    /* Call button icon */
    drawnewcircle(p,  BM_LBICON_WIDTH, BM_LBICON_HEIGHT, SCALE(1),        0,   SCALE(38));
    drawsubcircle(p,  BM_LBICON_WIDTH, BM_LBICON_HEIGHT, SCALE(1),        0,   SCALE(30));
    drawnewcircle2(p, BM_LBICON_WIDTH, BM_LBICON_HEIGHT, SCALE(18), SCALE(4),  SCALE(6), 0);
    drawnewcircle2(p, BM_LBICON_WIDTH, BM_LBICON_HEIGHT, SCALE(6),  SCALE(16), SCALE(6), 1);
    loadalpha(BM_CALL, p, BM_LBICON_WIDTH, BM_LBICON_HEIGHT);
    p += BM_LBICON_WIDTH * BM_LBICON_HEIGHT;

    /* Video start end bitmap */
    int x, y;
    uint8_t *data = p;
    /* left triangle lens thing */
    for(y = 0; y != BM_LBICON_HEIGHT; y++) {
        for(x = 0; x != SCALE(8); x++) {
            double d = abs(y - SCALE(9)) - 0.66 * (SCALE(8) - x);
            *data++ = pixel(d);
        }
        data += BM_LBICON_WIDTH - SCALE(8);
    }
    drawrectroundedsub(p, BM_LBICON_WIDTH, BM_LBICON_HEIGHT, SCALE(8), SCALE(1), SCALE(14), SCALE(14), SCALE(1));
    loadalpha(BM_VIDEO, p, BM_LBICON_WIDTH, BM_LBICON_HEIGHT);
    p += BM_LBICON_WIDTH * BM_LBICON_HEIGHT;

    /* user status: online */
    s = BM_STATUS_WIDTH * BM_STATUS_WIDTH;
    drawcircle(p, BM_STATUS_WIDTH);
    loadalpha(BM_ONLINE, p, BM_STATUS_WIDTH, BM_STATUS_WIDTH);
    p += s;

    /* user status: away */
    drawcircle(p, BM_STATUS_WIDTH);
    drawsubcircle(p, BM_STATUS_WIDTH, BM_STATUS_WIDTH / 2, 0.5 * BM_STATUS_WIDTH, 0.5 * BM_STATUS_WIDTH, SCALE(6));
    loadalpha(BM_AWAY, p, BM_STATUS_WIDTH, BM_STATUS_WIDTH);
    p += s;

    /* user status: busy */
    drawcircle(p, BM_STATUS_WIDTH);
    drawsubcircle(p, BM_STATUS_WIDTH, BM_STATUS_WIDTH / 2, 0.5 * BM_STATUS_WIDTH, 0.5 * BM_STATUS_WIDTH, SCALE(6));
    loadalpha(BM_BUSY, p, BM_STATUS_WIDTH, BM_STATUS_WIDTH);
    p += s;

    /* user status: offline */
    drawcircle(p, BM_STATUS_WIDTH);
    drawsubcircle(p, BM_STATUS_WIDTH, BM_STATUS_WIDTH, 0.5 * BM_STATUS_WIDTH, 0.5 * BM_STATUS_WIDTH, SCALE(6));
    loadalpha(BM_OFFLINE, p, BM_STATUS_WIDTH, BM_STATUS_WIDTH);
    p += s;

    /* user status: notification */
    drawcircle(p, BM_STATUS_NOTIFY_WIDTH);
    drawsubcircle(p, BM_STATUS_NOTIFY_WIDTH, BM_STATUS_NOTIFY_WIDTH, 0.5 * BM_STATUS_NOTIFY_WIDTH, 0.5 * BM_STATUS_NOTIFY_WIDTH, SCALE(10));
    loadalpha(BM_STATUS_NOTIFY, p, BM_STATUS_NOTIFY_WIDTH, BM_STATUS_NOTIFY_WIDTH);
    p += BM_STATUS_NOTIFY_WIDTH * BM_STATUS_NOTIFY_WIDTH;

    /* Generic button icons */
    drawrectrounded(p, BM_LBUTTON_WIDTH, BM_LBUTTON_HEIGHT, SCALE(4));
    loadalpha(BM_LBUTTON, p, BM_LBUTTON_WIDTH, BM_LBUTTON_HEIGHT);
    p += BM_LBUTTON_WIDTH * BM_LBUTTON_HEIGHT;

    drawrectrounded(p, BM_SBUTTON_WIDTH, BM_SBUTTON_HEIGHT, SCALE(4));
    loadalpha(BM_SBUTTON, p, BM_SBUTTON_WIDTH, BM_SBUTTON_HEIGHT);
    p += BM_SBUTTON_WIDTH * BM_SBUTTON_HEIGHT;

    /* Outer part of the switch */
    drawrectrounded(p, BM_SWITCH_WIDTH, BM_SWITCH_HEIGHT, SCALE(4));
    loadalpha(BM_SWITCH, p, BM_SWITCH_WIDTH, BM_SWITCH_HEIGHT);
    p += BM_SWITCH_WIDTH * BM_SWITCH_HEIGHT;

    /* Switch toggle */
    drawrectrounded(p, BM_SWITCH_TOGGLE_WIDTH, BM_SWITCH_TOGGLE_HEIGHT, SCALE(4));
    loadalpha(BM_SWITCH_TOGGLE, p, BM_SWITCH_TOGGLE_WIDTH, BM_SWITCH_TOGGLE_HEIGHT);
    p += BM_SWITCH_TOGGLE_WIDTH * BM_SWITCH_TOGGLE_HEIGHT;

    /* Draw file transfer buttons */
    drawrectroundedex(p, BM_FT_CAP_WIDTH, BM_FTB_HEIGHT, SCALE(4), 13);
    loadalpha(BM_FT_CAP, p, BM_FT_CAP_WIDTH, BM_FTB_HEIGHT);
    p += BM_FT_CAP_WIDTH * BM_FTB_HEIGHT;

    drawrectrounded(p, BM_FT_WIDTH, BM_FT_HEIGHT, SCALE(4));
    loadalpha(BM_FT, p, BM_FT_WIDTH, BM_FT_HEIGHT);
    p += BM_FT_WIDTH * BM_FT_HEIGHT;

    drawrectroundedex(p, BM_FTM_WIDTH, BM_FT_HEIGHT, SCALE(4), 13);
    loadalpha(BM_FTM, p, BM_FTM_WIDTH, BM_FT_HEIGHT);
    p += BM_FTM_WIDTH * BM_FT_HEIGHT;

    drawrectroundedex(p,  BM_FTB_WIDTH, BM_FTB_HEIGHT + SCALE(1), SCALE(4), 0);
    loadalpha(BM_FTB1, p, BM_FTB_WIDTH, BM_FTB_HEIGHT + SCALE(1));
    p += BM_FTB_WIDTH * (BM_FTB_HEIGHT + SCALE(1));

    drawrectroundedex(p, BM_FTB_WIDTH, BM_FTB_HEIGHT, SCALE(4), 14);
    loadalpha(BM_FTB2, p, BM_FTB_WIDTH, BM_FTB_HEIGHT);
    p += BM_FTB_WIDTH * BM_FTB_HEIGHT;


    /* Used by the next few lines */
    s = BM_FB_WIDTH * BM_FB_HEIGHT;

    drawxcross(p, BM_FB_WIDTH, BM_FB_HEIGHT, BM_FB_HEIGHT);
    loadalpha(BM_NO, p, BM_FB_WIDTH, BM_FB_HEIGHT);
    p += s;

    drawlinevert(p, BM_FB_WIDTH, BM_FB_HEIGHT, SCALE(1.5), SCALE(2.5));
    drawlinevert(p, BM_FB_WIDTH, BM_FB_HEIGHT, SCALE(8.5), SCALE(2.5));
    loadalpha(BM_PAUSE, p, BM_FB_WIDTH, BM_FB_HEIGHT);
    p += s;

    drawline(p,     BM_FB_WIDTH, BM_FB_HEIGHT, SCALE(2.5),  SCALE(7),   SCALE(5), SCALE(1));
    drawline(p,     BM_FB_WIDTH, BM_FB_HEIGHT, SCALE(8),    SCALE(7),   SCALE(5), SCALE(1));
    drawlinedown(p, BM_FB_WIDTH, BM_FB_HEIGHT, SCALE(2.5),  SCALE(2.5), SCALE(5), SCALE(1));
    drawlinedown(p, BM_FB_WIDTH, BM_FB_HEIGHT, SCALE(8),    SCALE(2.5), SCALE(5), SCALE(1));
    loadalpha(BM_RESUME, p, BM_FB_WIDTH, BM_FB_HEIGHT);
    p += s;

    drawline(p,     BM_FB_WIDTH, BM_FB_HEIGHT, SCALE(8), SCALE(6), SCALE(8),   SCALE(1));
    drawlinedown(p, BM_FB_WIDTH, BM_FB_HEIGHT, SCALE(3), SCALE(6), SCALE(3.5), SCALE(1));
    loadalpha(BM_YES, p, BM_FB_WIDTH, BM_FB_HEIGHT);
    p += s;

    /* the two small chat buttons... */
    drawrectroundedex(p, BM_CHAT_BUTTON_WIDTH, BM_CHAT_BUTTON_HEIGHT, SCALE(4), 13);
    loadalpha(BM_CHAT_BUTTON_LEFT, p, BM_CHAT_BUTTON_WIDTH, BM_CHAT_BUTTON_HEIGHT);
    p += BM_CHAT_BUTTON_WIDTH * BM_CHAT_BUTTON_HEIGHT;

    drawrectroundedex(p, BM_CHAT_BUTTON_WIDTH, BM_CHAT_BUTTON_HEIGHT, SCALE(4), 0);
    loadalpha(BM_CHAT_BUTTON_RIGHT, p, BM_CHAT_BUTTON_WIDTH, BM_CHAT_BUTTON_HEIGHT);
    p += BM_CHAT_BUTTON_WIDTH * BM_CHAT_BUTTON_HEIGHT;


    /* Draw chat send button */
    drawrectroundedex(p, BM_CHAT_SEND_WIDTH, BM_CHAT_SEND_HEIGHT, SCALE(8), 14);
    loadalpha(BM_CHAT_SEND, p, BM_CHAT_SEND_WIDTH, BM_CHAT_SEND_HEIGHT);
    p += BM_CHAT_SEND_WIDTH * BM_CHAT_SEND_HEIGHT;

    /* Draw chat send overlay */
    drawnewcircle(p, BM_CHAT_SEND_OVERLAY_WIDTH, BM_CHAT_SEND_OVERLAY_HEIGHT, SCALE(20), SCALE(14), SCALE(26));
    drawtri(      p, BM_CHAT_SEND_OVERLAY_WIDTH, BM_CHAT_SEND_OVERLAY_HEIGHT, SCALE(30), SCALE(18), SCALE(12), 0);
    loadalpha(BM_CHAT_SEND_OVERLAY, p, BM_CHAT_SEND_OVERLAY_WIDTH, BM_CHAT_SEND_OVERLAY_HEIGHT);
    p += BM_CHAT_SEND_OVERLAY_WIDTH * BM_CHAT_SEND_OVERLAY_HEIGHT;

    /* screen shot button overlay */
    /* Rounded frame */
    drawrectroundedsub(p, BM_CHAT_BUTTON_OVERLAY_WIDTH,               BM_CHAT_BUTTON_OVERLAY_HEIGHT,
                          SCALE(1),                                  SCALE(1),
                          BM_CHAT_BUTTON_OVERLAY_WIDTH - (SCALE(8)), BM_CHAT_BUTTON_OVERLAY_HEIGHT - (SCALE(8)),
                          SCALE(1));
    drawrectroundedneg(p, BM_CHAT_BUTTON_OVERLAY_WIDTH,               BM_CHAT_BUTTON_OVERLAY_HEIGHT, /* width, height */
                          SCALE(4),                                  SCALE(4),                     /* start x, y */
                          BM_CHAT_BUTTON_OVERLAY_WIDTH - (SCALE(12)), BM_CHAT_BUTTON_OVERLAY_HEIGHT - (SCALE(12)),
                          SCALE(1));
    /* camera shutter circle */
    drawnewcircle(p, BM_CHAT_BUTTON_OVERLAY_WIDTH,        BM_CHAT_BUTTON_OVERLAY_HEIGHT,
                     BM_CHAT_BUTTON_OVERLAY_WIDTH * 0.75, BM_CHAT_BUTTON_OVERLAY_HEIGHT * 0.75,
                     SCALE(12 ));
    drawsubcircle(p, BM_CHAT_BUTTON_OVERLAY_WIDTH,        BM_CHAT_BUTTON_OVERLAY_HEIGHT,
                     BM_CHAT_BUTTON_OVERLAY_WIDTH * 0.75, BM_CHAT_BUTTON_OVERLAY_HEIGHT * 0.75,
                     SCALE(4));
    /* shutter lines */
    svgdraw_line_neg(p,      BM_CHAT_BUTTON_OVERLAY_WIDTH,        BM_CHAT_BUTTON_OVERLAY_HEIGHT,
                             BM_CHAT_BUTTON_OVERLAY_WIDTH * 0.80, BM_CHAT_BUTTON_OVERLAY_HEIGHT * 0.65,
                             SCALE(4), 0.1);
    svgdraw_line_neg(p,      BM_CHAT_BUTTON_OVERLAY_WIDTH,        BM_CHAT_BUTTON_OVERLAY_HEIGHT,
                             BM_CHAT_BUTTON_OVERLAY_WIDTH * 0.73, BM_CHAT_BUTTON_OVERLAY_HEIGHT * 0.87,
                             SCALE(4), 0.1);
    svgdraw_line_down_neg(p, BM_CHAT_BUTTON_OVERLAY_WIDTH,        BM_CHAT_BUTTON_OVERLAY_HEIGHT,
                             BM_CHAT_BUTTON_OVERLAY_WIDTH * 0.65, BM_CHAT_BUTTON_OVERLAY_HEIGHT * 0.70,
                             SCALE(4), 0.1);
    svgdraw_line_down_neg(p, BM_CHAT_BUTTON_OVERLAY_WIDTH,        BM_CHAT_BUTTON_OVERLAY_HEIGHT,
                             BM_CHAT_BUTTON_OVERLAY_WIDTH * 0.85, BM_CHAT_BUTTON_OVERLAY_HEIGHT * 0.81,
                             SCALE(4), 0.1);
    loadalpha(BM_CHAT_BUTTON_OVERLAY_SCREENSHOT, p, BM_CHAT_BUTTON_OVERLAY_WIDTH, BM_CHAT_BUTTON_OVERLAY_HEIGHT);
    p += BM_CHAT_BUTTON_OVERLAY_WIDTH * BM_CHAT_BUTTON_OVERLAY_HEIGHT;

    if(p - svg_data != size) {
        debug("SVG:\tSVG data size mismatch...\n");
    }

    if(!needmemory) {
        free(svg_data);
        svg_data = NULL;
    }

    return 1;
}
