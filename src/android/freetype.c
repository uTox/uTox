#include "main.h"

#include "freetype.h"
#include "gl.h"

#include "../macros.h"

#include "../ui.h"
#include "../ui/draw.h"

#define PIXELS(x) (((x) + 32) / 64)

FT_Library ftlib;
FONT       font[16], *sfont;

GLYPH *font_getglyph(FONT *f, uint32_t ch) {
    uint32_t hash = ch % 128;
    GLYPH *  g = f->glyphs[hash], *s = g;
    if (g) {
        while (g->ucs4 != ~0) {
            if (g->ucs4 == ch) {
                return g;
            }
            g++;
        }

        uint32_t count = (uint32_t)(g - s);
        g              = realloc(s, (count + 2) * sizeof(GLYPH));
        if (!g) {
            return NULL;
        }

        f->glyphs[hash] = g;
        g += count;
    } else {
        g = malloc(sizeof(GLYPH) * 2);
        if (!g) {
            return NULL;
        }

        f->glyphs[hash] = g;
    }

    g[1].ucs4     = ~0;
    FT_UInt index = FT_Get_Char_Index(f->face, ch);
    FT_Load_Glyph(f->face, index, FT_LOAD_RENDER);
    FT_GlyphSlotRec *p = f->face->glyph;

    g->ucs4     = ch;
    g->x        = p->bitmap_left;
    g->y        = PIXELS(f->face->size->metrics.ascender) - p->bitmap_top;
    g->width    = p->bitmap.width;
    g->height   = p->bitmap.rows;
    g->xadvance = (p->advance.x + (1 << 5)) >> 6;

    if (f->x + g->width > 512) {
        f->x = 0;
        f->y = f->my;
    }

    g->mx = f->x;
    g->my = f->y;

    glBindTexture(GL_TEXTURE_2D, f->texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, f->x, f->y, g->width, g->height, GL_ALPHA, GL_UNSIGNED_BYTE, p->bitmap.buffer);

    f->x += g->width;

    if (f->y + g->height > f->my) {
        f->my = f->y + g->height;
    }

    return g;
}

void initfonts(void) {
    FT_Init_FreeType(&ftlib);
}

static bool font_open(FONT *f, double size, uint8_t weight) {
    FT_New_Face(ftlib, "/system/fonts/Roboto-Regular.ttf", 0, &f->face);
    FT_Set_Char_Size(f->face, (size * 64.0 + 0.5), (size * 64.0 + 0.5), 0, 0);

    f->fontmap = malloc(512 * 512);
    f->x       = 0;
    f->y       = 0;
    f->my      = 0;
    f->height  = 512;

    glGenTextures(1, &f->texture);
    glBindTexture(GL_TEXTURE_2D, f->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 512, 512, 0, GL_ALPHA, GL_UNSIGNED_BYTE, f->fontmap);

    return 1;
}

void loadfonts(void) {
    font_open(&font[FONT_TEXT], SCALE(12.0), 0);

    font_open(&font[FONT_TITLE], SCALE(12.0), 1);

    font_open(&font[FONT_SELF_NAME], SCALE(14.0), 1);
    font_open(&font[FONT_STATUS], SCALE(11.0), 0);

    font_open(&font[FONT_LIST_NAME], SCALE(12.0), 0);

    // font_open(&font[FONT_MSG], F(11.0), 2);
    // font_open(&font[FONT_MSG_NAME], F(10.0), 2);
    font_open(&font[FONT_MISC], SCALE(10.0), 0);
    // font_open(&font[FONT_MSG_LINK], F(11.0), 2);

    font_small_lineheight = (font[FONT_TEXT].face->size->metrics.height + (1 << 5)) >> 6;
    // font_msg_lineheight = (font[FONT_MSG].face->size->metrics.height + (1 << 5)) >> 6;
}

void freefonts(void) {
    for (size_t i = 0; i != COUNTOF(font); i++) {
        FONT *f = &font[i];
        if (f->face) {
            FT_Done_Face(f->face);
        }

        for (size_t j = 0; j != COUNTOF(f->glyphs); j++) {
            GLYPH *g = f->glyphs[j];
            if (g) {
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

static int _drawtext(int x, int xmax, int y, char *str, uint16_t length) {
    return GL_drawtext(x, xmax, y, str, length);
}

#include "../shared/freetype-text.c"
