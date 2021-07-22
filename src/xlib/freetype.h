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

extern FT_Library ftlib;
extern FONT       font[16], *sfont;
extern FcCharSet *charset;
extern FcFontSet *fs;

extern bool ft_vert, ft_swap_blue_red;

Picture loadglyphpic(uint8_t *data, int width, int height, int pitch, bool no_subpixel, bool vertical,
                     bool swap_blue_red);
GLYPH *font_getglyph(FONT *f, uint32_t ch);
void initfonts(void);
void loadfonts(void);
void freefonts(void);
