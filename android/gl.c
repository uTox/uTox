typedef struct
{
    int16_t x, y;
    uint16_t tx, ty;
}VERTEX2D;

typedef struct
{
    VERTEX2D vertex[4];
}QUAD2D;

const char vertex_shader[] =
    "uniform vec4 matrix;"
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
static GLint matrix, k, k2, samp;
static GLuint bitmap[32];

static QUAD2D quads[64];

static EGLDisplay display;
static EGLSurface surface;
static EGLContext context;
static EGLConfig config;

#ifndef NO_OPENGL_ES
#define glDrawQuads(x,y) glDrawElements(GL_TRIANGLES, (y) * 6, GL_UNSIGNED_BYTE, &quad_indices[(x) * 6])
static uint8_t quad_indices[384];
#else
#define glDrawQuads(x,y) glDrawArrays(GL_QUADS, (x), 4 * (y))
#endif

static void makequad(QUAD2D *quad, int16_t x, int16_t y, int16_t right, int16_t bottom)
{
    quad->vertex[0].x = x;
    quad->vertex[0].y = y;
    quad->vertex[0].tx = 0;
    quad->vertex[0].ty = 0;

    quad->vertex[1].x = right;
    quad->vertex[1].y = y;
    quad->vertex[1].tx = 32768;
    quad->vertex[1].ty = 0;

    quad->vertex[2].x = right;
    quad->vertex[2].y = bottom;
    quad->vertex[2].tx = 32768;
    quad->vertex[2].ty = 32768;

    quad->vertex[3].x = x;
    quad->vertex[3].y = bottom;
    quad->vertex[3].tx = 0;
    quad->vertex[3].ty = 32768;
}

static void makeline(QUAD2D *quad, int16_t x, int16_t y, int16_t x2, int16_t y2)
{
    quad->vertex[0].x = x;
    quad->vertex[0].y = y;

    quad->vertex[1].x = x2;
    quad->vertex[1].y = y2;
}


static void makeglyph(QUAD2D *quad, int16_t x, int16_t y, uint16_t mx, uint16_t my, uint16_t width, uint16_t height)
{
    quad->vertex[0].x = x;
    quad->vertex[0].y = y;
    quad->vertex[0].tx = mx * 64;
    quad->vertex[0].ty = my * 64;

    quad->vertex[1].x = x + width;
    quad->vertex[1].y = y;
    quad->vertex[1].tx = (mx + width) * 64;
    quad->vertex[1].ty = my * 64;

    quad->vertex[2].x = x + width;
    quad->vertex[2].y = y + height;
    quad->vertex[2].tx = (mx + width) * 64;
    quad->vertex[2].ty = (my + height) * 64;

    quad->vertex[3].x = x;
    quad->vertex[3].y = y + height;
    quad->vertex[3].tx = mx * 64;
    quad->vertex[3].ty = (my + height) * 64;
}

static void set_color(uint32_t a)
{
    union {
        uint32_t c;
        struct {
            uint8_t r, g, b, a;
        };
    } color;
    color.c = a;
    float c[] = {
        (float)color.r / 255.0, (float)color.g / 255.0, (float)color.b / 255.0
    };

    glUniform3fv(k, 1, c);
}

uint32_t colori;
float colorf[3];

uint32_t setcolor(uint32_t a)
{
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
    colori = a;
    return s;
}

void drawrect(int x, int y, int right, int bottom, uint32_t color)
{
    set_color(color);
    glBindTexture(GL_TEXTURE_2D, white);
    makequad(&quads[0], x, y, right, bottom);
    glDrawQuads(0, 1);
}

void drawrectw(int x, int y, int width, int height, uint32_t color)
{
    drawrect(x, y, x + width, y + height, color);
}

void framerect(int x, int y, int right, int bottom, uint32_t color)
{
    set_color(color);
    glBindTexture(GL_TEXTURE_2D, white);
    makequad(&quads[0], x, y, right, bottom);
    glDrawArrays(GL_LINE_LOOP, 0, 4);
}

void drawhline(int x, int y, int x2, uint32_t color)
{
    set_color(color);
    glBindTexture(GL_TEXTURE_2D, white);
    makeline(&quads[0], x, y + 1, x2, y + 1);
    glDrawArrays(GL_LINES, 0, 2);
}

void drawvline(int x, int y, int y2, uint32_t color)
{
    set_color(color);
    glBindTexture(GL_TEXTURE_2D, white);
    makeline(&quads[0], x + 1, y, x + 1, y2);
    glDrawArrays(GL_LINES, 0, 2);
}

void drawalpha(int bm, int x, int y, int width, int height, uint32_t color)
{
    set_color(color);
    glBindTexture(GL_TEXTURE_2D, bitmap[bm]);
    makequad(&quads[0], x, y, x + width, y + height);
    glDrawQuads(0, 1);
}

void loadalpha(int bm, void *data, int width, int height)
{
    glBindTexture(GL_TEXTURE_2D, bitmap[bm]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, data);
}

typedef struct {
    int16_t x, y;
    uint16_t width, height;
} RECT;

static RECT clip[16];
static int clipk;

void pushclip(int left, int top, int w, int h)
{
    if(!clipk) {
        glEnable(GL_SCISSOR_TEST);
    }

    RECT *r = &clip[clipk++];
    r->x = left;
    r->y = height - (top + h);
    r->width = w;
    r->height = h;

    glScissor(r->x, r->y, r->width, r->height);
}

void popclip(void)
{
    clipk--;
    if(!clipk) {
        glDisable(GL_SCISSOR_TEST);
        return;
    }

    RECT *r = &clip[clipk - 1];

    glScissor(r->x, r->y, r->width, r->height);
}

void enddraw(int x, int y, int width, int height)
{
    eglSwapBuffers(display, surface);
}

_Bool gl_init(void)
{
    GLuint vertshader, fragshader;
    GLint status;
    const GLchar *data;

    vertshader = glCreateShader(GL_VERTEX_SHADER);
    if(!vertshader) {
        debug("glCreateShader() failed (vert)\n");
        return 0;
    }

    data = &vertex_shader[0];
    glShaderSource(vertshader, 1, &data, NULL);
    glCompileShader(vertshader);
    glGetShaderiv(vertshader, GL_COMPILE_STATUS, &status);
    if(!status) {
        #ifdef DEBUG
        debug("glCompileShader() failed (vert):\n%s\n", data);
        GLint infologsize = 0;
        glGetShaderiv(vertshader, GL_INFO_LOG_LENGTH, &infologsize);
        if(infologsize)
        {
            char* infolog = malloc(infologsize);
            glGetShaderInfoLog(vertshader, infologsize, NULL, (GLbyte*)infolog);
            debug("Infolog: %s\n", infolog);
            free(infolog);
        }
        #endif
        return 0;
    }

    fragshader = glCreateShader(GL_FRAGMENT_SHADER);
    if(!fragshader)
    {
        return 0;
    }

    data = &fragment_shader[0];
    glShaderSource(fragshader, 1, &data, NULL);
    glCompileShader(fragshader);
    glGetShaderiv(fragshader, GL_COMPILE_STATUS, &status);
    if(!status)
    {
        #ifdef DEBUG
        debug("glCompileShader failed (frag):\n%s\n", data);
        GLint infologsize = 0;
        glGetShaderiv(fragshader, GL_INFO_LOG_LENGTH, &infologsize);
        if(infologsize)
        {
            char* infolog = malloc(infologsize);
            glGetShaderInfoLog(fragshader, infologsize, NULL, (GLbyte*)infolog);
            debug("Infolog: %s\n", infolog);
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
    if(!status)
    {
        #ifdef DEBUG
        debug("glLinkProgram failed\n");
        GLint infologsize = 0;
        glGetShaderiv(prog, GL_INFO_LOG_LENGTH, &infologsize);
        if(infologsize)
        {
            char* infolog = malloc(infologsize);
            glGetShaderInfoLog(prog, infologsize, NULL, (GLbyte*)infolog);
            debug("Infolog: %s\n", infolog);
            free(infolog);
        }
        #endif
        return 0;
    }

    glUseProgram(prog);

    matrix = glGetUniformLocation(prog, "matrix");
    k = glGetUniformLocation(prog, "k");
    k2 = glGetUniformLocation(prog, "k2");
    samp = glGetUniformLocation(prog, "samp");

    debug("uniforms: %i %i %i\n", matrix, k, samp);

    GLint zero = 0;
    float one[] = {1.0, 1.0, 1.0};
    glUniform1iv(samp, 1, &zero);
    glUniform3fv(k2, 1, one);

    uint8_t wh = {255};
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

    //Alpha blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    //
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    #ifndef NO_OPENGL_ES
    uint8_t i = 0;
    uint16_t ii = 0;
    do {
        quad_indices[ii] = i + 0;
        quad_indices[ii + 1] = i + 1;
        quad_indices[ii + 2] = i + 3;
        quad_indices[ii + 3] = i + 3;
        quad_indices[ii + 4] = i + 1;
        quad_indices[ii + 5] = i + 2;
        i += 4;
        ii += 6;
    } while(i);
    #endif

    glGenTextures(countof(bitmap), bitmap);

    svg_draw(0);
    loadfonts();

    float vec[4];
    vec[0] = -(float)width / 2.0;
    vec[1] = -(float)height / 2.0;
    vec[2] = 2.0 / (float)width;
    vec[3] = -2.0 / (float)height;
    glUniform4fv(matrix, 1, vec);

    ui_size(width, height);

    glViewport(0, 0, width, height);

    redraw();

    return 1;
}
