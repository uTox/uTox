#include "tray.h"

#include "window.h"

#include "../debug.h"
#include "../macros.h"

#include "../native/image.h"
#include "../native/ui.h"

#include "../ui.h"
#include "../ui/draw.h"
#include "../layout/xlib_tray.h"

#include <string.h>
#include <inttypes.h>

static void send_message(Display *dpy, /* display */
                  Window w, /* sender (tray window) */
                  long message, /* message opcode */
                  long data1, /* message data 1 */
                  long data2, /* message data 2 */
                  long data3 /* message data 3 */)
{
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

struct native_window tray_window = {
    ._.x = 0,
    ._.y = 0,
    ._.w = 128u,
    ._.h = 128u,
    ._.next = NULL,
    ._.panel = &root_xlib_tray,
    .window = 0,
    .gc     = 0,
    .visual = NULL,
    .drawbuf = 0,
    .renderpic = 0,
    .pictformat = NULL,
};

static void tray_reposition(void) {
    LOG_NOTE("XLib Tray", "Reposition Tray");

    uint32_t null;
    XGetGeometry(display, tray_window.window, &root_window, &tray_window._.x, &tray_window._.y,
                &tray_window._.w, &tray_window._.h,
                &null, &null);
    LOG_NOTE("XLib Tray", "New geometry x %u y %u w %u h %u", tray_window._.x, tray_window._.y, tray_window._.w, tray_window._.h);

    LOG_INFO("XLib Tray", "Setting to square");
    tray_window._.w = tray_window._.h = MIN(tray_window._.w, tray_window._.h);
    XResizeWindow(display, tray_window.window, tray_window._.w, tray_window._.h);

    XFreePixmap(display, tray_window.drawbuf);
    tray_window.drawbuf = XCreatePixmap(display, tray_window.window,
                                        tray_window._.w, tray_window._.h,
                                        default_depth);

    XRenderFreePicture(display, tray_window.renderpic);
    tray_window.renderpic = XRenderCreatePicture(display, tray_window.drawbuf, tray_window.pictformat, 0, NULL);

    // XMoveResizeWindow(display, tray_window.window, tray_window._.x, tray_window._.y,
    //                                                tray_window._.w, tray_window._.h);
    /* TODO use xcb instead of xlib here!
    xcb_get_geometry_cookie_t xcb_get_geometry (xcb_connection_t *connection,
                                            xcb_drawable_t    drawable );
    xcb_get_geometry_reply_t *xcb_get_geometry_reply (xcb_connection_t          *connection,
                                                  xcb_get_geometry_cookie_t  cookie,
                                                  xcb_generic_error_t      **error);
    free (geom);*/
}

static void tray_xembed(XClientMessageEvent *ev) {
    LOG_NOTE("XEMBED Tray", "ClientMessage on display %u", ev->display);
    LOG_NOTE("XEMBED Tray", "Format (%i) as long %lu %lu parent window %lu proto version %lu %lu",
        ev->format, ev->data.l[0], ev->data.l[1], ev->data.l[2], ev->data.l[3], ev->data.l[4]);
    tray_reposition();
}

void create_tray_icon(void) {
    LOG_NOTE("XLib Tray", "Create Tray Icon");

    LOG_NOTE("XLib Tray", "Resolution %u %u", tray_window._.w, tray_window._.h);
    tray_window.window = XCreateSimpleWindow(display, RootWindow(display, def_screen_num), 0, 0,
                                             tray_window._.w, tray_window._.h, 0,
                                             BlackPixel(display, def_screen_num),
                                             WhitePixel(display, def_screen_num));

    XSelectInput(display, tray_window.window,
                 ExposureMask        | ButtonPressMask | ButtonReleaseMask |
                 PointerMotionMask   | EnterWindowMask | LeaveWindowMask   |
                 StructureNotifyMask | FocusChangeMask | PropertyChangeMask);

    /* Get ready to draw a tray icon */
    tray_window.gc        = XCreateGC(display, root_window, 0, 0);
    tray_window.drawbuf   = XCreatePixmap(display, tray_window.window,
                                          tray_window._.w, tray_window._.h,
                                          default_depth);
    XWindowAttributes attr;
    XGetWindowAttributes(display, root_window, &attr);

    // Todo, try and alloc on the stack for this
    XSizeHints *size_hints  = XAllocSizeHints();
    size_hints->flags       = PSize | PBaseSize | PMinSize | PMaxSize;
    size_hints->base_width  = tray_window._.w;
    size_hints->base_height = tray_window._.h;
    size_hints->min_width   = 16;
    size_hints->min_height  = 16;
    size_hints->max_width   = tray_window._.w;
    size_hints->max_height  = tray_window._.h;
    XSetWMNormalHints(display, tray_window.window, size_hints);
    XFree(size_hints);

    tray_window.pictformat = XRenderFindVisualFormat(display, attr.visual);
    tray_window.renderpic = XRenderCreatePicture(display, tray_window.drawbuf, tray_window.pictformat, 0, NULL);

    /* Send icon to the tray */
    send_message(display, XGetSelectionOwner(display, XInternAtom(display, "_NET_SYSTEM_TRAY_S0", false)),
                 SYSTEM_TRAY_REQUEST_DOCK, tray_window.window, 0, 0);
    /* Draw the tray */
}

void destroy_tray_icon(void) {
    XDestroyWindow(display, tray_window.window);
}

bool tray_window_event(XEvent event) {
    if (event.xany.window != tray_window.window) {
        LOG_ERR("TRAY", "in %u oun %u", event.xany.window, tray_window.window);
        return false;
    }

    native_window_set_target(&tray_window);
    switch (event.type) {
        case Expose: {
            LOG_NOTE("XLib Tray", "Expose");
            panel_draw(tray_window._.panel, 0, 0, tray_window._.w, tray_window._.h);
            return true;
        }
        case NoExpose: {
            LOG_INFO("XLib Tray", "NoExpose");
            return true;
        }
        case ClientMessage: {
            XClientMessageEvent msg = event.xclient;
            if (msg.message_type == XInternAtom(msg.display, "_XEMBED", true)) {
                tray_xembed(&msg);
                return true;
            }

            char *name = XGetAtomName(msg.display, msg.message_type);
            LOG_ERR("XLib Tray", "ClientMessage send_event %u display %u atom %u -- %s",
                msg.send_event, msg.display, msg.message_type, name);
            LOG_WARN("XLib Tray", "Format (%i) as long %lu %lu %lu %lu %lu",
                msg.format, msg.data.l[0], msg.data.l[1], msg.data.l[2], msg.data.l[3], msg.data.l[4]);
            return true;
        }

        case ConfigureNotify: {
            LOG_NOTE("XLib Tray", "Tray configure event");
            XConfigureEvent *ev = &event.xconfigure;
            tray_window._.x = ev->x;
            tray_window._.y = ev->y;
            if (tray_window._.w != (unsigned)ev->width || tray_window._.h != (unsigned)ev->height) {
                LOG_NOTE("Tray", "Tray resized w:%i h:%i\n", ev->width, ev->height);

                if ((unsigned)ev->width > tray_window._.w || (unsigned)ev->height > tray_window._.h) {
                    tray_window._.w = ev->width;
                    tray_window._.h = ev->height;

                    XFreePixmap(ev->display, tray_window.drawbuf);
                    tray_window.drawbuf = XCreatePixmap(ev->display, tray_window.window,
                                                        tray_window._.w, tray_window._.h,
                                                        24); // TODO get default_depth from X not code
                    XRenderFreePicture(ev->display, tray_window.renderpic);
                    tray_window.renderpic = XRenderCreatePicture(ev->display, tray_window.drawbuf,
                                XRenderFindStandardFormat(ev->display, PictStandardRGB24), 0, NULL);
                }

                tray_window._.w = ev->width;
                tray_window._.h = ev->height;
            }

            panel_draw(tray_window._.panel, 0, 0, tray_window._.w, tray_window._.h);
            return true;
        }

        case MotionNotify: {
            XMotionEvent *ev = &event.xmotion;

            LOG_TRACE("XLib Tray", "MotionEvent: (%i %i) %u", ev->x, ev->y, ev->state);
            LOG_DEBUG("XLib Tray", "        root (%i %i)",    ev->x_root, ev->y_root);

            static int mx, my;
            int dx, dy;

            dx = ev->x - mx;
            dy = ev->y - my;
            mx = ev->x;
            my = ev->y;

            panel_mmove(tray_window._.panel, 0, 0, tray_window._.w, tray_window._.h, ev->x, ev->y, dx, dy);
            panel_draw(tray_window._.panel, 0, 0, tray_window._.w, tray_window._.h);
            return true;
        }

        case ButtonPress: {
            XButtonEvent *ev = &event.xbutton;
            LOG_INFO("XLib Tray",  "ButtonPress");
            switch (ev->button) {
                case Button1: {
                    // todo: better double/triple click detect
                    static Time lastclick, lastclick2;
                    panel_mmove(tray_window._.panel, 0, 0, tray_window._.w, tray_window._.h, ev->x, ev->y, 0, 0);
                    panel_mdown(tray_window._.panel);
                    if (ev->time - lastclick < 300) {
                        bool triclick = (ev->time - lastclick2 < 600);
                        panel_dclick(tray_window._.panel, triclick);
                        if (triclick) {
                            lastclick = 0;
                        }
                    }
                    lastclick2 = lastclick;
                    lastclick  = ev->time;
                    return true;
                }

                case Button3: {
                    panel_mright(tray_window._.panel);
                    return true;
                }
            }

            panel_draw(tray_window._.panel, 0, 0, tray_window._.w, tray_window._.h);
            return true;
        }

        case ButtonRelease: {
            LOG_INFO("XLib Tray", "ButtonRelease");
            panel_mup(tray_window._.panel);
            panel_draw(tray_window._.panel, 0, 0, tray_window._.w, tray_window._.h);
            return true;
        }

        case MapNotify: {
            LOG_INFO("XLib Tray", "MapNotify");
            panel_draw(tray_window._.panel, 0, 0, tray_window._.w, tray_window._.h);
            return true;
        }

        case FocusIn: {
            LOG_INFO("XLib Tray", "FocusIn");
            return true;
        }
        case FocusOut: {
            LOG_INFO("XLib Tray", "FocusOut");
            return true;
        }

        case EnterNotify: {
            LOG_INFO("XLib Tray", "EnterNotify");
            return true;
        }
        case LeaveNotify: {
            LOG_INFO("XLib Tray", "LeaveNotify");
            return true;
        }

        case ReparentNotify: {
            LOG_WARN("XLib Tray", "ReparentNotify");
            panel_draw(tray_window._.panel, 0, 0, tray_window._.w, tray_window._.h);
            return true;
        }

        default: {
            LOG_ERR("XLib Tray", "Incoming tray window event (%u)", event.type);
            break;
        }
    }

    LOG_ERR("XLib tray", "Reached end of function, this is bad juju!");
    return false;
}

void tray_drop_init(PANEL *p)
{
    // TODO dont' assume right side tray, calculate and push
    // int x = ScreenOfDisplay(display, 0)->width;
    Window w_; // Dummy to avoid null deref
    int i_;    // Dummy to avoid null deref
    uint u_;   // Dummy to avoid null deref

    int rx, ry;

    XQueryPointer(display, tray_window.window, &w_, &w_, &rx, &ry, &i_, &i_, &u_);

    const int w = 300, h = 200;

    native_window_create_notify(rx - w, ry, w, h, p);
}
