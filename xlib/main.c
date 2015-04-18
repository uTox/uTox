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

// TODO replace xlib (old slow and broken) with xcb
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_util.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include "xembed.h"

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


// xlib globals
Display *display;
int screen;
Window window;
xcb_window_t tray_window;
GC gc;
Colormap cmap;
Visual *visual;

// xcb globals
xcb_connection_t *xcb_connection;

Picture bitmap[BM_CI1 + 1];

Cursor cursors[8];

uint8_t pointergrab;
int grabx, graby, grabpx, grabpy;
GC grabgc;

XSizeHints *xsh;

_Bool havefocus = 0;

// create a window handle for each possible friend.
Window video_win[MAX_NUM_FRIENDS];

Atom wm_protocols, wm_delete_window;

uint32_t scolor;

Atom XA_CLIPBOARD, XA_UTF8_STRING, targets, XA_INCR;
Atom XdndAware, XdndEnter, XdndLeave, XdndPosition, XdndStatus, XdndDrop, XdndSelection, XdndDATA, XdndActionCopy;
Atom XA_URI_LIST, XA_PNG_IMG;
Atom XRedraw;

// Atom tray_wm_name; remove?

Pixmap drawbuf, drawbuf_tray;
Picture renderpic;
Picture colorpic;

_Bool _redraw;

uint16_t drawwidth, drawheight;

XIC xic = NULL;

XImage *screen_image;


/* pointers to dynamically loaded libs */
void *libgtk;
#include "gtk.c"

#include "freetype.c"

_Bool utox_portable;

struct
{
    int len;
    char_t data[65536]; //TODO: De-hardcode this value.
}clipboard;

struct
{
    int len;
    char_t data[65536]; //TODO: De-hardcode this value.
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

xcb_intern_atom_cookie_t *emwh_cookie;
xcb_ewmh_connection_t    *ewmh_conn;

xcb_intern_atom_cookie_t tray_atom_cookie;

static void init_xcb(){
    xcb_connection = xcb_connect(NULL, NULL);
    // For now we only bind to the first screen's systray, todo allow user to pick the systray screen!

    // Get the ewmh connection, and cookies
    ewmh_conn = calloc(1, sizeof(*ewmh_conn));
    emwh_cookie = xcb_ewmh_init_atoms(xcb_connection, ewmh_conn);
    xcb_ewmh_init_atoms_replies(ewmh_conn, emwh_cookie, (void *)0);

    tray_atom_cookie = xcb_intern_atom(xcb_connection, false, strlen("_NET_SYSTEM_TRAY_S0"), "_NET_SYSTEM_TRAY_S0");
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

void enddraw(int x, int y, int width, int height){
    XCopyArea(display, drawbuf, window, gc, x, y, width, height, x, y);
    XCopyArea(display, drawbuf, tray_window, gc, x, y, width, height, x, y);
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
    } else if(sitem->item == ITEM_FRIEND) {
        len = messages_selection(&messages_friend, clipboard.data, sizeof(clipboard.data), value);
    } else {
        len = messages_selection(&messages_group, clipboard.data, sizeof(clipboard.data), value);
    }

    if(len) {
        clipboard.len = len;
        setclipboard();
    }

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
            friend_sendimage((FRIEND*)sitem->data, native_image, width, height, png_image, size);
        }
    } else if (type == XA_URI_LIST) {
        char *path = malloc(len + 1);
        formaturilist(path, (char*) data, len);
        tox_postmessage(TOX_SEND_NEW_FILE, (FRIEND*)sitem->data - friend, 0xFFFF, path);
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

    XImage *img = XCreateImage(display, visual, 24, ZPixmap, 0, (char*)out, width, height, 32, width * 4);

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
 * returns 0 and 1 on sucess and failure.
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

    if(utox_window_width > 320 * SCALE && utox_window_height > 160 * SCALE)
    {
        /* wont get a resize event, call this manually */
        ui_size(utox_window_width, utox_window_height);
    }
}

void notify(char_t *title, STRING_IDX title_length, char_t *msg, STRING_IDX msg_length, FRIEND *f)
{
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

void showkeyboard(_Bool show){
    // Used in android, does nothing here.
}

void redraw(void)
{
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

void update_tray(void){
    // Coming soon, promise!
}

#include "event.c"

int depth;
Screen *scr;

void config_osdefaults(UTOX_SAVE *r)
{
    r->window_x = 0;
    r->window_y = 0;
    r->window_width = DEFAULT_WIDTH;
    r->window_height = DEFAULT_HEIGHT;
}

static int systemlang(void)
{
    char *str = getenv("LC_MESSAGES");
    if(!str) {
        str = getenv("LANG");
    }
    if(!str) {
        return DEFAULT_LANG;
    }
    return ui_guess_lang_by_posix_locale(str, DEFAULT_LANG);
}

void utox_send_window_to_systray(xcb_window_t tray, xcb_window_t dock){
    #define SYSTEM_TRAY_REQUEST_DOCK    0
    #define SYSTEM_TRAY_BEGIN_MESSAGE   1
    #define SYSTEM_TRAY_CANCEL_MESSAGE  2

    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(xcb_connection, tray_atom_cookie, NULL);

    /* Call xcb and ask to shove a window into the tray */
    xcb_client_message_event_t *tray_reqest = calloc(1, sizeof(*tray_reqest));
    tray_reqest->response_type = XCB_CLIENT_MESSAGE;
    tray_reqest->window = dock;
    tray_reqest->type = reply->atom;
    tray_reqest->format = 32;
    tray_reqest->data.data32[0] = XCB_CURRENT_TIME;
    tray_reqest->data.data32[1] = SYSTEM_TRAY_REQUEST_DOCK;
    tray_reqest->data.data32[2] = tray;
    tray_reqest->data.data32[3] = 0;
    tray_reqest->data.data32[4] = 0;

    debug("systray %u, trayw %u\n", dock, tray);


    xcb_send_event(xcb_connection, 0, dock, XCB_EVENT_MASK_NO_EVENT, (const char*)tray_reqest);
    xcb_flush(xcb_connection);
    free(tray_reqest);
}

/*static Window search_for_systray_window(Display *disp, Window target_window){

    Atom     net_wm_type = XInternAtom(display, "_NET_WM_WINDOW_TYPE", True);
    Atom     type_return;
    int32_t format_return;
    uint64_t nitems;
    uint64_t bytes_after;
    uint8_t  *prop_return;

    XGetWindowProperty(disp, target_window, net_wm_type, 0, 1,False, XA_ATOM, &type_return, &format_return,
                            &nitems, &bytes_after, &prop_return);


    Atom     search_type = XInternAtom(display, "WM_NAME", True);
    Atom     type_return2;
    int32_t format_return2;
    uint64_t nitems2;
    uint64_t bytes_after2;
    uint8_t  *prop_return2;

    XGetWindowProperty(disp, target_window, search_type, 0, 40,False, XA_STRING, &type_return2, &format_return2,
                            &nitems2, &bytes_after2, &prop_return2);

    if(prop_return){
        if(*prop_return == 96){
            debug("property number %5s\n", prop_return);
            debug("property name   %s\n", prop_return2);
            debug("wind id         %lu\n", target_window);
            XFree(prop_return);
            return target_window;
        } else {
            XFree(prop_return);
        }
    } else {
        Window root_window_return;
        Window parent_window_return;
        Window *children_each_return;
        Window resulted_window;
        uint32_t number_children_return = 0;
        if(XQueryTree(display, target_window, &root_window_return, &parent_window_return, &children_each_return, &number_children_return)){
            for(uint32_t i = 0; i < number_children_return; i++){
                resulted_window = search_for_systray_window(display, children_each_return[i]);
                if(resulted_window){
                    debug("Resulted win %lu\n", resulted_window);
                    return resulted_window;
                }
            }
        }
    }
    return 0;
}*/

xcb_window_t utox_xcb_find_systray(xcb_connection_t *conn, xcb_window_t windoww) {
    xcb_get_property_cookie_t cookie_prop;
    xcb_get_property_reply_t *reply_prop;

    xcb_atom_t type_atom        = XCB_ATOM_ATOM;
    xcb_atom_t net_wm_type      = ewmh_conn->_NET_WM_WINDOW_TYPE;

    cookie_prop = xcb_get_property(conn, 0, windoww, net_wm_type, type_atom, 0, 1);
    if((reply_prop = xcb_get_property_reply(conn, cookie_prop, NULL))) {
        int len = xcb_get_property_value_length(reply_prop);
        if(len != 0){
            xcb_atom_t net_wm_type_dock = ewmh_conn->_NET_WM_WINDOW_TYPE_DOCK;
            xcb_atom_t tmp = *((xcb_atom_t*)xcb_get_property_value(reply_prop));
            if(tmp == net_wm_type_dock){
                debug("got the dock\n\n");
                free(reply_prop);
                return windoww;
            }
        }
    }
    xcb_query_tree_cookie_t cookie_tree;
    xcb_query_tree_reply_t *reply_tree;
    cookie_tree = xcb_query_tree(conn, windoww);
    if((reply_tree = xcb_query_tree_reply(conn, cookie_tree, NULL))) {
        xcb_window_t *children = xcb_query_tree_children(reply_tree);
        for (int i = 0; i < xcb_query_tree_children_length(reply_tree); i++){
            xcb_window_t result = utox_xcb_find_systray(conn, children[i]);
            if(result){
                free(reply_prop);
                return result;
            }
        }
    } else {
        debug("NO PROP REPLY BLERG!!!! = %u\n", windoww);
    }
    free(reply_prop);
    return 0;
}

void utox_xcb_requests(){
    // Does nothing yet
}

void utox_tray_draw(){

    uint32_t size;
    uint8_t *icon_data = file_raw("./icons/utox-64x64.png", &size);
    if (!icon_data) {
        return;
    }

    uint16_t w, h;
    UTOX_NATIVE_IMAGE *image = png_to_image((UTOX_PNG_IMAGE)icon_data, size, &w, &h, 1);
    if(!UTOX_NATIVE_IMAGE_IS_VALID(image)) {
        draw_image(image, 0, 0, 64, 64, 0, 0); // TODO we'll need to get w&h
    }

    free(icon_data);
}

/*
 * Dev notes about moving to xcb; think of it like a client server model (That's what it is!)
 * You should make all of your requests as early as you know you want them, then let xcb make the actual request, and
 * then fetch the responses as you actually need the data. For uTox just throw your request into the provided function:
 * utox_xcb_requests, then reference your needed information in you real function. Be kind and make sure you comment
 * where the next dev can find it!
 */


static void utox_create_tray_icon(){
    /* Get the first screen */
    xcb_screen_iterator_t  screen_iter = xcb_setup_roots_iterator(xcb_get_setup(xcb_connection));
    xcb_screen_t          *tray_screen      = screen_iter.data;

    /* Find the system tray we need to dock to! */
    xcb_window_t systray = utox_xcb_find_systray(xcb_connection, tray_screen->root);



    /* Create the tray window */
    tray_window = xcb_generate_id(xcb_connection);
    debug("systray %u, trayw %u\n", systray, tray_window);

    uint32_t  mask = 0;
    uint32_t  values[2];
    mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    values[0] = tray_screen->white_pixel;
    values[1] = XCB_EVENT_MASK_EXPOSURE     | XCB_EVENT_MASK_KEY_PRESS      | XCB_EVENT_MASK_BUTTON_PRESS     |
                XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_KEY_RELEASE    | XCB_EVENT_MASK_BUTTON_RELEASE   |
                XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_STRUCTURE_NOTIFY |
                XCB_EVENT_MASK_PROPERTY_CHANGE ;


    xcb_create_window(xcb_connection, depth, tray_window, tray_screen->root, 0, 0, 64, 64, 0,
                      XCB_WINDOW_CLASS_COPY_FROM_PARENT, tray_screen->root_visual, mask, values);

    // Set the generic WM name
    xcb_icccm_set_wm_name(xcb_connection, tray_window, XA_STRING, 8, 9, "uTox Tray");
    xcb_icccm_set_wm_class(xcb_connection, tray_window, 4, "uTox");
    // Set the _NET icon name string
    xcb_change_property(xcb_connection, XCB_PROP_MODE_REPLACE, tray_window, ewmh_conn->_NET_WM_ICON_NAME, XA_STRING,
                        8, 14, "uTox tray icon");

    // Todo replace
    #include "../icons/utox_icon128.h"
    int length = (2 + (128 * 128));
    Atom net_wm_icon = XInternAtom(display, "_NET_WM_ICON", False);
    XChangeProperty(display, tray_window, net_wm_icon, XA_CARDINAL, 32, PropModeReplace, (const unsigned char*)&utox_icon128, length);




    /* Map the window on the screen */
    xcb_map_window(xcb_connection, tray_window );
    // We don't actually need to map the window to make it tray... probably

    utox_send_window_to_systray(tray_window, systray);

    // /* Make sure commands are sent before we pause so that the tray_window gets shown */
    xcb_flush(xcb_connection);

            // Pixmap icon = XCreateBitmapFromData(display, tray_window, (const unsigned char*)utox_icon128, 128, 128);
            // XWMHints *tray_wm_hints = XAllocWMHints();
            // tray_wm_hints->icon_pixmap = icon;
            // tray_wm_hints->flags = IconPixmapHint;
            // XSetWMHints(display, tray_window, tray_wm_hints);

            // Xlib version.
            // Window systray = search_for_systray_window(display, XDefaultRootWindow(display));
}


_Bool parse_args_wait_for_theme;

int main(int argc, char *argv[]){

    // Call all the xcb init's we'll need.
    init_xcb();

    parse_args_wait_for_theme = 0;
    theme = THEME_DEFAULT;

    if (argc > 1)
        for (int i = 1; i < argc; i++) {
            if (parse_args_wait_for_theme) {
                if(!strcmp(argv[i], "default")) {
                    theme = THEME_DEFAULT;
                    parse_args_wait_for_theme = 0;
                    continue;
                }
                if(!strcmp(argv[i], "dark")) {
                    theme = THEME_DARK;
                    parse_args_wait_for_theme = 0;
                    continue;
                }
                if(!strcmp(argv[i], "light")) {
                    theme = THEME_LIGHT;
                    parse_args_wait_for_theme = 0;
                    continue;
                }
                if(!strcmp(argv[i], "highcontrast")) {
                    theme = THEME_HIGHCONTRAST;
                    parse_args_wait_for_theme = 0;
                    continue;
                }
                debug("Please specify correct theme (please check user manual for list of correct values).");
                return 1;
            }

            if(!strcmp(argv[i], "--version")) {
                debug("%s\n", VERSION);
                return 0;
            }
            if(!strcmp(argv[i], "--portable")) {
                debug("Launching uTox in portable mode: All data will be saved to the tox folder in the current working directory\n");
                utox_portable = 1;
            }
            if(!strcmp(argv[i], "--theme")) {
                parse_args_wait_for_theme = 1;
            }
            printf("arg %d: %s\n", i, argv[i]);
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

    XIM xim;
    setlocale(LC_ALL, "");
    XSetLocaleModifiers("");
    if((xim = XOpenIM(display, 0, 0, 0)) == NULL) {
        printf("Cannot open input method\n");
    }

    screen = DefaultScreen(display);
    cmap = DefaultColormap(display, screen);
    visual = DefaultVisual(display, screen);
    gc = DefaultGC(display, screen);
    depth = DefaultDepth(display, screen);
    // Check color bit here (See github issue);
    scr = DefaultScreenOfDisplay(display);

    XSetWindowAttributes attrib = {
        .background_pixel = WhitePixel(display, screen),
        .border_pixel = BlackPixel(display, screen),
        .event_mask = ExposureMask | ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask |
                    PointerMotionMask | StructureNotifyMask | KeyPressMask | KeyReleaseMask | FocusChangeMask |
                    PropertyChangeMask,
    };

    theme_load(theme);

    /* load save data */
    UTOX_SAVE *save = config_load();

    /* create window */
    window = XCreateWindow(display, RootWindow(display, screen), save->window_x, save->window_y, save->window_width, save->window_height, 0, depth, InputOutput, visual, CWBackPixmap | CWBorderPixel | CWEventMask, &attrib);

    /* create tray icon window */
    utox_create_tray_icon();


    /* choose available libraries for optional UI stuff */
    if(!(libgtk = gtk_load())) {
        //try Qt
    }

    /* start the tox thread */
    thread(tox_thread, NULL);

    /* load atoms */
    wm_protocols     = XInternAtom(display, "WM_PROTOCOLS", 0);
    wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", 0);

    XA_CLIPBOARD     = XInternAtom(display, "CLIPBOARD", 0);
    XA_UTF8_STRING   = XInternAtom(display, "UTF8_STRING", 1);
    if(XA_UTF8_STRING == None) {
        XA_UTF8_STRING = XA_STRING;
    }
    targets         = XInternAtom(display, "TARGETS", 0);
    XA_INCR         = XInternAtom(display, "INCR", False);

    XdndAware       = XInternAtom(display, "XdndAware",      False);
    XdndEnter       = XInternAtom(display, "XdndEnter",      False);
    XdndLeave       = XInternAtom(display, "XdndLeave",      False);
    XdndPosition    = XInternAtom(display, "XdndPosition",   False);
    XdndStatus      = XInternAtom(display, "XdndStatus",     False);
    XdndDrop        = XInternAtom(display, "XdndDrop",       False);
    XdndSelection   = XInternAtom(display, "XdndSelection",  False);
    XdndDATA        = XInternAtom(display, "XdndDATA",       False);
    XdndActionCopy  = XInternAtom(display, "XdndActionCopy", False);

    XA_URI_LIST     = XInternAtom(display, "text/uri-list", False);
    XA_PNG_IMG      = XInternAtom(display, "image/png", False);

    XRedraw = XInternAtom(display, "XRedraw", False);

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
    dropdown_language.selected = dropdown_language.over = LANG;

    /* make the window visible */
    XMapWindow(display, window);

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
    panel_draw(&panel_main, 0, 0, utox_window_width, utox_window_height);

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
            panel_draw(&panel_main, 0, 0, utox_window_width, utox_window_height);
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
    Pixmap pixmap = XCreatePixmap(display, window, attrs.width, attrs.height, 24);
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

int video_getframe(vpx_image_t *image)
{
    if(utox_v4l_fd == -1) {
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
