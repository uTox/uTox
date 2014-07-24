#include <ft2build.h>
#include <freetype.h>

#define PIXELS(x) (((x) + 32) / 64)

typedef struct
{
    uint32_t ucs4;
    int16_t x, y;
    uint16_t width, height, xadvance, xxxx;
    int16_t mx, my;
} GLYPH;

typedef struct
{
    FT_Face face;
    uint8_t *fontmap;
    uint16_t x, y, my, height;
    GLuint texture;
    GLYPH *glyphs[128];
} FONT;

FT_Library ftlib;
FONT font[16], *sfont;

GLYPH* font_getglyph(FONT *f, uint32_t ch)
{
    uint32_t hash = ch % 128;
    GLYPH *g = f->glyphs[hash], *s = g;
    if(g) {
        while(g->ucs4 != ~0) {
            if(g->ucs4 == ch) {
                return g;
            }
            g++;
        }

        uint32_t count = (uint32_t)(g - s);
        g = realloc(s, (count + 2) * sizeof(GLYPH));
        if(!g) {
            return NULL;
        }

        f->glyphs[hash] = g;
        g += count;
    } else {
        g = malloc(sizeof(GLYPH) * 2);
        if(!g) {
            return NULL;
        }

        f->glyphs[hash] = g;
    }

    g[1].ucs4 = ~0;
    FT_UInt index = FT_Get_Char_Index(f->face, ch);
    FT_Load_Glyph(f->face, index, FT_LOAD_RENDER);
    FT_GlyphSlotRec *p = f->face->glyph;

    g->ucs4 = ch;
    g->x = p->bitmap_left;
    g->y = PIXELS(f->face->size->metrics.ascender) - p->bitmap_top;
    g->width = p->bitmap.width;
    g->height = p->bitmap.rows;
    g->xadvance = (p->advance.x + (1 << 5)) >> 6;

    if(f->x + g->width > 512) {
        f->x = 0;
        f->y = f->my;
    }

    g->mx = f->x;
    g->my = f->y;

    glBindTexture(GL_TEXTURE_2D, f->texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, f->x, f->y, g->width, g->height, GL_ALPHA, GL_UNSIGNED_BYTE, p->bitmap.buffer);

    f->x += g->width;

    if(f->y + g->height > f->my) {
        f->my = f->y + g->height;
    }

    return g;
}

static void initfonts(void)
{
    FT_Init_FreeType(&ftlib);
}

static _Bool font_open(FONT *f, double size, uint8_t weight)
{
    FT_New_Face(ftlib, "/system/fonts/Roboto-Regular.ttf", 0, &f->face);
    FT_Set_Char_Size(f->face, (size * 64.0 + 0.5), (size * 64.0 + 0.5), 0, 0);

    f->fontmap = malloc(512 * 512);
    f->x = 0;
    f->y = 0;
    f->my = 0;
    f->height = 512;

    glGenTextures(1, &f->texture);
    glBindTexture(GL_TEXTURE_2D, f->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 512, 512, 0, GL_ALPHA, GL_UNSIGNED_BYTE, f->fontmap);

    return 1;
}

static void loadfonts(void)
{
    #define F(x) (x * SCALE / 2.0)
    font_open(&font[FONT_TEXT], F(12.0), 0);

    font_open(&font[FONT_TITLE], F(12.0), 1);

    font_open(&font[FONT_SELF_NAME], F(14.0), 1);
    font_open(&font[FONT_STATUS], F(11.0), 0);

    font_open(&font[FONT_LIST_NAME], F(12.0), 0);

    font_open(&font[FONT_MSG], F(11.0), 2);
    font_open(&font[FONT_MSG_NAME], F(10.0), 2);
    font_open(&font[FONT_MISC], F(10.0), 0);
    font_open(&font[FONT_MSG_LINK], F(11.0), 2);
    #undef F

    font_small_lineheight = (font[FONT_TEXT].face->size->metrics.height + (1 << 5)) >> 6;
    font_msg_lineheight = (font[FONT_MSG].face->size->metrics.height + (1 << 5)) >> 6;
}

static void freefonts(void)
{
    int i;
    for(i = 0; i != countof(font); i++) {
        FONT *f = &font[i];
        if(f->face) {
            FT_Done_Face(f->face);
        }

        int j = 0;
        for(j = 0; j != countof(f->glyphs); j++) {
            GLYPH *g = f->glyphs[j];
            if(g) {
                /*while(g->ucs4 != ~0) {
                    if(g->pic) {
                        XRenderFreePicture(display, g->pic);
                    }
                    g++;
                }*/

                free(f->glyphs[j]);
                f->glyphs[j] = NULL;
            }
        }
    }
}

static int _drawtext(int x, int xmax, int y, uint8_t *str, uint16_t length)
{
    glUniform3fv(k, 1, colorf);
    glBindTexture(GL_TEXTURE_2D, sfont->texture);
    int c = 0;

    GLYPH *g;
    uint8_t len;
    uint32_t ch;
    while(length) {
        len = utf8_len_read(str, &ch);
        str += len;
        length -= len;

        g = font_getglyph(sfont, ch);
        if(g) {
            if(x + g->xadvance > xmax) {
                x = -x;
                break;
            }

            if(c == 64) {
                glDrawQuads(0, 64);
                c = 0;
            }

            makeglyph(&quads[c++], x + g->x, y + g->y, g->mx, g->my, g->width, g->height);

            x += g->xadvance;
        }
    }

    glDrawQuads(0, c);

    return x;
}

#include "../shared/freetype-text.c"

