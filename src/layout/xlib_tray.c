#include "xlib_tray.h"

#include "../debug.h"

#include "../native/image.h"
#include "../native/main.h"
#include "../xlib/tray.h" // TODO remove this line if the tray api becomes common and abstracted

#include "../ui/button.h"
#include "../ui/draw.h"
#include "../ui/panel.h"

#include <stddef.h>
#include <stdint.h>

// Converted to a binary and linked at build time
extern uint8_t _binary_icons_utox_128x128_png_start;
extern size_t  _binary_icons_utox_128x128_png_size;

static void draw_tray(int x, int y, int h, int w)
{
    LOG_NOTE("XLib Tray", "Draw Tray");

    static uint16_t width, height;
    static uint8_t *icon_data = (uint8_t *)&_binary_icons_utox_128x128_png_start;
    static size_t   icon_size = (size_t)&_binary_icons_utox_128x128_png_size;
    static NATIVE_IMAGE *icon = NULL;
    if (!icon) {
        icon = utox_image_to_native(icon_data, icon_size, &width, &height, 0);
    }

    if (NATIVE_IMAGE_IS_VALID(icon)) {
        /* Get tray window size */
        /* Resize the image from what the system tray dock tells us to be */
        double scale = (w > h) ? ((double)h / (double)width) : ((double)w / (double)height);

        image_set_scale(icon, scale);
        image_set_filter(icon, FILTER_BILINEAR);

        /* Draw the image and copy to the window */
        drawrect(x, y, w, h, ~0);
        draw_image(icon, x, y, width, height, 0, 0);
    } else {
        LOG_ERR("XLIB TRAY", "Tray no workie, that not gud!");
        free(icon);
        icon = NULL;
    }
}

static void tmp(x, y, w, h) {
    (void)w;
    (void)h;
    setfont(FONT_SELF_NAME);
    drawstr(x + SCALE(10), y + SCALE(245), PROFILE_PASSWORD);
    LOG_ERR("Tmp", "drawing for tray_drop window");
}

// TODO replace with BUTTON
PANEL xlib_tray_drop_button = {
    .type = PANEL_NONE,
    .drawfunc = tmp,
    .child = (PANEL*[]) {
        NULL,
    }
};

PANEL xlib_tray_rdrop = {
    .type = PANEL_NONE,
    .child = (PANEL*[]) {
        (PANEL*)&xlib_tray_drop_button,
        NULL,
    }
};


static void xlib_btn_mup(void) {
    LOG_ERR("XLIB_TRAY Layout", "mup");
    togglehide();
}

static void xlib_btn_rup(void) {
    LOG_ERR("XLIB_TRAY Layout", "rup");
    tray_drop_init(&xlib_tray_rdrop);
    // create tray popup
}

BUTTON xlib_btn = {
    .panel = {
        .type   = PANEL_BUTTON,
        .x      = 0,
        .y      = 0,
        .width  = 512,
        .height = 512,
    },
    .nodraw = true,
    .on_mup = xlib_btn_mup,
    .onright = xlib_btn_rup,
};

PANEL root_xlib_tray = {
    .type = PANEL_NONE,
    .drawfunc = draw_tray,
    .child = (PANEL*[]) {
        (PANEL*)&xlib_btn,
        NULL,
    }
};

