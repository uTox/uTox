#include "main.h"

#include "gtk.h"

#include "../flist.h"
#include "../friend.h"
#include "../logging_native.h"
#include "../theme.h"
#include "../tox.h"
#include "../util.h"
#include "../utox.h"
#include "../av/utox_av.h"
#include "../ui/draw.h"
#include "../ui/dropdowns.h"
#include "../ui/edit.h"

// FIXME: Required for UNUSED()
#include "../main.h"

bool hidden = false;

uint32_t tray_width = 32, tray_height = 32;

XIC xic = NULL;

void *ugtk_load(void);
void  ugtk_openfilesend(void);
void  ugtk_openfileavatar(void);
void ugtk_native_select_dir_ft(uint32_t fid, FILE_TRANSFER *file);
void ugtk_file_save_inline(FILE_TRANSFER *file);

void setclipboard(void) {
    XSetSelectionOwner(display, XA_CLIPBOARD, window, CurrentTime);
}

void postmessage_utox(UTOX_MSG msg, uint16_t param1, uint16_t param2, void *data) {
    XEvent event = {
        .xclient = {.window = 0, .type = ClientMessage, .message_type = msg, .format = 8, .data = {.s = { param1, param2 } } }
    };

    memcpy(&event.xclient.data.s[2], &data, sizeof(void *));

    XSendEvent(display, window, False, 0, &event);
    XFlush(display);
}

#include <linux/input.h>
FILE    *ptt_keyboard_handle;
Display *ptt_display;
void     init_ptt(void) {
    settings.push_to_talk = 1;

    char path[UTOX_FILE_NAME_LENGTH];
    snprintf(path, UTOX_FILE_NAME_LENGTH, "%s/.config/tox/ppt-kbd", getenv("HOME")); // TODO DRY

    ptt_keyboard_handle = fopen((const char *)path, "r");
    if (!ptt_keyboard_handle) {
        debug("Could not access ptt-kbd in data directory\n");
        ptt_display = XOpenDisplay(0);
        XSynchronize(ptt_display, True);
    }
}

bool check_ptt_key(void) {
    if (!settings.push_to_talk) {
        // debug("PTT is disabled\n");
        return 1; /* If push to talk is disabled, return true. */
    }
    int ptt_key;

    /* First, we try for direct access to the keyboard. */
    ptt_key = KEY_LEFTCTRL; // TODO allow user to change this...
    if (ptt_keyboard_handle) {
        /* Nice! we have direct access to the keyboard! */
        char key_map[KEY_MAX / 8 + 1]; // Create a byte array the size of the number of keys
        memset(key_map, 0, sizeof(key_map));
        ioctl(fileno(ptt_keyboard_handle), EVIOCGKEY(sizeof(key_map)), key_map); // Fill the keymap with the current
                                                                                 // keyboard state
        int keyb = key_map[ptt_key / 8]; // The key we want (and the seven others around it)
        int mask = 1 << (ptt_key % 8);   // Put 1 in the same column as our key state

        if (keyb & mask) {
            debug("PTT key is down\n");
            return 1;
        } else {
            debug("PTT key is up\n");
            return 0;
        }
    }
    /* Okay nope, lets' fallback to xinput... *pouts*
     * Fall back to Querying the X for the current keymap. */
    ptt_key       = XKeysymToKeycode(display, XK_Control_L);
    char keys[32] = { 0 };
    /* We need our own connection, so that we don't block the main display... No idea why... */
    if (ptt_display) {
        XQueryKeymap(ptt_display, keys);
        if (keys[ptt_key / 8] & (0x1 << (ptt_key % 8))) {
            debug("PTT key is down (according to XQueryKeymap\n");
            return 1;
        } else {
            debug("PTT key is up (according to XQueryKeymap\n");
            return 0;
        }
    }
    /* Couldn't access the keyboard directly, and XQuery failed, this is really bad! */
    debug_error("Unable to access keyboard, you need to read the manual on how to enable utox to\nhave access to your "
                "keyboard.\nDisable push to talk to suppress this message.\n");
    return 0;
}

void exit_ptt(void) {
    if (ptt_keyboard_handle) {
        fclose(ptt_keyboard_handle);
    }
    if (ptt_display) {
        XCloseDisplay(ptt_display);
    }
    settings.push_to_talk = 0;
}

void image_set_scale(NATIVE_IMAGE *image, double scale) {
    uint32_t r = (uint32_t)(65536.0 / scale);

    /* transformation matrix to scale image */
    XTransform trans = { { { r, 0, 0 }, { 0, r, 0 }, { 0, 0, 65536 } } };
    XRenderSetPictureTransform(display, image->rgb, &trans);
    if (image->alpha) {
        XRenderSetPictureTransform(display, image->alpha, &trans);
    }
}

void image_set_filter(NATIVE_IMAGE *image, uint8_t filter) {
    const char *xfilter;
    switch (filter) {
        case FILTER_NEAREST: xfilter  = FilterNearest; break;
        case FILTER_BILINEAR: xfilter = FilterBilinear; break;
        default: debug("Warning: Tried to set image to unrecognized filter(%u).\n", filter); return;
    }
    XRenderSetPictureFilter(display, image->rgb, xfilter, NULL, 0);
    if (image->alpha) {
        XRenderSetPictureFilter(display, image->alpha, xfilter, NULL, 0);
    }
}

void thread(void func(void *), void *args) {
    pthread_t      thread_temp;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 1 << 20);
    pthread_create(&thread_temp, &attr, (void *(*)(void *))func, args);
    pthread_attr_destroy(&attr);
}

void yieldcpu(uint32_t ms) {
    usleep(1000 * ms);
}

uint64_t get_time(void) {
    struct timespec ts;
#ifdef CLOCK_MONOTONIC_RAW
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
#else
    clock_gettime(CLOCK_MONOTONIC, &ts);
#endif
    return ((uint64_t)ts.tv_sec * (1000 * 1000 * 1000)) + (uint64_t)ts.tv_nsec;
}

void openurl(char *str) {
    char *cmd = "xdg-open";
    if (!fork()) {
        execlp(cmd, cmd, str, (char *)0);
        exit(127);
    }
}

void openfilesend(void) {
    if (libgtk) {
        ugtk_openfilesend();
    }
}

void openfileavatar(void) {
    if (libgtk) {
        ugtk_openfileavatar();
    }
}

void setselection(char *data, uint16_t length) {
    if (!length) {
        return;
    }

    memcpy(primary.data, data, length);
    primary.len = length;
    XSetSelectionOwner(display, XA_PRIMARY, window, CurrentTime);
}

/* Tray icon stuff */

#define SYSTEM_TRAY_REQUEST_DOCK 0
#define SYSTEM_TRAY_BEGIN_MESSAGE 1
#define SYSTEM_TRAY_CANCEL_MESSAGE 2

void send_message(Display *dpy, /* display */ Window w, /* sender (tray window) */ long message, /* message opcode */
                  long data1, /* message data 1 */ long data2, /* message data 2 */ long data3 /* message data 3 */) {
    XEvent ev;

    memset(&ev, 0, sizeof(ev));
    ev.xclient.type         = ClientMessage;
    ev.xclient.window       = w;
    ev.xclient.message_type = XInternAtom(dpy, "_NET_SYSTEM_TRAY_OPCODE", False);
    ev.xclient.format       = 32;
    ev.xclient.data.l[0]    = CurrentTime;
    ev.xclient.data.l[1]    = message;
    ev.xclient.data.l[2]    = data1;
    ev.xclient.data.l[3]    = data2;
    ev.xclient.data.l[4]    = data3;

    XSendEvent(dpy, w, False, NoEventMask, &ev);
    XSync(dpy, False);
}

extern uint8_t _binary_icons_utox_128x128_png_start;
extern size_t  _binary_icons_utox_128x128_png_size;

void draw_tray_icon(void) {
    // debug("Draw Tray\n");

    uint16_t width, height;
    uint8_t *icon_data = (uint8_t *)&_binary_icons_utox_128x128_png_start;
    size_t   icon_size = (size_t)&_binary_icons_utox_128x128_png_size;

    NATIVE_IMAGE *icon = utox_image_to_native(icon_data, icon_size, &width, &height, 1);
    if (NATIVE_IMAGE_IS_VALID(icon)) {
        /* Get tray window size */
        int32_t  x_r = 0, y_r = 0;
        uint32_t border_r = 0, xwin_depth_r = 0;
        XMoveResizeWindow(display, tray_window, x_r, y_r, 32, 32);
        XGetGeometry(display, tray_window, &root, &x_r, &y_r, &tray_width, &tray_height, &border_r, &xwin_depth_r);
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
        XRenderComposite(display, PictOpOver, icon->rgb, icon->alpha, trayicon_renderpic, 0, 0, 0, 0, 0, 0, tray_width,
                         tray_height);
        XCopyArea(display, trayicon_drawbuf, tray_window, trayicon_gc, 0, 0, tray_width, tray_height, 0, 0);

        free(icon);
    } else {
        debug("Tray no workie, that not gud!\n");
    }
}

void create_tray_icon(void) {
    tray_window = XCreateSimpleWindow(display, RootWindow(display, screen), 0, 0, tray_width, tray_height, 0,
                                      BlackPixel(display, screen), WhitePixel(display, screen));
    XSelectInput(display, tray_window, ButtonPress);

    /* Get ready to draw a tray icon */
    trayicon_gc        = XCreateGC(display, root, 0, 0);
    trayicon_drawbuf   = XCreatePixmap(display, tray_window, tray_width, tray_height, xwin_depth);
    trayicon_renderpic = XRenderCreatePicture(display, trayicon_drawbuf, pictformat, 0, NULL);
    /* Send icon to the tray */
    send_message(display, XGetSelectionOwner(display, XInternAtom(display, "_NET_SYSTEM_TRAY_S0", False)),
                 SYSTEM_TRAY_REQUEST_DOCK, tray_window, 0, 0);
    /* Draw the tray */
    draw_tray_icon();
    /* Reset the tray draw/picture buffers with the new tray size */
    XFreePixmap(display, trayicon_drawbuf);
    trayicon_drawbuf = XCreatePixmap(display, tray_window, tray_width, tray_height, xwin_depth);
    XRenderFreePicture(display, trayicon_renderpic);
    trayicon_renderpic = XRenderCreatePicture(display, trayicon_drawbuf, pictformat, 0, NULL);
    /* Redraw the tray one last time! */
    draw_tray_icon();
}

void destroy_tray_icon(void) {
    XDestroyWindow(display, tray_window);
}

/** Toggles the main window to/from hidden to tray/shown. */
void togglehide(void) {
    if (hidden) {
        int      x, y;
        uint32_t w, h, border;
        XGetGeometry(display, window, &root, &x, &y, &w, &h, &border, (uint *)&xwin_depth);
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
    switch (event.type) {
        case ConfigureNotify: {
            XConfigureEvent *ev = &event.xconfigure;
            if (tray_width != (uint32_t)ev->width || tray_height != (uint32_t)ev->height) {
                debug("Tray resized w:%i h:%i\n", ev->width, ev->height);

                if ((uint32_t)ev->width > tray_width || (uint32_t)ev->height > tray_height) {
                    tray_width  = ev->width;
                    tray_height = ev->height;

                    XFreePixmap(display, trayicon_drawbuf);
                    trayicon_drawbuf = XCreatePixmap(display, tray_window, tray_width, tray_height,
                                                     24); // TODO get xwin_depth from X not code
                    XRenderFreePicture(display, trayicon_renderpic);
                    trayicon_renderpic = XRenderCreatePicture(
                        display, trayicon_drawbuf, XRenderFindStandardFormat(display, PictStandardRGB24), 0, NULL);
                }

                tray_width  = ev->width;
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

void pasteprimary(void) {
    Window owner = XGetSelectionOwner(display, XA_PRIMARY);
    if (owner) {
        XConvertSelection(display, XA_PRIMARY, XA_UTF8_STRING, targets, window, CurrentTime);
    }
}

void copy(int value) {
    int len;
    if (edit_active()) {
        len = edit_copy((char *)clipboard.data, sizeof(clipboard.data));
    } else if (flist_get_selected()->item == ITEM_FRIEND) {
        len = messages_selection(&messages_friend, clipboard.data, sizeof(clipboard.data), value);
    } else {
        len = messages_selection(&messages_group, clipboard.data, sizeof(clipboard.data), value);
    }

    if (len) {
        clipboard.len = len;
        setclipboard();
    }
}

int hold_x11s_hand(Display *UNUSED(d), XErrorEvent *event) {
    debug_error("X11 err:\tX11 tried to kill itself, so I hit him with a shovel.\n");
    debug_error("    err:\tResource: %lu || Serial %lu\n", event->resourceid, event->serial);
    debug_error("    err:\tError code: %u || Request: %u || Minor: %u \n", event->error_code, event->request_code,
                event->minor_code);
    debug_error("uTox:\tThis would be a great time to submit a bug!\n");

    return 0;
}

void paste(void) {
    Window owner = XGetSelectionOwner(display, XA_CLIPBOARD);

    /* Ask owner for supported types */
    if (owner) {
        XEvent event = {.xselectionrequest = {.type       = SelectionRequest,
                                              .send_event = True,
                                              .display    = display,
                                              .owner      = owner,
                                              .requestor  = window,
                                              .target     = targets,
                                              .selection  = XA_CLIPBOARD,
                                              .property   = XA_ATOM,
                                              .time       = CurrentTime } };

        XSendEvent(display, owner, 0, NoEventMask, &event);
        XFlush(display);
    }
}

void pastebestformat(const Atom atoms[], size_t len, Atom selection) {
    XSetErrorHandler(hold_x11s_hand);
    const Atom supported[] = { XA_PNG_IMG, XA_URI_LIST, XA_UTF8_STRING };
    size_t i, j;
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

static bool ishexdigit(char c) {
    c = toupper(c);
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F');
}

static char hexdecode(char upper, char lower) {
    upper = toupper(upper);
    lower = toupper(lower);
    return (upper >= 'A' ? upper - 'A' + 10 : upper - '0') * 16 + (lower >= 'A' ? lower - 'A' + 10 : lower - '0');
}

void formaturilist(char *out, const char *in, size_t len) {
    size_t i, removed = 0, start = 0;

    for (i = 0; i < len; i++) {
        // Replace CRLF with LF
        if (in[i] == '\r') {
            memcpy(out + start - removed, in + start, i - start);
            start = i + 1;
            removed++;
        } else if (in[i] == '%' && i + 2 < len && ishexdigit(in[i + 1]) && ishexdigit(in[i + 2])) {
            memcpy(out + start - removed, in + start, i - start);
            out[i - removed] = hexdecode(in[i + 1], in[i + 2]);
            start            = i + 3;
            removed += 2;
        }
    }
    if (start != len) {
        memcpy(out + start - removed, in + start, len - start);
    }

    out[len - removed] = 0;
    // out[len - removed - 1] = '\n';
}

// TODO(robinli): Go over this function and see if either len or size are removeable.
void pastedata(void *data, Atom type, size_t len, bool select) {
    // TODO we shouldn't blindly trust this function to return a friend.
    // We need to write another funtion that promises a friend (idealy the last active or null.)
    FRIEND *f = (FRIEND *)flist_get_selected()->data;
    size_t size = (size_t)len;
    if (type == XA_PNG_IMG) {
        uint16_t width, height;

        NATIVE_IMAGE *native_image = utox_image_to_native(data, size, &width, &height, 0);
        if (NATIVE_IMAGE_IS_VALID(native_image)) {
            debug_info("Pasted image: %dx%d\n", width, height);

            UTOX_IMAGE png_image = malloc(size);
            memcpy(png_image, data, size);
            friend_sendimage(f, native_image, width, height, png_image, size);
        }
    } else if (type == XA_URI_LIST) {
        char *path = malloc(len + 1);
        formaturilist(path, (char *)data, len);
        postmessage_toxcore(TOX_FILE_SEND_NEW, f->number, 0xFFFF, path);
    } else if (type == XA_UTF8_STRING && edit_active()) {
        edit_paste(data, len, select);
    }
}

// converts an XImage to a Picture usable by XRender, uses XRenderPictFormat given by
// 'format', uses the default format if it is NULL
Picture ximage_to_picture(XImage *img, const XRenderPictFormat *format) {
    Pixmap pixmap = XCreatePixmap(display, window, img->width, img->height, img->depth);
    GC     legc   = XCreateGC(display, pixmap, 0, NULL);
    XPutImage(display, pixmap, legc, img, 0, 0, 0, 0, img->width, img->height);

    if (format == NULL) {
        format = XRenderFindVisualFormat(display, visual);
    }
    Picture picture = XRenderCreatePicture(display, pixmap, format, 0, NULL);

    XFreeGC(display, legc);
    XFreePixmap(display, pixmap);

    return picture;
}

void loadalpha(int bm, void *data, int width, int height) {
    XImage *img = XCreateImage(display, CopyFromParent, 8, ZPixmap, 0, data, width, height, 8, 0);

    // create picture that only holds alpha values
    // NOTE: the XImage made earlier should really be freed, but calling XDestroyImage on it will also
    // automatically free the data it's pointing to(which we don't want), so there's no easy way to destroy them
    // currently
    bitmap[bm] = ximage_to_picture(img, XRenderFindStandardFormat(display, PictStandardA8));
}

/* generates an alpha bitmask based on the alpha channel in given rgba_data
 * returned picture will have 1 byte for each pixel, and have the same width and height as input
 */
static Picture generate_alpha_bitmask(const uint8_t *rgba_data, uint16_t width, uint16_t height, uint32_t rgba_size) {
    // we don't need to free this, that's done by XDestroyImage()
    uint8_t *out = malloc(rgba_size / 4);
    uint32_t i, j;
    for (i = j = 0; i < rgba_size; i += 4, j++) {
        out[j] = (rgba_data + i)[3] & 0xFF; // take only alpha values
    }

    // create 1-byte-per-pixel image and convert it to a Alpha-format Picture
    XImage *img     = XCreateImage(display, CopyFromParent, 8, ZPixmap, 0, (char *)out, width, height, 8, width);
    Picture picture = ximage_to_picture(img, XRenderFindStandardFormat(display, PictStandardA8));

    XDestroyImage(img);

    return picture;
}

NATIVE_IMAGE *utox_image_to_native(const UTOX_IMAGE data, size_t size, uint16_t *w, uint16_t *h, bool keep_alpha) {
    int      width, height, bpp;
    uint8_t *rgba_data = stbi_load_from_memory(data, size, &width, &height, &bpp, 4);

    if (rgba_data == NULL || width == 0 || height == 0) {
        return None; // invalid png data
    }

    uint32_t rgba_size = width * height * 4;

    // we don't need to free this, that's done by XDestroyImage()
    uint8_t *out = malloc(rgba_size);
    if (out == NULL) {
        debug("utox_image_to_native:\t Could mot allocate memory.\n");
        free(rgba_data);
        return NULL;
    }

    // colors are read into red, blue and green and written into the target pointer
    uint8_t   red, blue, green;
    uint32_t *target;

    uint32_t i;
    for (i = 0; i < rgba_size; i += 4) {
        red   = (rgba_data + i)[0] & 0xFF;
        green = (rgba_data + i)[1] & 0xFF;
        blue  = (rgba_data + i)[2] & 0xFF;

        target  = (uint32_t *)(out + i);
        *target = (red | (red << 8) | (red << 16) | (red << 24)) & visual->red_mask;
        *target |= (blue | (blue << 8) | (blue << 16) | (blue << 24)) & visual->blue_mask;
        *target |= (green | (green << 8) | (green << 16) | (green << 24)) & visual->green_mask;
    }

    XImage *img = XCreateImage(display, visual, xwin_depth, ZPixmap, 0, (char *)out, width, height, 32, width * 4);

    Picture rgb = ximage_to_picture(img, NULL);
    // 4 bpp -> RGBA
    Picture alpha = (bpp == 4 && keep_alpha) ? generate_alpha_bitmask(rgba_data, width, height, rgba_size) : None;

    free(rgba_data);

    *w = width;
    *h = height;

    NATIVE_IMAGE *image = malloc(sizeof(NATIVE_IMAGE));
    if (image == NULL) {
        debug("utox_image_to_native:\t Could mot allocate memory for image.\n");
        return NULL;
    }
    image->rgb   = rgb;
    image->alpha = alpha;

    XDestroyImage(img);
    return image;
}

void image_free(NATIVE_IMAGE *image) {
    if (!image) {
        return;
    }
    XRenderFreePicture(display, image->rgb);
    if (image->alpha) {
        XRenderFreePicture(display, image->alpha);
    }
    free(image);
}

/** Sets file system permissions to something slightly safer.
 *
 * returns 0 and 1 on success and failure.
 */
int ch_mod(uint8_t *file) {
    return chmod((char *)file, S_IRUSR | S_IWUSR);
}

void flush_file(FILE *file) {
    fflush(file);
    int fd = fileno(file);
    fsync(fd);
}

void setscale(void) {
    unsigned int i;
    for (i = 0; i != countof(bitmap); i++) {
        if (bitmap[i]) {
            XRenderFreePicture(display, bitmap[i]);
        }
    }

    svg_draw(0);

    if (xsh) {
        XFree(xsh);
    }

    // TODO, fork this to a function
    xsh             = XAllocSizeHints();
    xsh->flags      = PMinSize;
    xsh->min_width  = SCALE(MAIN_WIDTH);
    xsh->min_height = SCALE(MAIN_HEIGHT);

    XSetWMNormalHints(display, window, xsh);

    if (settings.window_width > (uint32_t)SCALE(MAIN_WIDTH) &&
        settings.window_height > (uint32_t)SCALE(MAIN_HEIGHT)) {
        /* wont get a resize event, call this manually */
        ui_size(settings.window_width, settings.window_height);
    }
}

void setscale_fonts(void) {
    freefonts();
    loadfonts();

    font_small_lineheight = (font[FONT_TEXT].info[0].face->size->metrics.height + (1 << 5)) >> 6;
    // font_msg_lineheight = (font[FONT_MSG].info[0].face->size->metrics.height + (1 << 5)) >> 6;
}

void notify(char *title, uint16_t UNUSED(title_length), const char *msg, uint16_t msg_length, void *object, bool is_group) {
    if (havefocus) {
        return;
    }

    uint8_t *f_cid = NULL;
    if (is_group) {
        // GROUPCHAT *obj = object;
    } else {
        FRIEND *obj = object;
        if (friend_has_avatar(obj)) {
            f_cid = obj->cid;
        }
    }

    XWMHints hints = {.flags = 256 };
    XSetWMHints(display, window, &hints);

#ifdef HAVE_DBUS
    char *str = tohtml(msg, msg_length);

    dbus_notify(title, str, f_cid);

    free(str);
#endif

#ifdef UNITY
    if (unity_running) {
        mm_notify(obj->name, f_cid);
    }
#endif
}

void showkeyboard(bool UNUSED(show)) {}

void edit_will_deactivate(void) {}

void update_tray(void) {}

void config_osdefaults(UTOX_SAVE *r) {
    r->window_x      = 0;
    r->window_y      = 0;
    r->window_width  = DEFAULT_WIDTH;
    r->window_height = DEFAULT_HEIGHT;
}

static UTOX_LANG systemlang(void) {
    char *str = getenv("LC_ALL");
    if (!str) {
        str = getenv("LC_MESSAGES");
    }
    if (!str) {
        str = getenv("LANG");
    }
    if (!str) {
        return DEFAULT_LANG;
    }
    return ui_guess_lang_by_posix_locale(str, DEFAULT_LANG);
}

int main(int argc, char *argv[]) {
    bool   theme_was_set_on_argv;
    int8_t should_launch_at_startup;
    int8_t set_show_window;
    bool   no_updater;

#ifdef HAVE_DBUS
    debug_info("Compiled with dbus support!\n");
#endif

    parse_args(argc, argv, &theme_was_set_on_argv, &should_launch_at_startup, &set_show_window, &no_updater);

    if (should_launch_at_startup == 1 || should_launch_at_startup == -1) {
        debug_notice("Start on boot not supported on this OS, please use your distro suggested method!\n");
    }

    if (no_updater == true) {
        debug_notice("Disabling the updater is not supported on this OS. Updates are managed by your distro's package "
                     "manager.\n");
    }

    XInitThreads();

    if ((display = XOpenDisplay(NULL)) == NULL) {
        debug_error("Cannot open display, must exit\n");
        return 1;
    }

    XSetErrorHandler(hold_x11s_hand);

    XIM xim;
    setlocale(LC_ALL, "");
    XSetLocaleModifiers("");
    if ((xim = XOpenIM(display, 0, 0, 0)) == NULL) {
        debug_error("Cannot open input method\n");
    }

    LANG                       = systemlang();
    dropdown_language.selected = dropdown_language.over = LANG;

    screen     = DefaultScreen(display);
    cmap       = DefaultColormap(display, screen);
    visual     = DefaultVisual(display, screen);
    gc         = DefaultGC(display, screen);
    xwin_depth = DefaultDepth(display, screen);
    scr        = DefaultScreenOfDisplay(display);
    root       = RootWindow(display, screen);

    XSetWindowAttributes attrib = {
        .background_pixel = WhitePixel(display, screen),
        .border_pixel     = BlackPixel(display, screen),
        .event_mask       = ExposureMask | ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask
                      | PointerMotionMask
                      | StructureNotifyMask
                      | KeyPressMask
                      | KeyReleaseMask
                      | FocusChangeMask
                      | PropertyChangeMask,
    };
    /* Also has as override_redirect member. I think this may be what we need for crating a clean mouse menu for the
     * tray icon */

    /* load save data */
    UTOX_SAVE *save = config_load();

    if (!theme_was_set_on_argv) {
        settings.theme = save->theme;
    }

    utox_init();

    debug_info("Setting theme to:\t%d\n", settings.theme);
    theme_load(settings.theme);

    /* create window */
    window = XCreateWindow(display, root, save->window_x, save->window_y, settings.window_width, settings.window_height,
                           0, xwin_depth, InputOutput, visual, CWBackPixmap | CWBorderPixel | CWEventMask, &attrib);

    /* choose available libraries for optional UI stuff */
    if (!(libgtk = ugtk_load())) {
        // try Qt
    }

    /* start the tox thread */
    thread(toxcore_thread, NULL);

    /* load atoms */
    wm_protocols     = XInternAtom(display, "WM_PROTOCOLS", 0);
    wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", 0);
    XA_CLIPBOARD     = XInternAtom(display, "CLIPBOARD", 0);
    XA_NET_NAME = XInternAtom(display, "_NET_WM_NAME", 0), XA_UTF8_STRING = XInternAtom(display, "UTF8_STRING", 1);

    if (XA_UTF8_STRING == None) {
        XA_UTF8_STRING = XA_STRING;
    }
    targets = XInternAtom(display, "TARGETS", 0);

    XA_INCR = XInternAtom(display, "INCR", False);

    XdndAware      = XInternAtom(display, "XdndAware", False);
    XdndEnter      = XInternAtom(display, "XdndEnter", False);
    XdndLeave      = XInternAtom(display, "XdndLeave", False);
    XdndPosition   = XInternAtom(display, "XdndPosition", False);
    XdndStatus     = XInternAtom(display, "XdndStatus", False);
    XdndDrop       = XInternAtom(display, "XdndDrop", False);
    XdndSelection  = XInternAtom(display, "XdndSelection", False);
    XdndDATA       = XInternAtom(display, "XdndDATA", False);
    XdndActionCopy = XInternAtom(display, "XdndActionCopy", False);

    XA_URI_LIST = XInternAtom(display, "text/uri-list", False);
    XA_PNG_IMG  = XInternAtom(display, "image/png", False);

    XRedraw = XInternAtom(display, "XRedraw", False);

    /* create the draw buffer */
    drawbuf = XCreatePixmap(display, window, settings.window_width, settings.window_height, xwin_depth);

    /* catch WM_DELETE_WINDOW */
    XSetWMProtocols(display, window, &wm_delete_window, 1);

    /* set WM_CLASS */
    XClassHint hint = {.res_name = "utox", .res_class = "utox" };

    XSetClassHint(display, window, &hint);

    /* set drag and drog version */
    Atom dndversion = 3;
    XChangeProperty(display, window, XdndAware, XA_ATOM, 32, PropModeReplace, (uint8_t *)&dndversion, 1);

    char title_name[128];
    snprintf(title_name, 128, "%s %s (version: %s)", TITLE, SUB_TITLE, VERSION);
    // Effett, I give up! No OS can agree how to handle non ascii bytes, so effemm!
    // may be needed when uTox becomes muTox
    // memmove(title_name, title_name+1, strlen(title_name))
    /* set the window name */
    XSetStandardProperties(display, window, title_name, "uTox", None, argv, argc, None);

    /* initialize fontconfig */
    initfonts();

    /* Set the default font so we don't segfault on ui_set_scale() when it goes looking for fonts. */
    loadfonts();
    setfont(FONT_TEXT);

    /* load fonts and scalable bitmaps */
    ui_set_scale(save->scale + 1);

    /* done with save */
    free(save);

    /* load the used cursors */
    cursors[CURSOR_NONE]     = XCreateFontCursor(display, XC_left_ptr);
    cursors[CURSOR_HAND]     = XCreateFontCursor(display, XC_hand2);
    cursors[CURSOR_TEXT]     = XCreateFontCursor(display, XC_xterm);
    cursors[CURSOR_SELECT]   = XCreateFontCursor(display, XC_crosshair);
    cursors[CURSOR_ZOOM_IN]  = XCreateFontCursor(display, XC_target);
    cursors[CURSOR_ZOOM_OUT] = XCreateFontCursor(display, XC_target);

    /* */
    XGCValues gcval;
    gcval.foreground     = XWhitePixel(display, 0);
    gcval.function       = GXxor;
    gcval.background     = XBlackPixel(display, 0);
    gcval.plane_mask     = gcval.background ^ gcval.foreground;
    gcval.subwindow_mode = IncludeInferiors;

    /* GC for the */
    grabgc = XCreateGC(display, RootWindow(display, screen), GCFunction | GCForeground | GCBackground | GCSubwindowMode,
                       &gcval);

    XWindowAttributes attr;
    XGetWindowAttributes(display, root, &attr);

    pictformat = XRenderFindVisualFormat(display, attr.visual);
    // XRenderPictFormat *pictformat = XRenderFindStandardFormat(display, PictStandardA8);

    /* Xft draw context/color */
    renderpic = XRenderCreatePicture(display, drawbuf, pictformat, 0, NULL);

    XRenderColor xrcolor = { 0 };
    colorpic             = XRenderCreateSolidFill(display, &xrcolor);

    /*xftdraw = XftDrawCreate(display, drawbuf, visual, cmap);
    XRenderColor xrcolor;
    xrcolor.red = 0x0;
    xrcolor.green = 0x0;
    xrcolor.blue = 0x0;
    xrcolor.alpha = 0xffff;
    XftColorAllocValue(display, visual, cmap, &xrcolor, &xftcolor);*/

    if (set_show_window) {
        if (set_show_window == 1) {
            settings.start_in_tray = 0;
        } else if (set_show_window == -1) {
            settings.start_in_tray = 1;
        }
    }

    /* make the window visible */
    if (settings.start_in_tray) {
        togglehide();
    } else {
        XMapWindow(display, window);
    }

    if (xim) {
        if ((xic = XCreateIC(xim, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, XNClientWindow, window,
                             XNFocusWindow, window, NULL))) {
            XSetICFocus(xic);
        } else {
            debug_error("Cannot open input method\n");
            XCloseIM(xim);
            xim = 0;
        }
    }

    /* set the width/height of the drawing region */
    ui_size(settings.window_width, settings.window_height);

    create_tray_icon();
/* Registers the app in the Unity MM */
#ifdef UNITY
    unity_running = is_unity_running();
    if (unity_running) {
        mm_register();
    }
#endif

    /* draw */
    panel_draw(&panel_root, 0, 0, settings.window_width, settings.window_height);

    /* event loop */
    while (1) {
        /* block on the first event, then process all events */
        XEvent event;

        XNextEvent(display, &event);
        if (!doevent(event)) {
            break;
        }

        while (XPending(display)) {
            XNextEvent(display, &event);
            if (!doevent(event)) {
                goto BREAK;
            }
        }

        if (_redraw) {
            panel_draw(&panel_root, 0, 0, settings.window_width, settings.window_height);
            _redraw = 0;
        }
    }
BREAK:

    postmessage_utoxav(UTOXAV_KILL, 0, 0, NULL);
    postmessage_toxcore(TOX_KILL, 0, 0, NULL);

    /* free client thread stuff */
    if (libgtk) {
    }

    destroy_tray_icon();

    Window       root_return, child_return;
    int          x_return, y_return;
    unsigned int width_return, height_return, i;
    XGetGeometry(display, window, &root_return, &x_return, &y_return, &width_return, &height_return, &i, &i);

    XTranslateCoordinates(display, window, root_return, 0, 0, &x_return, &y_return, &child_return);

    UTOX_SAVE d = {
        .window_x      = x_return < 0 ? 0 : x_return,
        .window_y      = y_return < 0 ? 0 : y_return,
        .window_width  = width_return,
        .window_height = height_return,
    };

    config_save(&d);

    FcFontSetSortDestroy(fs);
    freefonts();

    XFreePixmap(display, drawbuf);

    XFreeGC(display, grabgc);

    XRenderFreePicture(display, renderpic);
    XRenderFreePicture(display, colorpic);

    if (xic)
        XDestroyIC(xic);
    if (xim)
        XCloseIM(xim);

    XDestroyWindow(display, window);
    XCloseDisplay(display);

/* Unregisters the app from the Unity MM */
#ifdef UNITY
    if (unity_running) {
        mm_unregister();
    }
#endif

    // wait for tox_thread to exit
    while (tox_thread_init) {
        yieldcpu(1);
    }

    debug_error("XLIB main:\tClean exit\n");

    return 0;
}

/* Dummy functions used in other systems... */
/* Used in windows only... */
void launch_at_startup(int UNUSED(is_launch_at_startup)) {}
