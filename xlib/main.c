#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <X11/Xft/Xft.h>
#include <X11/Xatom.h>
#include <X11/X.h>

#include <pthread.h>
#include <unistd.h>

#include <dlfcn.h>

#include "keysym2ucs.c"

#define DEFAULT_BDWIDTH 1 /* border width */

char *text = "Hello Xft";
Display *display;
int screen;
Window window;
GC gc;
Colormap cmap;
Visual *visual;

Pixmap bitmap[16];

XftFont *font[16], *sfont;
XftDraw *xftdraw;
XftColor xftcolor;

uint32_t scolor;

Atom XA_CLIPBOARD, XA_UTF8_STRING, targets;

Pixmap drawbuf;

_Bool clipboard_id;

typedef struct
{
    void *data, *next;
}g_list;

void *libgtk;
void (*gtk_init)(int*, char***);
_Bool (*gtk_events_pending)(void);
void (*gtk_main_iteration)(void);
void* (*gtk_file_chooser_dialog_new)(const char*, void*, int, const char*, ...);
void (*gtk_file_chooser_set_select_multiple)(void*, _Bool);
void (*gtk_file_chooser_set_current_name)(void*, char*);
int (*gtk_dialog_run)(void*);
void* (*gtk_file_chooser_get_filename)(void*);
void* (*gtk_file_chooser_get_filenames)(void*);
void (*gtk_widget_destroy)(void*);

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

    XSendEvent(display, window, False, 0,  &event);
    XFlush(display);
}

void drawbitmap(int bm, int x, int y, int width, int height)
{
    //debug("%u %u\n", bm, bitmap[bm]);
    if(bm <= BM_EXIT) {
        XCopyPlane(display, bitmap[bm], drawbuf, gc, 0, 0, width, height, x, y, 1);
    } else {
        XCopyArea(display, bitmap[bm], drawbuf, gc, 0, 0, width, height, x, y);
    }
}

void drawbitmapalpha(int bm, int x, int y, int width, int height)
{
    XCopyArea(display, bitmap[bm], drawbuf, gc, 0, 0, width, height, x, y);
}

void drawtext(int x, int y, uint8_t *str, uint16_t length)
{
    XftDrawStringUtf8(xftdraw, &xftcolor, sfont, x, y + sfont->ascent, str, length);
}

void drawtextW(int x, int y, char_t *str, uint16_t length)
{
    drawtext(x, y, str, length);
}

int drawtext_getwidth(int x, int y, uint8_t *str, uint16_t length)
{
    XGlyphInfo extents;
    XftTextExtentsUtf8(display, sfont, str, length, &extents);

    XftDrawStringUtf8(xftdraw, &xftcolor, sfont, x, y + sfont->ascent, str, length);

    return extents.xOff;
}

int drawtext_getwidthW(int x, int y, char_t *str, uint16_t length)
{
    return drawtext_getwidth(x, y, str, length);
}

void drawtextwidth(int x, int width, int y, uint8_t *str, uint16_t length)
{
    pushclip(x, y, width, 256);

    drawtext(x, y, str, length);

    popclip();
}

void drawtextwidth_right(int x, int width, int y, uint8_t *str, uint16_t length)
{
    drawtext(x, y, str, length);
}

void drawtextwidth_rightW(int x, int width, int y, char_t *str, uint16_t length)
{
    drawtext(x, y, str, length);
}

void drawtextrange(int x, int x2, int y, uint8_t *str, uint16_t length)
{
    drawtext(x, y, str, length);
}

void drawtextrangecut(int x, int x2, int y, uint8_t *str, uint16_t length)
{
    drawtext(x, y, str, length);
}

void drawtextrangecutW(int x, int x2, int y, char_t *str, uint16_t length)
{
    drawtextrangecut(x, x2, y, str, length);
}

int textwidth(uint8_t *str, uint16_t length)
{
    XGlyphInfo extents;
    XftTextExtentsUtf8(display, sfont, str, length, &extents);

    return extents.xOff;
}

int textwidthW(char_t *str, uint16_t length)
{
    return textwidth(str, length);
}

int textfit(uint8_t *str, uint16_t length, int width)
{
    int i = 0;
    while(i < length) {
        uint8_t len = utf8_len(str + i);
        i += len;
        XGlyphInfo extents;
        XftTextExtentsUtf8(display, sfont, str, i, &extents);
        if(extents.xOff >= width) {
            i -= len;
            break;
        }
    }

    return i;
}

int textfitW(char_t *str, uint16_t length, int width)
{
    return textfit(str, length, width);
}

void drawrect(int x, int y, int width, int height, uint32_t color)
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

void fillrect(RECT *r, uint32_t color)
{
    XSetForeground(display, gc, color);
    XFillRectangle(display, drawbuf, gc, r->left, r->top, r->right - r->left, r->bottom - r->top);
}

void framerect(RECT *r, uint32_t color)
{
    XSetForeground(display, gc, color);
    XDrawRectangle(display, drawbuf, gc, r->left, r->top, r->right - r->left - 1, r->bottom - r->top - 1);
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
    #else
    clock_gettime(CLOCK_MONOTONIC, &ts);
    #endif

    return ((uint64_t)ts.tv_sec * (1000 * 1000 * 1000)) + (uint64_t)ts.tv_nsec;
}


void address_to_clipboard(void)
{
    XSetSelectionOwner(display, XA_CLIPBOARD, window, CurrentTime);
    clipboard_id = 1;
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

void openfilesend()
{
    if(libgtk) {
        void *dialog = gtk_file_chooser_dialog_new("Open File", NULL, 0, "gtk-cancel", -6, "gtk-open", -3, NULL);
        gtk_file_chooser_set_select_multiple(dialog, 1);
        int result = gtk_dialog_run(dialog);
        if(result == -3) {
            char *out = malloc(65536), *outp = out;
            g_list *list = gtk_file_chooser_get_filenames(dialog), *p = list;
            while(p) {
                outp = stpcpy(outp, p->data);
                *outp++ = '\n';
                //g_free(p->data)
                p = p->next;
            }
            //g_slist_free(list)
            debug("files: %s\n", out);

            tox_postmessage(TOX_SENDFILES, (FRIEND*)sitem->data - friend, 0xFFFF, out);
        }

        gtk_widget_destroy(dialog);
        while(gtk_events_pending()) {
            gtk_main_iteration();
        }
    }
}

void savefilerecv(uint32_t fid, MSG_FILE *file)
{
    if(libgtk) {
        void *dialog = gtk_file_chooser_dialog_new("Save File", NULL, 1, "gtk-cancel", -6, "gtk-save", -3, NULL);
        char buf[sizeof(file->name) + 1];
        memcpy(buf, file->name, file->name_length);
        buf[file->name_length] = 0;
        gtk_file_chooser_set_current_name(dialog, buf);
        int result = gtk_dialog_run(dialog);
        if(result == -3) {
            char *name = gtk_file_chooser_get_filename(dialog);
            int len = strlen(name) + 1;
            char *path = malloc(len);
            memcpy(path, name, len);
            //g_free(name)

            //debug("name: %s\npath: %s\n", name, path);

            tox_postmessage(TOX_ACCEPTFILE, fid, file->filenumber, path);
        }

        gtk_widget_destroy(dialog);
        while(gtk_events_pending()) {
            gtk_main_iteration();
        }
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
    clipboard_id = 0;
}

static void setclipboard(void)
{
    XSetSelectionOwner(display, XA_CLIPBOARD, window, CurrentTime);
    clipboard_id = 0;
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

static Pixmap createbitmap(int width, int height, void *data)
{
    XGCValues gcvalues;

    XImage *img = XCreateImage(display, CopyFromParent, 24, ZPixmap, 0, data, width, height, 32, 0);
    Pixmap p = XCreatePixmap(display, window, width, height, 24);
    GC gc = XCreateGC(display, p, 0, &gcvalues);

    XPutImage(display, p, gc, img, 0, 0, 0, 0, width, height);

    return p;
}

int main(int argc, char *argv[])
{
    _Bool done = 0;

    XEvent event;

    XSizeHints xsh = {
        .flags = PSize | PMinSize,
        .width = 800,
        .min_width = 480,
        .height = 600,
        .min_height = 320
    };

    /*XWMHints xwmh = {
        .flags = InputHint | StateHint,
        .input = False,
        .initial_state = NormalState
    };*/

    XInitThreads();

    if((display = XOpenDisplay(NULL)) == NULL) {
        printf("Cannot open display\n");
        return 1;
    }

    libgtk = dlopen("libgtk-x11-2.0.so.0", RTLD_LAZY);
    if(libgtk) {
        debug("have GTK, our system is bloated\n");

        gtk_init = dlsym(libgtk, "gtk_init");
        gtk_main_iteration = dlsym(libgtk, "gtk_main_iteration");
        gtk_events_pending = dlsym(libgtk, "gtk_events_pending");
        gtk_file_chooser_dialog_new = dlsym(libgtk, "gtk_file_chooser_dialog_new");
        gtk_dialog_run = dlsym(libgtk, "gtk_dialog_run");
        gtk_file_chooser_get_filename = dlsym(libgtk, "gtk_file_chooser_get_filename");
        gtk_file_chooser_get_filenames = dlsym(libgtk, "gtk_file_chooser_get_filenames");
        gtk_file_chooser_set_select_multiple = dlsym(libgtk, "gtk_file_chooser_set_select_multiple");
        gtk_file_chooser_set_current_name = dlsym(libgtk, "gtk_file_chooser_set_current_name");
        gtk_widget_destroy = dlsym(libgtk, "gtk_widget_destroy");

        if(!gtk_init || !gtk_main_iteration || !gtk_events_pending || !gtk_file_chooser_dialog_new || !gtk_dialog_run || !gtk_file_chooser_get_filename ||
           !gtk_file_chooser_get_filenames || !gtk_file_chooser_set_select_multiple || !gtk_file_chooser_set_current_name || !gtk_widget_destroy) {
            debug("bad GTK\n");
            dlclose(libgtk);
            libgtk = NULL;
        } else {
            gtk_init(&argc, &argv);
        }
    } else {
        debug("no GTK, our system is clean\n");
    }


    thread(tox_thread, NULL);

    screen = DefaultScreen(display);
    cmap = DefaultColormap(display, screen);
    visual = DefaultVisual(display, screen);

    XA_CLIPBOARD = XInternAtom(display, "CLIPBOARD", 0);
    XA_UTF8_STRING = XInternAtom(display, "UTF8_STRING", 1);
    if (XA_UTF8_STRING == None) {
	    XA_UTF8_STRING = XA_STRING;
    }
    targets = XInternAtom(display, "TARGETS", 0);

    Atom XdndAware = XInternAtom(display, "XdndAware", False);
    Atom XdndEnter = XInternAtom(display, "XdndEnter", False);
    Atom XdndLeave = XInternAtom(display, "XdndLeave", False);
    Atom XdndPosition = XInternAtom(display, "XdndPosition", False);
    Atom XdndStatus = XInternAtom(display, "XdndStatus", False);
    Atom XdndDrop = XInternAtom(display, "XdndDrop", False);
    Atom XdndSelection = XInternAtom(display, "XdndSelection", False);
    Atom XdndDATA = XInternAtom(display, "XdndDATA", False);
    Atom XdndActionCopy = XInternAtom(display, "XdndActionCopy", False);

    /*int nvi = 0, x;
    XVisualInfo template;
    template.depth = 32;
    template.screen = screen;
    XVisualInfo *vlist = XGetVisualInfo(display, VisualDepthMask, &template, &nvi);

    for(x = 0; x < nvi; x++)
    {
        if(vlist[x].depth == 32)
            break;
    }

    visual = vlist[x].visual;*/


    XSetWindowAttributes attrib = {
        .background_pixel = WhitePixel(display, screen),
        .border_pixel = BlackPixel(display, screen),
        .event_mask = ExposureMask | ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask |
                    PointerMotionMask | StructureNotifyMask | KeyPressMask | KeyReleaseMask,
    };


    font[FONT_TITLE] = XftFontOpen(display, screen, XFT_FAMILY, XftTypeString, "DejaVu Sans", XFT_PIXEL_SIZE, XftTypeDouble, 20.0, NULL);
    font[FONT_SUBTITLE] = XftFontOpen(display, screen, XFT_FAMILY, XftTypeString, "DejaVu Sans", XFT_PIXEL_SIZE, XftTypeDouble, 18.0, NULL);
    font[FONT_MED] = XftFontOpen(display, screen, XFT_FAMILY, XftTypeString, "DejaVu Sans", XFT_PIXEL_SIZE, XftTypeDouble, 16.0, NULL);
    font[FONT_TEXT_LARGE] = XftFontOpen(display, screen, XFT_FAMILY, XftTypeString, "DejaVu Sans", XFT_PIXEL_SIZE, XftTypeDouble, 14.0, NULL);
    font[FONT_TEXT] = XftFontOpen(display, screen, XFT_FAMILY, XftTypeString, "DejaVu Sans", XFT_PIXEL_SIZE, XftTypeDouble, 12.0, NULL);

    font[FONT_MSG] = XftFontOpen(display, screen, XFT_FAMILY, XftTypeString, "DejaVu Sans", XFT_PIXEL_SIZE, XftTypeDouble, 16.0, NULL);
    font[FONT_MSG_NAME] = XftFontOpen(display, screen, XFT_FAMILY, XftTypeString, "DejaVu Sans", XFT_PIXEL_SIZE, XftTypeDouble, 16.0, XFT_WEIGHT, XftTypeInteger, FC_WEIGHT_BOLD, NULL);
    font[FONT_MSG_LINK] = XftFontOpen(display, screen, XFT_FAMILY, XftTypeString, "DejaVu Sans", XFT_PIXEL_SIZE, XftTypeDouble, 16.0, NULL);


    font_small_lineheight = font[FONT_TEXT]->height;
    font_msg_lineheight = font[FONT_MSG]->height;

    //window = XCreateSimpleWindow(display, RootWindow(display, screen), 0, 0, 800, 600, 0, BlackPixel(display, screen), WhitePixel(display, screen));
    window = XCreateWindow(display, RootWindow(display, screen), 0, 0, 800, 600, 0, 24, InputOutput, visual, CWBackPixel | CWBorderPixel | CWEventMask, &attrib);
    drawbuf = XCreatePixmap(display, window, 800, 600, 24);
    //XSelectInput(display, window, ExposureMask | ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask | PointerMotionMask | StructureNotifyMask | KeyPressMask | KeyReleaseMask);

    Atom version = 3;
    XChangeProperty(display, window, XdndAware, XA_ATOM, 32, PropModeReplace, (uint8_t*)&version, 1);


    bitmap[BM_MINIMIZE] = XCreateBitmapFromData(display, window, (char*)bm_minimize_bits, 16, 10);
    bitmap[BM_RESTORE] = XCreateBitmapFromData(display, window, (char*)bm_restore_bits, 16, 10);
    bitmap[BM_MAXIMIZE] = XCreateBitmapFromData(display, window, (char*)bm_maximize_bits, 16, 10);
    bitmap[BM_EXIT] = XCreateBitmapFromData(display, window, (char*)bm_exit_bits, 16, 10);

    bitmap[BM_ONLINE] = createbitmap(10, 10, bm_online_bits);
    bitmap[BM_AWAY] = createbitmap(10, 10, bm_away_bits);
    bitmap[BM_BUSY] = createbitmap(10, 10, bm_busy_bits);
    bitmap[BM_OFFLINE] = createbitmap(10, 10, bm_offline_bits);
    bitmap[BM_CONTACT] = createbitmap(48, 48, bm_contact_bits);
    bitmap[BM_GROUP] = createbitmap(48, 48, bm_group_bits);
    bitmap[BM_FILE] = createbitmap(48, 48, bm_file_bits);

    uint32_t test[64];
    int xx = 0;
    while(xx < 8) {
        int y = 0;
        while(y < 8) {
            uint32_t value = 0xFFFFFF;
            if(xx + y >= 7) {
                int a = xx % 3, b = y % 3;
                if(a == 1) {
                    if(b == 0) {
                        value = 0xB6B6B6;
                    } else if(b == 1) {
                        value = 0x999999;
                    }
                } else if(a == 0 && b == 1) {
                    value = 0xE0E0E0;
                }
            }

            test[y * 8 + xx] = value;
            y++;
        }
        xx++;
    }

    bitmap[BM_CORNER] = createbitmap(8, 8, test);

    XSetStandardProperties(display, window, "uTox", "uTox", None, argv, argc, &xsh);
    //XSetWMHints(display, window, &xwmh);

    /*struct MwmHints {
        unsigned long flags;
        unsigned long functions;
        unsigned long decorations;
        long input_mode;
        unsigned long status;
    };

     #define MWM_DECOR_NONE          0
     #define MWM_DECOR_ALL           (1L << 0)
     #define MWM_DECOR_BORDER        (1L << 1)
     #define MWM_DECOR_RESIZEH       (1L << 2)
     #define MWM_DECOR_TITLE         (1L << 3)
     #define MWM_DECOR_MENU          (1L << 4)
     #define MWM_DECOR_MINIMIZE      (1L << 5)
     #define MWM_DECOR_MAXIMIZE      (1L << 6)

    enum {
        MWM_HINTS_FUNCTIONS = (1L << 0),
        MWM_HINTS_DECORATIONS =  (1L << 1),

        MWM_FUNC_ALL = (1L << 0),
        MWM_FUNC_RESIZE = (1L << 1),
        MWM_FUNC_MOVE = (1L << 2),
        MWM_FUNC_MINIMIZE = (1L << 3),
        MWM_FUNC_MAXIMIZE = (1L << 4),
        MWM_FUNC_CLOSE = (1L << 5)
    };

    Atom mwmHintsProperty = XInternAtom(display, "_MOTIF_WM_HINTS", 0);
    struct MwmHints hints;
    hints.flags = MWM_HINTS_DECORATIONS;
    hints.decorations = MWM_DECOR_BORDER;
    XChangeProperty(display, window, mwmHintsProperty, mwmHintsProperty, 32, PropModeReplace, (unsigned char *)&hints, 5);*/


    /* Xft draw context */
    xftdraw = XftDrawCreate(display, drawbuf, visual, cmap);
    /* Xft text color */
    XRenderColor xrcolor;
    xrcolor.red = 0x0;
    xrcolor.green = 0x0;
    xrcolor.blue = 0x0;
    xrcolor.alpha = 0xffff;
    XftColorAllocValue(display, visual, cmap, &xrcolor, &xftcolor);

    XMapWindow(display, window);

    gc = DefaultGC(display, screen);//XCreateGC(display, window, 0, 0);

    width = 800;
    height = 600;

    //load fonts
    //load bitmaps

    //wait for tox_thread init
    while(!tox_thread_init) {
        yieldcpu(1);
    }

    list_start();

    redraw();

    while(!done) {
        XNextEvent(display, &event);
        switch(event.type) {
        case Expose: {
            redraw();
            break;
        }

        case ConfigureNotify: {
            XConfigureEvent *ev = &event.xconfigure;
            width = ev->width;
            height = ev->height;

            panel_update(&panel_main, 0, 0, width, height);

            XFreePixmap(display, drawbuf);
            drawbuf = XCreatePixmap(display, window, width, height, 24);

            XftDrawDestroy(xftdraw);
            xftdraw = XftDrawCreate(display, drawbuf, visual, cmap);

            redraw();
            break;
        }

        case LeaveNotify: {
            panel_mleave(&panel_main);
        }

        case MotionNotify: {
            XMotionEvent *ev = &event.xmotion;
            static int my;
            int dy;

            dy = ev->y - my;
            my = ev->y;

            hand = 0;

            panel_mmove(&panel_main, 0, 0, width, height, ev->x, ev->y, dy);

            //SetCursor(hand ? cursor_hand : cursor_arrow);

            //debug("MotionEvent: (%u %u) %u\n", ev->x, ev->y, ev->state);
            break;
        }

        case ButtonPress: {
            XButtonEvent *ev = &event.xbutton;
            switch(ev->button) {
            case Button1: {
                panel_mdown(&panel_main);
                mdown = 1;
                break;
            }

            case Button2: {
                pasteprimary();
                break;
            }

            case Button3: {
                panel_mright(&panel_main);
                break;
            }

            case Button4: {
                panel_mwheel(&panel_main, 0, 0, width, height, 1.0);
                break;
            }

            case Button5: {
                panel_mwheel(&panel_main, 0, 0, width, height, -1.0);
                break;
            }

            }

            //debug("ButtonEvent: %u %u\n", ev->state, ev->button);
            break;
        }

        case ButtonRelease: {
            XButtonEvent *ev = &event.xbutton;
            switch(ev->button) {
            case Button1: {
                panel_mup(&panel_main);
                mdown = 0;
                break;
            }
            }
            break;
        }

        case KeyPress: {
            XKeyEvent *ev = &event.xkey;
            KeySym sym = XLookupKeysym(ev, 0);//XKeycodeToKeysym(display, ev->keycode, 0)

            char buffer[16];
            //int len;

            XLookupString(ev, buffer, sizeof(buffer), &sym, NULL);

            //debug("KeyEvent: %u %u %u %.*s\n", ev->state, ev->keycode, sym, len, buffer);

            if(ev->state & 4) {
                if(!edit_active()) {
                    break;
                }

                if(sym == 'v') {
                    pasteclipboard();
                }

                if(sym == 'c') {
                    setclipboard();
                }

                if(sym == 'a') {
                    edit_selectall();
                }


                break;
            }

            if(edit_active()) {
                if(sym == XK_Delete) {
                    edit_delete();
                    break;
                }

                long key = keysym2ucs(sym);
                if (key != -1) {
                    edit_char((uint32_t)key, 0);
                } else {
                    edit_char(sym, 1);
                }
                break;
            } else {
                if(sym == XK_Delete) {
                    list_deletesitem();
                }
            }

            break;
        }

        case SelectionNotify: {

            debug("SelectionNotify\n");

            XSelectionEvent *ev = &event.xselection;

            if(ev->property == None) {
                break;
            }

            Atom type;
            int format;
            long unsigned int len, bytes_left;
            void *data;

            XGetWindowProperty(display, window, ev->property, 0, ~0L, True, AnyPropertyType, &type, &format, &len, &bytes_left, (unsigned char**)&data);

            if(!data) {
                break;
            }

            if(ev->property == XdndDATA) {
                char *path = malloc(len);
                memcpy(path, data, len);
                tox_postmessage(TOX_SENDFILES, (FRIEND*)sitem->data - friend, 0xFFFF, path);
                break;
            }

            if(edit_active()) {
                edit_paste(data, len);
            }

            XFree(data);

            break;
        }

        case SelectionRequest: {

            debug("SelectionRequest\n");

            XSelectionRequestEvent *ev = &event.xselectionrequest;

            XEvent resp = {
                .xselection = {
                    .type = SelectionNotify,
                    .property = ev->property,
                    .requestor = ev->requestor,
                    .selection = ev->selection,
                    .target = ev->target,
                    .time = ev->time
                }
            };

            if(ev->target == XA_UTF8_STRING) {

                if(clipboard_id) {
                    XChangeProperty(display, ev->requestor, ev->property, ev->target, 8, PropModeReplace, self.id, sizeof(self.id));
                } else {
                    void *data = malloc(65536);
                    int len = 0;

                    if(sitem->item == ITEM_FRIEND) {
                        len = messages_selection(&messages_friend, data, 65536);
                    }

                    if(sitem->item == ITEM_GROUP) {
                        len = messages_selection(&messages_group, data, 65536);
                    }

                    XChangeProperty(display, ev->requestor, ev->property, ev->target, 8, PropModeReplace, data, len);

                    free(data);
                }

            } else if(ev->target == targets) {
                Atom supported[]={XA_UTF8_STRING};
                XChangeProperty (display,
                    ev->requestor,
                    ev->property,
                    targets,
                    8,
                    PropModeReplace,
                    (unsigned char *)(&supported),
                    sizeof(supported)
                );
            }
            else {
                resp.xselection.property = None;
            }

            XSendEvent(display, ev->requestor, 0, 0, &resp);

            break;
        }

        case ClientMessage:
            {
                XClientMessageEvent *ev = &event.xclient;
                if(ev->window == 0) {
                    void *data;
                    memcpy(&data, &ev->data.s[2], sizeof(void*));
                    tox_message(ev->message_type, ev->data.s[0], ev->data.s[1], data);
                } else {
                    if(ev->message_type == XdndEnter) {
                        debug("enter\n");
                    } else if(ev->message_type == XdndPosition) {
                        Window src = ev->data.l[0];
                        XEvent event = {
                            .xclient = {
                                .type = ClientMessage,
                                .display = display,
                                .window = src,
                                .message_type = XdndStatus,
                                .format = 32,
                                .data = {
                                    .l = {window, 1, 0, 0, XdndActionCopy}
                                }
                            }
                        };

                        XSendEvent(display, src, 0, 0, &event);
                        //debug("position (version=%u)\n", ev->data.l[1] >> 24);
                    } else if(ev->message_type == XdndStatus) {
                        debug("status\n");
                    } else if(ev->message_type == XdndDrop) {
                        XConvertSelection(display, XdndSelection, XA_STRING, XdndDATA, window, CurrentTime);
                        debug("drop\n");
                    } else if(ev->message_type == XdndLeave) {
                        debug("leave\n");
                    } else {
                        debug("dragshit\n");
                    }
                }

                break;
            }

        }
    }

    XFreePixmap(display, drawbuf);

    XftDrawDestroy(xftdraw);
    XftColorFree(display, visual, cmap, &xftcolor);

    XDestroyWindow(display, window);
    XCloseDisplay(display);

    return 0;
}
