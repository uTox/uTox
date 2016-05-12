#include "main.h"

void inline_video_draw(INLINE_VID *UNUSED(p), int x, int y, int width, int height){
    if (!settings.inline_video) {
        return;
    }

    debug("Inline Video:\tDrawing new frame.\n");

    uint8_t *rgba_data = current_frame->img;

    // we don't need to free this, that's done by XDestroyImage()
    uint8_t *out = malloc(current_frame->size);

    // colors are read into red, blue and green and written into the target pointer
    uint8_t red, blue, green;
    uint32_t *target;

    uint32_t i;
    for (i = 0; i < current_frame->size; i += 4) {
        red = (rgba_data+i)[0] & 0xFF;
        green = (rgba_data+i)[1] & 0xFF;
        blue = (rgba_data+i)[2] & 0xFF;

        target = (uint32_t *)(out+i);
        *target = (red | (red << 8) | (red << 16) | (red << 24)) & visual->red_mask;
        *target |= (blue | (blue << 8) | (blue << 16) | (blue << 24)) & visual->blue_mask;
        *target |= (green | (green << 8) | (green << 16) | (green << 24)) & visual->green_mask;
    }

    XImage *img = XCreateImage(display, visual, depth, ZPixmap, 0, (char*)out, current_frame->w, current_frame->h, 32, current_frame->w * 4);

    Picture rgb = ximage_to_picture(img, NULL);
    // 4 bpp -> RGBA
    Picture alpha = None;

    UTOX_NATIVE_IMAGE *image = malloc(sizeof(UTOX_NATIVE_IMAGE));
    image->rgb = rgb;
    image->alpha = alpha;

    XDestroyImage(img);

    draw_image(image, x, y, current_frame->w, current_frame->h, 0, 0);
}

_Bool inline_video_mmove(INLINE_VID *UNUSED(p), int UNUSED(x), int UNUSED(y), int UNUSED(width), int UNUSED(height), int UNUSED(mx), int UNUSED(my), int UNUSED(dx), int UNUSED(dy)) {
    return 0;
}

_Bool inline_video_mdown(INLINE_VID *UNUSED(p)) {
    return 0;
}

_Bool inline_video_mright(INLINE_VID *UNUSED(p)) {
    return 0;
}

_Bool inline_video_mwheel(INLINE_VID *UNUSED(p), int UNUSED(height), double UNUSED(d), _Bool UNUSED(smooth)) {
    return 0;
}

_Bool inline_video_mup(INLINE_VID *UNUSED(p)) {
    return 0;
}

_Bool inline_video_mleave(INLINE_VID *UNUSED(p)) {
    return 0;
}

