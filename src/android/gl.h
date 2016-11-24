#ifndef ANDROID_GL_H
#define ANDROID_GL_H

uint32_t colori;
float    colorf[3];

// OpenGL types
typedef struct {
    int16_t  x, y;
    uint16_t tx, ty;
} VERTEX2D;

typedef struct { VERTEX2D vertex[4]; } QUAD2D;

typedef struct {
    int16_t  x, y;
    uint16_t width, height;
} RECT;

RECT clip[16];
int  clipk;

QUAD2D quads[64];

GLuint prog, white;
GLint  matrix, k, k2, samp;
GLuint bitmap[32];

EGLDisplay display;
EGLSurface GL_surface;
EGLContext GL_context;
EGLConfig  config;

#ifndef NO_OPENGL_ES
#define glDrawQuads(x, y) glDrawElements(GL_TRIANGLES, (y)*6, GL_UNSIGNED_BYTE, &quad_indices[(x)*6])
uint8_t quad_indices[384];
#else
#define glDrawQuads(x, y) glDrawArrays(GL_QUADS, (x), 4 * (y))
#endif


#define PIXELS(x) (((x) + 32) / 64)

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

FT_Library ftlib;
FONT       font[16], *sfont;

#endif
