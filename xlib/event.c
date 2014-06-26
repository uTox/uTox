
_Bool doevent(void)
{
    XEvent event;
    XNextEvent(display, &event);
    if(event.xany.window && event.xany.window != window) {
        if(event.type == ClientMessage) {
            XClientMessageEvent *ev = &event.xclient;
            if((Atom)event.xclient.data.l[0] == wm_delete_window) {
                if(ev->window == video_win[0]) {
                    video_end(0);
                    video_preview = 0;
                    return 1;
                }

                int i;
                for(i = 0; i != countof(friend); i++) {
                    if(video_win[i + 1] == ev->window) {
                        FRIEND *f = &friend[i];
                        tox_postmessage(TOX_HANGUP, f->callid, 0, NULL);
                        break;
                    }
                }
                if(i == countof(friend)) {
                    debug("this should not happen\n");
                }
            }
        }
        return 1;
    }

    switch(event.type) {
    case Expose: {
        redraw();
        debug("expose\n");
        break;
    }

    case ConfigureNotify: {
        XConfigureEvent *ev = &event.xconfigure;
        width = ev->width;
        height = ev->height;

        panel_update(&panel_main, 0, 0, width, height);

        XFreePixmap(display, drawbuf);
        drawbuf = XCreatePixmap(display, window, width, height, 24);

        XftDrawDestroy(xftdraw);
        xftdraw = XftDrawCreate(display, drawbuf, visual, cmap);
        break;
    }

    case LeaveNotify: {
        panel_mleave(&panel_main);
    }

    case MotionNotify: {
        XMotionEvent *ev = &event.xmotion;
        static int my;
        int dy;

        dy = ev->y - my;
        my = ev->y;

        hand = 0;
        overtext = 0;

        panel_mmove(&panel_main, 0, 0, width, height, ev->x, ev->y, dy);

        XDefineCursor(display, window, hand ? cursor_hand : (overtext ? cursor_text : cursor_arrow));

        //SetCursor(hand ? cursor_hand : cursor_arrow);

        //debug("MotionEvent: (%u %u) %u\n", ev->x, ev->y, ev->state);
        break;
    }

    case ButtonPress: {
        XButtonEvent *ev = &event.xbutton;
        switch(ev->button) {
        case Button1: {
            //todo: better double/triple click detect
            static Time lastclick, lastclick2;
            panel_mdown(&panel_main);
            if(ev->time - lastclick < 300) {
                _Bool triclick = (ev->time - lastclick2 < 600);
                panel_dclick(&panel_main, triclick);
                if(triclick) {
                    lastclick = 0;
                }
            }
            lastclick2 = lastclick;
            lastclick = ev->time;
            mdown = 1;
            break;
        }

        case Button2: {
            pasteprimary();
            break;
        }

        case Button3: {
            panel_mright(&panel_main);
            break;
        }

        case Button4: {
            panel_mwheel(&panel_main, 0, 0, width, height, 1.0);
            break;
        }

        case Button5: {
            panel_mwheel(&panel_main, 0, 0, width, height, -1.0);
            break;
        }

        }

        //debug("ButtonEvent: %u %u\n", ev->state, ev->button);
        break;
    }

    case ButtonRelease: {
        XButtonEvent *ev = &event.xbutton;
        switch(ev->button) {
        case Button1: {
            panel_mup(&panel_main);
            mdown = 0;
            break;
        }
        }
        break;
    }

    case KeyPress: {
        XKeyEvent *ev = &event.xkey;
        KeySym sym = XLookupKeysym(ev, 0);//XKeycodeToKeysym(display, ev->keycode, 0)

        char buffer[16];
        //int len;

        XLookupString(ev, buffer, sizeof(buffer), &sym, NULL);
        if(edit_active()) {
            if(ev->state & 4) {
                switch(sym) {
                case 'v':
                    pasteclipboard();
                    return 1;
                case 'x':
                case 'c':
                    clipboard.len = edit_copy(clipboard.data, sizeof(clipboard.data));
                    setclipboard();
                    if(sym == 'x') {
                        edit_char(KEY_DEL, 1, 0);
                    }
                    return 1;
                }
            }

            if(sym == XK_Return && (ev->state & 1)) {
                edit_char('\n', 0, 0);
                break;
            }

            if(sym >= XK_KP_0 && sym <= XK_KP_9) {
                edit_char(sym - XK_KP_0 + '0', 0, ev->state);
                break;
            }

            uint32_t key = keysym2ucs(sym);
            if(key != ~0) {
                edit_char(key, (ev->state & 4) != 0, ev->state);
            } else {
                edit_char(sym, 1, ev->state);
            }

            break;
        }

        messages_char(sym);

        if(ev->state & 4) {
            if(sym == 'c') {
                if(sitem->item == ITEM_FRIEND) {
                    clipboard.len = messages_selection(&messages_friend, clipboard.data, sizeof(clipboard.data));
                    setclipboard();
                } else if(sitem->item == ITEM_GROUP) {
                    clipboard.len = messages_selection(&messages_group, clipboard.data, sizeof(clipboard.data));
                    setclipboard();
                }
                break;
            }
        }

        if(sym == XK_Delete) {
            list_deletesitem();
        }

        break;
    }

    case SelectionNotify: {

        debug("SelectionNotify\n");

        XSelectionEvent *ev = &event.xselection;

        if(ev->property == None) {
            break;
        }

        Atom type;
        int format;
        long unsigned int len, bytes_left;
        void *data;

        XGetWindowProperty(display, window, ev->property, 0, ~0L, True, AnyPropertyType, &type, &format, &len, &bytes_left, (unsigned char**)&data);

        if(!data) {
            break;
        }

        if(ev->property == XdndDATA) {
            char *path = malloc(len);
            memcpy(path, data, len);
            tox_postmessage(TOX_SENDFILES, (FRIEND*)sitem->data - friend, 0xFFFF, path);
            break;
        }

        if(edit_active()) {
            edit_paste(data, len);
        }

        XFree(data);

        break;
    }

    case SelectionRequest: {

        debug("SelectionRequest\n");

        XSelectionRequestEvent *ev = &event.xselectionrequest;

        XEvent resp = {
            .xselection = {
                .type = SelectionNotify,
                .property = ev->property,
                .requestor = ev->requestor,
                .selection = ev->selection,
                .target = ev->target,
                .time = ev->time
            }
        };//self.id, sizeof(self.id)

        if(ev->target == XA_UTF8_STRING) {
            if(ev->property == XA_PRIMARY) {
                XChangeProperty(display, ev->requestor, ev->property, ev->target, 8, PropModeReplace, clipboard.data, clipboard.len);
            } else {
                XChangeProperty(display, ev->requestor, ev->property, ev->target, 8, PropModeReplace, clipboard.data, clipboard.len);
            }
        } else if(ev->target == targets) {
            Atom supported[]={XA_UTF8_STRING};
            XChangeProperty (display,
                ev->requestor,
                ev->property,
                targets,
                8,
                PropModeReplace,
                (unsigned char *)(&supported),
                sizeof(supported)
            );
        }
        else {
            resp.xselection.property = None;
        }

        XSendEvent(display, ev->requestor, 0, 0, &resp);

        break;
    }

    case ClientMessage:
        {
            XClientMessageEvent *ev = &event.xclient;
            if(ev->window == 0) {
                void *data;
                memcpy(&data, &ev->data.s[2], sizeof(void*));
                tox_message(ev->message_type, ev->data.s[0], ev->data.s[1], data);
                break;
            }

            if(ev->message_type == wm_protocols) {
                if((Atom)event.xclient.data.l[0] == wm_delete_window) {
                    return 0;
                }
                break;
            }

            if(ev->message_type == XdndEnter) {
                debug("enter\n");
            } else if(ev->message_type == XdndPosition) {
                Window src = ev->data.l[0];
                XEvent event = {
                    .xclient = {
                        .type = ClientMessage,
                        .display = display,
                        .window = src,
                        .message_type = XdndStatus,
                        .format = 32,
                        .data = {
                            .l = {window, 1, 0, 0, XdndActionCopy}
                        }
                    }
                };

                XSendEvent(display, src, 0, 0, &event);
                //debug("position (version=%u)\n", ev->data.l[1] >> 24);
            } else if(ev->message_type == XdndStatus) {
                debug("status\n");
            } else if(ev->message_type == XdndDrop) {
                XConvertSelection(display, XdndSelection, XA_STRING, XdndDATA, window, CurrentTime);
                debug("drop\n");
            } else if(ev->message_type == XdndLeave) {
                debug("leave\n");
            } else {
                debug("dragshit\n");
            }
            break;
        }

    }

    return 1;
}
