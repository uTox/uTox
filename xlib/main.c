#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <X11/Xatom.h>
#include <X11/X.h>
#include <X11/cursorfont.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <X11/extensions/Xrender.h>
#include <ft2build.h>
#include <fontconfig/fontconfig.h>
#include <fontconfig/fcfreetype.h>

#include <X11/extensions/XShm.h>
#include <sys/shm.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <pthread.h>
#include <unistd.h>

#include <dlfcn.h>

#include "audio.c"
#include "v4l.c"

#if !(defined(__APPLE__) || defined(NO_DBUS))
#define HAVE_DBUS
#include "dbus.c"
#endif

#include "keysym2ucs.c"

#ifdef __APPLE__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#define DEFAULT_WIDTH (382 * DEFAULT_SCALE)
#define DEFAULT_HEIGHT (320 * DEFAULT_SCALE)

Display *display;
int screen;
Window window;
GC gc;
Colormap cmap;
Visual *visual;

Picture bitmap[32];

Cursor cursors[8];

uint8_t pointergrab;
int grabx, graby, grabpx, grabpy;
GC grabgc;

XSizeHints *xsh;

_Bool havefocus = 0;

Window video_win[256];

Atom wm_protocols, wm_delete_window;

uint32_t scolor;

Atom XA_CLIPBOARD, XA_UTF8_STRING, targets, XA_INCR;
Atom XdndAware, XdndEnter, XdndLeave, XdndPosition, XdndStatus, XdndDrop, XdndSelection, XdndDATA, XdndActionCopy;
Atom XA_URI_LIST, XA_PNG_IMG;

Pixmap drawbuf;
Picture renderpic;
Picture colorpic;

_Bool _redraw;

uint16_t drawwidth, drawheight;

XImage *screen_image;

/* pointers to dynamically loaded libs */
void *libgtk;
#include "gtk.c"

#include "freetype.c"

struct
{
    int len;
    uint8_t data[65536];
}clipboard;

struct
{
    int len;
    uint8_t data[65536];
}primary;

struct
{
    int len, left;
    Atom type;
    void *data;
} pastebuf;

static void setclipboard(void)
{
    XSetSelectionOwner(display, XA_CLIPBOARD, window, CurrentTime);
}

/*static XftFont* getfont(XftFont **font, uint32_t ch)
{
    XftFont *first = font[0];
    if(!FcCharSetHasChar(charset, ch)) {
        return first;
    }

    while(*font) {
        if(XftGlyphExists(display, *font, ch)) {
            return *font;
        }
        font++;
    }

    FcResult result;
    int i;
    for(i = 0; i != fs->nfont; i++) {
        FcCharSet *cs;
        result = FcPatternGetCharSet(fs->fonts[i], FC_CHARSET, 0, &cs);
        if(FcCharSetHasChar(cs, ch)) {
            FcPattern *p = FcPatternDuplicate(fs->fonts[i]), *pp;

            double size;
            if(!FcPatternGetDouble(first->pattern, FC_PIXEL_SIZE, 0, &size)) {
                FcPatternAddDouble(p, FC_PIXEL_SIZE, size);
            }

            pp = XftFontMatch(display, screen, p, &result);
            *font = XftFontOpenPattern(display, pp);
            FcPatternDestroy(p);
            return *font;
        }
    }

    //should never happen
    return first;
}*/

void postmessage(uint32_t msg, uint16_t param1, uint16_t param2, void *data)
{
    XEvent event = {
        .xclient = {
            .window = 0,
            .type = ClientMessage,
            .message_type = msg,
            .format = 8,
            .data = {
                .s = {param1, param2}
            }
        }
    };

    memcpy(&event.xclient.data.s[2], &data, sizeof(void*));

    XSendEvent(display, window, False, 0, &event);
    XFlush(display);
}

void drawalpha(int bm, int x, int y, int width, int height, uint32_t color)
{
    XRenderColor xrcolor = {
        .red = ((color >> 8) & 0xFF00) | 0x80,
        .green = ((color) & 0xFF00) | 0x80,
        .blue = ((color << 8) & 0xFF00) | 0x80,
        .alpha = 0xFFFF
    };

    Picture src = XRenderCreateSolidFill(display, &xrcolor);

    XRenderComposite(display, PictOpOver, src, bitmap[bm], renderpic, 0, 0, 0, 0, x, y, width, height);

    XRenderFreePicture(display, src);
}

void drawimage(void *data, int x, int y, int width, int height, int maxwidth, _Bool zoom, double position)
{
    Picture bm = (Picture)data;
    if(!zoom && width > maxwidth) {
        uint32_t v = (width * 65536) / maxwidth;
        XTransform trans = {
            {{v, 0, 0},
            {0, v, 0},
            {0, 0, 65536}}
        };
        XRenderSetPictureTransform(display, bm, &trans);
        XRenderSetPictureFilter(display, bm, FilterBilinear, NULL, 0);

        XRenderComposite(display, PictOpSrc, bm, None, renderpic, 0, 0, 0, 0, x, y, maxwidth, height * maxwidth / width);

        XTransform trans2 = {
            {{65536, 0, 0},
            {0, 65536, 0},
            {0, 0, 65536}}
        };
        XRenderSetPictureFilter(display, bm, FilterNearest, NULL, 0);
        XRenderSetPictureTransform(display, bm, &trans2);
    } else {
        if(width > maxwidth) {
            XRenderComposite(display, PictOpSrc, bm, None, renderpic, (int)((double)(width - maxwidth) * position), 0, 0, 0, x, y, maxwidth, height);
        } else {
            XRenderComposite(display, PictOpSrc, bm, None, renderpic, 0, 0, 0, 0, x, y, width, height);
        }
    }
}

static int _drawtext(int x, int xmax, int y, uint8_t *str, uint16_t length)
{
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
                return -x;
            }

            if(g->pic) {
                XRenderComposite(display, PictOpOver, colorpic, g->pic, renderpic, 0, 0, 0, 0, x + g->x, y + g->y, g->width, g->height);
            }
            x += g->xadvance;
        }
    }

    return x;
}

#include "../shared/freetype-text.c"

void framerect(int x, int y, int right, int bottom, uint32_t color)
{
    XSetForeground(display, gc, color);
    XDrawRectangle(display, drawbuf, gc, x, y, right - x - 1, bottom - y - 1);
}

void drawrect(int x, int y, int right, int bottom, uint32_t color)
{
    XSetForeground(display, gc, color);
    XFillRectangle(display, drawbuf, gc, x, y, right - x, bottom - y);
}

void drawrectw(int x, int y, int width, int height, uint32_t color)
{
    XSetForeground(display, gc, color);
    XFillRectangle(display, drawbuf, gc, x, y, width, height);
}

void drawhline(int x, int y, int x2, uint32_t color)
{
    XSetForeground(display, gc, color);
    XDrawLine(display, drawbuf, gc, x, y, x2, y);
}

void drawvline(int x, int y, int y2, uint32_t color)
{
    XSetForeground(display, gc, color);
    XDrawLine(display, drawbuf, gc, x, y, x, y2);
}

uint32_t setcolor(uint32_t color)
{
    XRenderColor xrcolor;
    xrcolor.red = ((color >> 8) & 0xFF00) | 0x80;
    xrcolor.green = ((color) & 0xFF00) | 0x80;
    xrcolor.blue = ((color << 8) & 0xFF00) | 0x80;
    xrcolor.alpha = 0xFFFF;

    XRenderFreePicture(display, colorpic);
    colorpic = XRenderCreateSolidFill(display, &xrcolor);

    uint32_t old = scolor;
    scolor = color;
    //xftcolor.pixel = color;
    XSetForeground(display, gc, color);
    return old;
}

static XRectangle clip[16];
static int clipk;

void pushclip(int left, int top, int width, int height)
{
    if(!clipk) {
        //XSetClipMask(display, gc, drawbuf);
    }

    XRectangle *r = &clip[clipk++];
    r->x = left;
    r->y = top;
    r->width = width;
    r->height = height;

    XSetClipRectangles(display, gc, 0, 0, r, 1, Unsorted);
    XRenderSetPictureClipRectangles(display, renderpic, 0, 0, r, 1);
}

void popclip(void)
{
    clipk--;
    if(!clipk) {
        XSetClipMask(display, gc, None);

        XRenderPictureAttributes pa;
        pa.clip_mask = None;
        XRenderChangePicture(display, renderpic, CPClipMask, &pa);
        return;
    }

    XRectangle *r = &clip[clipk - 1];

    XSetClipRectangles(display, gc, 0, 0, r, 1, Unsorted);
    XRenderSetPictureClipRectangles(display, renderpic, 0, 0, r, 1);
}

void enddraw(int x, int y, int width, int height)
{
    XCopyArea(display, drawbuf, window, gc, x, y, width, height, x, y);
}

void thread(void func(void*), void *args)
{
    pthread_t thread_temp;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 1 << 18);
    pthread_create(&thread_temp, &attr, (void*(*)(void*))func, args);
    pthread_attr_destroy(&attr);
}

void yieldcpu(uint32_t ms)
{
    usleep(1000 * ms);
}

uint64_t get_time(void)
{
    struct timespec ts;
    #ifdef CLOCK_MONOTONIC_RAW
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    #elif defined(__APPLE__)
    clock_serv_t muhclock;
    mach_timespec_t machtime;
    host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &muhclock);
    clock_get_time(muhclock, &machtime);
    mach_port_deallocate(mach_task_self(), muhclock);
    ts.tv_sec = machtime.tv_sec;
    ts.tv_nsec = machtime.tv_nsec;
    #else
    clock_gettime(CLOCK_MONOTONIC, &ts);
    #endif

    return ((uint64_t)ts.tv_sec * (1000 * 1000 * 1000)) + (uint64_t)ts.tv_nsec;
}


void address_to_clipboard(void)
{
    memcpy(clipboard.data, self.id, sizeof(self.id));
    clipboard.len = sizeof(self.id);
    setclipboard();

    setselection(self.id, sizeof(self.id));
}

void openurl(char_t *str)
{
    char cmd[1024], *p = cmd;

    #ifdef __APPLE__
    p += sprintf(p, "open \"");
    #else
    p += sprintf(p, "xdg-open \"");
    #endif

    while(*str) {
        if(*str == '"' || *str == '\\') {
            *p++ = '\\';
        }
        *p++ = *str++;
    }
    *p++ = '\"';
    *p++ = ' ';
    *p++ = '&';
    *p = 0;

    debug("cmd: %s\n", cmd);
    system(cmd);
}

void openfilesend(void)
{
    if(libgtk) {
        gtk_openfilesend();
    }
}

void savefilerecv(uint32_t fid, MSG_FILE *file)
{
    if(libgtk) {
        gtk_savefilerecv(fid, file);
    } else {
        //fall back to working dir
        char *path = malloc(file->name_length + 1);
        memcpy(path, file->name, file->name_length);
        path[file->name_length] = 0;

        tox_postmessage(TOX_ACCEPTFILE, fid, file->filenumber, path);
    }
}

void savefiledata(MSG_FILE *file)
{
    if(libgtk) {
        gtk_savefiledata(file);
    } else {
        //fall back to working dir inline.png
        FILE *fp = fopen("inline.png", "wb");
        if(fp) {
            fwrite(file->path + ((file->flags & 1) ? 4 : 0), file->size, 1, fp);
            fclose(fp);

            free(file->path);
            file->path = (uint8_t*)strdup("inline.png");
            file->inline_png = 0;
        }
    }
}

void setselection(uint8_t *data, uint16_t length)
{
    if(!length) {
        return;
    }

    memcpy(primary.data, data, length);
    primary.len = length;
    XSetSelectionOwner(display, XA_PRIMARY, window, CurrentTime);
}

static void pasteprimary(void)
{
    Window owner = XGetSelectionOwner(display, XA_PRIMARY);
    if(owner) {
        XConvertSelection(display, XA_PRIMARY, XA_UTF8_STRING, targets, window, CurrentTime);
    }
}

void copy(int value)
{
    if(edit_active()) {
        clipboard.len = edit_copy(clipboard.data, sizeof(clipboard.data));
    } else if(sitem->item == ITEM_FRIEND) {
        clipboard.len = messages_selection(&messages_friend, clipboard.data, sizeof(clipboard.data), value);
    } else {
        clipboard.len = messages_selection(&messages_group, clipboard.data, sizeof(clipboard.data), value);
    }
    setclipboard();
}

void paste(void)
{
    Window owner = XGetSelectionOwner(display, XA_CLIPBOARD);

    /* Ask owner for supported types */
    if (owner) {
        XEvent event = {
            .xselectionrequest = {
                .type = SelectionRequest,
                .send_event = True,
                .display = display,
                .owner = owner,
                .requestor = window,
                .target = targets,
                .selection = XA_CLIPBOARD,
                .property = XA_ATOM,
                .time = CurrentTime
            }
        };

        XSendEvent(display, owner, 0, NoEventMask, &event);
        XFlush(display);
    }
}

static void pastebestformat(const Atom atoms[], int len, Atom selection)
{
    const Atom supported[] = {XA_PNG_IMG, XA_URI_LIST, XA_UTF8_STRING};
    int i, j;
    for (i = 0; i < len; i++) {
        debug("Supported type: %s\n", XGetAtomName(display, atoms[i]));
    }

    for (i = 0; i < len; i++) {
        for (j = 0; j < countof(supported); j++) {
            if (atoms[i] == supported[j]) {
                XConvertSelection(display, selection, supported[j], targets, window, CurrentTime);
                return;
            }
        }
    }
}

static _Bool ishexdigit(char c)
{
    c = toupper(c);
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F');
}

static char hexdecode(char upper, char lower)
{
    upper = toupper(upper);
    lower = toupper(lower);
    return (upper >= 'A' ? upper - 'A' + 10 : upper - '0') * 16 +
        (lower >= 'A' ? lower - 'A' + 10 : lower - '0');
}

static void formaturilist(char *out, const char *in, int len) {
    int i, removed = 0, start = 0;

    for (i = 0; i < len; i++) {
        //Replace CRLF with LF
        if (in[i] == '\r') {
            memcpy(out + start - removed, in + start, i - start);
            start = i + 1;
            removed++;
        } else if (in[i] == '%' && i + 2 < len && ishexdigit(in[i+1]) && ishexdigit(in[i+2])) {
            memcpy(out + start - removed, in + start, i - start);
            out[i - removed] = hexdecode(in[i+1], in[i+2]);
            start = i + 3;
            removed += 2;
        }
    }
    if (start != len) {
        memcpy(out + start - removed, in + start, len - start);
    }

    out[len - removed] = 0;
    out[len - removed - 1] = '\n';
}

static void pastedata(void *data, Atom type, int len, _Bool select)
{
   if (type == XA_PNG_IMG) {
        uint16_t width, height;
        uint32_t size = len;

        void *pngdata = malloc(len + 4);
        void *img = png_to_image(data, &width, &height, len);
        if (img) {
            debug("Pasted image: %dx%d\n", width, height);

            memcpy(pngdata, &size, 4);
            memcpy(pngdata + 4, data, len);
            friend_sendimage((FRIEND*)sitem->data, img, pngdata, width, height);
        }
    } else if (type == XA_URI_LIST) {
        char *path = malloc(len + 2);
        formaturilist(path, (char*) data, len);
        tox_postmessage(TOX_SENDFILES, (FRIEND*)sitem->data - friend, 0xFFFF, path);
    } else if(type == XA_UTF8_STRING && edit_active()) {
        edit_paste(data, len, select);
    }
}

void loadalpha(int bm, void *data, int width, int height)
{
    Pixmap pixmap = XCreatePixmap(display, window, width, height, 8);
    XImage *img = XCreateImage(display, CopyFromParent, 8, ZPixmap, 0, data, width, height, 8, 0);
    GC legc = XCreateGC(display, pixmap, 0, NULL);
    XPutImage(display, pixmap, legc, img, 0, 0, 0, 0, width, height);

    Picture picture = XRenderCreatePicture(display, pixmap, XRenderFindStandardFormat(display, PictStandardA8), 0, NULL);

    XFreeGC(display, legc);
    XFreePixmap(display, pixmap);

    bitmap[bm] = picture;
}

static Picture image_to_picture(XImage *img)
{
    Pixmap pixmap = XCreatePixmap(display, window, img->width, img->height, 24);
    GC legc = XCreateGC(display, pixmap, 0, NULL);
    XPutImage(display, pixmap, legc, img, 0, 0, 0, 0, img->width, img->height);

    Picture picture = XRenderCreatePicture(display, pixmap, XRenderFindVisualFormat(display, visual), 0, NULL);

    XFreeGC(display, legc);
    XFreePixmap(display, pixmap);
    //XDestroyImage(img);

    return picture;
}

void* png_to_image(void *data, uint16_t *w, uint16_t *h, uint32_t size)
{
    uint8_t *out;
    unsigned width, height;
    unsigned r = lodepng_decode32(&out, &width, &height, data, size);
    //free(data);

    if(r != 0 || !width || !height) {
        return NULL;
    }

    uint8_t red, blue, green;
    uint8_t *p, *end = out + width * height * 4;
    uint32_t *temp;

    for (p = out; p != end; p += 4) {
        red = p[0] & 0xFF;
        green = p[1] & 0xFF;
        blue = p[2] & 0xFF;

        temp = (uint32_t *)p;
        *temp = (red | (red << 8) | (red << 16) | (red << 24)) & visual->red_mask;
        *temp |= (blue | (blue << 8) | (blue << 16) | (blue << 24)) & visual->blue_mask;
        *temp |= (green | (green << 8) | (green << 16) | (green << 24)) & visual->green_mask;
    }

    *w = width;
    *h = height;
    XImage *img = XCreateImage(display, visual, 24, ZPixmap, 0, (char*)out, width, height, 32, width * 4);
    Picture picture = image_to_picture(img);
    free(out);

    return (void*)picture;
}

void* loadsavedata(uint32_t *len)
{
    char *home = getenv("HOME");
    char path[256];
    sprintf(path, "%.230s/.config/tox/data", home);

    void *data;
    if((data = file_raw("tox_save", len))) {
        return data;
    }

    return file_raw(path, len);
}

void writesavedata(void *data, uint32_t len)
{
    char *home = getenv("HOME");
    char path[256];
    int l = sprintf(path, "%.230s/.config/tox/data", home);
    path[l - 5] = 0;
    mkdir(path, 0700);
    path[l - 5] = '/';

    FILE *file;
    /*file = fopen(path, "wb");
    if(file) {
        fwrite(data, len, 1, file);
        fclose(file);
    }*/

    file = fopen("tox_save", "wb");
    if(file) {
        fwrite(data, len, 1, file);
        fclose(file);
    }
}

void setscale(void)
{
    int i;
    for(i = 0; i != countof(bitmap); i++) {
        if(bitmap[i]) {
            XRenderFreePicture(display, bitmap[i]);
        }
    }

    svg_draw(0);

    freefonts();
    loadfonts();

    font_small_lineheight = (font[FONT_TEXT].info[0].face->size->metrics.height + (1 << 5)) >> 6;
    //font_msg_lineheight = (font[FONT_MSG].info[0].face->size->metrics.height + (1 << 5)) >> 6;

    if(xsh) {
        XFree(xsh);
    }

    xsh = XAllocSizeHints();
    xsh->flags = PMinSize;
    xsh->min_width = 320 * SCALE;
    xsh->min_height = 160 * SCALE;

    XSetWMNormalHints(display, window, xsh);

    if(width > 320 * SCALE && height > 160 * SCALE)
    {
        /* wont get a resize event, call this manually */
        ui_size(width, height);
    }
}

void notify(uint8_t *title, uint16_t title_length, uint8_t *msg, uint16_t msg_length)
{
    if(havefocus) {
        return;
    }

    XWMHints hints = {.flags = 256};
    XSetWMHints(display, window, &hints);

    #ifdef HAVE_DBUS
    uint8_t *str = tohtml(msg, msg_length);

    dbus_notify((char*)title, (char*)str);

    free(str);
    #endif
}

void showkeyboard(_Bool show)
{

}

void redraw(void)
{
    _redraw = 1;
}

#include "event.c"

int depth;
Screen *scr;

static UTOX_SAVE* loadconfig(void)
{
    UTOX_SAVE *r = file_raw("utox_save", NULL);
    if(r) {
        if(r->version == 0) {
            return r;
        } else {
            free(r);
            r = NULL;
        }
    }

    r = malloc(sizeof(UTOX_SAVE));
    r->version = 0;
    r->scale = DEFAULT_SCALE - 1;
    r->window_x = 0;
    r->window_y = 0;
    r->window_width = DEFAULT_WIDTH;
    r->window_height = DEFAULT_HEIGHT;
    return r;
}

static int systemlang(void)
{
    char *str = getenv("LANG");
    if(!str) {
        return LANG_EN;
    }
    if(strstr(str, "fr")) {
        return LANG_FR;
    }
    if(strstr(str, "es")) {
        return LANG_ES;
    }
    if(strstr(str, "de")) {
        return LANG_DE;
    }
    if(strstr(str, "ru")) {
        return LANG_RU;
    }
    if(strstr(str, "pl")) {
        return LANG_PL;
    }

    return LANG_EN;
}

int main(int argc, char *argv[])
{
    if(argc == 2 && argv[1]) {
        if(!strcmp(argv[1], "--version")) {
            debug("%s\n", VERSION);
            return 0;
        }
    }

    XInitThreads();

    if((display = XOpenDisplay(NULL)) == NULL) {
        printf("Cannot open display\n");
        return 1;
    }

    screen = DefaultScreen(display);
    cmap = DefaultColormap(display, screen);
    visual = DefaultVisual(display, screen);
    gc = DefaultGC(display, screen);
    depth = DefaultDepth(display, screen);
    scr = DefaultScreenOfDisplay(display);

    XSetWindowAttributes attrib = {
        .background_pixel = WhitePixel(display, screen),
        .border_pixel = BlackPixel(display, screen),
        .event_mask = ExposureMask | ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask |
                    PointerMotionMask | StructureNotifyMask | KeyPressMask | KeyReleaseMask | FocusChangeMask |
                    PropertyChangeMask,
    };

    /* load save data */
    UTOX_SAVE *save = loadconfig();

    /* create window */
    window = XCreateWindow(display, RootWindow(display, screen), save->window_x, save->window_y, save->window_width, save->window_height, 0, depth, InputOutput, visual, CWBackPixmap | CWBorderPixel | CWEventMask, &attrib);

    /* start the tox thread */
    thread(tox_thread, NULL);

    /* load atoms */
    wm_protocols = XInternAtom(display, "WM_PROTOCOLS", 0);
    wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", 0);

    XA_CLIPBOARD = XInternAtom(display, "CLIPBOARD", 0);
    XA_UTF8_STRING = XInternAtom(display, "UTF8_STRING", 1);
    if(XA_UTF8_STRING == None) {
        XA_UTF8_STRING = XA_STRING;
    }
    targets = XInternAtom(display, "TARGETS", 0);

    XA_INCR = XInternAtom(display, "INCR", False);

    XdndAware = XInternAtom(display, "XdndAware", False);
    XdndEnter = XInternAtom(display, "XdndEnter", False);
    XdndLeave = XInternAtom(display, "XdndLeave", False);
    XdndPosition = XInternAtom(display, "XdndPosition", False);
    XdndStatus = XInternAtom(display, "XdndStatus", False);
    XdndDrop = XInternAtom(display, "XdndDrop", False);
    XdndSelection = XInternAtom(display, "XdndSelection", False);
    XdndDATA = XInternAtom(display, "XdndDATA", False);
    XdndActionCopy = XInternAtom(display, "XdndActionCopy", False);

    XA_URI_LIST = XInternAtom(display, "text/uri-list", False);
    XA_PNG_IMG = XInternAtom(display, "image/png", False);

    /* create the draw buffer */
    drawbuf = XCreatePixmap(display, window, DEFAULT_WIDTH, DEFAULT_HEIGHT, depth);

    /* catch WM_DELETE_WINDOW */
    XSetWMProtocols(display, window, &wm_delete_window, 1);

    /* set WM_CLASS */
    XClassHint hint = {
        .res_name = "utox",
        .res_class = "utox"
    };

    XSetClassHint(display, window, &hint);

    /* set drag and drog version */
    Atom dndversion = 3;
    XChangeProperty(display, window, XdndAware, XA_ATOM, 32, PropModeReplace, (uint8_t*)&dndversion, 1);

    /* set the window name */
    XSetStandardProperties(display, window, "uTox", "uTox", None, argv, argc, None);

    /* choose available libraries for optional UI stuff */
    if(!(libgtk = gtk_load())) {
        //try Qt
    }

    /* initialize fontconfig */
    initfonts();

    /* load fonts and scalable bitmaps */
    dropdown_dpi.selected = dropdown_dpi.over = save->scale;
    ui_scale(save->scale + 1);

    /* done with save */
    free(save);

    /* load the used cursors */
    cursors[CURSOR_NONE] = XCreateFontCursor(display, XC_left_ptr);
    cursors[CURSOR_HAND] = XCreateFontCursor(display, XC_hand2);
    cursors[CURSOR_TEXT] = XCreateFontCursor(display, XC_xterm);
    cursors[CURSOR_SELECT] = XCreateFontCursor(display, XC_crosshair);
    cursors[CURSOR_ZOOM_IN] = XCreateFontCursor(display, XC_target);
    cursors[CURSOR_ZOOM_OUT] = XCreateFontCursor(display, XC_target);

    /* */
    XGCValues gcval;
    gcval.foreground = XWhitePixel(display, 0);
    gcval.function = GXxor;
    gcval.background = XBlackPixel(display, 0);
    gcval.plane_mask = gcval.background ^ gcval.foreground;
    gcval.subwindow_mode = IncludeInferiors;

    grabgc = XCreateGC(display, RootWindow(display, screen), GCFunction | GCForeground | GCBackground | GCSubwindowMode, &gcval);

    /* Xft draw context/color */
    renderpic = XRenderCreatePicture (display, drawbuf, XRenderFindStandardFormat(display, PictStandardRGB24), 0, NULL);

    XRenderColor xrcolor = {0};
    colorpic = XRenderCreateSolidFill(display, &xrcolor);

    /*xftdraw = XftDrawCreate(display, drawbuf, visual, cmap);
    XRenderColor xrcolor;
    xrcolor.red = 0x0;
    xrcolor.green = 0x0;
    xrcolor.blue = 0x0;
    xrcolor.alpha = 0xffff;
    XftColorAllocValue(display, visual, cmap, &xrcolor, &xftcolor);*/

    LANG = systemlang();
    dropdown_language.selected = LANG;

    /* set-up desktop video input */
    dropdown_add(&dropdown_video, (uint8_t*)"None", NULL);
    dropdown_add(&dropdown_video, (uint8_t*)"Desktop", (void*)1);

    /* make the window visible */
    XMapWindow(display, window);

    /* set the width/height of the drawing region */
    width = DEFAULT_WIDTH;
    height = DEFAULT_HEIGHT;
    ui_size(width, height);

    /* wait for the tox thread to finish initializing */
    while(!tox_thread_init) {
        yieldcpu(1);
    }

    /* set up the contact list */
    list_start();

    /* draw */
    panel_draw(&panel_main, 0, 0, width, height);

    /* event loop */
    while(1) {
        /* block on the first event, then process all events */
        XEvent event;

        XNextEvent(display, &event);
        if(!doevent(event)) {
            break;
        }

        while(XPending(display)) {
            XNextEvent(display, &event);
            if(!doevent(event)) {
                goto BREAK;
            }
        }

        if(_redraw) {
            panel_draw(&panel_main, 0, 0, width, height);
            _redraw = 0;
        }
    }
    BREAK:

    toxaudio_postmessage(AUDIO_KILL, 0, 0, NULL);
    toxvideo_postmessage(VIDEO_KILL, 0, 0, NULL);
    tox_postmessage(TOX_KILL, 0, 0, NULL);

    /* free client thread stuff */
    if(libgtk) {

    }

    Window root_return, child_return;
    int x_return, y_return;
    unsigned int width_return, height_return, i;
    XGetGeometry(display, window, &root_return, &x_return, &y_return, &width_return, &height_return, &i, &i);

    XTranslateCoordinates(display, window, root_return, 0, 0, &x_return, &y_return, &child_return);

    UTOX_SAVE d = {
        .version = 0,
        .scale = SCALE - 1,
        .window_x = x_return < 0 ? 0 : x_return,
        .window_y = y_return < 0 ? 0 : y_return,
        .window_width = width_return,
        .window_height = height_return,
    };

    FILE *file = fopen("utox_save", "wb");
    if(file) {
        fwrite(&d, sizeof(d), 1, file);
        fclose(file);
    }

    FcFontSetSortDestroy(fs);
    freefonts();

    XFreePixmap(display, drawbuf);

    XFreeGC(display, grabgc);

    XRenderFreePicture(display, renderpic);
    XRenderFreePicture(display, colorpic);

    XDestroyWindow(display, window);
    XCloseDisplay(display);

    /* wait for threads to exit */
    while(tox_thread_init) {
        yieldcpu(1);
    }

    debug("clean exit\n");

    return 0;
}

void video_frame(uint32_t id, uint8_t *img_data, uint16_t width, uint16_t height, _Bool resize)
{
    if(!video_win[id]) {
        debug("frame for null window\n");
        return;
    }

    if(resize) {
        XWindowChanges changes = {
            .width = width,
            .height = height
        };
        XConfigureWindow(display, video_win[id], CWWidth | CWHeight, &changes);
    }

    XWindowAttributes attrs;
    XGetWindowAttributes(display, video_win[id], &attrs);

    XImage image = {
        .width = attrs.width,
        .height = attrs.height,
        .depth = 24,
        .bits_per_pixel = 32,
        .format = ZPixmap,
        .byte_order = LSBFirst,
        .bitmap_unit = 8,
        .bitmap_bit_order = LSBFirst,
        .bytes_per_line = attrs.width * 4,
        .red_mask = 0xFF0000,
        .green_mask = 0xFF00,
        .blue_mask = 0xFF,
        .data = (char*)img_data
    };

    /* scale image if needed */
    uint8_t new_data[attrs.width * attrs.height * 4];
    if(attrs.width != width || attrs.height != height) {
        scale_rgbx_image(img_data, width, height, new_data, attrs.width, attrs.height);
        image.data = (char*)new_data;
    }


    GC gc = DefaultGC(display, screen);
    Pixmap pixmap = XCreatePixmap(display, window, attrs.width, attrs.height, 24);
    XPutImage(display, pixmap, gc, &image, 0, 0, 0, 0, attrs.width, attrs.height);
    XCopyArea(display, pixmap, video_win[id], gc, 0, 0, attrs.width, attrs.height, 0, 0);
    XFreePixmap(display, pixmap);
}

void video_begin(uint32_t id, uint8_t *name, uint16_t name_length, uint16_t width, uint16_t height)
{
    Window *win = &video_win[id];
    if(*win) {
        return;
    }

    *win = XCreateSimpleWindow(display, RootWindow(display, screen), 0, 0, width, height, 0, BlackPixel(display, screen), WhitePixel(display, screen));
    XStoreName(display, *win, (char*)name);
    XSetWMProtocols(display, *win, &wm_delete_window, 1);

    /* set WM_CLASS */
    XClassHint hint = {
        .res_name = "utoxvideo",
        .res_class = "utoxvideo"
    };

    XSetClassHint(display, *win, &hint);

    XMapWindow(display, *win);
}

void video_end(uint32_t id)
{
    if(!video_win[id]) {
        return;
    }

    XDestroyWindow(display, video_win[id]);
    video_win[id] = None;
}

Display *deskdisplay;
int deskscreen;

XShmSegmentInfo shminfo;

void initshm(void)
{
    deskdisplay = XOpenDisplay(NULL);
    deskscreen = DefaultScreen(deskdisplay);
    debug("desktop: %u %u\n", scr->width, scr->height);
    max_video_width = scr->width;
    max_video_height = scr->height;
}

void* video_detect(void)
{
    char dev_name[] = "/dev/videoXX", *first = NULL;

    #ifdef __APPLE__
    #else
    int i;
    for(i = 0; i != 64; i++) {
        sprintf(dev_name + 10, "%i", i);

        struct stat st;
        if (-1 == stat(dev_name, &st)) {
            continue;
            //debug("Cannot identify '%s': %d, %s\n", dev_name, errno, strerror(errno));
            //return 0;
        }

        if (!S_ISCHR(st.st_mode)) {
            continue;
            //debug("%s is no device\n", dev_name);
            //return 0;
        }

        void *p = malloc(sizeof(void*) + sizeof(dev_name)), *pp = p + sizeof(void*);
        memcpy(p, &pp, sizeof(void*));
        memcpy(p + sizeof(void*), dev_name, sizeof(dev_name));
        if(!first) {
            first = pp;
            postmessage(NEW_VIDEO_DEVICE, 1, 0, p);
        } else {
            postmessage(NEW_VIDEO_DEVICE, 0, 0, p);
        }

    }
    #endif

    initshm();

    return first;
}

void desktopgrab(_Bool video)
{
    pointergrab = 1 + video;
    XGrabPointer(display, window, False, Button1MotionMask | ButtonPressMask | ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None, cursors[CURSOR_SELECT], CurrentTime);
}

static uint16_t video_x, video_y;

_Bool video_init(void *handle)
{
    if(isdesktop(handle)) {
        fd = -1;

        video_x = volatile(grabx);
        video_y = volatile(graby);
        video_width = volatile(grabpx);
        video_height = volatile(grabpy);

        if(video_width & 1) {
            if(video_x & 1) {
                video_x--;
            }
            video_width++;
        }

        if(video_height & 1) {
            if(video_y & 1) {
                video_y--;
            }
            video_height++;
        }

        if(!(screen_image = XShmCreateImage(deskdisplay, DefaultVisual(deskdisplay, deskscreen), DefaultDepth(deskdisplay, deskscreen), ZPixmap, NULL, &shminfo, video_width, video_height))) {
            return 0;
        }

        if((shminfo.shmid = shmget(IPC_PRIVATE, screen_image->bytes_per_line * screen_image->height, IPC_CREAT | 0777)) < 0) {
            return 0;
        }

        if((shminfo.shmaddr = screen_image->data = (char*)shmat(shminfo.shmid, 0, 0)) == (char*)-1) {
            return 0;
        }

        shminfo.readOnly = False;
        if(!XShmAttach(deskdisplay, &shminfo)) {
            return 0;
        }



        return 1;
    }

    return v4l_init(handle);
}

void video_close(void *handle)
{
    if(isdesktop(handle)) {
        XShmDetach(deskdisplay, &shminfo);
        return;
    }

    v4l_close();
}

_Bool video_startread(void)
{
    if(fd == -1) {
        return 1;
    }

    return v4l_startread();
}

_Bool video_endread(void)
{
    if(fd == -1) {
        return 1;
    }

    return v4l_endread();
}

int video_getframe(vpx_image_t *image)
{
    if(fd == -1) {
        static uint64_t lasttime;
        uint64_t t = get_time();
        if(t - lasttime >= (uint64_t)1000 * 1000 * 1000 / 24) {
            XShmGetImage(deskdisplay,RootWindow(deskdisplay, deskscreen), screen_image, video_x, video_y, AllPlanes);
            rgbxtoyuv420(image->planes[0], image->planes[1], image->planes[2], (uint8_t*)screen_image->data, screen_image->width, screen_image->height);
            lasttime = t;
            return 1;
        }
        return 0;
    }

    return v4l_getframe(image);
}
