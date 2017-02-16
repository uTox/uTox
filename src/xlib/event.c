#include "main.h"

#include "window.h"
#include "screen_grab.h"

#include "../flist.h"
#include "../friend.h"
#include "../debug.h"
#include "../macros.h"
#include "../main_native.h" // Needed for redraw(), this is probably wrong
#include "../notify.h"
#include "../settings.h"
#include "../tox.h"
#include "../ui.h"
#include "../utox.h"

#include "../av/utox_av.h"
#include "../ui/draw.h" // Needed for enddraw. This should probably be changed.
#include "../ui/edit.h"

#include "keysym2ucs.h"

#include <assert.h>
#include <stddef.h>

#include "../layout/friend.h"
#include "../layout/group.h"
#include "../layout/settings.h"


#include "../main.h" // STBI

extern XIC xic;

static void expose(void) {

}

static void mouse_move(XMotionEvent *event, UTOX_WINDOW *window) {
    if (pointergrab) { // TODO super globals are bad mm'kay?
        GRAB_POS grab = grab_pos();
        XDrawRectangle(display, RootWindow(display, def_screen_num), scr_grab_window.gc,
                       MIN(grab.dn_x, grab.up_x), MIN(grab.dn_y, grab.up_y),
                       grab.dn_x < grab.up_x ? grab.up_x - grab.dn_x : grab.dn_x - grab.up_x,
                       grab.dn_y < grab.up_y ? grab.up_y - grab.dn_y : grab.dn_y - grab.up_y);

        grab_up(event->x_root, event->y_root);
        grab = grab_pos();

        XDrawRectangle(display, RootWindow(display, def_screen_num), scr_grab_window.gc,
                       MIN(grab.dn_x, grab.up_x), MIN(grab.dn_y, grab.up_y),
                       grab.dn_x < grab.up_x ? grab.up_x - grab.dn_x : grab.dn_x - grab.up_x,
                       grab.dn_y < grab.up_y ? grab.up_y - grab.dn_y : grab.dn_y - grab.up_y);

        return;
    }

    static int mx, my;
    int        dx, dy;

    dx = event->x - mx;
    dy = event->y - my;
    mx = event->x;
    my = event->y;

    cursor = CURSOR_NONE;
    panel_mmove(window->_.panel, 0, 0, window->_.w, window->_.h, event->x, event->y, dx, dy);

    XDefineCursor(display, window->window, cursors[cursor]);

    LOG_TRACE("XLIB", "MotionEvent: (%u %u) %u\n", event->x, event->y, event->state);
}

static void mouse_down(XButtonEvent *event, UTOX_WINDOW *window) {
    switch (event->button) {
        case Button1: {
            if (pointergrab) {
                grab_up(event->x_root, event->y_root);
                grab_dn(event->x_root, event->y_root);
                return;
            }

            // todo: better double/triple click detect
            static Time lastclick, lastclick2;
            panel_mmove(window->_.panel, 0, 0, window->_.w, window->_.h, event->x, event->y, 0, 0);
            panel_mdown(window->_.panel);
            if (event->time - lastclick < 300) {
                bool triclick = (event->time - lastclick2 < 600);
                panel_dclick(window->_.panel, triclick);
                if (triclick) {
                    lastclick = 0;
                }
            }
            lastclick2 = lastclick;
            lastclick  = event->time;
            break;
        }

        case Button3: {
            if (pointergrab) {
                XUngrabPointer(display, CurrentTime);
                pointergrab = 0;
                break;
            }

            panel_mright(window->_.panel);
            break;
        }

        case Button4: {
            // TODO: determine precise deltas if possible
            panel_mwheel(window->_.panel, 0, 0, window->_.w, window->_.h, 1.0, 0);
            break;
        }

        case Button5: {
            // TODO: determine precise deltas if possible
            panel_mwheel(window->_.panel, 0, 0, window->_.w, window->_.h, -1.0, 0);
            break;
        }
    }

    LOG_TRACE("XLIB", "ButtonEvent: %u %u\n", event->state, event->button);
}

static void mouse_up(XButtonEvent *event, UTOX_WINDOW *window) {
    switch (event->button) {
        case Button1: {
            if (pointergrab) {
                XUngrabPointer(display, CurrentTime);
                GRAB_POS grab = grab_pos();
                if (grab.dn_x < grab.up_x) {
                    grab.up_x -= grab.dn_x;
                } else {
                    int w  = grab.dn_x - grab.up_x;
                    grab.dn_x  = grab.up_x;
                    grab.up_x = w;
                }

                if (grab.dn_y < grab.up_y) {
                    grab.up_y -= grab.dn_y;
                } else {
                    int w  = grab.dn_y - grab.up_y;
                    grab.dn_y  = grab.up_y;
                    grab.up_y = w;
                }

                /* enforce min size */

                if (grab.up_x * grab.up_y < 100) {
                    pointergrab = 0;
                    break;
                }

                XDrawRectangle(display, RootWindow(display, def_screen_num), scr_grab_window.gc, grab.dn_x, grab.dn_y, grab.up_x, grab.up_y);
                if (pointergrab == 1) {
                    FRIEND *f = flist_get_selected()->data;
                    if (flist_get_selected()->item == ITEM_FRIEND && f->online) {
                        XImage *img = XGetImage(display, RootWindow(display, def_screen_num), grab.dn_x, grab.dn_y, grab.up_x,
                                                grab.up_y, XAllPlanes(), ZPixmap);
                        if (img) {
                            uint8_t * temp, *p;
                            uint32_t *pp = (void *)img->data, *end = &pp[img->width * img->height];
                            p = temp = malloc(img->width * img->height * 3);
                            while (pp != end) {
                                uint32_t i = *pp++;
                                *p++       = i >> 16;
                                *p++       = i >> 8;
                                *p++       = i;
                            }
                            int      size = -1;
                            uint8_t *out  = stbi_write_png_to_mem(temp, 0, img->width, img->height, 3, &size);
                            free(temp);

                            uint16_t w = img->width;
                            uint16_t h = img->height;

                            NATIVE_IMAGE *image = malloc(sizeof(NATIVE_IMAGE));
                            image->rgb          = ximage_to_picture(img, NULL);
                            image->alpha        = None;
                            friend_sendimage(f, image, w, h, (UTOX_IMAGE)out, size);
                        }
                    }
                } else {
                    postmessage_utoxav(UTOXAV_SET_VIDEO_IN, 1, 0, NULL);
                }
                pointergrab = 0;
            } else {
                panel_mup(window->_.panel);
            }
            break;
        }
    }
    LOG_TRACE("XLIB", "ButtonEvent: %u %u\n", event->state, event->button);
}


// Should return false if the result of the action should close/exit the window.
static bool popup_event(XEvent event, UTOX_WINDOW *win) {
    switch (event.type) {
        case Expose: {
            LOG_TRACE("XLIB", "Main window expose\n");
            native_window_set_target(win);
            panel_draw(win->_.panel , 0, 0, win->_.w, win->_.h);
            XCopyArea(display, win->drawbuf, win->window, win->gc, 0, 0, win->_.w, win->_.h, 0, 0);
            break;
        }
        case ClientMessage: {
            /* This could be noop code, I'm not convinced we need to support _NET_WM_PING but
             * in case we do, we already have the response ready.  */
            Atom ping = XInternAtom(display, "_NET_WM_PING", 0);
            if ((Atom)event.xclient.data.l[0] == ping) {
                LOG_TRACE("XLIB", "ping");
                event.xany.window = root_window;
                XSendEvent(display, root_window, False, NoEventMask, &event);
            } else {
                LOG_TRACE("XLIB", "not ping");
            }
            break;
        }
        case MotionNotify: {
            mouse_move(&event.xmotion, win);
            break;
        }
        case ButtonPress: {
            mouse_down(&event.xbutton, win);
            break;
        }
        case ButtonRelease: {
            mouse_up(&event.xbutton, win);
            break;
        }

        case EnterNotify: {
            LOG_TRACE("XLIB", "set focus");
            window_set_focus(win);
            break;
        }

        case LeaveNotify: {
            break;
        }
        default: {
            LOG_WARN("XLIB", "other event: %u\n", event.type);
            break;
        }

    }

    return true;
}

bool doevent(XEvent event) {
    if (XFilterEvent(&event, None)) {
        return true;
    }

    if (event.xany.window && event.xany.window != main_window.window) {

        if (native_window_find_notify(event.xany.window)) {
            // TODO perhaps we should roll this into one?
            return popup_event(event, native_window_find_notify(event.xany.window));
            // return true;
        }

        if (event.xany.window == tray_window.window) {
            tray_window_event(event);
            return true;
        }

        if (event.type == ClientMessage) {
            XClientMessageEvent *ev = &event.xclient;
            if ((Atom)event.xclient.data.l[0] == wm_delete_window) {
                if (ev->window == video_win[0]) {
                    postmessage_utoxav(UTOXAV_STOP_VIDEO, 1, 0, NULL);
                    return true;
                }

                int i;
                for (i = 0; i != COUNTOF(friend); i++) {
                    if (video_win[i + 1] == ev->window) {
                        FRIEND *f = &friend[i];
                        postmessage_utoxav(UTOXAV_STOP_VIDEO, f->number, 0, NULL);
                        break;
                    }
                }
                assert(i != COUNTOF(friend));
            }
        }

        return true;
    }

    switch (event.type) {
        case Expose: {
            enddraw(0, 0, settings.window_width, settings.window_height);
            draw_tray_icon();
            break;
        }

        case FocusIn: {
            if (xic) {
                XSetICFocus(xic);
            }

            #ifdef UNITY
            if (unity_running) {
                mm_rm_entry(NULL);
            }
            #endif

            havefocus      = true;
            XWMHints hints = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
            XSetWMHints(display, main_window.window, &hints);
            break;
        }

        case FocusOut: {
            if (xic) {
                XUnsetICFocus(xic);
            }

            #ifdef UNITY
            if (unity_running) {
                mm_save_cid();
            }
            #endif

            havefocus = false;
            break;
        }

        case ConfigureNotify: {
            XConfigureEvent *ev = &event.xconfigure;
            if (settings.window_width != (unsigned)ev->width || settings.window_height != (unsigned)ev->height) {
                // Resize

                if (ev->width > drawwidth || ev->height > drawheight) {
                    drawwidth  = ev->width + 10;
                    drawheight = ev->height + 10;

                    XFreePixmap(display, main_window.drawbuf);
                    main_window.drawbuf = XCreatePixmap(display, main_window.window, drawwidth, drawheight, default_depth);
                    XRenderFreePicture(display, main_window.renderpic);
                    main_window.renderpic = XRenderCreatePicture(display, main_window.drawbuf, main_window.pictformat, 0, NULL);
                }

                main_window._.w = settings.window_width  = ev->width;
                main_window._.h = settings.window_height = ev->height;

                ui_size(settings.window_width, settings.window_height);

                redraw();
            }

            break;
        }

        case LeaveNotify: {
            ui_mouseleave();
            break;
        }

        case MotionNotify: {
            mouse_move(&event.xmotion, &main_window);
            break;
        }

        case ButtonPress: {
            mouse_down(&event.xbutton, &main_window);
            break;
        }

        case ButtonRelease: {
            mouse_up(&event.xbutton, &main_window);
        }

        case KeyRelease: {
            // XKeyEvent *ev = &event.xkey;
            // KeySym sym = XLookupKeysym(ev, 0);
            break;
        }

        case KeyPress: {
            XKeyEvent *ev  = &event.xkey;
            KeySym     sym = XLookupKeysym(ev, 0); // XKeycodeToKeysym(display, ev->keycode, 0)

            if (pointergrab && sym == XK_Escape) {
                XUngrabPointer(display, CurrentTime);
                pointergrab = 0;
                break;
            }

            wchar_t buffer[16];
            int     len;

            if (xic) {
                len = XwcLookupString(xic, ev, buffer, sizeof(buffer), &sym, NULL);
            } else {
                len = XLookupString(ev, (char *)buffer, sizeof(buffer), &sym, NULL);
            }

            if (sym == XK_ISO_Left_Tab) {
                // XK_ISO_Left_Tab == Shift+Tab, but we just look at whether shift is pressed
                sym = XK_Tab;
            } else if (sym >= XK_KP_0 && sym <= XK_KP_9) {
                // normalize keypad and non-keypad numbers
                sym = sym - XK_KP_0 + XK_0;
            }

            // NOTE: Don't use keys like KEY_TAB, KEY_PAGEUP, etc. from xlib/main.h here, they're
            // overwritten by linux header linux/input.h, so they'll be different

            if (ev->state & ControlMask) {
                if ((sym == XK_Tab && (ev->state & ShiftMask)) || sym == XK_Page_Up) {
                    flist_previous_tab();
                    redraw();
                    break;
                } else if (sym == XK_Tab || sym == XK_Page_Down) {
                    flist_next_tab();
                    redraw();
                    break;
                }
            }


            if (ev->state & ControlMask || ev->state & Mod1Mask) { // Mod1Mask == alt
                if (sym >= XK_1 && sym <= XK_9) {
                    flist_selectchat(sym - XK_1);
                    redraw();
                    break;
                } else if (sym == XK_0) {
                    flist_selectchat(9);
                    redraw();
                    break;
                }
            }

            if (edit_active()) {
                if (ev->state & ControlMask) {
                    switch (sym) {
                        case 'v':
                        case 'V': paste(); return true;
                        case 'c':
                        case 'C':
                        case XK_Insert: copy(0); return true;
                        case 'x':
                        case 'X':
                            copy(0);
                            edit_char(KEY_DEL, 1, 0);
                            return true;
                        case 'w':
                        case 'W':
                            /* Sent ctrl + backspace to active edit */
                            edit_char(KEY_BACK, 1, 4);
                            return true;
                    }
                }

                if (ev->state & ShiftMask) {
                    switch (sym) {
                        case XK_Insert: paste(); return true;
                        case XK_Delete:
                            copy(0);
                            edit_char(KEY_DEL, 1, 0);
                            return true;
                    }
                }

                if (sym == XK_KP_Enter) {
                    sym = XK_Return;
                }

                if (sym == XK_Return && (ev->state & 1)) {
                    edit_char('\n', 0, 0);
                    break;
                }

                if (sym == XK_KP_Space) {
                    sym = XK_space;
                }

                if (sym >= XK_KP_Home && sym <= XK_KP_Begin) {
                    sym -= 0x45;
                }

                if (sym >= XK_KP_Multiply && sym <= XK_KP_Equal) {
                    sym -= 0xFF80;
                }

                if (!sym) {
                    int i;
                    for (i = 0; i < len; i++)
                        edit_char(buffer[i], (ev->state & 4) != 0, ev->state);
                }
                uint32_t key = keysym2ucs(sym);
                if (key != ~0u) {
                    edit_char(key, (ev->state & 4) != 0, ev->state);
                } else {
                    edit_char(sym, 1, ev->state);
                }

                break;
            }

            if (messages_char(sym)) {
                redraw();
            }

            if (ev->state & 4) {
                if (sym == 'c' || sym == 'C') {
                    if (flist_get_selected()->item == ITEM_FRIEND) {
                        clipboard.len = messages_selection(&messages_friend, clipboard.data, sizeof(clipboard.data), 0);
                        setclipboard();
                    } else if (flist_get_selected()->item == ITEM_GROUP) {
                        clipboard.len = messages_selection(&messages_group, clipboard.data, sizeof(clipboard.data), 0);
                        setclipboard();
                    }
                    break;
                }
            }

            break;
        }

        case SelectionNotify: {

            LOG_TRACE(__FILE__, "SelectionNotify" );

            XSelectionEvent *ev = &event.xselection;

            if (ev->property == None) {
                break;
            }

            Atom type;
            int  format;
            void *data;
            long unsigned int len, bytes_left;

            XGetWindowProperty(display, main_window.window, ev->property, 0, ~0L, True, AnyPropertyType, &type, &format, &len,
                               &bytes_left, (unsigned char **)&data);

            if (!data) {
                break;
            }

            LOG_TRACE(__FILE__, "Type: %s" , XGetAtomName(display, type));
            LOG_TRACE(__FILE__, "Property: %s" , XGetAtomName(display, ev->property));

            if (ev->property == XA_ATOM) {
                pastebestformat((Atom *)data, len, ev->selection);
            } else if (ev->property == XdndDATA) {
                char *path = malloc(len + 1);
                formaturilist(path, (char *)data, len);
                postmessage_toxcore(TOX_FILE_SEND_NEW, (FRIEND *)(flist_get_selected()->data) - friend, 0xFFFF, path);
            } else if (type == XA_INCR) {
                if (pastebuf.data) {
                    /* already pasting something, give up on that */
                    free(pastebuf.data);
                    pastebuf.data = NULL;
                }
                pastebuf.len  = *(unsigned long *)data;
                pastebuf.left = pastebuf.len;
                pastebuf.data = malloc(pastebuf.len);
                /* Deleting the window property triggers incremental paste */
            } else {
                pastedata(data, type, len, ev->selection == XA_PRIMARY);
            }

            XFree(data);

            break;
        }

        case SelectionRequest: {
            XSelectionRequestEvent *ev = &event.xselectionrequest;

            XEvent resp = {
                .xselection = {
                    .type      = SelectionNotify,
                    .property  = ev->property,
                    .requestor = ev->requestor,
                    .selection = ev->selection,
                    .target    = ev->target,
                    .time      = ev->time
                }
            };

            if (ev->target == XA_UTF8_STRING || ev->target == XA_STRING) {
                if (ev->selection == XA_PRIMARY) {
                    XChangeProperty(display, ev->requestor, ev->property, ev->target, 8, PropModeReplace,
                                    (const unsigned char *)primary.data, primary.len);
                } else {
                    XChangeProperty(display, ev->requestor, ev->property, ev->target, 8, PropModeReplace,
                                    (const unsigned char *)clipboard.data, clipboard.len);
                }
            } else if (ev->target == targets) {
                Atom supported[] = { XA_STRING, XA_UTF8_STRING };
                XChangeProperty(display, ev->requestor, ev->property, XA_ATOM, 32, PropModeReplace, (void *)&supported,
                                COUNTOF(supported));
            } else {
                LOG_NOTE("XLIB selection request", " unknown request");
                resp.xselection.property = None;
            }

            XSendEvent(display, ev->requestor, 0, 0, &resp);

            break;
        }

        case PropertyNotify: {
            XPropertyEvent *ev = &event.xproperty;
            if (ev->state == PropertyNewValue && ev->atom == targets && pastebuf.data) {
                LOG_TRACE(__FILE__, "Property changed: %s" , XGetAtomName(display, ev->atom));

                Atom              type;
                int               format;
                unsigned long int len, bytes_left;
                void *            data;

                XGetWindowProperty(display, main_window.window, ev->atom, 0, ~0L, True, AnyPropertyType, &type, &format, &len,
                                   &bytes_left, (unsigned char **)&data);

                if (len == 0) {
                    LOG_TRACE(__FILE__, "Got 0 length data, pasting" );
                    pastedata(pastebuf.data, type, pastebuf.len, False);
                    pastebuf.data = NULL;
                    break;
                }

                if (pastebuf.left > 0 && (unsigned)pastebuf.left < len) {
                    pastebuf.len += len - pastebuf.left;
                    pastebuf.data = realloc(pastebuf.data, pastebuf.len);
                    pastebuf.left = len;
                }

                memcpy(pastebuf.data + pastebuf.len - pastebuf.left, data, len);
                pastebuf.left -= len;

                XFree(data);
            }
            break;
        }

        case ClientMessage: {
            XClientMessageEvent *ev = &event.xclient;
            if (ev->window == 0) {
                void *data;
                memcpy(&data, &ev->data.s[2], sizeof(void *));
                utox_message_dispatch(ev->message_type, ev->data.s[0], ev->data.s[1], data);
                break;
            }

            if (ev->message_type == wm_protocols) {
                if ((Atom)event.xclient.data.l[0] == wm_delete_window) {
                    if (settings.close_to_tray) {
                        LOG_TRACE(__FILE__, "Closing to tray." );
                        togglehide();
                    } else {
                        return false;
                    }
                }
                break;
            }

            if (ev->message_type == XdndEnter) {
                LOG_TRACE(__FILE__, "enter" );
            } else if (ev->message_type == XdndPosition) {
                Window src         = ev->data.l[0];
                XEvent reply_event = {.xclient = {.type         = ClientMessage,
                                                  .display      = display,
                                                  .window       = src,
                                                  .message_type = XdndStatus,
                                                  .format       = 32,
                                                  .data = {.l = { main_window.window, 1, 0, 0, XdndActionCopy } } } };

                XSendEvent(display, src, 0, 0, &reply_event);
                // LOG_TRACE(__FILE__, "position (version=%u)" , ev->data.l[1] >> 24);
            } else if (ev->message_type == XdndStatus) {
                LOG_TRACE(__FILE__, "status" );
            } else if (ev->message_type == XdndDrop) {
                XConvertSelection(display, XdndSelection, XA_STRING, XdndDATA, main_window.window, CurrentTime);
                LOG_NOTE("XLIB", "Drag was dropped\n");
            } else if (ev->message_type == XdndLeave) {
                LOG_TRACE(__FILE__, "leave" );
            } else {
                LOG_TRACE(__FILE__, "dragshit" );
            }
            break;
        }
    }

    return true;
}
