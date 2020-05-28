#include "tray.h"

#include "../macros.h"
#include "../theme.h"

#include "../ui/button.h"
#include "../ui/draw.h"
#include "../ui/panel.h"
#include "../ui/text.h"

static void tray_draw(int x, int y, int w, int h) {
    drawrect(x, y, w, h, COLOR_BKGRND_MAIN);
    draw_rect_frame(x, y, w, h, 0);
    setcolor(COLOR_MENU_TEXT);
    setfont(FONT_SELF_NAME);
    drawtext(SCALE(5), SCALE(5), "This is a test of the new uTox popup", 36);
}

PANEL panel_tray = {
    .type = PANEL_NONE,
    .drawfunc = tray_draw,
    .disabled = 0,
    .child = (PANEL*[]) {
        (PANEL*) &btn_tray_exit,
        NULL,
    }
};

static void tray_exit(void) {
    LOG_FATAL_ERR(EXIT_SUCCESS, "Layout Notify", "Exit pressed");
}

BUTTON btn_tray_exit = {
    .panel = {
        .type   = PANEL_BUTTON,
        .x      = 5,
        .y      = 5,
        .width  = 270,
        .height = 40,
    },
    // .bm_fill = BM_SBUTTON,
    .update = button_setcolors_success,
    .on_mup = tray_exit,
    .nodraw = false,

    .button_text  = {.i18nal = STR_EXIT },
    .tooltip_text = {.i18nal = STR_EXIT },
};
