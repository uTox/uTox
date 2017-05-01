#include "../ui/draw.h"

#include "freetype.h"
#include "main.h"
#include "window.h"

#include "../macros.h"
#include "../text.h"
#include "../ui.h"

#include <stdlib.h>

static uint32_t scolor;

void redraw(void) {
    _redraw = 1;
}

void force_redraw(void *UNUSED(args)) {
    XEvent ev = {
        .xclient = {
            .type         = ClientMessage,
            .display      = display,
            .window       = curr->window,
            .message_type = XRedraw,
            .format       = 8,
            .data = {
                .s = { 0, 0 }
            }
        }
    };

    _redraw = 1;
    XSendEvent(display, curr->window, 0, 0, &ev);
    XFlush(display);
}

void draw_image(const NATIVE_IMAGE *image, int x, int y, uint32_t width, uint32_t height, uint32_t imgx, uint32_t imgy) {
    XRenderComposite(display, PictOpOver, image->rgb, image->alpha, curr->renderpic, imgx, imgy, imgx, imgy, x, y, width,
                     height);

}

void draw_inline_image(uint8_t *img_data, size_t size, uint16_t w, uint16_t h, int x, int y) {
    const uint8_t *rgba_data = img_data;

    // we don't need to free this, that's done by XDestroyImage()
    uint8_t *out = malloc(size);

    uint32_t *target;

    for (uint32_t i = 0; i < size; i += 4) {
        // colors are read into red, blue and green and written into the target pointer
        const uint8_t red   = (rgba_data + i)[0] & 0xFF;
        const uint8_t green = (rgba_data + i)[1] & 0xFF;
        const uint8_t blue  = (rgba_data + i)[2] & 0xFF;

        target  = (uint32_t *)(out + i);
        *target = (red | (red << 8) | (red << 16) | (red << 24)) & curr->visual->red_mask;
        *target |= (blue | (blue << 8) | (blue << 16) | (blue << 24)) & curr->visual->blue_mask;
        *target |= (green | (green << 8) | (green << 16) | (green << 24)) & curr->visual->green_mask;
    }

    XImage *img = XCreateImage(display, curr->visual, default_depth, ZPixmap, 0, (char *)out, w, h, 32, w * 4);

    Picture rgb = ximage_to_picture(img, NULL);
    // 4 bpp -> RGBA
    Picture alpha = None;

    NATIVE_IMAGE *image = malloc(sizeof(NATIVE_IMAGE));
    image->rgb          = rgb;
    image->alpha        = alpha;

    XDestroyImage(img);

    draw_image(image, x, y, w, h, 0, 0);
    free(image);
}

void drawalpha(int bm, int x, int y, int width, int height, uint32_t color) {
    XRenderColor xrcolor = {.red   = ((color >> 8) & 0xFF00) | 0x80,
                            .green = ((color)&0xFF00) | 0x80,
                            .blue  = ((color << 8) & 0xFF00) | 0x80,
                            .alpha = 0xFFFF };

    Picture src = XRenderCreateSolidFill(display, &xrcolor);

    XRenderComposite(display, PictOpOver, src, bitmap[bm], curr->renderpic, 0, 0, 0, 0, x, y, width, height);

    XRenderFreePicture(display, src);
}

static int _drawtext(int x, int xmax, int y, const char *str, uint16_t length) {
    GLYPH *  g;
    uint8_t  len;
    uint32_t ch;
    while (length) {
        len = utf8_len_read(str, &ch);
        str += len;
        length -= len;

        g = font_getglyph(sfont, ch);
        if (g) {
            if (x + g->xadvance + SCALE(10) > xmax && length) {
                return -x;
            }

            if (g->pic) {
                XRenderComposite(display, PictOpOver, curr->colorpic, g->pic, curr->renderpic, 0, 0, 0, 0, x + g->x, y + g->y,
                                 g->width, g->height);
            }
            x += g->xadvance;
        }
    }

    return x;
}

// Needs to be included after ../ui/draw.h
#include "../shared/freetype-text.c"

void draw_rect_frame(int x, int y, int width, int height, uint32_t color) {
    XSetForeground(display, curr->gc, color);
    XDrawRectangle(display, curr->drawbuf, curr->gc, x, y, width - 1, height - 1);
}

void drawrect(int x, int y, int right, int bottom, uint32_t color) {
    XSetForeground(display, curr->gc, color);
    XFillRectangle(display, curr->drawbuf, curr->gc, x, y, right - x, bottom - y);
}

void draw_rect_fill(int x, int y, int width, int height, uint32_t color) {
    XSetForeground(display, curr->gc, color);
    XFillRectangle(display, curr->drawbuf, curr->gc, x, y, width, height);
}

void drawhline(int x, int y, int x2, uint32_t color) {
    XSetForeground(display, curr->gc, color);
    XDrawLine(display, curr->drawbuf, curr->gc, x, y, x2, y);
}

void drawvline(int x, int y, int y2, uint32_t color) {
    XSetForeground(display, curr->gc, color);
    XDrawLine(display, curr->drawbuf, curr->gc, x, y, x, y2);
}

uint32_t setcolor(uint32_t color) {
    XRenderColor xrcolor;
    xrcolor.red   = ((color >> 8) & 0xFF00) | 0x80;
    xrcolor.green = ((color)&0xFF00) | 0x80;
    xrcolor.blue  = ((color << 8) & 0xFF00) | 0x80;
    xrcolor.alpha = 0xFFFF;

    XRenderFreePicture(display, curr->colorpic);
    curr->colorpic = XRenderCreateSolidFill(display, &xrcolor);

    uint32_t old = scolor;
    scolor       = color;
    // xftcolor.pixel = color;
    XSetForeground(display, curr->gc, color);
    return old;
}

static XRectangle clip[16];
static int        clipk;

void pushclip(int left, int top, int width, int height) {
    if (!clipk) {
        // XSetClipMask(display, curr->gc, curr->drawbuf);
    }

    XRectangle *r = &clip[clipk++];
    r->x          = left;
    r->y          = top;
    r->width      = width;
    r->height     = height;

    XSetClipRectangles(display, curr->gc, 0, 0, r, 1, Unsorted);
    XRenderSetPictureClipRectangles(display, curr->renderpic, 0, 0, r, 1);
}

void popclip(void) {
    clipk--;
    if (!clipk) {
        XSetClipMask(display, curr->gc, None);

        XRenderPictureAttributes pa;
        pa.clip_mask = None;
        XRenderChangePicture(display, curr->renderpic, CPClipMask, &pa);
        return;
    }

    XRectangle *r = &clip[clipk - 1];

    XSetClipRectangles(display, curr->gc, 0, 0, r, 1, Unsorted);
    XRenderSetPictureClipRectangles(display, curr->renderpic, 0, 0, r, 1);
}

void enddraw(int x, int y, int width, int height) {
    XCopyArea(display, curr->drawbuf, curr->window, curr->gc, x, y, width, height, x, y);
}
