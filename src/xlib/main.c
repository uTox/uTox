#include "main.h"

#include "dbus.h"
#include "freetype.h"
#include "gtk.h"
#include "tray.h"
#include "window.h"

#include "../avatar.h"
#include "../debug.h"
#include "../filesys.h"
#include "../flist.h"
#include "../friend.h"
#include "../macros.h"
#include "../main.h" // MAIN_WIDTH, MAIN_WIDTH, DEFAULT_SCALE, parse_args, utox_init
#include "../settings.h"
#include "../stb.h"
#include "../text.h"
#include "../theme.h"
#include "../tox.h"
#include "../updater.h"
#include "../utox.h"

#include "../av/utox_av.h"

#include "../native/image.h"
#include "../native/notify.h"
#include "../native/ui.h"

#include "../ui/draw.h"
#include "../ui/edit.h"

#include "../layout/background.h"
#include "../layout/sidebar.h"
#include "../layout/friend.h"
#include "../layout/group.h"
#include "../layout/settings.h"

#include <ctype.h>
#include <locale.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

bool hidden = false;

XIC xic = NULL;
static XSizeHints *xsh = NULL;
static bool shutdown = false;

void setclipboard(void) {
    XSetSelectionOwner(display, XA_CLIPBOARD, main_window.window, CurrentTime);
}

void postmessage_utox(UTOX_MSG msg, uint16_t param1, uint16_t param2, void *data) {
    XEvent event = {
        .xclient = {
            .window = 0,
            .type = ClientMessage,
            .message_type = msg,
            .format = 8,
            .data = {
                .s = {
                    param1,
                    param2
                }
            }
        }
    };

    memcpy(&event.xclient.data.s[2], &data, sizeof(void *));

    XSendEvent(display, main_window.window, False, 0, &event);
    XFlush(display);
}

static FILE *   ptt_keyboard_handle;
static Display *ptt_display;

void init_ptt(void) {
    settings.push_to_talk = 1;

    char path[UTOX_FILE_NAME_LENGTH];
    snprintf(path, UTOX_FILE_NAME_LENGTH, "%s/.config/tox/ppt-kbd", getenv("HOME")); // TODO DRY

    ptt_keyboard_handle = fopen((const char *)path, "r");
    if (!ptt_keyboard_handle) {
        LOG_TRACE("XLIB", "Could not access ptt-kbd in data directory" );
        ptt_display = XOpenDisplay(0);
        XSynchronize(ptt_display, True);
    }
}



#ifdef __linux__
#include <linux/input.h>
static bool linux_check_ptt(void) {
    /* First, we try for direct access to the keyboard. */
    int ptt_key = KEY_LEFTCTRL; // TODO allow user to change this...
    if (ptt_keyboard_handle) {
        /* Nice! we have direct access to the keyboard! */
        char key_map[KEY_MAX / 8 + 1]; // Create a byte array the size of the number of keys
        memset(key_map, 0, sizeof(key_map));
        ioctl(fileno(ptt_keyboard_handle), EVIOCGKEY(sizeof(key_map)), key_map); // Fill the keymap with the current
                                                                                 // keyboard state
        int keyb = key_map[ptt_key / 8]; // The key we want (and the seven others around it)
        int mask = 1 << (ptt_key % 8);   // Put 1 in the same column as our key state

        if (keyb & mask) {
            LOG_TRACE("XLIB", "PTT key is down" );
            return true;
        } else {
            LOG_TRACE("XLIB", "PTT key is up" );
            return false;
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
            LOG_TRACE("XLIB", "PTT key is down (according to XQueryKeymap" );
            return true;
        } else {
            LOG_TRACE("XLIB", "PTT key is up (according to XQueryKeymap" );
            return false;
        }
    }
    /* Couldn't access the keyboard directly, and XQuery failed, this is really bad! */
    LOG_ERR("XLIB", "Unable to access keyboard, you need to read the manual on how to enable utox to\nhave access to your "
                "keyboard.\nDisable push to talk to suppress this message.\n");
    return false;
}
#else
static bool bsd_check_ptt(void) {
    return false;
}
#endif

bool check_ptt_key(void) {
    if (!settings.push_to_talk) {
        // LOG_TRACE("XLIB", "PTT is disabled" );
        return true; /* If push to talk is disabled, return true. */
    }

#ifdef __linux__
    return linux_check_ptt();
#else
    return bsd_check_ptt();
#endif
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
        default: LOG_TRACE("XLIB", "Warning: Tried to set image to unrecognized filter(%u)." , filter); return;
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
    if (try_open_tox_uri(str)) {
        redraw();
        return;
    }

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
    XSetSelectionOwner(display, XA_PRIMARY, main_window.window, CurrentTime);
}


/** Toggles the main window to/from hidden to tray/shown. */
void togglehide(void) {
    if (hidden) {
        XMoveWindow(display, main_window.window, main_window._.x, main_window._.y);
        XMapWindow(display, main_window.window);
        redraw();
        hidden = 0;
    } else {
        XWithdrawWindow(display, main_window.window, def_screen_num);
        hidden = 1;
    }
}

void pasteprimary(void) {
    Window owner = XGetSelectionOwner(display, XA_PRIMARY);
    if (owner) {
        XConvertSelection(display, XA_PRIMARY, XA_UTF8_STRING, targets, main_window.window, CurrentTime);
    }
}

void copy(int value) {
    int len;
    if (edit_active()) {
        len = edit_copy((char *)clipboard.data, sizeof(clipboard.data));
    } else if (flist_get_friend()) {
        len = messages_selection(&messages_friend, clipboard.data, sizeof(clipboard.data), value);
    } else if (flist_get_groupchat()) {
        len = messages_selection(&messages_group, clipboard.data, sizeof(clipboard.data), value);
    } else {
        LOG_ERR("XLIB", "Copy from Unsupported flist type.");
        return;
    }

    if (len) {
        clipboard.len = len;
        setclipboard();
    }
}

int hold_x11s_hand(Display *UNUSED(d), XErrorEvent *event) {
    LOG_ERR("XLIB", "X11 err:\tX11 tried to kill itself, so I hit him with a shovel.");
    LOG_ERR("XLIB", "    err:\tResource: %lu || Serial %lu", event->resourceid, event->serial);
    LOG_ERR("XLIB", "    err:\tError code: %u || Request: %u || Minor: %u",
        event->error_code, event->request_code, event->minor_code);
    LOG_ERR("uTox", "This would be a great time to submit a bug!");

    return 0;
}

void paste(void) {
    Window owner = XGetSelectionOwner(display, XA_CLIPBOARD);

    /* Ask owner for supported types */
    if (owner) {
        XEvent event = {
            .xselectionrequest = {
                .type       = SelectionRequest,
                .send_event = True,
                .display    = display,
                .owner      = owner,
                .requestor  = main_window.window,
                .target     = targets,
                .selection  = XA_CLIPBOARD,
                .property   = XA_ATOM,
                .time       = CurrentTime
            }
        };

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
            LOG_TRACE("XLIB", "Supported type: %s" , name);
        } else {
            LOG_TRACE("XLIB", "Unsupported type!!: Likely a bug, please report!" );
        }
    }

    for (i = 0; i < len; i++) {
        for (j = 0; j < COUNTOF(supported); j++) {
            if (atoms[i] == supported[j]) {
                XConvertSelection(display, selection, supported[j], targets, main_window.window, CurrentTime);
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

    size_t size = len;
    if (type == XA_PNG_IMG) {
        FRIEND *f = flist_get_friend();
        if (!f) {
            LOG_ERR("XLIB", "Can't paste data to missing friend.");
            return;
        }
        uint16_t width, height;

        NATIVE_IMAGE *native_image = utox_image_to_native(data, size, &width, &height, 0);
        if (NATIVE_IMAGE_IS_VALID(native_image)) {
            LOG_INFO("XLIB MAIN", "Pasted image: %dx%d", width, height);

            UTOX_IMAGE png_image = malloc(size);
            if (!png_image){
                LOG_ERR("XLIB", "Could not allocate memory for an image");
                free(native_image);
                return;
            }

            memcpy(png_image, data, size);
            friend_sendimage(f, native_image, width, height, png_image, size);
        }
    } else if (type == XA_URI_LIST) {
        FRIEND *f = flist_get_friend();
        if (!f) {
            LOG_ERR("XLIB", "Can't paste data to missing friend.");
            return;
        }
        char *path = malloc(len + 1);
        if (!path) {
            LOG_ERR("XLIB", "Could not allocate memory for path.");
            return;
        }
        formaturilist(path, (char *)data, len);
        postmessage_toxcore(TOX_FILE_SEND_NEW, f->number, 0xFFFF, path);
    } else if (type == XA_UTF8_STRING && edit_active()) {
        edit_paste(data, len, select);
    }
}

// converts an XImage to a Picture usable by XRender, uses XRenderPictFormat given by
// 'format', uses the default format if it is NULL
Picture ximage_to_picture(XImage *img, const XRenderPictFormat *format) {
    Pixmap pixmap = XCreatePixmap(display, main_window.window, img->width, img->height, img->depth);
    GC     legc   = XCreateGC(display, pixmap, 0, NULL);
    XPutImage(display, pixmap, legc, img, 0, 0, 0, 0, img->width, img->height);

    if (format == NULL) {
        format = XRenderFindVisualFormat(display, default_visual);
    }
    Picture picture = XRenderCreatePicture(display, pixmap, format, 0, NULL);

    XFreeGC(display, legc);
    XFreePixmap(display, pixmap);

    return picture;
}

void loadalpha(int bm, void *data, int width, int height) {
    if (bm < 0){
        LOG_ERR("XLIB", "Can not get object from array. Index %d", bm);
        return;
    }

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

/* Swaps out the PNG color order for the native color order */
static void native_color_mask(uint8_t *data, uint32_t size, uint32_t mask_red, uint32_t mask_blue, uint32_t mask_green) {
    uint8_t   red, blue, green;
    uint32_t *dest;
    for (uint32_t i = 0; i < size; i += 4) {
        red   = (data + i)[0] & 0xFF;
        green = (data + i)[1] & 0xFF;
        blue  = (data + i)[2] & 0xFF;
        dest = (uint32_t*)(data + i);
        *dest  = (red | (red << 8) | (red << 16) | (red << 24)) & mask_red;
        *dest |= (blue | (blue << 8) | (blue << 16) | (blue << 24)) & mask_blue;
        *dest |= (green | (green << 8) | (green << 16) | (green << 24)) & mask_green;
    }
}

NATIVE_IMAGE *utox_image_to_native(const UTOX_IMAGE data, size_t size, uint16_t *w, uint16_t *h, bool keep_alpha) {
    int      width, height, bpp;
    uint8_t *rgba_data = stbi_load_from_memory(data, size, &width, &height, &bpp, 4);
    // we don't need to free this, that's done by XDestroyImage()

    if (rgba_data == NULL || width == 0 || height == 0) {
        return None; // invalid png data
    }

    uint32_t rgba_size = width * height * 4;
    Picture alpha = (bpp == 4 && keep_alpha) ? generate_alpha_bitmask(rgba_data, width, height, rgba_size) : None;
    native_color_mask(rgba_data, rgba_size, default_visual->red_mask, default_visual->blue_mask, default_visual->green_mask);

    XImage *img = XCreateImage(display, default_visual, default_depth, ZPixmap, 0, (char *)rgba_data, width, height, 32, width * 4);
    Picture rgb = ximage_to_picture(img, NULL);
    XDestroyImage(img);

    *w = width;
    *h = height;

    NATIVE_IMAGE *image = malloc(sizeof(NATIVE_IMAGE));
    if (image == NULL) {
        LOG_ERR("utox_image_to_native", "Could mot allocate memory for image." );
        return NULL;
    }
    image->rgb   = rgb;
    image->alpha = alpha;

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
    for (i = 0; i != COUNTOF(bitmap); i++) {
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

    XSetWMNormalHints(display, main_window.window, xsh);

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
    if (have_focus) {
        return;
    }

    uint8_t *f_cid = NULL;
    if (is_group) {
        // GROUPCHAT *obj = object;
    } else {
        FRIEND *obj = object;
        if (friend_has_avatar(obj)) {
            f_cid = obj->id_bin;
        }
    }

    XWMHints hints = {.flags = 256 };
    XSetWMHints(display, main_window.window, &hints);

#ifdef HAVE_DBUS
    char *str = tohtml(msg, msg_length);
    dbus_notify(title, str, f_cid);
    free(str);
#else
    (void)title; // I don't like this either, but this is all going away soon!
    (void)msg;
    (void)msg_length;
#endif

#ifdef UNITY
    if (unity_running) {
        mm_notify(obj->name, f_cid);
    }
#else
    (void)f_cid;
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

static void atom_init(void) {
    wm_protocols     = XInternAtom(display, "WM_PROTOCOLS", 0);
    wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", 0);
    XA_CLIPBOARD     = XInternAtom(display, "CLIPBOARD", 0);
    XA_NET_NAME      = XInternAtom(display, "_NET_WM_NAME", 0);
    XA_UTF8_STRING   = XInternAtom(display, "UTF8_STRING", 1);

    if (XA_UTF8_STRING == None) {
        XA_UTF8_STRING = XA_STRING;
    }
    targets = XInternAtom(display, "TARGETS", 0);

    XA_INCR = XInternAtom(display, "INCR", false);

    XdndAware      = XInternAtom(display, "XdndAware", false);
    XdndEnter      = XInternAtom(display, "XdndEnter", false);
    XdndLeave      = XInternAtom(display, "XdndLeave", false);
    XdndPosition   = XInternAtom(display, "XdndPosition", false);
    XdndStatus     = XInternAtom(display, "XdndStatus", false);
    XdndDrop       = XInternAtom(display, "XdndDrop", false);
    XdndSelection  = XInternAtom(display, "XdndSelection", false);
    XdndDATA       = XInternAtom(display, "XdndDATA", false);
    XdndActionCopy = XInternAtom(display, "XdndActionCopy", false);

    XA_URI_LIST = XInternAtom(display, "text/uri-list", false);
    XA_PNG_IMG  = XInternAtom(display, "image/png", false);

    XRedraw = XInternAtom(display, "XRedraw", false);
}

static void cursors_init(void) {
    cursors[CURSOR_NONE]     = XCreateFontCursor(display, XC_left_ptr);
    cursors[CURSOR_HAND]     = XCreateFontCursor(display, XC_hand2);
    cursors[CURSOR_TEXT]     = XCreateFontCursor(display, XC_xterm);
    cursors[CURSOR_SELECT]   = XCreateFontCursor(display, XC_crosshair);
    cursors[CURSOR_ZOOM_IN]  = XCreateFontCursor(display, XC_target);
    cursors[CURSOR_ZOOM_OUT] = XCreateFontCursor(display, XC_target);
}

static void signal_handler(int signal)
{
    LOG_INFO("XLIB MAIN", "Got signal: %s (%i)", strsignal(signal), signal);
    shutdown = true;
}

#include "../ui/dropdown.h" // this is for dropdown.language TODO provide API
int main(int argc, char *argv[]) {
    if (!XInitThreads()) {
        LOG_FATAL_ERR(EXIT_FAILURE, "XLIB MAIN", "XInitThreads failed.");
    }
    if (!native_window_init()) {
        return 2;
    }
    initfonts();

    #ifdef HAVE_DBUS
    LOG_INFO("XLIB MAIN", "Compiled with dbus support!");
    #endif

    int8_t should_launch_at_startup;
    int8_t set_show_window;
    bool   skip_updater;
    parse_args(argc, argv,
               &skip_updater,
               &should_launch_at_startup,
               &set_show_window);

    // We need to parse_args before calling utox_init()
    utox_init();


    if (should_launch_at_startup == 1 || should_launch_at_startup == -1) {
        LOG_NOTE("XLIB", "Start on boot not supported on this OS, please use your distro suggested method!\n");
    }

    if (skip_updater == true) {
        LOG_ERR("XLIB", "Disabling the updater is not supported on this OS. "
                        "Updates are managed by your distro's package manager.\n");
    }

    LOG_INFO("XLIB MAIN", "Setting theme to:\t%d", settings.theme);
    theme_load(settings.theme);

    XSetErrorHandler(hold_x11s_hand);

    XIM xim;
    setlocale(LC_ALL, "");
    XSetLocaleModifiers("");
    if ((xim = XOpenIM(display, 0, 0, 0)) == NULL) {
        LOG_ERR("XLIB", "Cannot open input method");
    }

    atom_init();

    native_window_create_main(settings.window_x, settings.window_y, settings.window_width, settings.window_height, argv, argc);
    main_window.gc = DefaultGC(display, def_screen_num);
    main_window.drawbuf = XCreatePixmap(display, main_window.window, settings.window_width, settings.window_height, default_depth);

    /* choose available libraries for optional UI stuff */
    if (!(libgtk = ugtk_load())) {
        // try Qt
    }

    /* catch WM_DELETE_WINDOW */
    XSetWMProtocols(display, main_window.window, &wm_delete_window, 1);

    struct sigaction action;
    action.sa_handler = &signal_handler;

    /* catch terminating signals */
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGHUP, &action, NULL);
    sigaction(SIGTERM, &action, NULL);

    /* set drag and drog version */
    Atom dndversion = 3;
    XChangeProperty(display, main_window.window, XdndAware, XA_ATOM, 32, PropModeReplace, (uint8_t *)&dndversion, 1);

    /* initialize fontconfig */
    loadfonts();
    setfont(FONT_TEXT);

    cursors_init();

    ui_rescale(0);

    /* */
    XGCValues gcval;
    gcval.foreground     = XWhitePixel(display, 0);
    gcval.function       = GXxor;
    gcval.background     = XBlackPixel(display, 0);
    gcval.plane_mask     = gcval.background ^ gcval.foreground;
    gcval.subwindow_mode = IncludeInferiors;

    /* GC for the */
    scr_grab_window.gc = XCreateGC(display, RootWindow(display, def_screen_num), GCFunction | GCForeground | GCBackground | GCSubwindowMode,
                       &gcval);

    XWindowAttributes attr;
    XGetWindowAttributes(display, root_window, &attr);

    main_window.pictformat = XRenderFindVisualFormat(display, attr.visual);
    // XRenderPictFormat *pictformat = XRenderFindStandardFormat(display, PictStandardA8);

    /* Xft draw context/color */
    main_window.renderpic = XRenderCreatePicture(display, main_window.drawbuf, main_window.pictformat, 0, NULL);


    XRenderColor xrcolor = { 0,0,0,0 };
    main_window.colorpic = XRenderCreateSolidFill(display, &xrcolor);

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
        XMapWindow(display, main_window.window);
    }

    if (xim) {
        if ((xic = XCreateIC(xim, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, XNClientWindow, main_window.window,
                             XNFocusWindow, main_window.window, NULL))) {
            XSetICFocus(xic);
        } else {
            LOG_ERR("XLIB", "Cannot open input method");
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
    native_window_set_target(&main_window);
    panel_draw(&panel_root, 0, 0, settings.window_width, settings.window_height);

    // start toxcore thread
    thread(toxcore_thread, NULL);

    /* event loop */
    while (!shutdown) {
        XEvent event;
        XNextEvent(display, &event);
        if (!doevent(event)) {
            break;
        }

        if (XPending(display)) {
            continue;
        }

        if (_redraw) {
            native_window_set_target(&main_window);
            panel_draw(&panel_root, 0, 0, settings.window_width, settings.window_height);
            panel_draw(&panel_side_bar, 0, 0, settings.window_width, settings.window_height);
            _redraw = 0;
        }
    }

    postmessage_utoxav(UTOXAV_KILL, 0, 0, NULL);
    postmessage_toxcore(TOX_KILL, 0, 0, NULL);

    /* free client thread stuff */
    if (libgtk) {
    }

    destroy_tray_icon();

    Window       root_return, child_return;
    int          x_return, y_return;
    unsigned int width_return, height_return, i;
    XGetGeometry(display, main_window.window, &root_return, &x_return, &y_return, &width_return, &height_return, &i, &i);

    XTranslateCoordinates(display, main_window.window, root_return, 0, 0, &x_return, &y_return, &child_return);

    UTOX_SAVE d = {
        .window_x      = x_return < 0 ? 0 : x_return,
        .window_y      = y_return < 0 ? 0 : y_return,
        .window_width  = width_return,
        .window_height = height_return,
    };

    config_save(&d);

    FcFontSetSortDestroy(fs);
    freefonts();

    XFreePixmap(display, main_window.drawbuf);

    XFreeGC(display, scr_grab_window.gc);

    XRenderFreePicture(display, main_window.renderpic);
    XRenderFreePicture(display, main_window.colorpic);

    if (xic) {
        XDestroyIC(xic);
    }

    if (xim) {
        XCloseIM(xim);
    }

    XDestroyWindow(display, main_window.window);
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

    return 0;
}

/* Dummy functions used in other systems... */
void launch_at_startup(bool UNUSED(is_launch_at_startup)) {}
