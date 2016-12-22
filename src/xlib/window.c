#include "window.h"

#include "drawing.h"

#include "main.h"

#include "../ui/layout_notify.h"

bool window_init(void) {
    if ((display = XOpenDisplay(NULL)) == NULL) {
        debug_error("Cannot open display, must exit\n");
        return false;
    }

    default_screen = DefaultScreenOfDisplay(display);
    def_screen_num = DefaultScreen(display);
    default_visual = DefaultVisual(display, def_screen_num);
    default_depth  = DefaultDepth(display, def_screen_num);

    root_window    = RootWindow(display, def_screen_num);


    return true;
}

static Window window_create() {

    return 0;
}

Window window_create_main(int x, int y, int w, int h, char **argv, int argc) {

    XSetWindowAttributes attrib = {
        .background_pixel   = WhitePixel(display, def_screen_num),
        .border_pixel       = BlackPixel(display, def_screen_num),
        .event_mask         = ExposureMask    | ButtonPressMask   | ButtonReleaseMask   | EnterWindowMask |
                              LeaveWindowMask | PointerMotionMask | StructureNotifyMask | KeyPressMask    |
                              KeyReleaseMask  | FocusChangeMask   | PropertyChangeMask,
    };

    Window win = XCreateWindow(display, root_window, x, y, w, h, 0,
                               default_depth, InputOutput, default_visual,
                               CWBackPixmap | CWBorderPixel | CWEventMask,
                               &attrib);


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
    wm_hints->window_group  = win;
    // With WindowGroupHint wm_hints never get set properly TODO find out why

    class_hints->res_name   = *argv;
    class_hints->res_class  = "uTox";

    XSetWMProperties(display, win, &window_name, NULL, argv, argc, size_hints, wm_hints, class_hints);

    Atom a_pid  = XInternAtom(display, "_NET_WM_PID", 0);
    uint pid = getpid();
    XChangeProperty(display, win, a_pid, XA_CARDINAL, 32, PropModeReplace, (uint8_t *)&pid, 1);

    draw_window_set(&main_window);

    return win;
}

void window_create_video() {
    return;
}

Window window_create_notify(int x, int y, int w, int h) {


    XSetWindowAttributes attrib = {
        .background_pixel = WhitePixel(display, def_screen_num),
        .border_pixel     = BlackPixel(display, def_screen_num),
        .override_redirect  = True,
        .event_mask       = ExposureMask    | ButtonPressMask   | ButtonReleaseMask   | EnterWindowMask |
                            LeaveWindowMask | PointerMotionMask | StructureNotifyMask | KeyPressMask    |
                            KeyReleaseMask  | FocusChangeMask   | PropertyChangeMask,
    };

    Window win = XCreateWindow(display, root_window, x, y, w, h, 1,
                               default_depth, InputOutput, default_visual,
                               CWBackPixmap | CWBorderPixel | CWEventMask | CWColormap | CWOverrideRedirect,
                               &attrib);
    popup_window.window = win;

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
    wm_hints->window_group  = main_window.window;

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
    popup_window.drawbuf = XCreatePixmap(display, win, w, h, default_depth);
    /* catch WM_DELETE_WINDOW */
    XSetWMProtocols(display, win, &wm_delete_window, 1);
    popup_window.gc        = XCreateGC(display, root_window, 0, 0);

    XWindowAttributes attr;
    XGetWindowAttributes(display, root_window, &attr);
    popup_window.pictformat = XRenderFindVisualFormat(display, attr.visual);

    /* Xft draw context/color */
    popup_window.renderpic = XRenderCreatePicture(display, popup_window.drawbuf, popup_window.pictformat, 0, NULL);

    XRenderColor xrcolor = { 0 };
    popup_window.colorpic = XRenderCreateSolidFill(display, &xrcolor);

    draw_window_set(&popup_window);

    panel_draw(&panel_notify, 0, 0, 400, 150);
    XCopyArea(display, popup_window.drawbuf, win, popup_window.gc, x, y, w, h, x, y);

    XMapWindow(display, win);

    panel_draw(&panel_notify, 0, 0, 400, 150);
    XCopyArea(display, popup_window.drawbuf, win, popup_window.gc, x, y, w, h, x, y);

    return win;
}

void winodw_create_screen_select() {
    return;
}
