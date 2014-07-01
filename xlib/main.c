#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <X11/Xft/Xft.h>
#include <X11/Xatom.h>
#include <X11/X.h>
#include <X11/cursorfont.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <pthread.h>
#include <unistd.h>

#include <dlfcn.h>

#include "v4l.c"

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

Cursor cursor_arrow, cursor_hand, cursor_text;

XftFont *font[16][64], **sfont;
XftDraw *xftdraw;
XftColor xftcolor;
FcCharSet *charset;
FcFontSet *fs;

Window video_win[256];

Atom wm_protocols, wm_delete_window;

uint32_t scolor;

Atom XA_CLIPBOARD, XA_UTF8_STRING, targets;
Atom XdndAware, XdndEnter, XdndLeave, XdndPosition, XdndStatus, XdndDrop, XdndSelection, XdndDATA, XdndActionCopy;

Pixmap drawbuf;

XImage *screen_image;

/* pointers to dynamically loaded libs */
void *libgtk;
#include "gtk.c"

struct
{
    int len;
    uint8_t data[65536];
}clipboard;

static void setclipboard(void)
{
    XSetSelectionOwner(display, XA_CLIPBOARD, window, CurrentTime);
}

static XftFont* getfont(XftFont **font, uint32_t ch)
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
}

static void initfonts(void)
{
    FcResult result;
    FcPattern *pat = FcPatternCreate();
    FcPatternAddString(pat, FC_FAMILY, (uint8_t*)"Roboto");
    FcConfigSubstitute(0, pat, FcMatchPattern);
    FcDefaultSubstitute(pat);
    fs = FcFontSort(NULL, pat, 0, &charset, &result);

    FcPatternDestroy(pat);
}

static void loadfonts(void)
{
     #define F(x) (x * SCALE / 2.0)
    font[FONT_TEXT][0] = XftFontOpen(display, screen, XFT_FAMILY, XftTypeString, "Roboto", XFT_PIXEL_SIZE, XftTypeDouble, F(12.0), NULL);

    font[FONT_TITLE][0] = XftFontOpen(display, screen, XFT_FAMILY, XftTypeString, "Roboto", XFT_PIXEL_SIZE, XftTypeDouble, F(12.0), XFT_WEIGHT, XftTypeInteger, FC_WEIGHT_BOLD, NULL);

    font[FONT_SELF_NAME][0] = XftFontOpen(display, screen, XFT_FAMILY, XftTypeString, "Roboto", XFT_PIXEL_SIZE, XftTypeDouble, F(14.0), XFT_WEIGHT, XftTypeInteger, FC_WEIGHT_BOLD, NULL);
    font[FONT_STATUS][0] = XftFontOpen(display, screen, XFT_FAMILY, XftTypeString, "Roboto", XFT_PIXEL_SIZE, XftTypeDouble, F(11.0), NULL);

    font[FONT_LIST_NAME][0] = XftFontOpen(display, screen, XFT_FAMILY, XftTypeString, "Roboto", XFT_PIXEL_SIZE, XftTypeDouble, F(12.0), NULL);

    font[FONT_MSG][0] = XftFontOpen(display, screen, XFT_FAMILY, XftTypeString, "Roboto", XFT_PIXEL_SIZE, XftTypeDouble, F(11.0), XFT_WEIGHT, XftTypeInteger, FC_WEIGHT_LIGHT, NULL);
    font[FONT_MSG_NAME][0] = XftFontOpen(display, screen, XFT_FAMILY, XftTypeString, "Roboto", XFT_PIXEL_SIZE, XftTypeDouble, F(10.0), XFT_WEIGHT, XftTypeInteger, FC_WEIGHT_LIGHT, NULL);
    font[FONT_MISC][0] = XftFontOpen(display, screen, XFT_FAMILY, XftTypeString, "Roboto", XFT_PIXEL_SIZE, XftTypeDouble, F(10.0), NULL);
    font[FONT_MSG_LINK][0] = XftFontOpen(display, screen, XFT_FAMILY, XftTypeString, "Roboto", XFT_PIXEL_SIZE, XftTypeDouble, F(11.0), XFT_WEIGHT, XftTypeInteger, FC_WEIGHT_LIGHT, NULL);
    #undef F
}

static void freefonts(void)
{
    int i;
    for(i = 0; i != countof(font); i++) {
        XftFont **f = font[i];
        while(*f) {
            XftFontClose(display, *f);
            *f = NULL;
            f++;
        }
    }
}

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

    XRenderComposite(display, PictOpOver, src, bitmap[bm], XftDrawPicture(xftdraw), 0, 0, 0, 0, x, y, width, height);

    XRenderFreePicture(display, src);
}

int _drawtext(int x, int y, uint8_t *str, uint16_t length)
{
    XftFont *font = sfont[0], *nfont;
    int x1 = x;
    XGlyphInfo extents;
    uint8_t *a = str, *b = str, *end = str + length;
    while(a != end) {
        uint32_t ch;
        uint8_t len = utf8_len_read(a, &ch);
        nfont = getfont(sfont, ch);
        if(nfont != font) {
            XftTextExtentsUtf8(display, font, b, a - b, &extents);
            XftDrawStringUtf8(xftdraw, &xftcolor, font, x, y + font->ascent, b, a - b);
            x += extents.xOff;

            font = nfont;
            b = a;
        }

        a += len;
    }

    XftTextExtentsUtf8(display, font, b, a - b, &extents);
    XftDrawStringUtf8(xftdraw, &xftcolor, font, x, y + font->ascent, b, a - b);

    return x - x1 + extents.xOff;
}

void drawtext(int x, int y, uint8_t *str, uint16_t length)
{
    _drawtext(x, y, str, length);
}

int drawtext_getwidth(int x, int y, uint8_t *str, uint16_t length)
{
    return _drawtext(x, y, str, length);
}

void drawtextwidth(int x, int width, int y, uint8_t *str, uint16_t length)
{
    int fit = textfit(str, length, width);
    if(fit != length) {
        uint8_t buf[fit + 3];
        memcpy(buf, str, fit);
        memcpy(buf + fit, "...", 3);
        drawtext(x, y, buf, fit + 3);
    } else {
        drawtext(x, y, str, length);
    }
}

void drawtextrange(int x, int x2, int y, uint8_t *str, uint16_t length)
{
    drawtextwidth(x, x2 - x, y, str, length);
}

void drawtextwidth_right(int x, int width, int y, uint8_t *str, uint16_t length)
{
    int w = textwidth(str, length);
    drawtext(x + width - w, y, str, length);
}

void drawtextrangecut(int x, int x2, int y, uint8_t *str, uint16_t length)
{
    drawtext(x, y, str, length);
}

int textwidth(uint8_t *str, uint16_t length)
{
    XftFont *font = sfont[0], *nfont;
    int x = 0;
    XGlyphInfo extents;
    uint8_t *a = str, *b = str, *end = str + length;
    while(a != end) {
        uint32_t ch;
        uint8_t len = utf8_len_read(a, &ch);
        nfont = getfont(sfont, ch);
        if(nfont != font) {
            XftTextExtentsUtf8(display, font, b, a - b, &extents);
            x += extents.xOff;

            font = nfont;
            b = a;
        }

        a += len;
    }
    XftTextExtentsUtf8(display, font, b, a - b, &extents);

    return x + extents.xOff;
}

int textfit(uint8_t *str, uint16_t length, int width)
{
    int i = 0;
    int x = 0;
    XftFont *font;
    uint32_t ch;
    XGlyphInfo extents;
    while(i < length) {
        uint8_t len = utf8_len_read(str + i, &ch);
        font = getfont(sfont, ch);
        XftTextExtentsUtf8(display, font, str + i, len, &extents);

        if(x + extents.xOff / 2 > width) {
            break;
        }

        x += extents.xOff;
        i += len;
    }

    return i;
}

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

void setfont(int id)
{
    sfont = font[id];
}

uint32_t setcolor(uint32_t color)
{
    XRenderColor xrcolor;
    XftColorFree(display, visual, cmap, &xftcolor);
    xrcolor.red = ((color >> 8) & 0xFF00) | 0x80;
    xrcolor.green = ((color) & 0xFF00) | 0x80;
    xrcolor.blue = ((color << 8) & 0xFF00) | 0x80;
    xrcolor.alpha = 0xFFFF;
    XftColorAllocValue(display, visual, cmap, &xrcolor, &xftcolor);

    uint32_t old = scolor;
    scolor = color;
    xftcolor.pixel = color;
    XSetForeground(display, gc, color);
    return old;
}

void setbkcolor(uint32_t color)
{
    XSetBackground(display, gc, color);
}

void setbgcolor(uint32_t color)
{
    XSetBackground(display, gc, color);
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
    XftDrawSetClipRectangles(xftdraw, 0, 0, r, 1);
}

void popclip(void)
{
    clipk--;
    if(!clipk) {
        XSetClipMask(display, gc, None);
        XftDrawSetClip(xftdraw, None);
        return;
    }

    XRectangle *r = &clip[clipk - 1];

    XSetClipRectangles(display, gc, 0, 0, r, 1, Unsorted);
    XftDrawSetClipRectangles(xftdraw, 0, 0, r, 1);
}

void enddraw(int x, int y, int width, int height)
{
    XCopyArea(display, drawbuf, window, gc, x, y, width, height, x, y);
}

void thread(void func(void*), void *args)
{
    pthread_t thread_temp;
    pthread_create(&thread_temp, NULL, (void*(*)(void*))func, args);
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
}

void editpopup(void)
{

}

void listpopup(uint8_t item)
{

}

void openurl(char_t *str)
{
    char cmd[1024];
    sprintf(cmd, "xdg-open %s", str);
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

void sysmexit(void)
{

}

void sysmsize(void)
{

}

void sysmmini(void)
{

}

void setselection(void)
{
    XSetSelectionOwner(display, XA_PRIMARY, window, CurrentTime);
}

static void pasteprimary(void)
{
    Window owner = XGetSelectionOwner(display, XA_PRIMARY);
    if(owner) {
        XConvertSelection(display, XA_PRIMARY, XA_UTF8_STRING, targets, window, CurrentTime);
    }
}

static void pasteclipboard(void)
{
    Window owner = XGetSelectionOwner(display, XA_CLIPBOARD);
    if(owner) {
        XConvertSelection(display, XA_CLIPBOARD, XA_UTF8_STRING, targets, window, CurrentTime);
    }
}

static Picture loadrgba(void *data, int width, int height)
{
    Pixmap pixmap = XCreatePixmap(display, window, width, height, 32);
    XImage *img = XCreateImage(display, CopyFromParent, 32, ZPixmap, 0, data, width, height, 32, 0);
    GC legc = XCreateGC(display, pixmap, 0, NULL);
    XPutImage(display, pixmap, legc, img, 0, 0, 0, 0, width, height);

    Picture picture = XRenderCreatePicture(display, pixmap, XRenderFindStandardFormat(display, PictStandardARGB32), 0, NULL);

    XFreeGC(display, legc);
    XFreePixmap(display, pixmap);

    return picture;
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

XSizeHints *xsh;

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

    font_small_lineheight = font[FONT_TEXT][0]->height;
    font_msg_lineheight = font[FONT_MSG][0]->height;

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

#include "event.c"

int depth;
Screen *scr;

int main(int argc, char *argv[])
{
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
                    PointerMotionMask | StructureNotifyMask | KeyPressMask | KeyReleaseMask,
    };

    /* create window */
    window = XCreateWindow(display, RootWindow(display, screen), 0, 0, DEFAULT_WIDTH, DEFAULT_HEIGHT, 0, depth, InputOutput, visual, CWBackPixel | CWBorderPixel | CWEventMask, &attrib);

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

    XdndAware = XInternAtom(display, "XdndAware", False);
    XdndEnter = XInternAtom(display, "XdndEnter", False);
    XdndLeave = XInternAtom(display, "XdndLeave", False);
    XdndPosition = XInternAtom(display, "XdndPosition", False);
    XdndStatus = XInternAtom(display, "XdndStatus", False);
    XdndDrop = XInternAtom(display, "XdndDrop", False);
    XdndSelection = XInternAtom(display, "XdndSelection", False);
    XdndDATA = XInternAtom(display, "XdndDATA", False);
    XdndActionCopy = XInternAtom(display, "XdndActionCopy", False);

    /* create the draw buffer */
    drawbuf = XCreatePixmap(display, window, DEFAULT_WIDTH, DEFAULT_HEIGHT, depth);

    /* catch WM_DELETE_WINDOW */
    XSetWMProtocols(display, window, &wm_delete_window, 1);

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
    ui_scale(DEFAULT_SCALE);

    /* load the used cursors */
    cursor_arrow = XCreateFontCursor(display, XC_left_ptr);
    cursor_hand = XCreateFontCursor(display, XC_hand2);
    cursor_text = XCreateFontCursor(display, XC_xterm);

    /* Xft draw context/color */
    xftdraw = XftDrawCreate(display, drawbuf, visual, cmap);
    XRenderColor xrcolor;
    xrcolor.red = 0x0;
    xrcolor.green = 0x0;
    xrcolor.blue = 0x0;
    xrcolor.alpha = 0xffff;
    XftColorAllocValue(display, visual, cmap, &xrcolor, &xftcolor);

    /* set-up desktop video input */
    dropdown_add(&dropdown_video, (uint8_t*)"Desktop", (void*)"");

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
    redraw();

    /* event loop */
    while(doevent());

    toxav_postmessage(AV_KILL, 0, 0, NULL);
    tox_postmessage(TOX_KILL, 0, 0, NULL);

    /* free client thread stuff */
    if(libgtk) {

    }

    FcFontSetSortDestroy(fs);
    freefonts();

    XFreePixmap(display, drawbuf);

    XftDrawDestroy(xftdraw);
    XftColorFree(display, visual, cmap, &xftcolor);

    XDestroyWindow(display, window);
    XCloseDisplay(display);

    /* wait for threads to exit */
    while(tox_thread_init) {
        yieldcpu(1);
    }

    debug("clean exit\n");

    return 0;
}

void video_frame(uint32_t id, vpx_image_t *frame)
{
    uint8_t *img_data = malloc(frame->d_w * frame->d_h * 4);
    yuv420torgb(frame, img_data);

    XImage image = {
        .width = frame->d_w,
        .height = frame->d_h,
        .depth = 24,
        .bits_per_pixel = 32,
        .format = ZPixmap,
        .byte_order = LSBFirst,
        .bitmap_unit = 8,
        .bitmap_bit_order = LSBFirst,
        .bytes_per_line = frame->d_w * 4,
        .red_mask = 0xFF0000,
        .green_mask = 0xFF00,
        .blue_mask = 0xFF,
        .data = (void*)img_data
    };

    GC gc = DefaultGC(display, screen);
    Pixmap pixmap = XCreatePixmap(display, window, frame->d_w, frame->d_h, 24);
    XPutImage(display, pixmap, gc, &image, 0, 0, 0, 0, frame->d_w, frame->d_h);
    XCopyArea(display, pixmap, video_win[id], gc, 0, 0, frame->d_w, frame->d_h, 0, 0);
    XFreePixmap(display, pixmap);
    free(img_data);
}

void video_begin(uint32_t id, uint8_t *name, uint16_t name_length, uint16_t width, uint16_t height)
{
    Window *win = &video_win[id];
    *win = XCreateSimpleWindow(display, RootWindow(display, screen), 0, 0, width, height, 0, BlackPixel(display, screen), WhitePixel(display, screen));
    XStoreName(display, *win, (char*)name);
    XSetWMProtocols(display, *win, &wm_delete_window, 1);
    XMapWindow(display, *win);
}

void video_end(uint32_t id)
{
    XDestroyWindow(display, video_win[id]);
}

Display *deskdisplay;
int deskscreen;

void initshm(void)
{
    deskdisplay = XOpenDisplay(NULL);
    deskscreen = DefaultScreen(deskdisplay);
    //debug("desktop: %u %u\n", scr->width, scr->height);

    /*XShmSegmentInfo shminfo;
    if(!(screen_image = XShmCreateImage(deskdisplay, DefaultVisual(deskdisplay, deskscreen), DefaultDepth(deskdisplay, deskscreen), ZPixmap, NULL, &shminfo, 640, 480))) {
       return;
    }

    if((shminfo.shmid = shmget(IPC_PRIVATE, screen_image->bytes_per_line * screen_image->height, IPC_CREAT | 0777)) < 0) {
        return;
    }

    if((shminfo.shmaddr = screen_image->data = (char*)shmat(shminfo.shmid, 0, 0)) == (char*)-1) {
        return;
    }

    shminfo.readOnly = False;
    if(!XShmAttach(deskdisplay, &shminfo)) {
        return;
    }*/
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

    return first ? first : "";
}

_Bool video_init(void *handle)
{
    char *dev_name = handle;
    if(!dev_name[0]) {
        fd = -1;
        video_width = scr->width;
        video_height = scr->height;
        return 1;
    }

    return v4l_init(dev_name);
}

void video_close(void *handle)
{
    if(!*(uint8_t*)handle) {
        //desktop
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

_Bool video_getframe(vpx_image_t *image)
{
    if(fd == -1) {
        screen_image = XGetImage(deskdisplay,RootWindow(deskdisplay, DefaultScreen(deskdisplay)), 0, 0, scr->width, scr->height, XAllPlanes(), ZPixmap);

        //XShmGetImage(deskdisplay,RootWindow(deskdisplay, deskscreen), screen_image, 0, 0, AllPlanes);
        rgbxtoyuv420(image->planes[0], image->planes[1], image->planes[2], (uint8_t*)screen_image->data, screen_image->width, screen_image->height);
        XDestroyImage(screen_image);
        return 1;
    }

    return v4l_getframe(image);
}
