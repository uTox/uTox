#include "main.h"
#include "math.h"

#include "icons/contact.c"
#include "icons/group.c"

#include "icons/groups.c"
#include "icons/transfer.c"
#include "icons/settings.c"

#include "icons/file.c"
#include "icons/call.c"

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
                if(d >= 1.0) {
                    *data++ = 0;
                } else if(d <= 0.0) {
                    *data++ = 0xFF;
                } else {
                    *data++ = (1.0 - d) * 255.0;
                }
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
                if(d >= 1.0) {
                    *data++ = 0;
                } else if(d <= 0.0) {
                    *data++ = 0xFF;
                } else {
                    *data++ = (1.0 - d) * 255.0;
                }
            } else {
                *data++ = 0xFF;
            }
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
            if(d >= 1.0) {
                *data++ = 0;
            } else if(d <= 0.0) {
                *data++ = 0xFF;
            } else {
                *data++ = (1.0 - d) * 255.0;
            }
        }
    }
}

static void drawsubcircle(uint8_t *data, int width, int subwidth)
{
    int x, y;
    double hw = (double)width / 2.0 - 0.5, sw = (double)subwidth / 2.0 - 1.0;

    for(y = 0; y != width; y++) {
        for(x = 0; x != width; x++) {
            double dx = (x - hw), dy = (y - hw);
            double d = sqrt(dx * dx  + dy * dy) - sw;
            if(d >= 1.0) {
                    data++;
                //*data++ = 0;
            } else if(d <= 0.0) {
                *data++ = 0;
            } else {
                *data++ = d * 255.0;
            }
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
            if(d >= 1.0) {
                *data++ = 0;
            } else if(d <= 0.0) {
                *data++ = 0xFF;
            } else {
                *data++ = (1.0 - d) * 255.0;
            }
        }
    }
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
    bm_groups = malloc(s);
    convert(bm_groups, bm_groups_bits, s);

    bm_transfer = malloc(s);
    convert(bm_transfer, bm_transfer_bits, s);

    bm_settings = malloc(s);
    convert(bm_settings, bm_settings_bits, s);

    s = BM_CONTACT_WIDTH * BM_CONTACT_WIDTH;
    bm_contact = malloc(s);
    convert(bm_contact, bm_contact_bits, s);

    bm_group = malloc(s);
    convert(bm_group, bm_group_bits, s);

    s = BM_LBICON_WIDTH * BM_LBICON_HEIGHT;
    bm_file = malloc(s);
    convert(bm_file, bm_file_bits, s);

    bm_call = malloc(s);
    convert(bm_call, bm_call_bits, s);

    #define S (BM_STATUS_WIDTH * BM_STATUS_WIDTH)
    bm_status_bits = malloc(S * 4);
    drawcircle(bm_status_bits, BM_STATUS_WIDTH);
    drawcircle(bm_status_bits + S, BM_STATUS_WIDTH);
    drawcircle(bm_status_bits + S * 2, BM_STATUS_WIDTH);
    drawcircle(bm_status_bits + S * 3, BM_STATUS_WIDTH);
    drawsubcircle(bm_status_bits + S * 3, BM_STATUS_WIDTH, 4 * SCALE);
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

}
