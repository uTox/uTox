#include <stdint.h>

#include <ft2build.h>
#include <freetype/freetype.h>

#include <GLES2/gl2.h>

typedef struct {
    uint32_t ucs4;
    int16_t  x, y;
    uint16_t width, height, xadvance, xxxx;
    int16_t  mx, my;
} GLYPH;

typedef struct {
    FT_Face  face;
    uint8_t *fontmap;
    uint16_t x, y, my, height;
    GLuint   texture;
    GLYPH *  glyphs[128];
} FONT;

extern FT_Library ftlib;
extern FONT       font[16], *sfont;

void initfonts(void);
void loadfonts(void);
void freefonts(void);
