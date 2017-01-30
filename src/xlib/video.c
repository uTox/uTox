#include "window.h"
#include "main.h"

#include "../logging_native.h"
#include "../macros.h"
#include "../main_native.h"

#include "../ui.h"

#include "../av/video.h"

#include <stdio.h>

#include "../main.h"

void video_frame(uint32_t id, uint8_t *img_data, uint16_t width, uint16_t height, bool resize) {
    if (!video_win[id]) {
        debug("frame for null window %u\n", id);
        return;
    }

    if (resize) {
        XWindowChanges changes = {.width = width, .height = height };
        XConfigureWindow(display, video_win[id], CWWidth | CWHeight, &changes);
    }

    XWindowAttributes attrs;
    XGetWindowAttributes(display, video_win[id], &attrs);

    XImage image = {.width            = attrs.width,
                    .height           = attrs.height,
                    .depth            = 24,
                    .bits_per_pixel   = 32,
                    .format           = ZPixmap,
                    .byte_order       = LSBFirst,
                    .bitmap_unit      = 8,
                    .bitmap_bit_order = LSBFirst,
                    .bytes_per_line   = attrs.width * 4,
                    .red_mask         = 0xFF0000,
                    .green_mask       = 0xFF00,
                    .blue_mask        = 0xFF,
                    .data             = (char *)img_data };

    /* scale image if needed */
    uint8_t *new_data = malloc(attrs.width * attrs.height * 4);
    if (new_data && (attrs.width != width || attrs.height != height)) {
        scale_rgbx_image(img_data, width, height, new_data, attrs.width, attrs.height);
        image.data = (char *)new_data;
    }

    GC     default_gc = DefaultGC(display, def_screen_num);
    Pixmap pixmap     = XCreatePixmap(display, main_window.window, attrs.width, attrs.height, default_depth);
    XPutImage(display, pixmap, default_gc, &image, 0, 0, 0, 0, attrs.width, attrs.height);
    XCopyArea(display, pixmap, video_win[id], default_gc, 0, 0, attrs.width, attrs.height, 0, 0);
    XFreePixmap(display, pixmap);
    free(new_data);
}

void video_begin(uint32_t id, char *name, uint16_t name_length, uint16_t width, uint16_t height) {
    Window *win = &video_win[id];
    if (*win) {
        return;
    }

    *win = XCreateSimpleWindow(display, RootWindow(display, def_screen_num), 0, 0, width, height, 0,
                               BlackPixel(display, def_screen_num), WhitePixel(display, def_screen_num));

    // Fallback name in ISO8859-1.
    XStoreName(display, *win, "Video Preview");
    // UTF-8 name for those WMs that can display it.
    XChangeProperty(display, *win, XA_NET_NAME, XA_UTF8_STRING, 8, PropModeReplace, (uint8_t *)name, name_length);
    XSetWMProtocols(display, *win, &wm_delete_window, 1);

    /* set WM_CLASS */
    XClassHint hint = {.res_name = "utoxvideo", .res_class = "utoxvideo" };

    XSetClassHint(display, *win, &hint);

    XMapWindow(display, *win);
    debug("new window %u\n", id);
}

void video_end(uint32_t id) {
    if (!video_win[id]) {
        return;
    }

    XDestroyWindow(display, video_win[id]);
    video_win[id] = None;
    debug("killed window %u\n", id);
}

Display *deskdisplay;
int      deskscreen;

XShmSegmentInfo shminfo;

void initshm(void) {
    deskdisplay = XOpenDisplay(NULL);
    deskscreen  = DefaultScreen(deskdisplay);
    debug("desktop: %u %u\n", default_screen->width, default_screen->height);
    max_video_width  = default_screen->width;
    max_video_height = default_screen->height;
}

uint16_t native_video_detect(void) {
    char     dev_name[] = "/dev/videoXX", *first = NULL;
    uint16_t device_count = 1; /* start at 1 for the desktop input */

    // Indicate that we support desktop capturing.
    utox_video_append_device((void *)1, 1, (void *)STR_VIDEO_IN_DESKTOP, 0);

    int i;
    for (i = 0; i != 64; i++) { /* TODO: magic numbers are bad mm'kay? */
        snprintf(dev_name + 10, sizeof(dev_name) - 10, "%i", i);

        struct stat st;
        if (-1 == stat(dev_name, &st)) {
            continue;
            // debug("Cannot identify '%s': %d, %s\n", dev_name, errno, strerror(errno));
            // return 0;
        }

        if (!S_ISCHR(st.st_mode)) {
            continue;
            // debug("%s is no device\n", dev_name);
            // return 0;
        }

        void *p = malloc(sizeof(void *) + sizeof(dev_name)), *pp = p + sizeof(void *);
        memcpy(p, &pp, sizeof(void *));
        memcpy(p + sizeof(void *), dev_name, sizeof(dev_name));
        if (!first) {
            first = pp;
            utox_video_append_device(p, 0, p + sizeof(void *), 1);
        } else {
            utox_video_append_device(p, 0, p + sizeof(void *), 0);
        }
        device_count++;
    }

    initshm();

    return device_count;
}

static uint16_t video_x, video_y;

bool native_video_init(void *handle) {
    if (isdesktop(handle)) {
        utox_v4l_fd = -1;

        video_x      = volatile(grabx);
        video_y      = volatile(graby);
        video_width  = volatile(grabpx);
        video_height = volatile(grabpy);

        if (video_width & 1) {
            if (video_x & 1) {
                video_x--;
            }
            video_width++;
        }

        if (video_height & 1) {
            if (video_y & 1) {
                video_y--;
            }
            video_height++;
        }

        if (!(screen_image = XShmCreateImage(deskdisplay, DefaultVisual(deskdisplay, deskscreen),
                                             DefaultDepth(deskdisplay, deskscreen), ZPixmap, NULL, &shminfo,
                                             video_width, video_height))) {
            return 0;
        }

        if ((shminfo.shmid = shmget(IPC_PRIVATE, screen_image->bytes_per_line * screen_image->height, IPC_CREAT | 0777))
            < 0) {
            return 0;
        }

        if ((shminfo.shmaddr = screen_image->data = (char *)shmat(shminfo.shmid, 0, 0)) == (char *)-1) {
            return 0;
        }

        shminfo.readOnly = False;
        if (!XShmAttach(deskdisplay, &shminfo)) {
            return 0;
        }

        return 1;
    }

    return v4l_init(handle);
}

void native_video_close(void *handle) {
    if (isdesktop(handle)) {
        XShmDetach(deskdisplay, &shminfo);
        return;
    }

    v4l_close();
}

bool native_video_startread(void) {
    if (utox_v4l_fd == -1) {
        return 1;
    }

    return v4l_startread();
}

bool native_video_endread(void) {
    if (utox_v4l_fd == -1) {
        return 1;
    }

    return v4l_endread();
}

int native_video_getframe(uint8_t *y, uint8_t *u, uint8_t *v, uint16_t width, uint16_t height) {
    if (utox_v4l_fd == -1) {
        static uint64_t lasttime;
        uint64_t        t = get_time();
        if (t - lasttime >= (uint64_t)1000 * 1000 * 1000 / 24) {
            XShmGetImage(deskdisplay, RootWindow(deskdisplay, deskscreen), screen_image, video_x, video_y, AllPlanes);
            if (width != video_width || height != video_height) {
                debug("uTox:\twidth/height mismatch %u %u != %u %u\n", width, height, screen_image->width,
                      screen_image->height);
                return 0;
            }

            bgrxtoyuv420(y, u, v, (uint8_t *)screen_image->data, screen_image->width, screen_image->height);
            lasttime = t;
            return 1;
        }
        return 0;
    }

    return v4l_getframe(y, u, v, width, height);
}
