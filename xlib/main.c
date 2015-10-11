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

#define _GNU_SOURCE
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <pthread.h>
#include <unistd.h>
#include <locale.h>
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

#ifdef UNITY
#include <messaging-menu/messaging-menu.h>
#include <unity.h>
#include "mmenu.c"
#endif

/* Main window */
Display  *display;
int      screen;
Window   root, window;
GC       gc;
Colormap cmap;
Visual   *visual;
Pixmap   drawbuf;
Picture  renderpic;
Picture  colorpic;
_Bool    hidden = 0;
XRenderPictFormat *pictformat;


/* Tray icon window */
Window   tray_window;
Pixmap   trayicon_drawbuf;
Picture  trayicon_renderpic;
GC       trayicon_gc;
uint32_t tray_width = 32, tray_height = 32;

Picture bitmap[BM_ENDMARKER];
Cursor cursors[8];

/* Screen grab vars */
uint8_t pointergrab;
int grabx, graby, grabpx, grabpy;
GC grabgc;

XSizeHints *xsh;

_Bool havefocus = 0;

Window video_win[MAX_NUM_FRIENDS];

Atom wm_protocols, wm_delete_window;

uint32_t scolor;

Atom XA_CLIPBOARD, XA_UTF8_STRING, targets, XA_INCR;
Atom XdndAware, XdndEnter, XdndLeave, XdndPosition, XdndStatus, XdndDrop, XdndSelection, XdndDATA, XdndActionCopy;
Atom XA_URI_LIST, XA_PNG_IMG;
Atom XRedraw;

_Bool _redraw;

uint16_t drawwidth, drawheight;

XIC xic = NULL;

XImage *screen_image;


/* pointers to dynamically loaded libs */
void *libgtk;
#include "gtk.c"

#include "freetype.c"

_Bool utox_portable;

struct {
    int len;
    char_t data[65536]; //TODO: De-hardcode this value.
} clipboard;

struct {
    int len;
    char_t data[65536]; //TODO: De-hardcode this value.
} primary;

struct {
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


#include <linux/input.h>
FILE *ptt_keyboard_handle;
Display *ptt_display;
void init_ptt(void){
    push_to_talk = 1;
    uint8_t path[UTOX_FILE_NAME_LENGTH], *p;
    p = path + datapath(path);
    strcpy((char*)p, "ptt-kbd");

    ptt_keyboard_handle = fopen((const char*)path, "r");
    if (!ptt_keyboard_handle){
        debug("Could not access ptt-kbd in data directory\n");
        ptt_display = XOpenDisplay(0);
        XSynchronize(ptt_display, TRUE);
    }

}

_Bool check_ptt_key(void){
    if (!push_to_talk) {
        // debug("PTT is disabled\n");
        return 1; /* If push to talk is disabled, return true. */
    }
    int ptt_key;

    /* First, we try for direct access to the keyboard. */
    ptt_key = KEY_LEFTCTRL;                                      // TODO allow user to change this...
    if (ptt_keyboard_handle) {
        /* Nice! we have direct access to the keyboard! */
        char key_map[KEY_MAX/8 + 1];                             // Create a byte array the size of the number of keys
        memset(key_map, 0, sizeof(key_map));
        ioctl(fileno(ptt_keyboard_handle), EVIOCGKEY(sizeof(key_map)), key_map); // Fill the keymap with the current keyboard state
        int keyb = key_map[ptt_key/8];                           // The key we want (and the seven others around it)
        int mask = 1 << (ptt_key % 8);                           // Put 1 in the same column as our key state

        if (keyb & mask){
            // debug("PTT key is down\n");
            return 1;
        } else {
            // debug("PTT key is up\n");
            return 0;
        }
    }
    /* Okay nope, lets' fallback to xinput... *pouts*
     * Fall back to Querying the X for the current keymap. */
    ptt_key = XKeysymToKeycode(display, XK_Control_L);
    char keys[32] = {0};
    /* We need our own connection, so that we don't block the main display... No idea why... */
    if ( ptt_display ) {
        XQueryKeymap(ptt_display, keys);
        if (keys[ptt_key/8] & (0x1 << ( ptt_key % 8 ))) {
            // debug("PTT key is down (according to XQueryKeymap\n");
            return 1;
        } else {
            // debug("PTT key is up (according to XQueryKeymap\n");
            return 0;
        }
    }
    /* Couldn't access the keyboard directly, and XQuery failed, this is really bad! */
    debug("Unable to access keyboard, you need to read the manual on how to enable utox to\nhave access to your key"
          "board.\nDisable push to talk to suppress this message.\n");
    return 0;

}

void exit_ptt(void){
    if (ptt_keyboard_handle){
        fclose(ptt_keyboard_handle);
    }
    if (ptt_display) {
        XCloseDisplay(ptt_display);
    }
    push_to_talk = 0;
}

void image_set_scale(UTOX_NATIVE_IMAGE *image, double scale)
{
    uint32_t r = (uint32_t)(65536.0 / scale);

    /* transformation matrix to scale image */
    XTransform trans = {
        {{r, 0, 0},
         {0, r, 0},
         {0, 0, 65536}}
    };
    XRenderSetPictureTransform(display, image->rgb, &trans);
    if (image->alpha) {
        XRenderSetPictureTransform(display, image->alpha, &trans);
    }
}

void image_set_filter(UTOX_NATIVE_IMAGE *image, uint8_t filter)
{
    const char *xfilter;
    switch (filter) {
    case FILTER_NEAREST:
        xfilter = FilterNearest;
        break;
    case FILTER_BILINEAR:
        xfilter = FilterBilinear;
        break;
    default:
        debug("Warning: Tried to set image to unrecognized filter(%u).\n", filter);
        return;
    }
    XRenderSetPictureFilter(display, image->rgb, xfilter, NULL, 0);
    if (image->alpha) {
        XRenderSetPictureFilter(display, image->alpha, xfilter, NULL, 0);
    }
}

void draw_image(const UTOX_NATIVE_IMAGE *image, int x, int y, uint32_t width, uint32_t height, uint32_t imgx, uint32_t imgy)
{
    XRenderComposite(display, PictOpOver, image->rgb, image->alpha, renderpic, imgx, imgy, imgx, imgy, x, y, width, height);
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

static int _drawtext(int x, int xmax, int y, char_t *str, STRING_IDX length)
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
            if(x + g->xadvance + SCALE * 5 > xmax && length) {
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

void draw_rect_frame(int x, int y, int width, int height, uint32_t color) {
    XSetForeground(display, gc, color);
    XDrawRectangle(display, drawbuf, gc, x, y, width - 1, height - 1);
}

void drawrect(int x, int y, int right, int bottom, uint32_t color)
{
    XSetForeground(display, gc, color);
    XFillRectangle(display, drawbuf, gc, x, y, right - x, bottom - y);
}

void draw_rect_fill(int x, int y, int width, int height, uint32_t color) {
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


void openurl(char_t *str)
{
    char *cmd = "xdg-open";
#ifdef __APPLE__
    cmd = "open";
#endif
    if(!fork()) {
        execlp(cmd, cmd, str, (char *)0);
        exit(127);
    }
}

void openfilesend(void)
{
    if(libgtk) {
        gtk_openfilesend();
    }
}

void openfileavatar(void)
{
    if(libgtk) {
        gtk_openfileavatar();
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
            fwrite(file->path, file->size, 1, fp);
            fclose(fp);

            free(file->path);
            file->path = (uint8_t*)strdup("inline.png");
            file->inline_png = 0;
        }
    }
}

void setselection(char_t *data, STRING_IDX length)
{
    if(!length) {
        return;
    }

    memcpy(primary.data, data, length);
    primary.len = length;
    XSetSelectionOwner(display, XA_PRIMARY, window, CurrentTime);
}

/* Tray icon stuff */

#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2

void send_message(Display* dpy, /* display */ Window w, /* sender (tray window) */ long message, /* message opcode */
                  long data1, /* message data 1 */ long data2, /* message data 2 */ long data3    /* message data 3 */ ){
    XEvent ev;

    memset(&ev, 0, sizeof(ev));
    ev.xclient.type = ClientMessage;
    ev.xclient.window = w;
    ev.xclient.message_type = XInternAtom (dpy, "_NET_SYSTEM_TRAY_OPCODE", False );
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = CurrentTime;
    ev.xclient.data.l[1] = message;
    ev.xclient.data.l[2] = data1;
    ev.xclient.data.l[3] = data2;
    ev.xclient.data.l[4] = data3;

    XSendEvent(dpy, w, False, NoEventMask, &ev);
    XSync(dpy, False);
}

extern uint8_t _binary_icons_utox_128x128_png_start;
extern size_t  _binary_icons_utox_128x128_png_size;

void draw_tray_icon(void){
    // debug("Draw Tray\n");

    uint16_t width, height;
    uint8_t *icon_data = (uint8_t*)&_binary_icons_utox_128x128_png_start;
    size_t  icon_size  = (size_t)&_binary_icons_utox_128x128_png_size;

    UTOX_NATIVE_IMAGE *icon = png_to_image(icon_data, icon_size, &width, &height, 1);
    if(UTOX_NATIVE_IMAGE_IS_VALID(icon)) {
        /* Get tray window size */
        int32_t x_r, y_r;
        uint32_t border_r, depth_r;
        XGetGeometry(display, tray_window, &root, &x_r, &y_r, &tray_width, &tray_height, &border_r, &depth_r);
        /* TODO use xcb instead of xlib here!
        xcb_get_geometry_cookie_t xcb_get_geometry (xcb_connection_t *connection,
                                                xcb_drawable_t    drawable );
        xcb_get_geometry_reply_t *xcb_get_geometry_reply (xcb_connection_t          *connection,
                                                      xcb_get_geometry_cookie_t  cookie,
                                                      xcb_generic_error_t      **error);
        free (geom);*/
        // debug("Tray size == %i x %i\n", tray_width, tray_height);

        /* Resize the image from what the system tray dock tells us to be */
        double scale = (tray_width > tray_height) ? (double)tray_height / width : (double)tray_width / height;
        image_set_scale(icon, scale);
        image_set_filter(icon, FILTER_BILINEAR);

        /* Draw the image and copy to the window */
        XSetForeground(display, trayicon_gc, 0xFFFFFF);
        XFillRectangle(display, trayicon_drawbuf, trayicon_gc, 0, 0, tray_width, tray_height);
        /* TODO: copy method of grabbing background for tray from tray.c:tray_update_root_bg_pmap() (stalonetray) */
        XRenderComposite(display, PictOpOver, icon->rgb, icon->alpha, trayicon_renderpic, 0, 0, 0, 0, 0, 0, tray_width, tray_height);
        XCopyArea(display, trayicon_drawbuf, tray_window, trayicon_gc, 0, 0, tray_width, tray_height, 0, 0);

        free(icon);
    } else {
        debug("Tray no workie, that not gud!\n");
    }
}

void create_tray_icon(void){
    tray_window = XCreateSimpleWindow(display, RootWindow(display, screen), 0, 0, tray_width, tray_height, 0, BlackPixel(display, screen), WhitePixel(display, screen));
    XSelectInput(display, tray_window, ButtonPress);

    /* Get ready to draw a tray icon */
    trayicon_gc        = XCreateGC(display, root, 0, 0);
    trayicon_drawbuf   = XCreatePixmap(display, tray_window, tray_width, tray_height, depth);
    trayicon_renderpic = XRenderCreatePicture(display, trayicon_drawbuf, pictformat, 0, NULL);
    /* Send icon to the tray */
    send_message(display, XGetSelectionOwner(display, XInternAtom(display, "_NET_SYSTEM_TRAY_S0", False)), SYSTEM_TRAY_REQUEST_DOCK, tray_window, 0, 0);
    /* Draw the tray */
    draw_tray_icon();
    /* Reset the tray draw/picture buffers with the new tray size */
    XFreePixmap(display, trayicon_drawbuf);
    trayicon_drawbuf = XCreatePixmap(display, tray_window, tray_width, tray_height, depth);
    XRenderFreePicture(display, trayicon_renderpic);
    trayicon_renderpic = XRenderCreatePicture(display, trayicon_drawbuf, pictformat, 0, NULL);
    /* Redraw the tray one last time! */
    draw_tray_icon();
}

void destroy_tray_icon(void)
{
    XDestroyWindow(display, tray_window);
}

/** Toggles the main window to/from hidden to tray/shown. */
void togglehide(void) {
    if(hidden) {
        int x, y;
        uint32_t w, h, border;
        XGetGeometry(display, window, &root, &x, &y, &w, &h, &border, (uint*)&depth);
        XMapWindow(display, window);
        XMoveWindow(display, window, x, y);
        redraw();
        hidden = 0;
    } else {
        XWithdrawWindow(display, window, screen);
        hidden = 1;
    }
}

void tray_window_event(XEvent event) {
    switch(event.type){
    case ConfigureNotify: {
        XConfigureEvent *ev = &event.xconfigure;
        if(tray_width != ev->width || tray_height != ev->height) {
            debug("Tray resized w:%i h:%i\n", ev->width, ev->height);

            if(ev->width > tray_width || ev->height > tray_height) {
                tray_width = ev->width;
                tray_height = ev->height;

                XFreePixmap(display, trayicon_drawbuf);
                trayicon_drawbuf = XCreatePixmap(display, tray_window, tray_width, tray_height, 24); // TODO get depth from X not code
                XRenderFreePicture(display, trayicon_renderpic);
                trayicon_renderpic = XRenderCreatePicture(display, trayicon_drawbuf,XRenderFindStandardFormat(display, PictStandardRGB24), 0, NULL);
            }

            tray_width = ev->width;
            tray_height = ev->height;

            draw_tray_icon();
        }

        break;
    }
    case ButtonPress: {
        XButtonEvent *ev = &event.xbutton;

        if (ev->button == Button1) {
            togglehide();
        }
    }
    }
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
    int len;
    if(edit_active()) {
        len = edit_copy(clipboard.data, sizeof(clipboard.data));
    } else if(selected_item->item == ITEM_FRIEND) {
        len = messages_selection(&messages_friend, clipboard.data, sizeof(clipboard.data), value);
    } else {
        len = messages_selection(&messages_group, clipboard.data, sizeof(clipboard.data), value);
    }

    if(len) {
        clipboard.len = len;
        setclipboard();
    }

}

int hold_x11s_hand(Display *d, XErrorEvent *event) {
    debug("X11 err:\tX11 tried to kill itself, so I hit him with a shovel.\n");
    debug("    err:\tResource: %lu || Serial %lu\n", event->resourceid, event->serial);
    debug("    err:\tError code: %u || Request: %u || Minor: %u \n",
          event->error_code, event->request_code, event->minor_code);
    debug("uTox:\tThis would be a great time to submit a bug!\n");

    return 0;
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

static void pastebestformat(const Atom atoms[], int len, Atom selection) {
    XSetErrorHandler(hold_x11s_hand);
    const Atom supported[] = {XA_PNG_IMG, XA_URI_LIST, XA_UTF8_STRING};
    int i, j;
    for (i = 0; i < len; i++) {
        char *name = XGetAtomName(display, atoms[i]);
        if (name) {
            debug("Supported type: %s\n", name);
        } else {
            debug("Unsupported type!!: Likely a bug, please report!\n");
        }
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
    //out[len - removed - 1] = '\n';
}

static void pastedata(void *data, Atom type, int len, _Bool select)
{
   if (0 > len) {
       return; // Let my conscience be clear about signed->unsigned casts.
   }
   size_t size = (size_t) len;
   if (type == XA_PNG_IMG) {
        uint16_t width, height;

        UTOX_NATIVE_IMAGE *native_image = png_to_image(data, size, &width, &height, 0);
        if (UTOX_NATIVE_IMAGE_IS_VALID(native_image)) {
            debug("Pasted image: %dx%d\n", width, height);

            UTOX_PNG_IMAGE png_image = malloc(size);
            memcpy(png_image, data, size);
            friend_sendimage((FRIEND*)selected_item->data, native_image, width, height, png_image, size);
        }
    } else if (type == XA_URI_LIST) {
        char *path = malloc(len + 1);
        formaturilist(path, (char*) data, len);
        tox_postmessage(TOX_SEND_NEW_FILE, (FRIEND*)selected_item->data - friend, 0xFFFF, path);
    } else if(type == XA_UTF8_STRING && edit_active()) {
        edit_paste(data, len, select);
    }
}

// converts an XImage to a Picture usable by XRender, uses XRenderPictFormat given by
// 'format', uses the default format if it is NULL
static Picture ximage_to_picture(XImage *img, const XRenderPictFormat *format)
{
    Pixmap pixmap = XCreatePixmap(display, window, img->width, img->height, img->depth);
    GC legc = XCreateGC(display, pixmap, 0, NULL);
    XPutImage(display, pixmap, legc, img, 0, 0, 0, 0, img->width, img->height);

    if (format == NULL) {
        format = XRenderFindVisualFormat(display, visual);
    }
    Picture picture = XRenderCreatePicture(display, pixmap, format, 0, NULL);

    XFreeGC(display, legc);
    XFreePixmap(display, pixmap);

    return picture;
}

void loadalpha(int bm, void *data, int width, int height)
{
    XImage *img = XCreateImage(display, CopyFromParent, 8, ZPixmap, 0, data, width, height, 8, 0);

    // create picture that only holds alpha values
    // NOTE: the XImage made earlier should really be freed, but calling XDestroyImage on it will also
    // automatically free the data it's pointing to(which we don't want), so there's no easy way to destroy them currently
    bitmap[bm] = ximage_to_picture(img, XRenderFindStandardFormat(display, PictStandardA8));
}


/* generates an alpha bitmask based on the alpha channel in given rgba_data
 * returned picture will have 1 byte for each pixel, and have the same width and height as input
 */
static Picture generate_alpha_bitmask(const uint8_t *rgba_data, uint16_t width, uint16_t height, uint32_t rgba_size)
{
    // we don't need to free this, that's done by XDestroyImage()
    uint8_t *out = malloc(rgba_size / 4);
    uint32_t i, j;
    for (i = j = 0; i < rgba_size; i += 4, j++) {
        out[j] = (rgba_data+i)[3] & 0xFF; // take only alpha values
    }

    // create 1-byte-per-pixel image and convert it to a Alpha-format Picture
    XImage *img = XCreateImage(display, CopyFromParent, 8, ZPixmap, 0, (char*)out, width, height, 8, width);
    Picture picture = ximage_to_picture(img, XRenderFindStandardFormat(display, PictStandardA8));

    XDestroyImage(img);

    return picture;
}

UTOX_NATIVE_IMAGE *png_to_image(const UTOX_PNG_IMAGE data, size_t size, uint16_t *w, uint16_t *h, _Bool keep_alpha)
{
    uint8_t *rgba_data;
    unsigned width, height;
    unsigned r = lodepng_decode32(&rgba_data, &width, &height, data->png_data, size);

    if(r != 0 || !width || !height) {
        return None; // invalid png data
    }

    uint32_t rgba_size = width * height * 4;

    // we don't need to free this, that's done by XDestroyImage()
    uint8_t *out = malloc(rgba_size);

    // colors are read into red, blue and green and written into the target pointer
    uint8_t red, blue, green;
    uint32_t *target;

    uint32_t i;
    for (i = 0; i < rgba_size; i += 4) {
        red = (rgba_data+i)[0] & 0xFF;
        green = (rgba_data+i)[1] & 0xFF;
        blue = (rgba_data+i)[2] & 0xFF;

        target = (uint32_t *)(out+i);
        *target = (red | (red << 8) | (red << 16) | (red << 24)) & visual->red_mask;
        *target |= (blue | (blue << 8) | (blue << 16) | (blue << 24)) & visual->blue_mask;
        *target |= (green | (green << 8) | (green << 16) | (green << 24)) & visual->green_mask;
    }

    XImage *img = XCreateImage(display, visual, depth, ZPixmap, 0, (char*)out, width, height, 32, width * 4);

    Picture rgb = ximage_to_picture(img, NULL);
    Picture alpha = (keep_alpha) ? generate_alpha_bitmask(rgba_data, width, height, rgba_size) : None;

    free(rgba_data);

    *w = width;
    *h = height;

    UTOX_NATIVE_IMAGE *image = malloc(sizeof(UTOX_NATIVE_IMAGE));
    image->rgb = rgb;
    image->alpha = alpha;

    XDestroyImage(img);
    return image;
}

void image_free(UTOX_NATIVE_IMAGE *image)
{
    XRenderFreePicture(display, image->rgb);
    if (image->alpha) {
        XRenderFreePicture(display, image->alpha);
    }
    free(image);
}

int datapath_old(uint8_t *dest)
{
    return 0;
}

int datapath(uint8_t *dest)
{
    if (utox_portable) {
        int l = sprintf((char*)dest, "./tox");
        mkdir((char*)dest, 0700);
        dest[l++] = '/';

        return l;
    } else {
        char *home = getenv("HOME");
        int l = sprintf((char*)dest, "%.230s/.config/tox", home);
        mkdir((char*)dest, 0700);
        dest[l++] = '/';

        return l;
    }
}

int datapath_subdir(uint8_t *dest, const char *subdir)
{
    int l = datapath(dest);
    l += sprintf((char*)(dest+l), "%s", subdir);
    mkdir((char*)dest, 0700);
    dest[l++] = '/';

    return l;
}

/** Sets file system permissions to something slightly safer.
 *
 * returns 0 and 1 on success and failure.
 */
int ch_mod(uint8_t *file){
    return chmod((char*)file, S_IRUSR | S_IWUSR);
}

void flush_file(FILE *file)
{
    fflush(file);
    int fd = fileno(file);
    fsync(fd);
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

    if(xsh) {
        XFree(xsh);
    }

    // TODO, fork this to a function
    xsh = XAllocSizeHints();
    xsh->flags = PMinSize;
    xsh->min_width = 320 * SCALE;
    xsh->min_height = 160 * SCALE;

    XSetWMNormalHints(display, window, xsh);

    if(utox_window_width > 320 * SCALE && utox_window_height > 160 * SCALE)
    {
        /* wont get a resize event, call this manually */
        ui_size(utox_window_width, utox_window_height);
    }
}

void setscale_fonts(void)
{
    freefonts();
    loadfonts();

    font_small_lineheight = (font[FONT_TEXT].info[0].face->size->metrics.height + (1 << 5)) >> 6;
    //font_msg_lineheight = (font[FONT_MSG].info[0].face->size->metrics.height + (1 << 5)) >> 6;
}

int file_lock(FILE *file, uint64_t start, size_t length){
    int result = -1;
    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = start;
    fl.l_len = length;

    result = fcntl(fileno(file), F_SETLK, &fl);
    if(result != -1){
        return 1;
    } else {
        return 0;
    }
}

int file_unlock(FILE *file, uint64_t start, size_t length){
    int result = -1;
    struct flock fl;
    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = start;
    fl.l_len = length;

    result = fcntl(fileno(file), F_SETLK, &fl);
    if(result != -1){
        return 1;
    } else {
        return 0;
    }
}

void notify(char_t *title, STRING_IDX title_length, char_t *msg, STRING_IDX msg_length, FRIEND *f) {
    if(havefocus) {
        return;
    }

    XWMHints hints = {.flags = 256};
    XSetWMHints(display, window, &hints);

    #ifdef HAVE_DBUS
    char_t *str = tohtml(msg, msg_length);

    uint8_t *f_cid = NULL;
    if(friend_has_avatar(f)) {
        f_cid = f->cid;
    }

    dbus_notify((char*)title, (char*)str, (uint8_t*)f_cid);

    free(str);
    #endif

    #ifdef UNITY
    if(unity_running) {
        mm_notify(f->name, f->cid);
    }
    #endif
}

void showkeyboard(_Bool show) {}

void edit_will_deactivate(void) {}

void redraw(void) {
    _redraw = 1;
}

void force_redraw(void) {
    XEvent ev = {
        .xclient = {
            .type = ClientMessage,
            .display = display,
            .window = window,
            .message_type = XRedraw,
            .format = 8,
            .data = {
                .s = {0,0}
            }
        }
    };
    _redraw = 1;
    XSendEvent(display, window, 0, 0, &ev);
    XFlush(display);
}

void update_tray(void) {}

#include "event.c"
void config_osdefaults(UTOX_SAVE *r) {
    r->window_x = 0;
    r->window_y = 0;
    r->window_width = DEFAULT_WIDTH;
    r->window_height = DEFAULT_HEIGHT;
}

static int systemlang(void) {
    char *str = getenv("LC_MESSAGES");
    if(!str) {
        str = getenv("LANG");
    }
    if(!str) {
        return DEFAULT_LANG;
    }
    return ui_guess_lang_by_posix_locale(str, DEFAULT_LANG);
}

_Bool parse_args_wait_for_theme;

int main(int argc, char *argv[])
{
    parse_args_wait_for_theme = 0;
    _Bool theme_was_set_on_argv = 0;
    theme = THEME_DEFAULT;
    /* Variables for --set */
    int32_t set_show_window = 0;

    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            if (parse_args_wait_for_theme) {
                if(!strcmp(argv[i], "default")) {
                    theme = THEME_DEFAULT;
                    parse_args_wait_for_theme = 0;
                    theme_was_set_on_argv = 1;
                    continue;
                }
                if(!strcmp(argv[i], "dark")) {
                    theme = THEME_DARK;
                    parse_args_wait_for_theme = 0;
                    theme_was_set_on_argv = 1;
                    continue;
                }
                if(!strcmp(argv[i], "light")) {
                    theme = THEME_LIGHT;
                    parse_args_wait_for_theme = 0;
                    theme_was_set_on_argv = 1;
                    continue;
                }
                if(!strcmp(argv[i], "highcontrast")) {
                    theme = THEME_HIGHCONTRAST;
                    parse_args_wait_for_theme = 0;
                    theme_was_set_on_argv = 1;
                    continue;
                }
                if(!strcmp(argv[i], "zenburn")) {
                    theme = THEME_ZENBURN;
                    parse_args_wait_for_theme = 0;
                    theme_was_set_on_argv = 1;
                    continue;
                }
                debug("Please specify correct theme (please check user manual for list of correct values).");
                return 1;
            }

            if(!strcmp(argv[i], "--version")) {
                debug("%s\n", VERSION);
                return 0;
            } else if(!strcmp(argv[i], "--portable")) {
                debug("Launching uTox in portable mode: All data will be saved to the tox folder in the current working directory\n");
                utox_portable = 1;
            } else if(!strcmp(argv[i], "--theme")) {
                parse_args_wait_for_theme = 1;
            } else if(strncmp(argv[i], "--set", 5) == 0) {
                if(strncmp(argv[i]+5, "=", 1) == 0){
                    if(strcmp(argv[i]+6, "start-on-boot") == 0){
                        debug("Start on boot not supported on this OS, please use your distro suggested method!\n");
                    } else if(strcmp(argv[i]+6, "show-window") == 0){
                        set_show_window = 1;
                    } else if(strcmp(argv[i]+6, "hide-window") == 0){
                        set_show_window = -1;
                    }
                } else {
                    if(argv[i+1]){
                        if(strcmp(argv[i+1], "start-on-boot") == 0){
                            debug("Start on boot not supported on this OS, please use your distro suggested method!\n");
                        } else if(strcmp(argv[i+1], "show-window") == 0){
                            set_show_window = 1;
                        } else if(strcmp(argv[i+1], "hide-window") == 0){
                            set_show_window = -1;
                        }
                    }
                }
            }
            printf("arg %d: %s\n", i, argv[i]);
        }
    }

    if (parse_args_wait_for_theme) {
        debug("Expected theme name, but got nothing. -_-\n");
        return 0;
    }

    XInitThreads();

    if((display = XOpenDisplay(NULL)) == NULL) {
        printf("Cannot open display\n");

        return 1;
    }

    XSetErrorHandler(hold_x11s_hand);

    XIM xim;
    setlocale(LC_ALL, "");
    XSetLocaleModifiers("");
    if((xim = XOpenIM(display, 0, 0, 0)) == NULL) {
        printf("Cannot open input method\n");
    }

    LANG = systemlang();
    dropdown_language.selected = dropdown_language.over = LANG;

    screen  = DefaultScreen(display);
    cmap    = DefaultColormap(display, screen);
    visual  = DefaultVisual(display, screen);
    gc      = DefaultGC(display, screen);
    depth   = DefaultDepth(display, screen);
    scr     = DefaultScreenOfDisplay(display);
    root    = RootWindow(display, screen);

    XSetWindowAttributes attrib = {
        .background_pixel = WhitePixel(display, screen),
        .border_pixel = BlackPixel(display, screen),
        .event_mask = ExposureMask | ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask |
                    PointerMotionMask | StructureNotifyMask | KeyPressMask | KeyReleaseMask | FocusChangeMask |
                    PropertyChangeMask,
    };

    /* load save data */
    UTOX_SAVE *save = config_load();

    if (!theme_was_set_on_argv) {
        theme = save->theme;
    }
    printf("%d\n", theme);
    theme_load(theme);

    /* create window */
    window = XCreateWindow(display, root, save->window_x, save->window_y, save->window_width, save->window_height, 0, depth, InputOutput, visual, CWBackPixmap | CWBorderPixel | CWEventMask, &attrib);

    /* choose available libraries for optional UI stuff */
    if(!(libgtk = gtk_load())) {
        //try Qt
    }

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

    XdndAware       = XInternAtom(display, "XdndAware", False);
    XdndEnter       = XInternAtom(display, "XdndEnter", False);
    XdndLeave       = XInternAtom(display, "XdndLeave", False);
    XdndPosition    = XInternAtom(display, "XdndPosition", False);
    XdndStatus      = XInternAtom(display, "XdndStatus", False);
    XdndDrop        = XInternAtom(display, "XdndDrop", False);
    XdndSelection   = XInternAtom(display, "XdndSelection", False);
    XdndDATA        = XInternAtom(display, "XdndDATA", False);
    XdndActionCopy  = XInternAtom(display, "XdndActionCopy", False);

    XA_URI_LIST     = XInternAtom(display, "text/uri-list", False);
    XA_PNG_IMG      = XInternAtom(display, "image/png", False);

    XRedraw         = XInternAtom(display, "XRedraw", False);

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

    char title_name[128];
    snprintf(title_name, 128, "%s %s (version: %s)", TITLE, SUB_TITLE, VERSION);
    // Effett, I give up! No OS can agree how to handle non ascii bytes, so effemm!
    // may be needed when uTox becomes muTox
    //memmove(title_name, title_name+1, strlen(title_name))
    /* set the window name */
    XSetStandardProperties(display, window, title_name, "uTox", None, argv, argc, None);

    /* initialize fontconfig */
    initfonts();

    /* Set the default font so we don't segfault on ui_scale() when it goes looking for fonts. */
    loadfonts();
    setfont(FONT_TEXT);

    /* load fonts and scalable bitmaps */
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

    XWindowAttributes attr;
    XGetWindowAttributes(display, root, &attr);

    pictformat = XRenderFindVisualFormat(display, attr.visual);
    // XRenderPictFormat *pictformat = XRenderFindStandardFormat(display, PictStandardA8);

    /* Xft draw context/color */
    renderpic = XRenderCreatePicture (display, drawbuf, pictformat, 0, NULL);

    XRenderColor xrcolor = {0};
    colorpic = XRenderCreateSolidFill(display, &xrcolor);

    /*xftdraw = XftDrawCreate(display, drawbuf, visual, cmap);
    XRenderColor xrcolor;
    xrcolor.red = 0x0;
    xrcolor.green = 0x0;
    xrcolor.blue = 0x0;
    xrcolor.alpha = 0xffff;
    XftColorAllocValue(display, visual, cmap, &xrcolor, &xftcolor);*/

    if(set_show_window){
        if(set_show_window == 1){
            start_in_tray = 0;
        } else if(set_show_window == -1){
            start_in_tray = 1;
        }
    }

    /* make the window visible */
    if (start_in_tray) {
        togglehide();
    } else {
        XMapWindow(display, window);
    }

    if (xim) {
        if((xic = XCreateIC(xim, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, XNClientWindow, window, XNFocusWindow, window, NULL))) {
            XSetICFocus(xic);
        } else {
            printf("Cannot open input method\n");
            XCloseIM(xim);
            xim = 0;
        }
    }

    /* set the width/height of the drawing region */
    utox_window_width = DEFAULT_WIDTH;
    utox_window_height = DEFAULT_HEIGHT;
    ui_size(utox_window_width, utox_window_height);

    /* wait for the tox thread to finish initializing */
    while(!tox_thread_init) {
        yieldcpu(1);
    }

    create_tray_icon();
    /* Registers the app in the Unity MM */
    #ifdef UNITY
    unity_running = is_unity_running();
    if(unity_running) {
        mm_register();
    }
    #endif

    /* set up the contact list */
    list_start();

    /* draw */
    panel_draw(&panel_root, 0, 0, utox_window_width, utox_window_height);

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
            panel_draw(&panel_root, 0, 0, utox_window_width, utox_window_height);
            _redraw = 0;
        }
    }
    BREAK:

    toxaudio_postmessage(AUDIO_KILL, 0, 0, NULL);
    toxvideo_postmessage(VIDEO_KILL, 0, 0, NULL);
    toxav_postmessage(TOXAV_KILL, 0, 0, NULL);
    tox_postmessage(TOX_KILL, 0, 0, NULL);

    /* free client thread stuff */
    if(libgtk) {

    }

    destroy_tray_icon();

    Window root_return, child_return;
    int x_return, y_return;
    unsigned int width_return, height_return, i;
    XGetGeometry(display, window, &root_return, &x_return, &y_return, &width_return, &height_return, &i, &i);

    XTranslateCoordinates(display, window, root_return, 0, 0, &x_return, &y_return, &child_return);

    UTOX_SAVE d = {
        .window_x = x_return < 0 ? 0 : x_return,
        .window_y = y_return < 0 ? 0 : y_return,
        .window_width = width_return,
        .window_height = height_return,
        .theme = theme,
    };

    config_save(&d);

    FcFontSetSortDestroy(fs);
    freefonts();

    XFreePixmap(display, drawbuf);

    XFreeGC(display, grabgc);

    XRenderFreePicture(display, renderpic);
    XRenderFreePicture(display, colorpic);

    if (xic) XDestroyIC(xic);
    if (xim) XCloseIM(xim);

    XDestroyWindow(display, window);
    XCloseDisplay(display);

    /* Unregisters the app from the Unity MM */
    #ifdef UNITY
    if(unity_running) {
        mm_unregister();
    }
    #endif

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
    uint8_t *new_data = malloc(attrs.width * attrs.height * 4);
    if(new_data && (attrs.width != width || attrs.height != height)) {
        scale_rgbx_image(img_data, width, height, new_data, attrs.width, attrs.height);
        image.data = (char*)new_data;
    }


    GC default_gc = DefaultGC(display, screen);
    Pixmap pixmap = XCreatePixmap(display, window, attrs.width, attrs.height, depth);
    XPutImage(display, pixmap, default_gc, &image, 0, 0, 0, 0, attrs.width, attrs.height);
    XCopyArea(display, pixmap, video_win[id], default_gc, 0, 0, attrs.width, attrs.height, 0, 0);
    XFreePixmap(display, pixmap);
    free(new_data);
}

void video_begin(uint32_t id, char_t *name, STRING_IDX name_length, uint16_t width, uint16_t height)
{
    Window *win = &video_win[id];
    if(*win) {
        return;
    }

    *win = XCreateSimpleWindow(display, RootWindow(display, screen), 0, 0, width, height, 0, BlackPixel(display, screen), WhitePixel(display, screen));

    // Fallback name in ISO8859-1.
    XStoreName(display, *win, "Video Preview");
    // UTF-8 name for those WMs that can display it.
    XChangeProperty(display, *win,
                    XInternAtom(display, "_NET_WM_NAME", False),
                    XInternAtom(display, "UTF8_STRING", False),
                    8, PropModeReplace, name, name_length);

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

    // Indicate that we support desktop capturing.
    postmessage(NEW_VIDEO_DEVICE, STR_VIDEO_IN_DESKTOP, 0, (void*)1);

    #ifdef __APPLE__
    #else
    int i;
    for(i = 0; i != 64; i++) {
        snprintf(dev_name + 10, sizeof(dev_name) - 10, "%i", i);

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
            postmessage(NEW_VIDEO_DEVICE, UI_STRING_ID_INVALID, 1, p);
        } else {
            postmessage(NEW_VIDEO_DEVICE, UI_STRING_ID_INVALID, 0, p);
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
        utox_v4l_fd = -1;

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
    if(utox_v4l_fd == -1) {
        return 1;
    }

    return v4l_startread();
}

_Bool video_endread(void)
{
    if(utox_v4l_fd == -1) {
        return 1;
    }

    return v4l_endread();
}

int video_getframe(uint8_t *y, uint8_t *u, uint8_t *v, uint16_t width, uint16_t height)
{
    if(utox_v4l_fd == -1) {
        static uint64_t lasttime;
        uint64_t t = get_time();
        if(t - lasttime >= (uint64_t)1000 * 1000 * 1000 / 24) {
            XShmGetImage(deskdisplay,RootWindow(deskdisplay, deskscreen), screen_image, video_x, video_y, AllPlanes);
            if (width != video_width || height != video_height) {
                debug("width/height mismatch %u %u != %u %u\n", width, height, screen_image->width, screen_image->height);
                return 0;
            }

            bgrxtoyuv420(y, u, v, (uint8_t*)screen_image->data, screen_image->width, screen_image->height);
            lasttime = t;
            return 1;
        }
        return 0;
    }

    return v4l_getframe(y, u, v, width, height);
}

void launch_at_startup(int is_launch_at_startup)
{
}
