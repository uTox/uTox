#ifndef NATIVE_IMAGE_H
#define NATIVE_IMAGE_H

typedef struct native_image NATIVE_IMAGE;
typedef uint8_t *UTOX_IMAGE;

enum {
    FILTER_NEAREST, // ugly and quick filtering
    FILTER_BILINEAR // prettier and a bit slower filtering
};

/* set filtering method used when resizing given image to one of above enum */
void image_set_filter(NATIVE_IMAGE *image, uint8_t filter);

/* set scale of image so that when it's drawn it will be `scale' times as large(2.0 for double size, 0.5 for half, etc.)
 *  notes: theoretically lowest possible scale is (1.0/65536.0), highest is 65536.0, values outside of this range will
 * create weird issues
 *         scaling will be rounded to pixels, so it might not be exact
 */
void image_set_scale(NATIVE_IMAGE *image, double scale);

/* draws an utox image with or without alpha channel into the rect of (x,y,width,height) on the screen,
 * starting at position (imgx,imgy) of the image
 * WARNING: Windows can fail to show the image at all if the rect (imgx,imgy,width,height) contains even 1 pixel outside
 * of
 * the image's size AFTER SCALING, so be careful.
 * TODO: improve this so this function is safer to use
 */
void draw_image(const NATIVE_IMAGE *image, int x, int y, uint32_t width, uint32_t height, uint32_t imgx, uint32_t imgy);

/* Native wrapper to ready and call draw_image */
void draw_inline_image(uint8_t *img_data, size_t size, uint16_t w, uint16_t h, int x, int y);

/* converts a png to a NATIVE_IMAGE, returns a pointer to it, keeping alpha channel only if keep_alpha is 1 */
NATIVE_IMAGE *utox_image_to_native(const UTOX_IMAGE, size_t size, uint16_t *w, uint16_t *h, bool keep_alpha);

/* free an image created by utox_image_to_native */
void image_free(NATIVE_IMAGE *image);


// OS-dependent macros

#if defined __WIN32__ || defined _WIN32 || defined __CYGWIN__
#include "win/image.h"
#elif defined __ANDROID__
#include "android/image.h"
#elif defined __OBJC__
// TODO: OS X uses functions instead of macros.
#include "main.h"
#else
#include "xlib/image.h"
#endif

#endif
