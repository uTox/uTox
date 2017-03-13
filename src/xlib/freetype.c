#include "freetype.h"

#include "main.h"
#include "window.h"


#include "../debug.h"
#include "../macros.h"
#include "../ui.h"

#define UTOX_FONT_XLIB "Roboto"

static void font_info_open(FONT_INFO *i, FcPattern *pattern);

Picture loadglyphpic(uint8_t *data, int width, int height, int pitch, bool no_subpixel, bool vertical,
                     bool swap_blue_red) {
    if (!width || !height) {
        return None;
    }

    Picture picture;
    GC      legc;
    Pixmap  pixmap;
    XImage *img;

    if (no_subpixel) {
        pixmap = XCreatePixmap(display, main_window.window, width, height, 8);
        img    = XCreateImage(display, CopyFromParent, 8, ZPixmap, 0, (char *)data, width, height, 8, 0);
        legc   = XCreateGC(display, pixmap, 0, NULL);
        XPutImage(display, pixmap, legc, img, 0, 0, 0, 0, width, height);
        picture = XRenderCreatePicture(display, pixmap, XRenderFindStandardFormat(display, PictStandardA8), 0, NULL);
    } else {

        uint32_t *rgbx, *p, *end;

        rgbx = malloc(4 * width * height);
        if (!rgbx) {
            return None;
        }

        p     = rgbx;
        int i = height;
        if (!vertical) {
            do {
                end = p + width;
                while (p != end) {
                    *p++ = swap_blue_red ? RGB(data[2], data[1], data[0]) : RGB(data[0], data[1], data[2]);
                    data += 3;
                }
                data += pitch - width * 3;
            } while (--i);
        } else {
            do {
                end = p + width;
                while (p != end) {
                    *p++ = swap_blue_red ? RGB(data[2 * pitch], data[1 * pitch], data[0]) :
                                           RGB(data[0], data[1 * pitch], data[2 * pitch]);
                    data += 1;
                }
                data += (pitch - width) + (pitch * 2);
            } while (--i);
        }

        pixmap = XCreatePixmap(display, main_window.window, width, height, default_depth);
        img    = XCreateImage(display, CopyFromParent, default_depth, ZPixmap, 0, (char *)rgbx, width, height, 32, 0);
        legc   = XCreateGC(display, pixmap, 0, NULL);
        XPutImage(display, pixmap, legc, img, 0, 0, 0, 0, width, height);

        XRenderPictureAttributes attr = {.component_alpha = 1 };
        picture = XRenderCreatePicture(display, pixmap, XRenderFindStandardFormat(display, PictStandardRGB24),
                                       CPComponentAlpha, &attr);

        free(rgbx);
    }

    XFreeGC(display, legc);
    XFreePixmap(display, pixmap);

    return picture;
}

GLYPH *font_getglyph(FONT *f, uint32_t ch) {
    uint32_t hash = ch % 128;
    GLYPH *  g = f->glyphs[hash], *s = g;
    if (g) {
        while (g->ucs4 != ~0u) {
            if (g->ucs4 == ch) {
                return g;
            }
            g++;
        }

        if (!FcCharSetHasChar(charset, ch)) {
            return NULL;
        }

        uint32_t count = (uint32_t)(g - s);
        g              = realloc(s, (count + 2) * sizeof(GLYPH));
        if (!g) {
            return NULL;
        }

        f->glyphs[hash] = g;
        g += count;
    } else {
        if (!FcCharSetHasChar(charset, ch)) {
            return NULL;
        }

        g = malloc(sizeof(GLYPH) * 2);
        if (!g) {
            return NULL;
        }

        f->glyphs[hash] = g;
    }

    // return FcCharSetHasChar (pub->charset, ucs4);
    FONT_INFO *i = f->info;
    while (i->face) {
        if (FcCharSetHasChar(i->cs, ch)) {
            break;
        }
        i++;
    }

    if (!i->face) {
        uint32_t count = (uint32_t)(i - f->info);
        i              = realloc(f->info, (count + 2) * sizeof(FONT_INFO));
        if (!i) {
            return NULL;
        }

        f->info = i;
        i += count;

        i[1].face = NULL;

        int j;
        for (j = 0; j != fs->nfont; j++) {
            FcCharSet *cs;

            FcPatternGetCharSet(fs->fonts[j], FC_CHARSET, 0, &cs);
            if (FcCharSetHasChar(cs, ch)) {
                FcPattern *p = FcPatternDuplicate(fs->fonts[j]);

                double size;
                if (!FcPatternGetDouble(f->pattern, FC_PIXEL_SIZE, 0, &size)) {
                    FcPatternAddDouble(p, FC_PIXEL_SIZE, size);
                }

                font_info_open(i, p);
                FcPatternDestroy(p);
                break;
            }
        }

        if (!i->face) {
            // something went wrong
            LOG_TRACE("Freetype", "???" );
            return NULL;
        }
    }

    int lcd_filter = FC_LCD_DEFAULT;
    FcPatternGetInteger(f->pattern, FC_LCD_FILTER, 0, &lcd_filter);
    FT_Library_SetLcdFilter(ftlib, lcd_filter);

    int ft_flags        = FT_LOAD_DEFAULT;
    int ft_render_flags = FT_RENDER_MODE_NORMAL;

    bool hinting = 1, antialias = 1, vertical_layout = 0, autohint = 0;
    FcPatternGetBool(f->pattern, FC_HINTING, 0, (int *)&hinting);
    FcPatternGetBool(f->pattern, FC_ANTIALIAS, 0, (int *)&antialias);
    FcPatternGetBool(f->pattern, FC_VERTICAL_LAYOUT, 0, (int *)&vertical_layout);
    FcPatternGetBool(f->pattern, FC_AUTOHINT, 0, (int *)&autohint);

    int hint_style = FC_HINT_FULL;
    FcPatternGetInteger(f->pattern, FC_HINT_STYLE, 0, (int *)&hint_style);

    // int weight;
    // FcPatternGetInteger(f->pattern, FC_WEIGHT, 0, (int *)&weight);
    int subpixel = FC_RGBA_NONE;
    FcPatternGetInteger(f->pattern, FC_RGBA, 0, (int *)&subpixel);

    bool no_subpixel = (subpixel == FC_RGBA_NONE);
    bool vert        = ft_vert;

    if (no_subpixel) {
        ft_render_flags = FT_RENDER_MODE_NORMAL;
    } else {
        ft_render_flags |= (vert ? FT_RENDER_MODE_LCD_V : FT_RENDER_MODE_LCD);
    }

    if (antialias) {
        if (hint_style == FC_HINT_NONE) {
            ft_flags |= FT_LOAD_NO_HINTING;
        } else if (hint_style == FC_HINT_SLIGHT) {
            ft_flags |= FT_LOAD_TARGET_LIGHT;
        } else if (hint_style == FC_HINT_FULL && !no_subpixel) {
            ft_flags |= (vert ? FT_LOAD_TARGET_LCD_V : FT_LOAD_TARGET_LCD);
        } else {
            ft_flags |= FT_LOAD_TARGET_NORMAL;
        }
    } else {
        ft_flags |= FT_LOAD_TARGET_MONO;
        ft_render_flags = FT_RENDER_MODE_NORMAL;
    }

    if (vertical_layout)
        ft_flags |= FT_LOAD_VERTICAL_LAYOUT;

    if (autohint)
        ft_flags |= FT_LOAD_FORCE_AUTOHINT;

    g[1].ucs4 = ~0;
    FT_Load_Char(i->face, ch, ft_flags);
    FT_Render_Glyph(i->face->glyph, ft_render_flags);
    FT_GlyphSlotRec *p = i->face->glyph;

    g->ucs4     = ch;
    g->x        = p->bitmap_left;
    g->y        = PIXELS(i->face->size->metrics.ascender) - p->bitmap_top;
    g->height   = p->bitmap.rows;
    g->xadvance = (p->advance.x + (1 << 5)) >> 6;

    if (p->bitmap.pixel_mode == FT_PIXEL_MODE_MONO) {
        unsigned int r, x;
        uint8_t *    mybuf = malloc(p->bitmap.width * g->height);
        uint8_t *    sline = p->bitmap.buffer, *dest = mybuf;

        g->width = p->bitmap.width;
        for (r = 0; r < g->height; r++, sline += p->bitmap.pitch) {
            for (x = 0; x < g->width; x++, dest++) {
                *dest = (sline[(x >> 3)] & (0x80 >> (x & 7))) * 0xff;
            }
        }
        free(p->bitmap.buffer);
        p->bitmap.buffer = mybuf;
        no_subpixel      = 1;
    } else if (p->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY) {
        g->width    = p->bitmap.width;
        no_subpixel = 1;
    } else if (p->bitmap.pixel_mode == FT_PIXEL_MODE_LCD) {
        g->width    = p->bitmap.width / 3;
        no_subpixel = 0;
        vert        = 0;
    } else if (p->bitmap.pixel_mode == FT_PIXEL_MODE_LCD_V) {
        g->width    = p->bitmap.width;
        g->height   = p->bitmap.rows / 3;
        no_subpixel = 0;
        vert        = 1;
    } else {
        g->width    = p->bitmap.width;
        no_subpixel = 0;
    }

    // LOG_TRACE("Freetype", "%u %u %u %u %C" , PIXELS(i->face->size->metrics.height), g->width, g->height, p->bitmap.pitch, ch);
    g->pic = loadglyphpic(p->bitmap.buffer, g->width, g->height, p->bitmap.pitch, no_subpixel, vert, ft_swap_blue_red);

    return g;
}

void initfonts(void) {
    if (!FcInit()) {
        // error
        LOG_ERR("Freetype", "FcInit failed.");
    }

    FT_Init_FreeType(&ftlib);

    FcResult   result;
    FcPattern *pat = FcPatternCreate();
    FcPatternAddString(pat, FC_FAMILY, (uint8_t *)UTOX_FONT_XLIB);
    FcConfigSubstitute(0, pat, FcMatchPattern);
    FcDefaultSubstitute(pat);
    fs = FcFontSort(NULL, pat, 0, &charset, &result);

    FcPatternDestroy(pat);
}

/*static void default_sub(FcPattern *pattern)
{
    //this is actually mostly useless
    //FcValue   v;
    //double    dpi;

    //FcPatternAddBool (pattern, XFT_RENDER, XftDefaultGetBool (dpy, XFT_RENDER, screen, XftDefaultHasRender (dpy)));
    FcPatternAddBool (pattern, FC_ANTIALIAS, True);
    FcPatternAddBool (pattern, FC_EMBOLDEN, False);
    FcPatternAddBool (pattern, FC_HINTING, True);
    FcPatternAddInteger (pattern, FC_HINT_STYLE, FC_HINT_FULL);
    FcPatternAddBool (pattern, FC_AUTOHINT, False);

    int subpixel = FC_RGBA_UNKNOWN;
    //if (XftDefaultHasRender (dpy))
    {
        int render_order = XRenderQuerySubpixelOrder (display, screen);
        switch (render_order) {
        default:
        case SubPixelUnknown:   subpixel = FC_RGBA_UNKNOWN; break;
        case SubPixelHorizontalRGB: subpixel = FC_RGBA_RGB; break;
        case SubPixelHorizontalBGR: subpixel = FC_RGBA_BGR; break;
        case SubPixelVerticalRGB:   subpixel = FC_RGBA_VRGB; break;
        case SubPixelVerticalBGR:   subpixel = FC_RGBA_VBGR; break;
        case SubPixelNone:      subpixel = FC_RGBA_NONE; break;
        }
    }

    FcPatternAddInteger (pattern, FC_RGBA, subpixel);
    FcPatternAddInteger (pattern, FC_LCD_FILTER, FC_LCD_DEFAULT);
    FcPatternAddBool (pattern, FC_MINSPACE, False);

    //dpi = (((double) DisplayHeight (dpy, screen) * 25.4) / (double) DisplayHeightMM (dpy, screen));
    //FcPatternAddDouble (pattern, FC_DPI, dpi);
    FcPatternAddDouble (pattern, FC_SCALE, 1.0);
    //FcPatternAddInteger (pattern, XFT_MAX_GLYPH_MEMORY, XftDefaultGetInteger (dpy, XFT_MAX_GLYPH_MEMORY, screen,
XFT_FONT_MAX_GLYPH_MEMORY));

    FcDefaultSubstitute (pattern);
}*/

static void font_info_open(FONT_INFO *i, FcPattern *pattern) {
    uint8_t * filename;
    int       id = 0;
    double    size;
    FcMatrix *font_matrix;
    /*FT_Matrix matrix = {
        .xx = 0x10000,
        .xy = 0,
        .yx = 0,
        .yy = 0x10000,
    };*/

    FcPatternGetString(pattern, FC_FILE, 0, &filename);
    FcPatternGetInteger(pattern, FC_INDEX, 0, &id);
    FcPatternGetCharSet(pattern, FC_CHARSET, 0, &i->cs);
    if (FcPatternGetMatrix(pattern, FC_MATRIX, 0, &font_matrix) == FcResultMatch) {
        LOG_TRACE("Freetype", "has a matrix" );
    }

    FcPatternGetDouble(pattern, FC_PIXEL_SIZE, 0, &size);

    int ft_error = FT_New_Face(ftlib, (char *)filename, id, &i->face);

    if (ft_error != 0) {
        LOG_TRACE("Freetype", "Freetype error %u %s %i" , ft_error, filename, id);
        return;
    }

    ft_error = FT_Set_Char_Size(i->face, (size * 64.0 + 0.5), (size * 64.0 + 0.5), 0, 0);
    if (ft_error != 0) {
        LOG_TRACE("Freetype", "Freetype error %u %lf" , ft_error, size);
        return;
    }

    // LOG_TRACE("Freetype", "Loaded font %s %u %i %i" , filename, id, PIXELS(i->face->ascender), PIXELS(i->face->descender));
}

static bool font_open(FONT *a_font, ...) {
    /* add error checks */
    va_list    va;
    FcPattern *pat;
    FcPattern *match;
    FcResult   result;

    va_start(va, a_font);
    pat = FcPatternVaBuild(NULL, va);
    va_end(va);

    FcConfigSubstitute(NULL, pat, FcMatchPattern);
    // default_sub(pat);
    match = FcFontMatch(NULL, pat, &result);
    FcPatternDestroy(pat);

    a_font->info = malloc(sizeof(FONT_INFO) * 2);

    font_info_open(a_font->info, match);

    a_font->pattern = match;

    a_font->info[1].face = NULL;

    return true;
}

void loadfonts(void) {
    int render_order = XRenderQuerySubpixelOrder(display, def_screen_num);
    if (render_order == SubPixelHorizontalBGR || render_order == SubPixelVerticalBGR) {
        ft_swap_blue_red = 1;
        LOG_TRACE("Freetype", "ft_swap_blue_red" );
    }

    if (render_order == SubPixelVerticalBGR || render_order == SubPixelVerticalRGB) {
        ft_vert = 1;
        LOG_TRACE("Freetype", "ft_vert" );
    }

    font_open(&font[FONT_TEXT], FC_FAMILY, FcTypeString, UTOX_FONT_XLIB, FC_PIXEL_SIZE, FcTypeDouble, UI_FSCALE(12.0),
              FC_WEIGHT, FcTypeInteger, FC_WEIGHT_NORMAL, FC_SLANT, FcTypeInteger, FC_SLANT_ROMAN, NULL);

    font_open(&font[FONT_TITLE], FC_FAMILY, FcTypeString, UTOX_FONT_XLIB, FC_PIXEL_SIZE, FcTypeDouble, UI_FSCALE(12.0),
              FC_WEIGHT, FcTypeInteger, FC_WEIGHT_BOLD, NULL);

    font_open(&font[FONT_SELF_NAME], FC_FAMILY, FcTypeString, UTOX_FONT_XLIB, FC_PIXEL_SIZE, FcTypeDouble, UI_FSCALE(14.0),
              FC_WEIGHT, FcTypeInteger, FC_WEIGHT_BOLD, NULL);
    font_open(&font[FONT_STATUS], FC_FAMILY, FcTypeString, UTOX_FONT_XLIB, FC_PIXEL_SIZE, FcTypeDouble, UI_FSCALE(11.0),
              FC_WEIGHT, FcTypeInteger, FC_WEIGHT_NORMAL, FC_SLANT, FcTypeInteger, FC_SLANT_ROMAN, NULL);

    font_open(&font[FONT_LIST_NAME], FC_FAMILY, FcTypeString, UTOX_FONT_XLIB, FC_PIXEL_SIZE, FcTypeDouble, UI_FSCALE(12.0),
              FC_WEIGHT, FcTypeInteger, FC_WEIGHT_NORMAL, FC_SLANT, FcTypeInteger, FC_SLANT_ROMAN, NULL);

    // font_open(&font[FONT_MSG],      FC_FAMILY, FcTypeString, UTOX_FONT_XLIB, FC_PIXEL_SIZE, FcTypeDouble, UI_FSCALE(11.0),
    //           FC_WEIGHT, FcTypeInteger, FC_WEIGHT_LIGHT,  NULL);
    // font_open(&font[FONT_MSG_NAME], FC_FAMILY, FcTypeString, UTOX_FONT_XLIB, FC_PIXEL_SIZE, FcTypeDouble, UI_FSCALE(10.0),
    // FC_WEIGHT, FcTypeInteger, FC_WEIGHT_LIGHT,  NULL);
    font_open(&font[FONT_MISC], FC_FAMILY, FcTypeString, UTOX_FONT_XLIB, FC_PIXEL_SIZE, FcTypeDouble, UI_FSCALE(10.0),
              FC_WEIGHT, FcTypeInteger, FC_WEIGHT_NORMAL, FC_SLANT, FcTypeInteger, FC_SLANT_ROMAN, NULL);
    // font_open(&font[FONT_MSG_LINK], FC_FAMILY, FcTypeString, UTOX_FONT_XLIB, FC_PIXEL_SIZE, FcTypeDouble, UI_FSCALE(11.0),
    //           FC_WEIGHT, FcTypeInteger, FC_WEIGHT_LIGHT,  NULL);
}

void freefonts(void) {
    for (size_t i = 0; i < COUNTOF(font); i++) {
        FONT *f = &font[i];
        if (f->pattern) {
            FcPatternDestroy(f->pattern);
        }

        if (f->info) {
            FONT_INFO *fi = f->info;
            while (fi->face) {
                FT_Done_Face(fi->face);
                fi++;
            }
            free(f->info);
        }

        for (size_t j = 0; j < COUNTOF(f->glyphs); j++) {
            GLYPH *g = f->glyphs[j];
            if (g) {
                while (g->ucs4 != ~0u) {
                    if (g->pic) {
                        XRenderFreePicture(display, g->pic);
                    }
                    g++;
                }

                free(f->glyphs[j]);
                f->glyphs[j] = NULL;
            }
        }
    }
}
