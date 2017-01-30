// TODO, whole file should go away
#include "draw_helpers.h"

#include "buttons.h"
#include "draw.h"
#include "svg.h"
#include "text.h"

#include "../dns.h"
#include "../flist.h"
#include "../friend.h"
#include "../groups.h"
#include "../macros.h"
#include "../main_native.h"
#include "../self.h"
#include "../settings.h"
#include "../theme.h"

#include "../layout/tree.h"

#include "../layout/side_bar.h"

#include "../main.h"


void draw_notification(int x, int y, int w, int h) {
    if (!tox_thread_init) {
        return;
    }

    drawrect(x, y, w, h, COLOR_BKGRND_MAIN);

    if (self_has_avatar()) {
        draw_avatar_image(self.avatar->img, SIDEBAR_AVATAR_LEFT, SIDEBAR_AVATAR_TOP, self.avatar->width,
                          self.avatar->height, BM_CONTACT_WIDTH, BM_CONTACT_WIDTH);
    } else {
        drawalpha(BM_CONTACT, SIDEBAR_AVATAR_LEFT, SIDEBAR_AVATAR_TOP, BM_CONTACT_WIDTH, BM_CONTACT_WIDTH,
                  COLOR_MENU_TEXT);
    }

    setcolor(!button_name.mouseover ? COLOR_MENU_TEXT : COLOR_MENU_TEXT_SUBTEXT);
    setfont(FONT_SELF_NAME);
    drawtext(SIDEBAR_NAME_LEFT, SIDEBAR_NAME_TOP, "This is a test of the new uTox popup", 36);
}

void draw_splash_page(int x, int y, int w, int h) {
    setcolor(COLOR_MAIN_TEXT);

    x += SCALE(10);

    /* Generic Splash */
    setfont(FONT_SELF_NAME);
    int ny = utox_draw_text_multiline_within_box(x, y, w + x, y, y + h, font_small_lineheight, S(SPLASH_TITLE),
                                                 SLEN(SPLASH_TITLE), ~0, ~0, 0, 0, 1);
    setfont(FONT_TEXT);
    ny = utox_draw_text_multiline_within_box(x, ny, w + x, ny, ny + h, font_small_lineheight, S(SPLASH_TEXT),
                                             SLEN(SPLASH_TEXT), ~0, ~0, 0, 0, 1);

    ny += SCALE(30);
    /* Change log */
    setfont(FONT_SELF_NAME);
    ny = utox_draw_text_multiline_within_box(x, ny, w + x, y, ny + h, font_small_lineheight, S(CHANGE_LOG_TITLE),
                                             SLEN(CHANGE_LOG_TITLE), ~0, ~0, 0, 0, 1);
    setfont(FONT_TEXT);
    ny = utox_draw_text_multiline_within_box(x, ny, w + x, ny, ny + h, font_small_lineheight, S(CHANGE_LOG_TEXT),
                                             SLEN(CHANGE_LOG_TEXT), ~0, ~0, 0, 0, 1);
}

#include "../layout/settings.h" // TODO move this
void draw_background(int UNUSED(x), int UNUSED(y), int width, int height) {
    /* Default background                */
    drawrect(0, 0, width, height, COLOR_BKGRND_MAIN);
    /* Friend list (roster) background   */
    drawrect(0, 0, SIDEBAR_WIDTH, height, COLOR_BKGRND_LIST);
    /* Current user badge background     */
    drawrect(0, 0, SIDEBAR_WIDTH, ROSTER_TOP, COLOR_BKGRND_MENU);

    if (!panel_chat.disabled) {
        /* Top frame for main chat panel */
        drawrect(MAIN_LEFT, 0, width, MAIN_TOP_FRAME_THICK, COLOR_BKGRND_ALT);
        drawhline(MAIN_LEFT, MAIN_TOP_FRAME_THICK - 1, width, COLOR_EDGE_NORMAL);
        /* Frame for the bottom chat text entry box */
        drawrect(MAIN_LEFT, height + CHAT_BOX_TOP, width, height, COLOR_BKGRND_ALT);
        drawhline(MAIN_LEFT, height + CHAT_BOX_TOP, width, COLOR_EDGE_NORMAL);
    }
    // Chat and chat header separation
    if (panel_settings_master.disabled) {
        drawhline(MAIN_LEFT, MAIN_TOP_FRAME_THICK - 1, width, COLOR_EDGE_NORMAL);
    } else {
        drawhline(MAIN_LEFT, MAIN_TOP_FRAME_THIN - 1, width, COLOR_EDGE_NORMAL);
    }
}

/* These remain for legacy reasons, PANEL_MAIN calls these by default when not given it's own function to call */
void background_draw(PANEL *UNUSED(p), int UNUSED(x), int UNUSED(y), int UNUSED(width), int UNUSED(height)) {
    return;
}
bool background_mmove(PANEL *UNUSED(p), int UNUSED(x), int UNUSED(y), int UNUSED(width), int UNUSED(height),
                      int UNUSED(mx), int UNUSED(my), int UNUSED(dx), int UNUSED(dy)) {
    return 0;
}
bool background_mdown(PANEL *UNUSED(p)) {
    return 0;
}
bool background_mright(PANEL *UNUSED(p)) {
    return 0;
}
bool background_mwheel(PANEL *UNUSED(p), int UNUSED(height), double UNUSED(d), bool UNUSED(smooth)) {
    return 0;
}
bool background_mup(PANEL *UNUSED(p)) {
    return 0;
}
bool background_mleave(PANEL *UNUSED(p)) {
    return 0;
}
