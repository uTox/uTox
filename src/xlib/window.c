#include "window.h"

#include "main.h"

static Window window_create() {

    return 0;
}

static Window master;

Window window_create_main(Display *display, int screen, int x, int y, int w, int h, char **argv, int argc) {

    XSetWindowAttributes attrib = {
        .background_pixel   = WhitePixel(display, screen),
        .border_pixel       = BlackPixel(display, screen),
        .event_mask         = ExposureMask    | ButtonPressMask   | ButtonReleaseMask   | EnterWindowMask |
                              LeaveWindowMask | PointerMotionMask | StructureNotifyMask | KeyPressMask    |
                              KeyReleaseMask  | FocusChangeMask   | PropertyChangeMask,
    };

    int depth    = DefaultDepth(display, screen);
    visual       = DefaultVisual(display, screen);
    root         = RootWindow(display, screen);

    Window win = XCreateWindow(display, root, x, y, w, h, 0,
                               depth, InputOutput, visual,
                               CWBackPixmap | CWBorderPixel | CWEventMask,
                               &attrib);

    // TODO don't use a global here
    master = win;

    char *title_name = calloc(256, 1);
    snprintf(title_name, 256, "%s %s (version: %s)", TITLE, SUB_TITLE, VERSION);
    XTextProperty window_name;
    if (XStringListToTextProperty(&title_name, 1, &window_name) == 0 ) {
        debug("FATAL ERROR: Unable to alloc for a sting during window creation\n");
        exit(30);
    }

    /* I was getting some errors before, and made this change, but I'm not convinced
     * these can't be moved to the stack. I'd rather not XAlloc but it works now so
     * in true Linux fashion DONT TOUCH ANYTING! */
    /*  Allocate memory for xlib... */
    XSizeHints *size_hints  = XAllocSizeHints();
    XWMHints   *wm_hints    = XAllocWMHints();
    XClassHint *class_hints = XAllocClassHint();
    if (!size_hints || !wm_hints || !class_hints) {
        fprintf(stderr, "XLIB_Windows: couldn't allocate memory.\n");
        exit(30);
    }

    size_hints->flags       = PBaseSize | PMinSize | PMaxSize;
    size_hints->base_width  = w;
    size_hints->base_height = h;
    size_hints->min_width   = 600;
    size_hints->min_height  = 300;
    size_hints->max_width   = 6000;
    size_hints->max_height  = 8000;

    wm_hints->flags         = StateHint | InputHint | WindowGroupHint;
    wm_hints->initial_state = NormalState;
    wm_hints->input         = True;
    wm_hints->window_group  = master;
    // With WindowGroupHint wm_hints never get set properly TODO find out why

    class_hints->res_name   = *argv;
    class_hints->res_class  = "uTox";

    XSetWMProperties(display, win, &window_name, NULL, argv, argc, size_hints, wm_hints, class_hints);

    Atom a_pid  = XInternAtom(display, "_NET_WM_PID", 0);
    uint pid = getpid();
    XChangeProperty(display, win, a_pid, XA_CARDINAL, 32, PropModeReplace, (uint8_t *)&pid, 1);

    return win;
}

void window_create_video() {
    return;
}

Window window_create_notify(Display *display, int screen, int x, int y, int w, int h) {
    int depth    = DefaultDepth(display, screen);
    visual       = DefaultVisual(display, screen);
    root         = RootWindow(display, screen);

    XSetWindowAttributes attrib = {
        .background_pixel = WhitePixel(display, screen),
        .border_pixel     = BlackPixel(display, screen),
        .override_redirect  = True,
        .event_mask       = ExposureMask    | ButtonPressMask   | ButtonReleaseMask   | EnterWindowMask |
                            LeaveWindowMask | PointerMotionMask | StructureNotifyMask | KeyPressMask    |
                            KeyReleaseMask  | FocusChangeMask   | PropertyChangeMask,
    };

    Window win = XCreateWindow(display, root, x, y, w, h, 1,
                               depth, InputOutput, visual,
                               CWBackPixmap | CWBorderPixel | CWEventMask | CWColormap | CWOverrideRedirect,
                               &attrib);
    popup_win = win;

    // Why?
    // "Because FUCK your use of sane coding strategies" -Xlib... probably...
    char *title_name = calloc(6, 1);
    memcpy(title_name, "blerg", 6);
    XTextProperty window_name;
    if (XStringListToTextProperty(&title_name, 1, &window_name) == 0 ) {
        debug("FATAL ERROR: Unable to alloc for a sting during window creation\n");
        exit(30);
    }

    XSizeHints *size_hints  = XAllocSizeHints();
    XWMHints   *wm_hints    = XAllocWMHints();
    XClassHint *class_hints = XAllocClassHint();
    if (!size_hints || !wm_hints || !class_hints) {
        fprintf(stderr, "XLIB_Windows: couldn't allocate memory.\n");
        exit(30);
    }

    size_hints->flags       = PPosition | PBaseSize | PMinSize | PMaxSize | PWinGravity;
    size_hints->x           = 40;
    size_hints->y           = 50;
    size_hints->base_width  = w;
    size_hints->base_height = h;
    size_hints->min_width   = 100;
    size_hints->min_height  = 100;
    size_hints->max_width   = w * 100;
    size_hints->max_height  = h * 100;
    size_hints->win_gravity = NorthEastGravity;

    wm_hints->flags         = StateHint | InputHint | WindowGroupHint;
    wm_hints->initial_state = NormalState;
    wm_hints->input         = True;
    wm_hints->window_group  = master;

    class_hints->res_name   = "Alert";
    class_hints->res_class  = "uTox";

    XSetWMProperties(display, win, &window_name, NULL, NULL, 0, size_hints, wm_hints, class_hints);

    Atom a_pid  = XInternAtom(display, "_NET_WM_PID", 0);
    uint pid = getpid();
    XChangeProperty(display, win, a_pid, XA_CARDINAL, 32, PropModeReplace, (uint8_t *)&pid, 1);


    Atom a_type = XInternAtom(display, "_NET_WM_WINDOW_TYPE", 0);
    Atom a_util = XInternAtom(display, "_NET_WM_WINDOW_TYPE_UTILITY", 0);
    XChangeProperty(display, win, a_type, XA_ATOM, 32, PropModeReplace, (uint8_t *)&a_util, 1);

    // Atom a_win  = XInternAtom(display, "WM_CLIENT_LEADER", 0);
    // XChangeProperty(display, win, a_win, XA_WINDOW, 32, PropModeReplace, (uint8_t *)&master, 1);
    // XSetTransientForHint(display, win, master);

    // Atom a_role = XInternAtom(display, "WM_WINDOW_ROLE", 0);
    // uint8_t *name = calloc(6, 1); // TODO leaks
    // memcpy(name, "alert", 6);
    // XChangeProperty(display, win, a_role, XA_STRING, 8, PropModeReplace, name, 5);


    Atom list[] = {
        wm_delete_window,
        // XInternAtom(display, "WM_TAKE_FOCUS", 0),
        // XInternAtom(display, "_NET_WM_PING", 0),
        // XInternAtom(display, "_NET_WM_SYNC_REQUEST", 0),
    };
    XSetWMProtocols(display, win, list, 1);

    /* create the draw buffer */
    popup_drawbuf = XCreatePixmap(display, win, w, h, xwin_depth);
    /* catch WM_DELETE_WINDOW */
    XSetWMProtocols(display, window, &wm_delete_window, 1);

    XWindowAttributes attr;
    XGetWindowAttributes(display, root, &attr);
    popup_pictformat = XRenderFindVisualFormat(display, attr.visual);

    /* Xft draw context/color */
    popup_renderpic = XRenderCreatePicture(display, popup_drawbuf, popup_pictformat, 0, NULL);

    XRenderColor xrcolor = { 0 };
    popup_colorpic = XRenderCreateSolidFill(display, &xrcolor);

    panel_draw(&panel_root, 0, 0, 400, 150);
    XCopyArea(display, popup_drawbuf, win, gc, x, y, w, h, x, y);

    XMapWindow(display, win);

    panel_draw(&panel_root, 0, 0, 400, 150);
    XCopyArea(display, popup_drawbuf, win, gc, x, y, w, h, x, y);

    return win;
}

void winodw_create_screen_select() {
    return;
}
