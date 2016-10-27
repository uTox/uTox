#ifndef XLIB_FREETYPE_H
#define XLIB_FREETYPE_H

#include <X11/extensions/Xrender.h>
#include <inttypes.h>
#include <stdbool.h>

#include <ft2build.h>
#include FT_LCD_FILTER_H
#include <fontconfig/fontconfig.h>
// fontconfig.h must be before fcfreetype.h
#include <fontconfig/fcfreetype.h>

#define PIXELS(x) (((x) + 32) / 64)

typedef struct {
    uint32_t ucs4;
    int16_t  x, y;
    uint16_t width, height, xadvance, xxxx;
    Picture  pic;
} GLYPH;

typedef struct {
    FT_Face    face;
    FcCharSet *cs;
} FONT_INFO;

typedef struct {
    FcPattern *pattern;
    FONT_INFO *info;
    GLYPH *    glyphs[128];
} FONT;

FT_Library ftlib;
FONT       font[16], *sfont;
FcCharSet *charset;
FcFontSet *fs;

bool ft_vert, ft_swap_blue_red;

Picture loadglyphpic(uint8_t *data, int width, int height, int pitch, bool no_subpixel, bool vertical,
                     bool swap_blue_red);
GLYPH *font_getglyph(FONT *f, uint32_t ch);
void initfonts(void);
void loadfonts(void);
void freefonts(void);

#endif
