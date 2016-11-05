#include "../main.h"
#include "../ui/svg.h"

void *bitmap[BM_ENDMARKER + 1];

BLENDFUNCTION blend_function = {
    .BlendOp = AC_SRC_OVER, .BlendFlags = 0, .SourceConstantAlpha = 0xFF, .AlphaFormat = AC_SRC_ALPHA
};

#define utf8tonative(str, out, length) MultiByteToWideChar(CP_UTF8, 0, (char *)str, length, out, length)


void drawalpha(int bm, int x, int y, int width, int height, uint32_t color) {
    if (!bitmap[bm]) {
        return;
    }

    BITMAPINFO bmi = {.bmiHeader = {
                          .biSize        = sizeof(BITMAPINFOHEADER),
                          .biWidth       = width,
                          .biHeight      = -height,
                          .biPlanes      = 1,
                          .biBitCount    = 32,
                          .biCompression = BI_RGB,
                      } };

    // create pointer to beginning and end of the alpha-channel-only bitmap
    uint8_t *alpha_pixel = bitmap[bm], *end = alpha_pixel + width * height;


    // create temporary bitmap we'll combine the alpha and colors on
    uint32_t *out_pixel;
    HBITMAP   temp = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, (void **)&out_pixel, NULL, 0);
    SelectObject(hdcMem, temp);

    // create pixels for the drawable bitmap based on the alpha value of
    // each pixel in the alpha bitmap and the color given by 'color',
    // the Win32 API requires we pre-apply our alpha channel as well by
    // doing (color * alpha / 255) for each color channel
    // NOTE: Input color is in the format 0BGR, output pixel is in the format BGRA
    while (alpha_pixel != end) {
        uint8_t alpha = *alpha_pixel++;
        *out_pixel++  = (((color & 0xFF) * alpha / 255) << 16)         // red
                       | ((((color >> 8) & 0xFF) * alpha / 255) << 8)  // green
                       | ((((color >> 16) & 0xFF) * alpha / 255) << 0) // blue
                       | (alpha << 24);                                // alpha
    }

    // draw temporary bitmap on screen
    AlphaBlend(hdc, x, y, width, height, hdcMem, 0, 0, width, height, blend_function);

    // clean up
    DeleteObject(temp);
}

void image_set_filter(NATIVE_IMAGE *image, uint8_t filter) {
    switch (filter) {
        case FILTER_NEAREST: image->stretch_mode  = COLORONCOLOR; break;
        case FILTER_BILINEAR: image->stretch_mode = HALFTONE; break;
        default: debug("Warning: Tried to set image to unrecognized filter(%u).\n", filter); return;
    }
}

void image_set_scale(NATIVE_IMAGE *image, double img_scale) {
    image->scaled_width  = (uint32_t)(((double)image->width * img_scale) + 0.5);
    image->scaled_height = (uint32_t)(((double)image->height * img_scale) + 0.5);
}

static bool image_is_stretched(const NATIVE_IMAGE *image) {
    return image->width != image->scaled_width || image->height != image->scaled_height;
}

// NOTE: This function is way more complicated than the XRender variant, because
// the Win32 API is a lot more limited, so all scaling, clipping, and handling
// transparency has to be done explicitly
void draw_image(const NATIVE_IMAGE *image, int x, int y, uint32_t width, uint32_t height, uint32_t imgx, uint32_t imgy) {
    HDC     drawdc;     // device context we'll do the eventual drawing with
    HBITMAP tmp = NULL; // used when scaling

    if (!image_is_stretched(image)) {

        SelectObject(hdcMem, image->bitmap);
        drawdc = hdcMem;

    } else {
        // temporary device context for the scaling operation
        drawdc = CreateCompatibleDC(NULL);

        // set stretch mode from image
        SetStretchBltMode(drawdc, image->stretch_mode);

        // scaled bitmap will be drawn onto this bitmap
        tmp = CreateCompatibleBitmap(hdcMem, image->scaled_width, image->scaled_height);
        SelectObject(drawdc, tmp);

        SelectObject(hdcMem, image->bitmap);

        // stretch image onto temporary bitmap
        if (image->has_alpha) {
            AlphaBlend(drawdc, 0, 0, image->scaled_width, image->scaled_height, hdcMem, 0, 0, image->width,
                       image->height, blend_function);
        } else {
            StretchBlt(drawdc, 0, 0, image->scaled_width, image->scaled_height, hdcMem, 0, 0, image->width,
                       image->height, SRCCOPY);
        }
    }

    // clip and draw
    if (image->has_alpha) {
        AlphaBlend(hdc, x, y, width, height, drawdc, imgx, imgy, width, height, blend_function);
    } else {
        BitBlt(hdc, x, y, width, height, drawdc, imgx, imgy, SRCCOPY);
    }

    // clean up
    if (image_is_stretched(image)) {
        DeleteObject(tmp);
        DeleteDC(drawdc);
    }
}

void draw_inline_image(uint8_t *img_data, size_t size, uint16_t w, uint16_t h, int x, int y) {

    BITMAPINFO bmi = {.bmiHeader = {
                          .biSize        = sizeof(BITMAPINFOHEADER),
                          .biWidth       = w,
                          .biHeight      = -h,
                          .biPlanes      = 1,
                          .biBitCount    = 32,
                          .biCompression = BI_RGB,
                      } };

    SetDIBitsToDevice(hdc, x, y, w, h, 0, 0, 0, h, img_data, &bmi, DIB_RGB_COLORS);
}

void drawtext(int x, int y, const char *str, uint16_t length) {
    wchar_t out[length];
    length = utf8tonative(str, out, length);

    TextOutW(hdc, x, y, out, length);
}

int drawtext_getwidth(int x, int y, const char *str, uint16_t length) {
    wchar_t out[length];
    length = utf8tonative(str, out, length);

    SIZE size;
    TextOutW(hdc, x, y, out, length);
    GetTextExtentPoint32W(hdc, out, length, &size);
    return size.cx;
}

void drawtextwidth(int x, int width, int y, const char *str, uint16_t length) {
    wchar_t out[length];
    length = utf8tonative(str, out, length);

    RECT r = { x, y, x + width, y + 256 };
    DrawTextW(hdc, out, length, &r, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
}

void drawtextwidth_right(int x, int width, int y, const char *str, uint16_t length) {
    wchar_t out[length];
    length = utf8tonative(str, out, length);

    RECT r = { x, y, x + width, y + 256 };
    DrawTextW(hdc, out, length, &r, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX | DT_RIGHT);
}

void drawtextrange(int x, int x2, int y, const char *str, uint16_t length) {
    wchar_t out[length];
    length = utf8tonative(str, out, length);

    RECT r = { x, y, x2, y + 256 };
    DrawTextW(hdc, out, length, &r, DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
}

void drawtextrangecut(int x, int x2, int y, const char *str, uint16_t length) {
    wchar_t out[length];
    length = utf8tonative(str, out, length);

    RECT r = { x, y, x2, y + 256 };
    DrawTextW(hdc, out, length, &r, DT_SINGLELINE | DT_NOPREFIX);
}

int textwidth(const char *str, uint16_t length) {
    wchar_t out[length];
    length = utf8tonative(str, out, length);

    SIZE size;
    GetTextExtentPoint32W(hdc, out, length, &size);
    return size.cx;
}

int textfit(const char *str, uint16_t len, int width) {
    wchar_t out[len];
    int   length = utf8tonative(str, out, len);

    int  fit;
    SIZE size;
    GetTextExtentExPointW(hdc, out, length, width, &fit, NULL, &size);

    return WideCharToMultiByte(CP_UTF8, 0, out, fit, (char *)str, len, NULL, 0);
}

int textfit_near(const char *str, uint16_t len, int width) {
    /*todo: near*/
    wchar_t out[len];
    int   length = utf8tonative(str, out, len);

    int  fit;
    SIZE size;
    GetTextExtentExPointW(hdc, out, length, width, &fit, NULL, &size);

    return WideCharToMultiByte(CP_UTF8, 0, out, fit, (char *)str, len, NULL, 0);
}

void draw_rect_frame(int x, int y, int width, int height, uint32_t color) {
    RECT r = { x, y, x + width, y + height };
    SetDCBrushColor(hdc, color);
    FrameRect(hdc, &r, hdc_brush);
}

void draw_rect_fill(int x, int y, int width, int height, uint32_t color) {
    RECT r = { x, y, x + width, y + height };
    SetDCBrushColor(hdc, color);
    FillRect(hdc, &r, hdc_brush);
}

void drawrect(int x, int y, int right, int bottom, uint32_t color) {
    RECT r = { x, y, right, bottom };
    SetDCBrushColor(hdc, color);
    FillRect(hdc, &r, hdc_brush);
}


void drawhline(int x, int y, int x2, uint32_t color) {
    RECT r = { x, y, x2, y + 1 };
    SetDCBrushColor(hdc, color);
    FillRect(hdc, &r, hdc_brush);
}

void drawvline(int x, int y, int y2, uint32_t color) {
    RECT r = { x, y, x + 1, y2 };
    SetDCBrushColor(hdc, color);
    FillRect(hdc, &r, hdc_brush);
}

void setfont(int id) {
    SelectObject(hdc, font[id]);
}

uint32_t setcolor(uint32_t color) {
    return SetTextColor(hdc, color);
}

RECT clip[16];

static int clipk;

void pushclip(int left, int top, int width, int height) {
    int right = left + width, bottom = top + height;

    RECT *r   = &clip[clipk++];
    r->left   = left;
    r->top    = top;
    r->right  = right;
    r->bottom = bottom;

    HRGN rgn = CreateRectRgn(left, top, right, bottom);
    SelectClipRgn(hdc, rgn);
    DeleteObject(rgn);
}

void popclip(void) {
    clipk--;
    if (!clipk) {
        SelectClipRgn(hdc, NULL);
        return;
    }

    RECT *r = &clip[clipk - 1];

    HRGN rgn = CreateRectRgn(r->left, r->top, r->right, r->bottom);
    SelectClipRgn(hdc, rgn);
    DeleteObject(rgn);
}

void enddraw(int x, int y, int width, int height) {
    SelectObject(hdc, hdc_bm);
    BitBlt(main_hdc, x, y, width, height, hdc, x, y, SRCCOPY);
}

void loadalpha(int bm, void *data, int width, int height) {
    bitmap[bm] = data;
}
