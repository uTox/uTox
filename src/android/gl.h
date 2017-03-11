#include "main.h"

#include "../main.h"

#include <GLES2/gl2.h>
#define _GNU_SOURCE
#include <EGL/egl.h>
#include <fcntl.h>
#include <sys/mman.h>

typedef struct {
    int16_t  x, y;
    uint16_t tx, ty;
} VERTEX2D;

typedef struct { VERTEX2D vertex[4]; } QUAD2D;

void makeglyph(QUAD2D *quad, int16_t x, int16_t y, uint16_t mx, uint16_t my, uint16_t width, uint16_t height);

uint32_t setcolor(uint32_t a);

void drawrect(int x, int y, int right, int bottom, uint32_t color);

void draw_rect_fill(int x, int y, int width, int height, uint32_t color);

void draw_rect_frame(int x, int y, int width, int height, uint32_t color);

void drawhline(int x, int y, int x2, uint32_t color);

void drawvline(int x, int y, int y2, uint32_t color);

void drawalpha(int bm, int x, int y, int width, int height, uint32_t color);

void loadalpha(int bm, void *data, int width, int height);

void pushclip(int left, int top, int w, int h);

void popclip(void);

void enddraw(int x, int y, int width, int height);

bool gl_init(void);

/* gl initialization with EGL */
bool init_display(ANativeWindow *window);

void GL_draw_image(const NATIVE_IMAGE *data, int x, int y, uint32_t width, uint32_t height, uint32_t imgx, uint32_t imgy);

NATIVE_IMAGE *GL_utox_image_to_native(const UTOX_IMAGE data, size_t size, uint16_t *w, uint16_t *h, bool keep_alpha);

int GL_utox_android_redraw_window();

void GL_raze_surface(void);

int GL_drawtext(int x, int xmax, int y, char *str, uint16_t length);

#if 0
void drawimage(NATIVE_IMAGE data, int x, int y, int width, int height, int maxwidth, bool zoom, double position)
{
    GLuint texture = data;

    if(!zoom && width > maxwidth) {
        makequad(&quads[0], x, y, x + maxwidth, y + (height * maxwidth / width));
    } else {
        makequad(&quads[0], x - (int)((double)(width - maxwidth) * position), y, x + width, y + height);
    }

    glBindTexture(GL_TEXTURE_2D, texture);

    float one[] = {1.0, 1.0, 1.0};
    float zero[] = {0.0, 0.0, 0.0};
    glUniform3fv(k, 1, one);
    glUniform3fv(k2, 1, zero);

    glDrawQuads(0, 1);

    glUniform3fv(k2, 1, one);
}
#endif
