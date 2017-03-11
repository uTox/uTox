#include "gl.h"

#include "main.h"
#include "freetype.h"

#include "../debug.h"
#include "../main_native.h"
#include "../settings.h"
#include "../macros.h"

#include "../ui.h"
#include "../ui/svg.h"

const char vertex_shader[] = "uniform vec4 matrix;"
                             "attribute vec2 pos;"
                             "attribute vec2 tex;"
                             "varying vec2 x;"
                             "void main(){"
                             "x = tex / 32768.0;"
                             "gl_Position = vec4((pos + matrix.xy) * matrix.zw, 0.0, 1.0);"
                             "}",
           fragment_shader[] =
#ifndef NO_OPENGL_ES
               "precision mediump float;"
#endif
               "uniform sampler2D samp;"
               "uniform vec3 k;"
               "uniform vec3 k2;"
               "varying vec2 x;"
               "void main(){"
               "gl_FragColor = (texture2D(samp, x) + vec4(k2, 0.0)) * vec4(k, 1.0);"
               "}";

static GLuint prog, white;
static GLint  matrix, k, k2, samp;
static GLuint bitmap[32];

static QUAD2D quads[64];

static EGLDisplay display;
static EGLSurface surface;
static EGLContext context;
static EGLConfig  config;

#ifndef NO_OPENGL_ES
#define glDrawQuads(x, y) glDrawElements(GL_TRIANGLES, (y)*6, GL_UNSIGNED_BYTE, &quad_indices[(x)*6])
static uint8_t quad_indices[384];
#else
#define glDrawQuads(x, y) glDrawArrays(GL_QUADS, (x), 4 * (y))
#endif

static void makequad(QUAD2D *quad, int16_t x, int16_t y, int16_t right, int16_t bottom) {
    quad->vertex[0].x  = x;
    quad->vertex[0].y  = y;
    quad->vertex[0].tx = 0;
    quad->vertex[0].ty = 0;

    quad->vertex[1].x  = right;
    quad->vertex[1].y  = y;
    quad->vertex[1].tx = 32768;
    quad->vertex[1].ty = 0;

    quad->vertex[2].x  = right;
    quad->vertex[2].y  = bottom;
    quad->vertex[2].tx = 32768;
    quad->vertex[2].ty = 32768;

    quad->vertex[3].x  = x;
    quad->vertex[3].y  = bottom;
    quad->vertex[3].tx = 0;
    quad->vertex[3].ty = 32768;
}

static void makeline(QUAD2D *quad, int16_t x, int16_t y, int16_t x2, int16_t y2) {
    quad->vertex[0].x = x;
    quad->vertex[0].y = y;

    quad->vertex[1].x = x2;
    quad->vertex[1].y = y2;
}


void makeglyph(QUAD2D *quad, int16_t x, int16_t y, uint16_t mx, uint16_t my, uint16_t width, uint16_t height) {
    quad->vertex[0].x  = x;
    quad->vertex[0].y  = y;
    quad->vertex[0].tx = mx * 64;
    quad->vertex[0].ty = my * 64;

    quad->vertex[1].x  = x + width;
    quad->vertex[1].y  = y;
    quad->vertex[1].tx = (mx + width) * 64;
    quad->vertex[1].ty = my * 64;

    quad->vertex[2].x  = x + width;
    quad->vertex[2].y  = y + height;
    quad->vertex[2].tx = (mx + width) * 64;
    quad->vertex[2].ty = (my + height) * 64;

    quad->vertex[3].x  = x;
    quad->vertex[3].y  = y + height;
    quad->vertex[3].tx = mx * 64;
    quad->vertex[3].ty = (my + height) * 64;
}

static void set_color(uint32_t a) {
    union {
        uint32_t c;
        struct {
            uint8_t r, g, b, a;
        };
    } color;
    color.c   = a;
    float c[] = { (float)color.r / 255.0, (float)color.g / 255.0, (float)color.b / 255.0 };

    glUniform3fv(k, 1, c);
}

uint32_t colori;
float    colorf[3];

uint32_t setcolor(uint32_t a) {
    union {
        uint32_t c;
        struct {
            uint8_t r, g, b, a;
        };
    } color;
    color.c = a;

    colorf[0] = (float)color.r / 255.0;
    colorf[1] = (float)color.g / 255.0;
    colorf[2] = (float)color.b / 255.0;

    uint32_t s = colori;
    colori     = a;
    return s;
}

void drawrect(int x, int y, int right, int bottom, uint32_t color) {
    set_color(color);
    glBindTexture(GL_TEXTURE_2D, white);
    makequad(&quads[0], x, y, right, bottom);
    glDrawQuads(0, 1);
}

void draw_rect_fill(int x, int y, int width, int height, uint32_t color) {
    drawrect(x, y, x + width, y + height, color);
}

void draw_rect_frame(int x, int y, int width, int height, uint32_t color) {
    set_color(color);
    glBindTexture(GL_TEXTURE_2D, white);
    makequad(&quads[0], x, y, x + width, y + height);
    glDrawArrays(GL_LINE_LOOP, 0, 4);
}

void drawhline(int x, int y, int x2, uint32_t color) {
    set_color(color);
    glBindTexture(GL_TEXTURE_2D, white);
    makeline(&quads[0], x, y + 1, x2, y + 1);
    glDrawArrays(GL_LINES, 0, 2);
}

void drawvline(int x, int y, int y2, uint32_t color) {
    set_color(color);
    glBindTexture(GL_TEXTURE_2D, white);
    makeline(&quads[0], x + 1, y, x + 1, y2);
    glDrawArrays(GL_LINES, 0, 2);
}

void drawalpha(int bm, int x, int y, int width, int height, uint32_t color) {
    set_color(color);
    glBindTexture(GL_TEXTURE_2D, bitmap[bm]);
    makequad(&quads[0], x, y, x + width, y + height);
    glDrawQuads(0, 1);
}

void loadalpha(int bm, void *data, int width, int height) {
    glBindTexture(GL_TEXTURE_2D, bitmap[bm]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, data);
}

typedef struct {
    int16_t  x, y;
    uint16_t width, height;
} RECT;

static RECT clip[16];
static int  clipk;

void pushclip(int left, int top, int w, int h) {
    if (!clipk) {
        glEnable(GL_SCISSOR_TEST);
    }

    RECT *r   = &clip[clipk++];
    r->x      = left;
    r->y      = settings.window_height - (top + h);
    r->width  = w;
    r->height = h;

    glScissor(r->x, r->y, r->width, r->height);
}

void popclip(void) {
    clipk--;
    if (!clipk) {
        glDisable(GL_SCISSOR_TEST);
        return;
    }

    RECT *r = &clip[clipk - 1];

    glScissor(r->x, r->y, r->width, r->height);
}

void enddraw(int x, int y, int width, int height) { eglSwapBuffers(display, surface); }

bool gl_init(void) {
    GLuint        vertshader, fragshader;
    GLint         status;
    const GLchar *data;

    vertshader = glCreateShader(GL_VERTEX_SHADER);
    if (!vertshader) {
        LOG_TRACE("gl", "glCreateShader() failed (vert)" );
        return 0;
    }

    data = &vertex_shader[0];
    glShaderSource(vertshader, 1, &data, NULL);
    glCompileShader(vertshader);
    glGetShaderiv(vertshader, GL_COMPILE_STATUS, &status);
    if (!status) {
#ifdef DEBUG
        LOG_TRACE("gl", "glCompileShader() failed (vert):\n%s" , data);
        GLint infologsize = 0;
        glGetShaderiv(vertshader, GL_INFO_LOG_LENGTH, &infologsize);
        if (infologsize) {
            char *infolog = malloc(infologsize);
            glGetShaderInfoLog(vertshader, infologsize, NULL, (GLbyte *)infolog);
            LOG_TRACE("gl", "Infolog: %s" , infolog);
            free(infolog);
        }
#endif
        return 0;
    }

    fragshader = glCreateShader(GL_FRAGMENT_SHADER);
    if (!fragshader) {
        return 0;
    }

    data = &fragment_shader[0];
    glShaderSource(fragshader, 1, &data, NULL);
    glCompileShader(fragshader);
    glGetShaderiv(fragshader, GL_COMPILE_STATUS, &status);
    if (!status) {
#ifdef DEBUG
        LOG_TRACE("gl", "glCompileShader failed (frag):\n%s" , data);
        GLint infologsize = 0;
        glGetShaderiv(fragshader, GL_INFO_LOG_LENGTH, &infologsize);
        if (infologsize) {
            char *infolog = malloc(infologsize);
            glGetShaderInfoLog(fragshader, infologsize, NULL, (GLbyte *)infolog);
            LOG_TRACE("gl", "Infolog: %s" , infolog);
            free(infolog);
        }
#endif
        return 0;
    }

    prog = glCreateProgram();
    glAttachShader(prog, vertshader);
    glAttachShader(prog, fragshader);
    glBindAttribLocation(prog, 0, "pos");
    glBindAttribLocation(prog, 1, "tex");

    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (!status) {
#ifdef DEBUG
        LOG_TRACE("gl", "glLinkProgram failed" );
        GLint infologsize = 0;
        glGetShaderiv(prog, GL_INFO_LOG_LENGTH, &infologsize);
        if (infologsize) {
            char *infolog = malloc(infologsize);
            glGetShaderInfoLog(prog, infologsize, NULL, (GLbyte *)infolog);
            LOG_TRACE("gl", "Infolog: %s" , infolog);
            free(infolog);
        }
#endif
        return 0;
    }

    glUseProgram(prog);

    matrix = glGetUniformLocation(prog, "matrix");
    k      = glGetUniformLocation(prog, "k");
    k2     = glGetUniformLocation(prog, "k2");
    samp   = glGetUniformLocation(prog, "samp");

    LOG_TRACE("gl", "uniforms: %i %i %i" , matrix, k, samp);

    GLint zero  = 0;
    float one[] = { 1.0, 1.0, 1.0 };
    glUniform1iv(samp, 1, &zero);
    glUniform3fv(k2, 1, one);

    uint8_t wh = { 255 };
    glGenTextures(1, &white);
    glBindTexture(GL_TEXTURE_2D, white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 1, 1, 0, GL_ALPHA, GL_UNSIGNED_BYTE, &wh);

    //
    glVertexAttribPointer(0, 2, GL_SHORT, GL_FALSE, sizeof(VERTEX2D), &quads[0]);
    glVertexAttribPointer(1, 2, GL_UNSIGNED_SHORT, GL_FALSE, sizeof(VERTEX2D), &quads[0].vertex[0].tx);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    // Alpha blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    //
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

#ifndef NO_OPENGL_ES
    uint8_t  i  = 0;
    uint16_t ii = 0;
    do {
        quad_indices[ii]     = i + 0;
        quad_indices[ii + 1] = i + 1;
        quad_indices[ii + 2] = i + 3;
        quad_indices[ii + 3] = i + 3;
        quad_indices[ii + 4] = i + 1;
        quad_indices[ii + 5] = i + 2;
        i += 4;
        ii += 6;
    } while (i);
#endif

    glGenTextures(COUNTOF(bitmap), bitmap);

    svg_draw(0);
    loadfonts();

    float vec[4];
    vec[0] = -(float)settings.window_width / 2.0;
    vec[1] = -(float)settings.window_height / 2.0;
    vec[2] = 2.0 / (float)settings.window_width;
    vec[3] = -2.0 / (float)settings.window_height;
    glUniform4fv(matrix, 1, vec);

    ui_size(settings.window_width, settings.window_height);

    glViewport(0, 0, settings.window_width, settings.window_height);

    redraw();

    return 1;
}

/* gl initialization with EGL */
bool init_display(ANativeWindow *window) {
    const EGLint attrib_list[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

    const EGLint attribs[] = { EGL_SURFACE_TYPE,
                               EGL_WINDOW_BIT,
                               EGL_RENDERABLE_TYPE,
                               EGL_OPENGL_ES2_BIT,
                               EGL_BLUE_SIZE,
                               8,
                               EGL_GREEN_SIZE,
                               8,
                               EGL_RED_SIZE,
                               8,
                               EGL_ALPHA_SIZE,
                               8,
                               EGL_DEPTH_SIZE,
                               0,
                               EGL_NONE };

    EGLint numConfigs;

    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, NULL, NULL);
    eglChooseConfig(display, attribs, &config, 1, &numConfigs);

    EGLint format;
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

    ANativeWindow_setBuffersGeometry(window, 0, 0, format);
    surface = eglCreateWindowSurface(display, config, window, NULL);
    context = eglCreateContext(display, config, NULL, attrib_list);

    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
        return 0;
    }

    int32_t w, h;
    eglQuerySurface(display, surface, EGL_WIDTH, &w);
    eglQuerySurface(display, surface, EGL_HEIGHT, &h);

    settings.window_width  = w;
    settings.window_height = h;

    return gl_init();
}

void GL_draw_image(const NATIVE_IMAGE *data, int x, int y, uint32_t width, uint32_t height, uint32_t imgx, uint32_t imgy) {
    GLuint texture = data;

    makequad(&quads[0], x - imgx, y - imgy, x + width, y + height);

    glBindTexture(GL_TEXTURE_2D, texture);

    float one[]  = { 1.0, 1.0, 1.0 };
    float zero[] = { 0.0, 0.0, 0.0 };
    glUniform3fv(k, 1, one);
    glUniform3fv(k2, 1, zero);

    glDrawQuads(0, 1);

    glUniform3fv(k2, 1, one);
}

NATIVE_IMAGE *GL_utox_image_to_native(const UTOX_IMAGE data, size_t size, uint16_t *w, uint16_t *h, bool keep_alpha) {
    unsigned width, height, bpp;
    uint8_t *out = stbi_load_from_memory(data, size, &width, &height, &bpp, 3);

    if (out == NULL || width == 0 || height == 0) {
        return 0;
    }

    *w = width;
    *h = height;
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, out);
    free(out);

    return texture;
}

int GL_utox_android_redraw_window() {
    int32_t new_width, new_height;
    eglQuerySurface(display, surface, EGL_WIDTH, &new_width);
    eglQuerySurface(display, surface, EGL_HEIGHT, &new_height);

    if (new_width != settings.window_width || new_height != settings.window_height) {
        settings.window_width  = new_width;
        settings.window_height = new_height;

        float vec[4];
        vec[0] = -(float)settings.window_width / 2.0;
        vec[1] = -(float)settings.window_height / 2.0;
        vec[2] = 2.0 / (float)settings.window_width;
        vec[3] = -2.0 / (float)settings.window_height;
        glUniform4fv(matrix, 1, vec);

        ui_size(settings.window_width, settings.window_height);

        glViewport(0, 0, settings.window_width, settings.window_height);

        return 1;
    }
}

void GL_raze_surface(void) {
    // eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(display, context);
    eglDestroySurface(display, surface);
    eglTerminate(display);
}

int GL_drawtext(int x, int xmax, int y, char *str, uint16_t length) {
    glUniform3fv(k, 1, colorf);
    glBindTexture(GL_TEXTURE_2D, sfont->texture);
    int c = 0;

    GLYPH *  g;
    uint8_t  len;
    uint32_t ch;
    while (length) {
        len = utf8_len_read(str, &ch);
        str += len;
        length -= len;

        g = font_getglyph(sfont, ch);
        if (g) {
            if (x + g->xadvance > xmax) {
                x = -x;
                break;
            }

            if (c == 64) {
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
