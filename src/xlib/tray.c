#include "tray.h"

#include "window.h"

#include "../debug.h"
#include "../main_native.h"

#include "../native/image.h"
#include "../native/ui.h"

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
        XMoveResizeWindow(display, tray_window.window, x_r, y_r, 32, 32);
        XGetGeometry(display, tray_window.window, &root_window, &x_r, &y_r,
                    (uint32_t*)&tray_window._.w, (uint32_t*)&tray_window._.h,
                    &border_r, &xwin_depth_r);

        /* TODO use xcb instead of xlib here!
        xcb_get_geometry_cookie_t xcb_get_geometry (xcb_connection_t *connection,
                                                xcb_drawable_t    drawable );
        xcb_get_geometry_reply_t *xcb_get_geometry_reply (xcb_connection_t          *connection,
                                                      xcb_get_geometry_cookie_t  cookie,
                                                      xcb_generic_error_t      **error);
        free (geom);*/
        // debug("Tray size == %i x %i\n", tray_window._.w, tray_window._.h);

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

        XCopyArea(display, tray_window.drawbuf, tray_window.window, tray_window.gc, 0, 0,
                  tray_window._.w, tray_window._.h, 0, 0);

        free(icon);
    } else {
        LOG_ERR("XLIB TRAY", "Tray no workie, that not gud!");
    }
}

void create_tray_icon(void) {
    tray_window.window = XCreateSimpleWindow(display, RootWindow(display, def_screen_num), 0, 0,
                                             tray_window._.w, tray_window._.h, 0,
                                             BlackPixel(display, def_screen_num),
                                             BlackPixel(display, def_screen_num));

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

void tray_window_event(XEvent event) {
    switch (event.type) {
        case ConfigureNotify: {
            XConfigureEvent *ev = &event.xconfigure;
            if (tray_window._.w != ev->width || tray_window._.h != ev->height) {
                LOG_NOTE("Tray", "Tray resized w:%i h:%i\n", ev->width, ev->height);

                if (ev->width > tray_window._.w || ev->height > tray_window._.h) {
                    tray_window._.w  = ev->width;
                    tray_window._.h = ev->height;

                    XFreePixmap(display, tray_window.drawbuf);
                    tray_window.drawbuf = XCreatePixmap(display, tray_window.window,
                                                        tray_window._.w, tray_window._.h,
                                                        24); // TODO get default_depth from X not code
                    XRenderFreePicture(display, tray_window.renderpic);
                    tray_window.renderpic = XRenderCreatePicture(
                        display, tray_window.drawbuf, XRenderFindStandardFormat(display, PictStandardRGB24), 0, NULL);
                }

                tray_window._.w  = ev->width;
                tray_window._.h = ev->height;

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
