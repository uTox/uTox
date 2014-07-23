
//FcFreeTypeCharIndex (face, ucs4);

#define PIXELS(x) (((x) + 32) / 64)

typedef struct
{
    uint32_t ucs4;
    int16_t x, y;
    uint16_t width, height, xadvance, xxxx;
    Picture pic;
} GLYPH;

typedef struct
{
    FT_Face face;
    FcCharSet *cs;
} FONT_INFO;

typedef struct
{
    FcPattern *pattern;
    FONT_INFO *info;
    GLYPH *glyphs[128];
} FONT;

FT_Library ftlib;
FONT font[16], *sfont;
FcCharSet *charset;
FcFontSet *fs;

static void font_info_open(FONT_INFO *i, FcPattern *pattern);

Picture loadglyphpic(uint8_t *data, int width, int height)
{
    if(!width || !height) {
        return None;
    }
   /* uint32_t *rgbx, *p, *end;

    rgbx = malloc(4 * width * height);
    if(!rgbx) {
        return None;
    }

    p = rgbx;
    end = rgbx + width * height;

    while(p != end) {
        *p++ = RGB(data[0], data[1], data[2]); data += 3;
    }*/

    Pixmap pixmap = XCreatePixmap(display, window, width, height, 8);
    XImage *img = XCreateImage(display, CopyFromParent, 8, ZPixmap, 0, (char*)data, width, height, 8, 0);
    GC legc = XCreateGC(display, pixmap, 0, NULL);
    XPutImage(display, pixmap, legc, img, 0, 0, 0, 0, width, height);

    Picture picture = XRenderCreatePicture(display, pixmap, XRenderFindStandardFormat(display, PictStandardA8), 0, NULL);

    XFreeGC(display, legc);
    XFreePixmap(display, pixmap);

    //free(rgbx);

    return picture;
}

GLYPH* font_getglyph(FONT *f, uint32_t ch)
{
    if(!FcCharSetHasChar(charset, ch)) {
        return NULL;
    }

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

    //return FcCharSetHasChar (pub->charset, ucs4);
    FONT_INFO *i = f->info;
    while(i->face) {
        if(FcCharSetHasChar(i->cs, ch)) {
            break;
        }
        i++;
    }

    if(!i->face) {
        uint32_t count = (uint32_t)(i - f->info);
        i = realloc(f->info, (count + 2) * sizeof(FONT_INFO));
        if(!i) {
            return NULL;
        }

        f->info = i;
        i += count;

        i[1].face = NULL;

        int j;
        for(j = 0; j != fs->nfont; j++) {
            FcCharSet *cs;

            FcPatternGetCharSet(fs->fonts[j], FC_CHARSET, 0, &cs);
            if(FcCharSetHasChar(cs, ch)) {
                FcPattern *p = FcPatternDuplicate(fs->fonts[j]);

                double size;
                if(!FcPatternGetDouble(f->pattern, FC_PIXEL_SIZE, 0, &size)) {
                    FcPatternAddDouble(p, FC_PIXEL_SIZE, size);
                }

                font_info_open(i, p);
                FcPatternDestroy(p);
                break;
            }
        }

        if(!i->face) {
            //something went wrong
            debug("???\n");
            return NULL;
        }
    }

    g[1].ucs4 = ~0;
    FT_UInt index = FcFreeTypeCharIndex(i->face, ch);
    FT_Load_Glyph(i->face, index, FT_LOAD_RENDER);
    FT_GlyphSlotRec *p = i->face->glyph;

    g->ucs4 = ch;
    g->x = p->bitmap_left;
    g->y = PIXELS(i->face->size->metrics.ascender) - p->bitmap_top;
    g->width = p->bitmap.width;
    g->height = p->bitmap.rows;
    g->xadvance = (p->advance.x + (1 << 5)) >> 6;

    //debug("%u %u %u %u %C\n", PIXELS(i->face->size->metrics.height), g->width, g->height, p->bitmap.pitch, ch);
    g->pic = loadglyphpic(p->bitmap.buffer, g->width, g->height);

    return g;
}

static void initfonts(void)
{
    if(!FcInit()) {
        //error
    }

    FT_Init_FreeType(&ftlib);

    FcResult result;
    FcPattern *pat = FcPatternCreate();
    FcPatternAddString(pat, FC_FAMILY, (uint8_t*)"Roboto");
    FcConfigSubstitute(0, pat, FcMatchPattern);
    FcDefaultSubstitute(pat);
    fs = FcFontSort(NULL, pat, 0, &charset, &result);

    FcPatternDestroy(pat);
}

static void default_sub(FcPattern *pattern)
{
    //FcValue	v;
    //double	dpi;

    //FcPatternAddBool (pattern, XFT_RENDER, XftDefaultGetBool (dpy, XFT_RENDER, screen, XftDefaultHasRender (dpy)));
    FcPatternAddBool (pattern, FC_ANTIALIAS, True);
    FcPatternAddBool (pattern, FC_EMBOLDEN, False);
    FcPatternAddBool (pattern, FC_HINTING, True);
    FcPatternAddInteger (pattern, FC_HINT_STYLE, FC_HINT_FULL);
    FcPatternAddBool (pattern, FC_AUTOHINT, False);

    int	subpixel = FC_RGBA_UNKNOWN;
	//if (XftDefaultHasRender (dpy))
	{
	    int render_order = XRenderQuerySubpixelOrder (display, screen);
	    switch (render_order) {
	    default:
	    case SubPixelUnknown:	subpixel = FC_RGBA_UNKNOWN; break;
	    case SubPixelHorizontalRGB:	subpixel = FC_RGBA_RGB; break;
	    case SubPixelHorizontalBGR:	subpixel = FC_RGBA_BGR; break;
	    case SubPixelVerticalRGB:	subpixel = FC_RGBA_VRGB; break;
	    case SubPixelVerticalBGR:	subpixel = FC_RGBA_VBGR; break;
	    case SubPixelNone:		subpixel = FC_RGBA_NONE; break;
	    }
	}

	FcPatternAddInteger (pattern, FC_RGBA, subpixel);
	FcPatternAddInteger (pattern, FC_LCD_FILTER, FC_LCD_DEFAULT);
	FcPatternAddBool (pattern, FC_MINSPACE, False);

	//dpi = (((double) DisplayHeight (dpy, screen) * 25.4) / (double) DisplayHeightMM (dpy, screen));
	//FcPatternAddDouble (pattern, FC_DPI, dpi);
	FcPatternAddDouble (pattern, FC_SCALE, 1.0);
	//FcPatternAddInteger (pattern, XFT_MAX_GLYPH_MEMORY, XftDefaultGetInteger (dpy, XFT_MAX_GLYPH_MEMORY, screen, XFT_FONT_MAX_GLYPH_MEMORY));

	FcDefaultSubstitute (pattern);
}

static void font_info_open(FONT_INFO *i, FcPattern *pattern)
{
    uint8_t *filename;
    int id = 0;
    double size;
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
    if(FcPatternGetMatrix(pattern, FC_MATRIX, 0, &font_matrix) == FcResultMatch) {
        debug("has a matrix\n");
    }

    FcPatternGetDouble(pattern, FC_PIXEL_SIZE, 0, &size);

    FT_New_Face(ftlib, (char*)filename, id, &i->face);
    FT_Set_Char_Size(i->face, (size * 64.0 + 0.5), (size * 64.0 + 0.5), 0, 0);
    debug("Loaded font %s %u %i %i\n", filename, id, PIXELS(i->face->ascender), PIXELS(i->face->descender));
}

static _Bool font_open(FONT *font, ...)
{
    /* add error checks */
    va_list	    va;
    FcPattern	    *pat;
    FcPattern	    *match;
    FcResult	    result;

    va_start (va, font);
    pat = FcPatternVaBuild(NULL, va);
    va_end (va);

    FcConfigSubstitute(NULL, pat, FcMatchPattern);
    default_sub(pat);
    match = FcFontMatch(NULL, pat, &result);
    FcPatternDestroy(pat);

    font->info = malloc(sizeof(FONT_INFO) * 2);

    font_info_open(font->info, match);

    font->pattern = match;

    font->info[1].face = NULL;

    return 1;
}

static void loadfonts(void)
{
     #define F(x) (x * SCALE / 2.0)
     font_open(&font[FONT_TEXT], FC_FAMILY, FcTypeString, "Roboto", FC_PIXEL_SIZE, FcTypeDouble, F(12.0), NULL);

     font_open(&font[FONT_TITLE], FC_FAMILY, FcTypeString, "Roboto", FC_PIXEL_SIZE, FcTypeDouble, F(12.0), FC_WEIGHT, FcTypeInteger, FC_WEIGHT_BOLD, NULL);

     font_open(&font[FONT_SELF_NAME], FC_FAMILY, FcTypeString, "Roboto", FC_PIXEL_SIZE, FcTypeDouble, F(14.0), FC_WEIGHT, FcTypeInteger, FC_WEIGHT_BOLD, NULL);
     font_open(&font[FONT_STATUS], FC_FAMILY, FcTypeString, "Roboto", FC_PIXEL_SIZE, FcTypeDouble, F(11.0), NULL);

     font_open(&font[FONT_LIST_NAME], FC_FAMILY, FcTypeString, "Roboto", FC_PIXEL_SIZE, FcTypeDouble, F(12.0), NULL);

     font_open(&font[FONT_MSG], FC_FAMILY, FcTypeString, "Roboto", FC_PIXEL_SIZE, FcTypeDouble, F(11.0), FC_WEIGHT, FcTypeInteger, FC_WEIGHT_LIGHT, NULL);
     font_open(&font[FONT_MSG_NAME], FC_FAMILY, FcTypeString, "Roboto", FC_PIXEL_SIZE, FcTypeDouble, F(10.0), FC_WEIGHT, FcTypeInteger, FC_WEIGHT_LIGHT, NULL);
     font_open(&font[FONT_MISC], FC_FAMILY, FcTypeString, "Roboto", FC_PIXEL_SIZE, FcTypeDouble, F(10.0), NULL);
     font_open(&font[FONT_MSG_LINK], FC_FAMILY, FcTypeString, "Roboto", FC_PIXEL_SIZE, FcTypeDouble, F(11.0), FC_WEIGHT, FcTypeInteger, FC_WEIGHT_LIGHT, NULL);
    #undef F
}

static void freefonts(void)
{
    int i;
    for(i = 0; i != countof(font); i++) {
        FONT *f = &font[i];
        if(f->pattern) {
            FcPatternDestroy(f->pattern);
        }

        if(f->info) {
            FONT_INFO *i = f->info;
            while(i->face) {
                FT_Done_Face(i->face);
                i++;
            }
            free(f->info);
        }

        int j = 0;
        for(j = 0; j != countof(f->glyphs); j++) {
            GLYPH *g = f->glyphs[j];
            if(g) {
                while(g->ucs4 != ~0) {
                    if(g->pic) {
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
