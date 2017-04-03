#include "tray.h"

#include "window.h"

#include "../debug.h"
#include "../main_native.h"

#include <string.h>
#include <inttypes.h>

void send_message(Display *dpy, /* display */
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
    ._.w = 64u,
    ._.h = 64u,
    ._.next = NULL,
    ._.panel = NULL,
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
    // XMoveResizeWindow(display, tray_window.window, tray_window._.x, tray_window._.y,
    //                                                tray_window._.w, tray_window._.h);
    XGetGeometry(display, tray_window.window, &root_window, &tray_window._.x, &tray_window._.y,
                &tray_window._.w, &tray_window._.h,
                &null, &null);
    LOG_WARN("XLib Tray", "New geometry x %u y %u w %u h %u", tray_window._.x, tray_window._.y, tray_window._.w, tray_window._.h);
    /* TODO use xcb instead of xlib here!
    xcb_get_geometry_cookie_t xcb_get_geometry (xcb_connection_t *connection,
                                            xcb_drawable_t    drawable );
    xcb_get_geometry_reply_t *xcb_get_geometry_reply (xcb_connection_t          *connection,
                                                  xcb_get_geometry_cookie_t  cookie,
                                                  xcb_generic_error_t      **error);
    free (geom);*/
}

void draw_tray_icon(void) {
    LOG_NOTE("XLib Tray", "Draw Tray");

    uint16_t width, height;
    uint8_t *icon_data = (uint8_t *)&_binary_icons_utox_128x128_png_start;
    size_t   icon_size = (size_t)&_binary_icons_utox_128x128_png_size;

    NATIVE_IMAGE *icon = utox_image_to_native(icon_data, icon_size, &width, &height, 1);
    if (NATIVE_IMAGE_IS_VALID(icon)) {
        /* Get tray window size */
        /* Resize the image from what the system tray dock tells us to be */
        double scale = (tray_window._.w > tray_window._.h) ?
                        (double)tray_window._.h / width : (double)tray_window._.w / height;

        image_set_scale(icon, scale);
        image_set_filter(icon, FILTER_BILINEAR);

        /* Draw the image and copy to the window */
        XSetForeground(display, tray_window.gc, 0xFFFFFF);
        XFillRectangle(display, tray_window.drawbuf, tray_window.gc, 0, 0,
                       tray_window._.w, tray_window._.h);

        /* TODO: copy method of grabbing background for tray from tray.c:tray_update_root_bg_pmap() (stalonetray) */
        XRenderComposite(display, PictOpOver, icon->rgb, icon->alpha, tray_window.renderpic,
                         0, 0, 0, 0, 0, 0, tray_window._.w, tray_window._.h);

        XCopyArea(display, tray_window.drawbuf, tray_window.window, tray_window.gc,
                    0, 0, tray_window._.w, tray_window._.h, 0, 0);

        free(icon);
    } else {
        LOG_ERR("XLIB TRAY", "Tray no workie, that not gud!");
    }
}

void create_tray_icon(void) {
    LOG_WARN("XLib Tray", "Create Tray Icon");

    LOG_WARN("XLib Tray", "Resolution %u %u", tray_window._.w, tray_window._.h);
    tray_window.window = XCreateSimpleWindow(display, RootWindow(display, def_screen_num), 0, 0,
                                             tray_window._.w, tray_window._.h, 0,
                                             BlackPixel(display, def_screen_num),
                                             WhitePixel(display, def_screen_num));

    XSelectInput(display, tray_window.window, ButtonPress);

    /* Get ready to draw a tray icon */
    tray_window.gc        = XCreateGC(display, root_window, 0, 0);
    tray_window.drawbuf   = XCreatePixmap(display, tray_window.window,
                                          tray_window._.w, tray_window._.h,
                                          default_depth);
    XWindowAttributes attr;
    XGetWindowAttributes(display, root_window, &attr);
    tray_window.pictformat = XRenderFindVisualFormat(display, attr.visual);
    tray_window.renderpic = XRenderCreatePicture(display, tray_window.drawbuf, tray_window.pictformat, 0, NULL);

    /* Send icon to the tray */
    send_message(display, XGetSelectionOwner(display, XInternAtom(display, "_NET_SYSTEM_TRAY_S0", false)),
                 SYSTEM_TRAY_REQUEST_DOCK, tray_window.window, 0, 0);
    /* Draw the tray */
    draw_tray_icon();
    /* Reset the tray draw/picture buffers with the new tray size */
    XFreePixmap(display, tray_window.drawbuf);
    tray_window.drawbuf = XCreatePixmap(display, tray_window.window,
                                        tray_window._.w, tray_window._.h,
                                        default_depth);

    XRenderFreePicture(display, tray_window.renderpic);
    tray_window.renderpic = XRenderCreatePicture(display, tray_window.drawbuf, tray_window.pictformat, 0, NULL);
    /* Redraw the tray one last time! */
    draw_tray_icon();
}

void destroy_tray_icon(void) {
    XDestroyWindow(display, tray_window.window);
}

void tray_xembed(XClientMessageEvent *ev) {
    LOG_WARN("XEMBED Tray", "ClientMessage on display %u ", ev->send_event, ev->display);
    LOG_WARN("XEMBED Tray", "Format (%i) as long %lu %lu parent window %lu proto version %lu %lu",
        ev->format, ev->data.l[0], ev->data.l[1], ev->data.l[2], ev->data.l[3], ev->data.l[4]);
    tray_reposition();
    draw_tray_icon();
}

bool tray_window_event(XEvent event) {
    if (event.xany.window != tray_window.window) {
        LOG_ERR("TRAY", "in %u oun %u", event.xany.window, tray_window.window);
        return false;
    }

    switch (event.type) {
        case Expose: {
            LOG_WARN("XLib Tray", "Expose");
            draw_tray_icon();
            return true;
        }
        case NoExpose: {
            LOG_WARN("XLib Tray", "NoExpose");
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
            LOG_TRACE("XLib Tray", "Format (%i) as long %lu %lu %lu %lu %lu",
                msg.format, msg.data.l[0], msg.data.l[1], msg.data.l[2], msg.data.l[3], msg.data.l[4]);
            return true;
        }

        case ConfigureNotify: {
            LOG_WARN("XLib Tray", "Tray configure event");
            XConfigureEvent *ev = &event.xconfigure;
            tray_window._.x = ev->x;
            tray_window._.y = ev->y;
            tray_window._.w = ev->width;
            tray_window._.h = ev->height;
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

                draw_tray_icon();
            }

            return true;
        }
        case ButtonPress: {
            LOG_NOTE("XLib Tray", "Tray button down event");
            XButtonEvent *ev = &event.xbutton;

            if (ev->button == Button1) {
                togglehide();
            }
            return true;
            tray_reposition();
        }

        default: {
            LOG_WARN("XLib Tray", "Incoming tray window event (%u)", event.type);
            break;
        }
    }
    LOG_ERR("XLib tray", "Reached end of function, this is bad juju!");
    return false;
}
