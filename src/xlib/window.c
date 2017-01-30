#include "window.h"

#include "main.h"

#include "../branding.h"
#include "../logging_native.h"
#include "../main_native.h"

#include "../ui/draw.h"

#include "../layout/tree.h" // TODO remove

#include "../main.h" // MAIN_WIDTH, MAIN_HEIGHT


bool native_window_init(void) {
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

static UTOX_WINDOW *native_window_create(UTOX_WINDOW *window, char *title, unsigned int class,
                                  int x, int y, int w, int h, int min_width, int min_height,
                                  void *gui_panel, bool override)
{
    if (!window) {
        window = calloc(1, sizeof(UTOX_WINDOW));
        if (!window) {
            return NULL;
        }
    }

    XSetWindowAttributes attrib = {
        .background_pixel   = WhitePixel(display, def_screen_num),
        .border_pixel       = BlackPixel(display, def_screen_num),
        .override_redirect  = override,
        .event_mask         = ExposureMask    | ButtonPressMask   | ButtonReleaseMask   | EnterWindowMask |
                              LeaveWindowMask | PointerMotionMask | StructureNotifyMask | KeyPressMask    |
                              KeyReleaseMask  | FocusChangeMask   | PropertyChangeMask,
    };

    window->window = XCreateWindow(display, root_window, x, y, w, h, 0,
                                   default_depth, InputOutput, default_visual, class, &attrib);

    /* Generate the title XLib needs */
    char *title_name = strdup(title);
    XTextProperty native_window_name;
    // Why?
    if (XStringListToTextProperty(&title_name, 1, &native_window_name) == 0 ) {
        debug_error("FATAL ERROR: Unable to alloc for a sting during window creation\n");
        return NULL;
    }
    // "Because FUCK your use of sane coding strategies" -Xlib... probably...


    /* I was getting some errors before, and made this change, but I'm not convinced
     * these can't be moved to the stack. I'd rather not XAlloc but it works now so
     * in true Linux fashion DONT TOUCH ANYTING THAT WORKS! */
    /*  Allocate memory for xlib... */
    XSizeHints *size_hints  = XAllocSizeHints();
    XWMHints   *wm_hints    = XAllocWMHints();
    XClassHint *class_hints = XAllocClassHint();
    if (!size_hints || !wm_hints || !class_hints) {
        debug_error("XLIB_Windows: couldn't allocate memory.\n");
        return NULL;
    }

    /* Set the Size information used by sane WMs */
    size_hints->flags       = PPosition | PBaseSize | PMinSize | PMaxSize | PWinGravity;
    size_hints->x           = x;
    size_hints->y           = y;
    size_hints->base_width  = w;
    size_hints->base_height = h;
    size_hints->min_width   = min_width  ? min_width  : w;
    size_hints->min_height  = min_height ? min_height : h;
    size_hints->max_width   = w * 100;
    size_hints->max_height  = h * 100;
    size_hints->win_gravity = NorthEastGravity;

    /* We default to main, this could be wrong */
    wm_hints->flags         = StateHint | InputHint | WindowGroupHint;
    wm_hints->initial_state = NormalState;
    wm_hints->input         = true;
    wm_hints->window_group  = main_window.window;

    /* Allows WMs to find shared resources */
    class_hints->res_name   = "utox";
    class_hints->res_class  = "uTox";

    XSetWMProperties(display, window->window, &native_window_name, NULL, NULL, 0, size_hints, wm_hints, class_hints);


    window->_.x = x;
    window->_.y = y;
    window->_.w = w;
    window->_.h = h;
    window->_.panel = gui_panel;

    return window;
}

void native_window_raze(UTOX_WINDOW *window) {
    if (window) {
        // do stuff
    } else {
        // don't do stuff
    }
}

UTOX_WINDOW *native_window_create_main(int x, int y, int w, int h, char **argv, int argc) {
    char *title = calloc(256, 1);
    snprintf(title, 256, "%s %s (version: %s)", TITLE, SUB_TITLE, VERSION);

    if (native_window_create(&main_window, title, CWBackPixmap | CWBorderPixel | CWEventMask,
                      x, y, w, h, MAIN_WIDTH, MAIN_HEIGHT, &panel_root, false)) {
        return NULL;
    }

    Atom a_pid  = XInternAtom(display, "_NET_WM_PID", 0);
    uint pid = getpid();
    XChangeProperty(display, main_window.window, a_pid, XA_CARDINAL, 32, PropModeReplace, (uint8_t *)&pid, 1);

    native_window_set_target(&main_window);

    return &main_window;
}

void native_window_create_video() {
    return;
}

UTOX_WINDOW *native_window_find_notify(Window window) {
    UTOX_WINDOW *win = &popup_window;
    while (win) {
        if (win->window == window) {
            return win;
        }
        win = win->_.next;
    }

    return NULL;
}

UTOX_WINDOW *native_window_create_notify(int x, int y, int w, int h, void* panel) {
    UTOX_WINDOW *next = NULL;

    if (!popup_window.window) {
        next = &popup_window;
    }

    UTOX_WINDOW *win;
    win = native_window_create(next, "uTox Alert",
                        CWBackPixmap | CWBorderPixel | CWEventMask | CWColormap | CWOverrideRedirect,
                        x, y, w, h, w, h, &panel_notify_generic, true);

    if (!win) {
        debug_error("XLIB_WIN:\tUnable to Alloc for a notification window\n");
        return NULL;
    }

    Atom a_pid  = XInternAtom(display, "_NET_WM_PID", 0);
    uint pid = getpid();
    XChangeProperty(display, win->window, a_pid, XA_CARDINAL, 32, PropModeReplace, (uint8_t *)&pid, 1);


    Atom a_type = XInternAtom(display, "_NET_WM_WINDOW_TYPE", 0);
    Atom a_util = XInternAtom(display, "_NET_WM_WINDOW_TYPE_UTILITY", 0);
    XChangeProperty(display, win->window, a_type, XA_ATOM, 32, PropModeReplace, (uint8_t *)&a_util, 1);

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
    XSetWMProtocols(display, win->window, list, 1);

    /* create the draw buffer */
    win->drawbuf = XCreatePixmap(display, win->window, w, h, default_depth);
    /* catch WM_DELETE_WINDOW */
    XSetWMProtocols(display, win->window, &wm_delete_window, 1);
    win->gc        = XCreateGC(display, root_window, 0, 0);

    XWindowAttributes attr;
    XGetWindowAttributes(display, root_window, &attr);
    win->pictformat = XRenderFindVisualFormat(display, attr.visual);

    /* Xft draw context/color */
    win->renderpic = XRenderCreatePicture(display, win->drawbuf, win->pictformat, 0, NULL);

    XRenderColor xrcolor = { 0,0,0,0 };
    win->colorpic = XRenderCreateSolidFill(display, &xrcolor);

    XMapWindow(display, win->window);

    UTOX_WINDOW *head = &popup_window;
    while (head->_.next) {
        head = head->_.next;
    }

    if (win != &popup_window){
        head->_.next = win;
    }

    win->_.panel = panel;

    return win;
}

static void notify_tween_thread(void *obj) {
    UTOX_WINDOW *target = obj;

    if (!target) {
        return;
    }

    XEvent ev = {
        .xclient = {
            .type         = ClientMessage,
            .display      = display,
            .window       = target->window,
            .message_type = XRedraw,
            .format       = 8,
            .data = {
                .s = { 0, 0 }
            }
        }
    };

    while (target->_.y > 2) {
        target->_.y -= 2;
        XMoveWindow(display, target->window, target->_.x, target->_.y);
        enddraw(0, 0, 400, 150);
        XSendEvent(display, target->window, 0, 0, &ev);
        XFlush(display);
        yieldcpu(1);
    }
}

static UTOX_WINDOW *focus;
void window_set_focus(UTOX_WINDOW *win) {
    focus = win;
}

void native_window_tween(UTOX_WINDOW *win) {
    thread(notify_tween_thread, win);
}


void native_window_create_screen_select() {
    return;
}

bool native_window_set_target(UTOX_WINDOW *new_win) {
    if (new_win == curr) {
        return false;
    }

    curr = new_win;
    return true;
}
