#include "main.h"

#include "drawing.h"
#include "window.h"

#include "../flist.h"
#include "../friend.h"
#include "../utox.h"

#include "keysym2ucs.h"

#include <stddef.h>

extern XIC xic;

static void expose(void) {

}

static void mouse_move(XMotionEvent *event, UTOX_WINDOW *window) {
    if (pointergrab) { // TODO super globals are bad mm'kay?
        XDrawRectangle(display, RootWindow(display, def_screen_num), scr_grab_window.gc,
                       grabx < grabpx ? grabx : grabpx,
                       graby < grabpy ? graby : grabpy,
                       grabx < grabpx ? grabpx - grabx : grabx - grabpx,
                       graby < grabpy ? grabpy - graby : graby - grabpy);

        grabpx = event->x_root;
        grabpy = event->y_root;

        XDrawRectangle(display, RootWindow(display, def_screen_num), scr_grab_window.gc,
                       grabx < grabpx ? grabx : grabpx,
                       graby < grabpy ? graby : grabpy,
                       grabx < grabpx ? grabpx - grabx : grabx - grabpx,
                       graby < grabpy ? grabpy - graby : graby - grabpy);

        return;
    }

    static int mx, my;
    int        dx, dy;

    dx = event->x - mx;
    dy = event->y - my;
    mx = event->x;
    my = event->y;

    cursor = CURSOR_NONE;
    panel_mmove(window->panel, 0, 0, window->w, window->h, event->x, event->y, dx, dy);

    XDefineCursor(display, window->window, cursors[cursor]);

    debug("MotionEvent: (%u %u) %u\n", event->x, event->y, event->state);
}

static void mouse_down(XButtonEvent *event, UTOX_WINDOW *window) {
    switch (event->button) {
        case Button1: {
            // todo: better double/triple click detect
            static Time lastclick, lastclick2;
            panel_mmove(window->panel, 0, 0, window->w, window->h, event->x, event->y, 0, 0);
            panel_mdown(window->panel);
            if (event->time - lastclick < 300) {
                bool triclick = (event->time - lastclick2 < 600);
                panel_dclick(window->panel, triclick);
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

            panel_mright(window->panel);
            break;
        }

        case Button4: {
            // FIXME: determine precise deltas if possible
            panel_mwheel(window->panel, 0, 0, window->w, window->h, 1.0, 0);
            break;
        }

        case Button5: {
            // FIXME: determine precise deltas if possible
            panel_mwheel(window->panel, 0, 0, window->w, window->h, -1.0, 0);
            break;
        }
    }

    debug("ButtonEvent: %u %u\n", event->state, event->button);
}

static void mouse_up(XButtonEvent *event, UTOX_WINDOW *window) {
    switch (event->button) {
        case Button1: {
            if (pointergrab) {
                if (grabx < grabpx) {
                    grabpx -= grabx;
                } else {
                    int w  = grabx - grabpx;
                    grabx  = grabpx;
                    grabpx = w;
                }

                if (graby < grabpy) {
                    grabpy -= graby;
                } else {
                    int w  = graby - grabpy;
                    graby  = grabpy;
                    grabpy = w;
                }

                /* enforce min size */

                if (grabpx * grabpy < 100) {
                    pointergrab = 0;
                    XUngrabPointer(display, CurrentTime);
                    break;
                }

                XDrawRectangle(display, RootWindow(display, def_screen_num), scr_grab_window.gc, grabx, graby, grabpx, grabpy);
                XUngrabPointer(display, CurrentTime);
                if (pointergrab == 1) {
                    FRIEND *f = flist_get_selected()->data;
                    if (flist_get_selected()->item == ITEM_FRIEND && f->online) {
                        XImage *img = XGetImage(display, RootWindow(display, def_screen_num), grabx, graby, grabpx,
                                                grabpy, XAllPlanes(), ZPixmap);
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
                panel_mup(window->panel);
            }
            break;
        }
    }
    debug("ButtonEvent: %u %u\n", event->state, event->button);
}

static bool popup_event(XEvent event) {

    switch (event.type) {
        case Expose: {
            debug_error("expose\n");
            draw_window_set(&popup_window);
            panel_draw(&panel_notify, 0, 0, 400, 150);
            XCopyArea(display, popup_window.drawbuf, popup_window.window, popup_window.gc, 0, 0, 400, 150, 0, 0);
            break;
        }
        case ClientMessage: {
            Atom ping = XInternAtom(display, "_NET_WM_PING", 0);
            if ((Atom)event.xclient.data.l[0] == ping) {
                debug_error("ping\n");
                event.xany.window = root_window;
                XSendEvent(display, root_window, False, NoEventMask, &event);
            } else {
                debug_error("not ping\n");
            }
        }
        case MotionNotify: {
            mouse_move(&event.xmotion, &popup_window);
            break;
        }
        case ButtonPress: {
            mouse_down(&event.xbutton, &popup_window);
            break;
        }
        case ButtonRelease: {
            mouse_up(&event.xbutton, &popup_window);
            break;
        }
        default :{
            debug("other event: %u\n", event.type);
        }

    }

    return true;
}

bool doevent(XEvent event) {
    if (XFilterEvent(&event, None)) {
        return true;
    }

    if (event.xany.window && event.xany.window != main_window.window) {

        if (event.xany.window == popup_window.window) {
            return popup_event(event); // TODO perhaps we should roll this into one?
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
                for (i = 0; i != countof(friend); i++) {
                    if (video_win[i + 1] == ev->window) {
                        FRIEND *f = &friend[i];
                        postmessage_utoxav(UTOXAV_STOP_VIDEO, f->number, 0, NULL);
                        break;
                    }
                }
                if (i == countof(friend)) {
                    debug("this should not happen\n");
                }
            }
        }

        return true;
    }

    switch (event.type) {
        case Expose: {
            enddraw(0, 0, settings.window_width, settings.window_height);
            draw_tray_icon();
            // debug("expose\n");
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

            havefocus      = 1;
            XWMHints hints = { 0 };
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

            havefocus = 0;
            break;
        }

        case ConfigureNotify: {
            XConfigureEvent *ev = &event.xconfigure;
            if (settings.window_width != (uint32_t)ev->width || settings.window_height != (uint32_t)ev->height) {
                // debug("resize\n");

                if (ev->width > drawwidth || ev->height > drawheight) {
                    drawwidth  = ev->width + 10;
                    drawheight = ev->height + 10;

                    XFreePixmap(display, main_window.drawbuf);
                    main_window.drawbuf = XCreatePixmap(display, main_window.window, drawwidth, drawheight, default_depth);
                    XRenderFreePicture(display, main_window.renderpic);
                    main_window.renderpic = XRenderCreatePicture(display, main_window.drawbuf, main_window.pictformat, 0, NULL);
                }

                main_window.w = settings.window_width  = ev->width;
                main_window.h = settings.window_height = ev->height;

                ui_size(settings.window_width, settings.window_height);

                redraw();
            }

            break;
        }

        case LeaveNotify: {
            ui_mouseleave();
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
                if (key != ~0) {
                    edit_char(key, (ev->state & 4) != 0, ev->state);
                } else {
                    edit_char(sym, 1, ev->state);
                }

                break;
            }

            messages_char(sym);

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

            debug("SelectionNotify\n");

            XSelectionEvent *ev = &event.xselection;

            if (ev->property == None) {
                break;
            }

            Atom              type;
            int               format;
            long unsigned int len, bytes_left;
            void *            data;

            XGetWindowProperty(display, main_window.window, ev->property, 0, ~0L, True, AnyPropertyType, &type, &format, &len,
                               &bytes_left, (unsigned char **)&data);

            if (!data) {
                break;
            }

            debug("Type: %s\n", XGetAtomName(display, type));
            debug("Property: %s\n", XGetAtomName(display, ev->property));

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

            XEvent resp = {.xselection = {.type      = SelectionNotify,
                                          .property  = ev->property,
                                          .requestor = ev->requestor,
                                          .selection = ev->selection,
                                          .target    = ev->target,
                                          .time      = ev->time } };

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
                                countof(supported));
            } else {
                debug_notice("XLIB selection request: unknown request\n");
                resp.xselection.property = None;
            }

            XSendEvent(display, ev->requestor, 0, 0, &resp);

            break;
        }

        case PropertyNotify: {
            XPropertyEvent *ev = &event.xproperty;
            if (ev->state == PropertyNewValue && ev->atom == targets && pastebuf.data) {
                debug("Property changed: %s\n", XGetAtomName(display, ev->atom));

                Atom              type;
                int               format;
                unsigned long int len, bytes_left;
                void *            data;

                XGetWindowProperty(display, main_window.window, ev->atom, 0, ~0L, True, AnyPropertyType, &type, &format, &len,
                                   &bytes_left, (unsigned char **)&data);

                if (len == 0) {
                    debug("Got 0 length data, pasting\n");
                    pastedata(pastebuf.data, type, pastebuf.len, False);
                    pastebuf.data = NULL;
                    break;
                }

                if (pastebuf.left < (int)len) {
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
                        debug("Closing to tray.\n");
                        togglehide();
                    } else {
                        return false;
                    }
                }
                break;
            }

            if (ev->message_type == XdndEnter) {
                debug("enter\n");
            } else if (ev->message_type == XdndPosition) {
                Window src         = ev->data.l[0];
                XEvent reply_event = {.xclient = {.type         = ClientMessage,
                                                  .display      = display,
                                                  .window       = src,
                                                  .message_type = XdndStatus,
                                                  .format       = 32,
                                                  .data = {.l = { main_window.window, 1, 0, 0, XdndActionCopy } } } };

                XSendEvent(display, src, 0, 0, &reply_event);
                // debug("position (version=%u)\n", ev->data.l[1] >> 24);
            } else if (ev->message_type == XdndStatus) {
                debug("status\n");
            } else if (ev->message_type == XdndDrop) {
                XConvertSelection(display, XdndSelection, XA_STRING, XdndDATA, main_window.window, CurrentTime);
                debug("drop\n");
            } else if (ev->message_type == XdndLeave) {
                debug("leave\n");
            } else {
                debug("dragshit\n");
            }
            break;
        }
    }

    return true;
}
