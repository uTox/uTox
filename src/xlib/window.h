extern Display *display;
extern Screen  *default_screen;
extern int     def_screen_num;
extern Window  root_window;
extern Visual  *default_visual;

// TODO move
extern UTOX_WINDOW *curr;

extern int default_depth;

struct native_window {
    struct utox_window _; // Global struct shared across all platforms

    Window window;
    GC gc;

    Visual *visual;

    Pixmap drawbuf;

    Picture renderpic;
    Picture colorpic;

    XRenderPictFormat *pictformat;

};

extern struct native_window main_window;

extern struct native_window popup_window;
extern struct native_window scr_grab_window;

extern struct native_window tray_pop;

void window_set_focus(UTOX_WINDOW *win);
